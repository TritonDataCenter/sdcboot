/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: TSR functions support - retry and redirection */

#include "mode.h"
#include "modeint.h" /* data for interrupt handler */


/* This code relies on the memory layout of modeint.asm, which must be:     */
/* offset 0: far pointer to old intr 0x17 handler, new handler at offset 8. */
/* offset 4: array of 4 configuration bytes for LPT1 ... LPT4               */
/* configuration bytes are each "??r?  ?xxx":  set r for infinite retry.    */
/* the xxx value can be 1..4 to redirect to COM1..4, or 0 for no redirect.  */

/* local function: find loaded TSR (0) or create TSR memory area (1). */
unsigned int EnsureTSR(int create);

/* if create is 1, ensure that the handler is there. Else, just search it. */
unsigned int EnsureTSR(int create)
{
  char tsrname[] = "MODETSR\0";
  int i;
  unsigned int tsrseg = 0x71; /* cannot be earlier */
  int tsrfound = 0;

  tsrfound = 1;
  tsrseg = peek(0, (0x17 * 4) + 2); /* first guess: int 0x17 segment! */
  for (i = 8; (i < (sizeof(mode_int17)/2)); i++)
    if (mode_int17[i] != peekb(tsrseg, i))
      tsrfound = 0;


#if DMODE2
  if (create) {
    if (!tsrfound)
      printf("MODE must be resident for this function. ");
    else
      printf("MODE already resident: ");
  }
#endif

  if (!tsrfound)
  do {
    tsrfound = 1;
    for (i = 8; tsrfound && (i < (sizeof(mode_int17)/2)); i++)
      if (mode_int17[i] != peekb(tsrseg, i))
        tsrfound = 0;
    if (!tsrfound) tsrseg++;
    if (tsrseg == 0xa000)
      tsrseg = 0xc800; /* do not scan common video RAM / ROM area */
  } while ((!tsrfound) && (tsrseg<0xf000)); /* found, or cannot be later */

  if (tsrfound) {
#if DMODE
    if (create)
      printf("Found MODETSR handler at 0x%04x:0008.\r\n", tsrseg);
#endif
    return tsrseg;
  }

  if (!create)
    return 0; /* no TSR found, but no need to create one either */

  tsrseg = peek(_psp, 0x2c); /* abuse environment segment to store handler */
  if ( peek(tsrseg-1, 3) < ((sizeof(mode_int17)+31)>>4) ) {
    printf("Environment too small to hold %u bytes resident part, sorry.",
      sizeof(mode_int17));
    tsrseg = _psp + 8;       /* dump variables to cmdline buffer (ignore)! */
    return tsrseg;
  }
  poke(_psp, 0x2c, 0);       /* disable and disown our environment */
  poke(tsrseg-1, 1, 8);      /* change MCB owner to neutral "DOS"! */
  for (i = 0; i < 8; i++)
    pokeb(tsrseg-1, 8+i, tsrname[i]);  /* set MCB owner name */

  for (i = 0; i < sizeof(mode_int17); i++)
    pokeb(tsrseg, i, mode_int17[i]);   /* copy resident handler */

  disable();
  movedata(0, 0x17 * 4, tsrseg, 0, 4); /* copy intr 0x17 vector to TSR */
  poke(0, (0x17 * 4), 8);              /* our intr 0x17 handler, offset */
  poke(0, ((0x17 * 4) + 2), tsrseg);   /* our intr 0x17 handler, segment */
  enable();

  /* (not done yet: shrink and split memory block to save more RAM) */
#if DMODE
    printf("Installed MODETSR handler at 0x%04x:0008.\r\n", tsrseg);
#endif

  return tsrseg;

} /* EnsureTSR */



/* activate redirection of LPTn to COMm with n=pnum+1, m=cnum+1 */
int RedirectLptCom(int pnum, int cnum)	/* negative cnum stops redirect */
{
  unsigned int tseg;
  unsigned char rnum;

  if ((pnum < 0) || (pnum > 3)) return -1; /* invalid LPTn */
  if (cnum > 4) return -1;                 /* invalid COMm */

  rnum = (cnum < 0) ? 0 : (cnum+1);        /* negative cnum stops redirect */
    /* redirection to "COM5" (pnum 4) means "redirect to NUL" */

  tseg = EnsureTSR(1);
  pokeb(tseg, 4 + pnum, (peekb(tseg, 4 + pnum) & 0xf8) | rnum);

  DescribeTSR(pnum);
  return 0;
} /* RedirectLptCom */



/* set retry mode of LPTn to xretry (n=pnum+1) */
int SetRetry(int pnum, int xretry) /* xretry 3 means infinite retry */
{
  unsigned int tseg;
  unsigned char rflag;

  if ((pnum < 0) || (pnum > 3)) return -1; /* invalid LPTn */

  if ((xretry < 3) || (xretry > 4)) return -1; /* invalid retry mode */

  rflag = (xretry == 3) ? 0x20 : 0; /* 3 is infinite retry */
    /* 0 is "busy if busy", 1 is "error if busy", basically the same. */

  tseg = EnsureTSR(1);
  pokeb(tseg, 4 + pnum, (peekb(tseg, 4 + pnum) & 0xdf) | rflag);

  DescribeTSR(pnum);
  return 0;
} /* SetRetry */



/* if the TSR module is found, describe it. Return config flags. */
int DescribeTSR(int portnum) /* 0..3 for LPT1..LPT4, other undefined */
{
  int i;
  static unsigned int tseg = 0;

  if ( (portnum < 0) || (portnum > 3) )
    return -1;

  if (tseg == 1) /* already searched without finding before? */
    return 0;

  if (!tseg) { /* only search if not done before */
    tseg = EnsureTSR(0); /* only find, do not load if not found */
#ifdef DMODE
    if (tseg) {
#ifdef DMODE2
      printf("MODE int 17 handler at 0x%04x:0008. Old int 0x17 at 0x%04x:0x%04x.\r\n",
        tseg, peek(tseg, 2), peek(tseg, 0));
#else
      printf("     MODE is resident at segment 0x%04x.\r\n", tseg);
#endif
    } /* resident found */
#endif
  } /* first time we search */

  if (!tseg) {
    tseg = 1; /* do not search again */
#ifdef DMODE
    printf("     MODE is not resident in RAM.\r\n");
#endif
    return 0;
  }

  i = peekb(tseg, 4 + portnum);

  if (!i) {
#ifdef DMODE2
    printf("Resident module does not modify LPT%d access.\r\n", portnum+1);
#endif
    return 0; /* nothing interesting */
  }

  printf("LPT%d resident module setup: %s", portnum+1,
    (i & 0x20) ? "[persistent retry] " : "");
  switch (i & 7) {
    case 1:
    case 2:
    case 3:
    case 4:
      printf("Redirected to COM%d", i & 7);
      break;
    case 0:
      /* no redirection */
      break;
    case 5:
    case 6:
    case 7:
      printf("Redirected to NUL");
  } /* switch */
  printf("\r\n");

  return i; /* return configuration */
  
} /* DescribeTSR */

