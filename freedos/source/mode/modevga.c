/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: programm the video hardware. Called by modecon functions.  */

#include "mode.h"

#define VIDMAXROW peekb(0x40,0x84)

static union REGS r;

/* ------------------------------------------------ */

/* set a VESA mode - return 0 on success */
int set_vesa(int cols, int lines)
{
  unsigned char vesabuffer[256];
  unsigned int vesamodes[150]; /* we have to copy the list! */
  unsigned int i, xcols, xlines;
  unsigned long li;
  char far * str;
  unsigned int far * model;
  struct SREGS sregs;

  r.h.ah = 0x12; /* ah=0x12, bl=0x10: get EGA info */
  r.x.bx = 0xff10; /* init bh to -1 */
  r.x.cx = 0xffff; /* init cx to -1 */
  int86(0x10, &r, &r);
  if ( (r.x.cx == 0xffff) || (r.h.bh == 0xff) )
    return 1; /* not even EGA */
  r.x.ax = 0x1a00; /* get VGA display combination - VGA install check */
  int86(0x10, &r, &r);
  if (r.h.al != 0x1a)
    return 1; /* not even VGA */

  for (i = 0; i < sizeof(vesabuffer); i++)
    vesabuffer[i] = ' ';
  sregs.es = FP_SEG(&vesabuffer[0]);
  r.x.di = FP_OFF(&vesabuffer[0]);
  r.x.ax = 0x4f00; /* get VESA version and mode list */
  int86x(0x10, &r, &r, &sregs); /* call VESA version check */
  if ( (r.x.ax != 0x004f) || (vesabuffer[0]!='V') || (vesabuffer[1]!='E') ||
       (vesabuffer[2]!='S') || (vesabuffer[3]!='A') ||
     ( ((vesabuffer[5] == 1) && (vesabuffer[4] < 2)) || (vesabuffer[5] < 1) )
     ) {
    printf("No VESA, but %d x %d mode not supported on plain VGA yet.\r\n",
      cols, lines);
    if (r.x.ax == 0x004f) printf("VESA 1.2 or newer required.\r\n");
    return 1;
  }

  /* some BIOSes return the mode list *in* the vesabuffer! */
  /* so we have to copy it to protect it from overwriting. */
  model = MK_FP( peek(sregs.es, FP_OFF(&vesabuffer[0x10])),
    peek(sregs.es, FP_OFF(&vesabuffer[0x0e])) );
  i = 0;
  while ( (*model != 0xffff) && (i<149) ) {
    vesamodes[i] = *model;
    if (vesamodes[i]) i++; /* do not store the value 0 */
    model++;
  }
  vesamodes[i] = 0; /* we use 0, not -1, as end marker */

  r.x.ax = 0x4f03; /* get current VESA mode */
  int86(0x10, &r, &r); /* query current VESA mode */
  /* current mode is now in BX - high 7 bits are for flags: */
  /* 0x8000 did not clear screen 0x4000 linear framebuffer */

#ifdef VMODE
  /* *** prf.c simplified printf() does not support %hu, so... *** */
  printf("VESA %d.%d '", (int)(1*vesabuffer[5]), (int)(1*vesabuffer[4]));
  li = vesabuffer[0x13];
  li <<= 8;
  li |= vesabuffer[0x12];
  str = MK_FP( peek(sregs.es, FP_OFF(&vesabuffer[8])),
    peek(sregs.es, FP_OFF(&vesabuffer[6])) );
  while (*str) {
    printf("%c",*str);
    str++;
  }
  printf("', mode 0x%04x, %luk for BIOS, %u modes.\r\n", r.x.bx, li * 64, i);
#endif

  r.x.cx = r.x.bx & 0x01ff;
  r.x.ax = 0x4f01; /* get info about VESA mode */
  sregs.es = FP_SEG(&vesabuffer[0]);
  r.x.di = FP_OFF(&vesabuffer[0]);
  int86x(0x10, &r, &r, &sregs);
  if ( (r.x.ax == 0x004f) && ((vesabuffer[0] & 0x16) == 0x06) ) {
       /* info valid and BIOS supported text mode */
    if ( ( (!cols) ||  (vesabuffer[0x12] == cols)  ) &&
         ( (!lines) || (vesabuffer[0x14] == lines) ) ) {
#ifdef DMODE2
      printf("Current mode already has requested size.\r\n");
#endif
      return 0;
    } /* matching mode */
    /* first, try to find a mode which preserves unspecified variables */
    xcols = (cols) ? cols : vesabuffer[0x12];
    xlines = (lines) ? lines : vesabuffer[0x14];
  } /* valid mode */

  i = 0;  /* start with low-numbered modes */
  li = 0; /* try good fit first */
  while (1) {
    while (vesamodes[i]) {
      r.x.cx = vesamodes[i] & 0x01ff;
      r.x.ax = 0x4f01; /* get info about VESA mode */
      sregs.es = FP_SEG(&vesabuffer[0]);
      r.x.di = FP_OFF(&vesabuffer[0]);
      int86x(0x10, &r, &r, &sregs);
      if ( (r.x.ax == 0x004f) && ((vesabuffer[0] & 0x16) == 0x06) ) {
           /* info valid and BIOS supported reachable text mode */
           /* bits: 0 reachable??, 1 cols/lines known, 2 BIOS supported */
           /* 3 color, 4 graphical, 5 non-VGA (VBE 2.0+) ... 12 dual    */
        if ( ( (!xcols) || (vesabuffer[0x12] == xcols)  ) &&
             ( (!xlines) || (vesabuffer[0x14] == xlines) ) ) {
          r.x.ax = 0x4f02; /* set VESA mode */
          r.x.bx = vesamodes[i] & 0x0fff;
          int86(0x10, &r, &r);
          /* simplified printf() in prf.c has no %hu, so... */
          printf("VESA mode 0x%04x: %d x %d with %d x %d font.\r\n",
            vesamodes[i], (int)(0+vesabuffer[0x12]), (int)(0+vesabuffer[0x14]),
            (int)(0+vesabuffer[0x16]), (int)(0+vesabuffer[0x17]));
          /* could also check: 0x1b is 0 for text mode memory layout, */
          /*   0x13 and 0x14 are high bytes, 0x10 is memory line len. */
          return 0; /* SUCCESS! */
        } /* matching mode */
#ifdef DMODE2
#ifdef DMODE3
        else printf("Mismatch: %d x %d 0x%04x (not %u x %u, 0 is any).\r\n",
          (int)(0+vesabuffer[0x12]), (int)(0+vesabuffer[0x14]),
          vesamodes[i], xcols, xlines);
#else
        else printf("Mismatch: %d x %d mode 0x%04x.\r\n",
          (int)(0+vesabuffer[0x12]), (int)(0+vesabuffer[0x14]), vesamodes[i]);
#endif
#endif        
      } /* valid mode: BIOS text mode */
      i++; /* try next mode */
    } /* while */
    if (li) {
#ifdef DMODE2
      printf("Sorry, no such VESA mode found.\r\n");
#endif
      return 1; /* not even a sloppy match worked */
    }
    if ((!cols) || (!lines)) { /* only 1 specified? */
#ifdef DMODE2
      printf("Not possible to change only %s. Allowing %s to change, too.\r\n",
        (cols) ? "columns" : "lines", (cols) ? "lines" : "columns");
#endif
      /* if nothing found yet, allow any value for unspecified variables */
      xcols = cols;
      xlines = lines;
      i = 0; /* start searching again */
      li = 1; /* flag: already searching for sloppy fit */
    } else {
#ifdef DMODE2
      printf("Sorry, no such VESA mode. Try setting ONLY cols or ONLY lines.\r\n");
#endif
      return 1;
    }
  } /* loop 1st good fit, 2nd sloppy fit */

  /* return 1; - never reached */ /* failed */
} /* set_vesa */

/* ------------------------------------------------ */

/* set number of screen lines */
int set_lines(int lines)
{
  int havevga = 0;
  int oldlines = VIDMAXROW+1; /* only valid for EGa or better */

  r.h.ah = 0x12; /* ah=0x12, bl=0x10: get EGA info */
  r.x.bx = 0xff10; /* init bh to -1 */
  r.x.cx = 0xffff; /* init cx to -1 */
  int86(0x10, &r, &r);
  if ( (r.x.cx == 0xffff) || (r.h.bh == 0xff) ) {
    if (lines == 25) {
#ifdef DMODE2
      printf("Not EGA or better, assuming 25 lines.\r\n");
#endif
      return 0; /* 25 lines is tautologic for less-than-EGA */
    }
    printf("Only EGA or better allows more than 25 lines!\r\n");
    return 1;
  }

  r.x.ax = 0x1a00; /* get VGA display combination - VGA install check */
  int86(0x10, &r, &r);
  /* returned active / alternate display in BL / BH (use al=1 to SET...) */
  /* 0 none, 1 mono, 2 CGA, 4..5 EGA 6 PGA 7..8 VGA a..c MCGA */
  if (r.h.al == 0x1a)
    havevga = 1; /* VGA found */

  if ( (lines == 50) && (havevga == 0) ) { /* 50 lines requires VGA */
    printf("Only VGA or better allows more than 43 lines!\r\n");
    return 1;
  }

  if (oldlines == lines) {
#ifdef DMODE2
    printf("Number of lines already %d as requested.\r\n", lines);
    /* can be triggered implicitly by selecting BIOS mode 0..3, too */
#endif
    return 0; /* no action needed */
  }

  /* only reached for EGA, VGA or better */
  switch (lines) {
    case 8:	/* actually, font height, not lines */
    case 14:	/* actually, font height, not lines */
    case 16:	/* actually, font height, not lines */
      r.x.ax = 0x0500; /* select page 0 */
      int86(0x10, &r, &r);
      r.x.ax = 0x1112; /* activate 8x8 default font */
      if (lines > 8)
        r.x.ax = (lines == 16) ? 0x1114 : 0x1111; /* 8x16 and 8x14 font */
      if ((havevga!=1) && (r.x.ax==0x1114))
        r.x.ax = 0x1111; /* only EGA available */
      /* use AL 1x for full mode reset, 0x for font load only */
      /* 1x versions require page to be 0 and recalc rows and bufsize */
      r.h.bl = 0; /* font bank 0 */
      int86(0x10, &r, &r);
      break;
    case 25:
    /* case 30: 640x480 8x16 would require VGA chip programming */
#if 0
//    r.h.ah = 0x0f; /* get current video mode */
//    int86(0x10, &r, &r);
//    r.h.ah = 0;
//    /* AL: simply re-activate current mode */
//    if (r.h.al > 3)
//      r.h.al = 3; /* better use only known modes like 80x25 color */
//    int86(0x10, &r, &r);
#endif
      r.x.ax = (havevga==1) ? 0x1114 : 0x1111;
      /* activate 8x16 / 8x14 VGA / EGA (or MONO) default font */
      /* use AL 1x for full mode reset, 0x for font load only */
      r.h.bl = 0; /* font bank 0 */
      int86(0x10, &r, &r);
      break;
    case 43: /* 640x350 8x8 */
    case 28: /* 640x400 8x14 (640x392, actually) */
    case 50: /* 640x400 8x8 */
    /* case 60: 640x480 8x8 would require VGA chip programming */
      r.x.ax = 0x0500; /* select page 0 */
      int86(0x10, &r, &r);
      if (havevga==1) { /* we need extra 43 <-> 50 line switching in VGA */
        r.h.ah = 0x12; /* set resolution (with BL 0x30) */
        r.h.bl = 0x30; /* set resolution: 0 200, 1 350, 2 400 */
        r.h.al = (lines==43) ? 1 : 2; /* 1 means 350 pixels rows, 2 400 */
        
        int86(0x10, &r, &r);
        if (r.h.al==0x12) { /* did the call succeed? */
          r.h.ah = 0x0f; /* get current video mode */
          int86(0x10, &r, &r);
          r.h.ah = 0; /* set mode again, to let resolution change take effect! */
          /* r.h.al |= 0x80; */ /* mode |= flag "do not clear screen" */
          int86(0x10, &r, &r);
#ifdef VMODE
          printf("Using VGA %d line resolution.\r\n",
            (lines==43) ? 350 : 400);
#endif
        } else {
          printf("Could not select ???x%d pixel resolution, using default.\r\n",
            (lines==43) ? 350 : 400);
        }
      } /* havevga */
      r.x.ax = (lines==28) ? 0x1111 : 0x1112; /* font selection */
        /* activate 8x8 (or for 28 lines 8x14) default font */
      /* use AL 1x for full mode reset, 0x for font load only */
      r.h.bl = 0; /* font bank 0 */
      int86(0x10, &r, &r);
      break;
  } /* switch */

  oldlines = lines; /* desired value */
  lines = VIDMAXROW+1; /* value from BIOS */
  if (lines < 20)
    lines = 25;
  if ((lines!=oldlines) && (oldlines > 16)) {
    printf("Could only set %d rows, not %d.\r\n", lines, oldlines);
  }
  if ((oldlines <= 16) && (oldlines))
    printf("Selected 8x%d (or 9x%d) font: %d rows now.\r\n",
      oldlines, oldlines, lines);

  return 0; /* success */
} /* set_lines */

/* ------------------------------------------------ */

/* find out whether a CRTC with editable cursor register exists at port. */
/* CGA/MDA/HGC 6845 has registers 0..0x11 (only 0x0e..0x0f readable)     */
/* 0x0e/0x0f: cursor location (high, low!) (12bit in 6845)               */
/* EGA: 0x0c..0x0f readable, VGA: 0..7 write-protectable, regs 0..0x18   */
int testCRTC(unsigned int port)
{
  unsigned char one, other;
  disable(); /* block IRQs during test */
  outportb(port,0x0f);     /* 0x0f is cursor position register, lower byte */
  one = inportb(port+1);   /* current value */
  outportb(port,0x0f);
  other = one ^ 1;
  outportb(port+1,other);  /* a modified value */
  outportb(port,0x0f);
  other = inportb(port+1); /* did new value get stored? */
  outportb(port,0x0f);
  outportb(port+1,one);    /* restore value */
  enable();  /* enable IRQs again */
  return (one != other);   /* if we could modify the value, success! */
} /* testCRTC */

