/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: control for LPTx things */

#include "mode.h"

static union REGS r;

/* ------------------------------------------------ */

/* handle printer port related commands */
/* show status if no argument or argument "/STA..." */
/* style 1: cols,lines,retry (80/132, 6/8, p/n)     */
/* style 2: colr=... lines=... retry=... */
int printer(int pnum, char * what)
{
  unsigned int cols, lines, xretry;
  char retry;

#define lp(ch) r.h.ah=0; r.h.al=ch; r.x.dx=pnum; int86(0x17, &r, &r); \
  /* TODO: check returned r.h.ah status...? */

  if ( (what == NULL) || (!strncmp(what, "/STA", 4)) ) {
    static char * parbits[] = { "timeout", "", "", "I/O-error",
      "selected", "out-of-paper", "acknowledge", "ready" /* not busy */
    }; /* parbits */
    int n;

    if ( (pnum > 2) || (!peek(0x40, 8 + (pnum*2))) ) {
      printf("*** VIRTUAL PRINTER PORT %d INFORMATION ***\r\n", pnum+1);
      if (!DescribeTSR(pnum)) /* show resident module setup for this port */
        printf("LPT%d is free for redirection to serial ports.\r\n", pnum+1);
      return 0;
    };

    printf("*** PRINTER PORT %d STATUS ***\r\n", pnum+1); /* real printer */
    r.h.ah = 2;
    r.x.dx = pnum;
    if ( (r.h.ah & 0x30) == 0x30 ) {
      printf("Both out-of-paper and selected: no printer attached?\r\n");
    }
    printf("Port status: [ ");
    for (n=7; n>=0; n--) {
      if ( (r.h.ah & (1<<n)) != 0)
        printf("%s ", parbits[n]);
      /* a typical okay status is 0x90, ready, selected, no errors, no ack */
    }
    printf("]\r\n");

    DescribeTSR(pnum); /* show resident module setup for this port */

    return 0;
  }

  if (what[0] == '=') {
    what++;
    if (!strncmp(what,"NUL",3)) {
      return RedirectLptCom(pnum, 4); /* redirect LPTn to NUL ("COM5") */
    }
    if (!strncmp(what,"LPT",3)) {
      if (what[3] == (pnum + '1')) { /* "LPT2=LPT2" syntax */
        strcpy(what, "COM0"); /* translate into "COM0" (stop redirection) */
      }
    }
    if (strncmp(what,"COM",3)) {
      printf("You can only redirect to serial ports (COMn) or NUL.\r\n");
      printf("(Or stop redirection by redirecting a port to itself!)\r\n");
      return 1;
    }
    what+=3;
    if ((what[0] < '0') || (what[0] > '5')) {
      printf("You can only redirect to COM1-COM4. Alternatively, you can...\r\n");
      printf("- redirect to 'COM0' to stop redirection or\r\n");
      printf("- redirect to 'COM5' to redirect data to the bit bucket.\r\n");
      return 1;
    }
    return RedirectLptCom(pnum, what[0]-'1'); /* redirect LPTn to COMm */
  }

  if (strchr(what,'=') != NULL) {
    cols   = grabarg(what, "COLS=");
    lines  = grabarg(what, "LINES=");
    xretry  = grabarg(what, "RETRY=");
  } else {
    what = skipspace(what);
    if ( (what[0]!=',') && (!isdigit(what[0])) ) {
      printf("Syntax error in 'MODE LPTn ...', please check 'MODE /?'.\r\n");
      return 1;
    }
    cols   = posarg(what, 0);
    lines  = posarg(what, 1);
    xretry  = posarg(what, 2);
  }

  retry = (char)xretry;
  xretry = xlatretry(xretry);
  if (xretry<0) return 1; /* error */
  if ((xretry==1) || (xretry==2)) {
    printf("Retry / error reporting modes E and R not possible for printers.\r\n");
    exit(1);
  }
  /* default value 0 could be translated to value 4 here... */

  if (xretry>0) SetRetry(pnum, xretry); /* update resident part */
  /* xretry values: "if busy..." 0 busy 1 error 2 ignore 3 retry 4 noretry */

  if ( (cols!=0) && (cols!=80) && (cols!=132) && ((cols<10) || (cols>20)) ) {
    printf("Columns must be 80 or 132 or a cpi value in 10..20 range.\r\n");
    return 1; /* failed */
  }

  if ( (lines!=0) && (lines!=6) && (lines!=8) ) {
    printf("Lines per inch must be 6 or 8.\r\n");
    return 1; /* failed */
  }

  if ( (cols==0) && (lines==0) && (retry==0) ) {
    printf("Syntax error in 'MODE LPTn ...', please check 'MODE /?'.\r\n");
    return 1;
  }

#ifdef DMODE2 /* moved to DMODE2 */
  printf("MODE LPT%d cols=%d lines=%d retry=%c\r\n",
    pnum+1, cols, lines, retry ? retry : '-');
#endif

  if (lines!=0) {
    lp(27); /* ESC */
    if (lines<=6) {
      lp('2'); /* ESC 2: 6lpi */
    } else {
      lp('0'); /* ESC 0: 8lpi */
    }
#ifdef VMODE
    printf("Sent ESC %d (select %d lines per inch) to printer.\r\n",
      (lines==6) ? 2 : 0, (lines==6) ? 6 : 8);
#endif
  } /* lpi selection */

  /* Escape sequences for XX cpi, Epson / generic (Google & experience) */
  /* ESC P 10cpi ( 80 cols!) (usually the default) */
  /* ESC M 12cpi ( 96 cols?) (if 12cpi n/a: 120/132 cols, 15cpi) */
  /* ESC g 15cpi (120 cols?) (only if 12cpi (!) fonts exist as well) */

  /* Proprinter uses other codes: 0x18 10cpi, ESC : 12cpi, ESC g 15cpi */
  /* Epson also has: [ESC] 0x0f compressed (17/20/??cpi, ESC is optional) */
  /* ... and 0x18 ends compressed mode. Compression hint by Jeremy Davis. */

  if (cols!=0) {
    if (cols==80) cols=10;
    if (cols==132) cols=15;
    if (cols>20) cols=cols/8;	/* assume 8 inch wide printing area */

#ifdef VMODE
    printf("Sending 0x%2.2x ESC %c to the printer to select %d cpi.\r\n",
      (cols < 16) ? 0x18 : 0x0f,
      ((cols==10) || (cols==17)) ? 'P' :
        ( ((cols==12) || (cols==20)) ? 'M' : 'g' ),
      cols);
#endif
    switch (cols) {
      case 10: lp(0x18); lp(27); lp('P'); /* noncompressed ESC P 10cpi */
        break;
      case 12: lp(0x18); lp(27); lp('M'); /* noncompressed ESC M 12cpi */
        break;
      case 15: lp(0x18); lp(27); lp('g'); /* noncompressed ESC g 15cpi */
        break;
      case 17: lp(0x0f); lp(27); lp('P'); /* compressed ESC P 17cpi */
        break;
      case 20: lp(0x0f); lp(27); lp('M'); /* compressed ESC M 20cpi */
        break;
      default: printf("No ESC sequence known for %dcpi, sorry. Known:\r\n",
        cols);
        printf("Prefix 0x18 uncompressed 0x0f compressed\r\n");
        printf("ESC P  10cpi (80 cols)   17cpi\r\n");
        printf("ESC M  12cpi             20cpi\r\n");
        printf("ESC g  15cpi (100+ cols) invalid\r\n");
    } /* switch */
  } /* cpi selection */

#undef lp

  return 0;
} /* printer */

