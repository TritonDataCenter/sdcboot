/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: control for COMx things */

#include "mode.h"

#define DIVISOR_WORD 1	/* define to use word access for divisor */
 			/* should work on all AT systems with 16550+ */
 			/* 16450 is a faster 8250, 16550+ has FIFOs. */
#define SET_RTS_DTR 1	/* define to force RTS and DTR on when baudhard set */

static union REGS r;

/* handle serial port related commands */
/* show status if no argument or argument "/STA..." */
/* style 1: baud,parity,data,stop,retry (..., o/e/n/s/m, 5..8, 1..2, ...) */
/* style 2: baud=... parity=... data=... stop=... retry=... */
int serial(int snum, char * what)
{
  unsigned int xbaud, baud, xparity, data, stop, xretry;
  char parity, retry;

#ifdef DMODE
  static long int baudlist[] = { 110, 150, 300, 600,
    1200, 2400, 4800, 9600,   19200, 38400L, 57600L, 115200L
  }; /* baudlist */
#endif

  if ( (what == NULL) || (!strncmp(what, "/STA", 4)) ) {
    int i;
    static char * serbits[] = {
      "delta-CTS", "delta-DSR", "RI-trail", "delta-CD",
      "CTS", "DSR", "RI", "CD",
      /* modem: cleartosend, datasetready, ringindicator, carrierdetect */
      "data-received", "overrun", "parity-error", "frame-error",
      "break-received", "xmit-hold-empty", "xmit-shift-empty", "timeout"
    }; /* serbits */
    printf("*** SERIAL PORT %d STATUS ***\r\n", snum+1);

    r.x.ax = 0x0300; /* read status */
    r.x.dx = snum;
    int86(0x14, &r, &r);
    printf("Port status: [ ");
    for (i=15; i>=0; i--) {
      if ( (r.x.ax & (1<<i)) != 0)
        printf("%s ", serbits[i]);
    }
    printf("]\r\n");

#ifndef DMODE2 /* Tell about BAUDHARD=1 only if /STA... given, unless DMODE2 */
    if ( (what != NULL) && (!strncmp(what, "/STA", 4)) ) {
#else
    {
#endif
      printf("Use MODE COM%d BAUDHARD=1 to read *configuration* from UART.\r\n",
        snum+1);
    }

    return 0;
  }

  if (strchr(what,'=') != NULL) {
    baud   = grabarg(what, "BAUD=");
    xbaud   = grabarg(what, "BAUDHARD="); /* new 4/2004 */
    xparity = grabarg(what, "PARITY=");
    data   = grabarg(what, "DATA=");
    stop   = grabarg(what, "STOP=");
    xretry  = grabarg(what, "RETRY=");
  } else {
    what = skipspace(what);
    if ( (what[0]!=',') && (!isdigit(what[0])) ) {
      printf("Syntax error in 'MODE COMn ...', please check 'MODE /?'.\r\n");
      return 1;
    }
    baud   = posarg(what, 0);
    xbaud = 0;
    xparity = posarg(what, 1);
    data   = posarg(what, 2);
    stop   = posarg(what, 3);
    xretry  = posarg(what, 4);
  }

  if ( (baud==0) && (xparity==0) && (data==0) && (stop==0) && (xretry==0)
    && (xbaud==0) ) {
    printf("Syntax error in 'MODE COMn ...', please check 'MODE /?'.\r\n");
    return 1;
  }

  if ( (data>8) || ((data<5) && (data!=0)) ) {
    printf("Data bits must be 5, 6, 7 or 8.\r\n");
    return 1; /* failed */
  }

  if ( (stop>2) || (stop<0) ) {
    printf("Stop bits must be 1 or 2.\r\n");
    printf("(2 stopbits treated as 1.5 if 5 or 6 data bits).\r\n");
    return 1; /* failed */
  }

  retry = (char)xretry;
  xretry = xlatretry(retry);
  if (xretry<0) return 1; /* error */
  if ((xretry==2) || (xretry==3)) /* if busy: 0 busy  1 error  2 ready */
                                  /*          3 keep trying 4 no retry */
    printf("Not yet supported RETRY value - ignored.\r\n");
    /* Reason: We do not hook int 14h at all (yet) ;-) ! */

  parity = (char)xparity;
  switch (parity) {
    case 'N':
      xparity = 0; /* none */
      break;
    case 'O':
      xparity = 1; /* odd */
      break;
    case 'E':
      xparity = 2; /* even (3 for old style API) */
      break;
    case 'S':
      xparity = 3; /* space? (new style API only) */
      break;
    case 'M':
      xparity = 4; /* mark? (new style API only) */
      break;
    case 0:
      xparity = 0; /* default parity setting: no parity */
      parity = '-';
      break;
    default:
      printf("Parity must be N, O, E, S or M (none, odd, even, space, mark).\r\n");
      return 1; /* failed */
  } /* switch */

  if (stop==0)
    stop = (baud==110) ? 2 : 1; /* default number of stop bits */

  if ((baud % 10) == 0) baud /= 10; /* strip at most 2 trailing zeroes */
  if ((baud % 10) == 0) baud /= 10; /* strip at most 2 trailing zeroes */

  switch (baud) {
    case 11:  /*   110 (caveat: 11 could also abbreviate 115200 (*)) */
      baud = 0;
      break;
    case 15:  /*   150 */
      baud = 1;
      break;
    case 3:   /*   300 (caveat: 3 could also abbreviate 38400 (*)) */
      baud = 2;
      break;
    case 6:   /*   600 (*) */
      baud = 3;
      break;
    case 12:  /*  1200 */
      baud = 4;
      break;
    case 0: /* default baud value is 2400 in most DOS versions */
    case 24:  /*  2400 */
      baud = 5;
      break;
    case 48:  /*  4800 */
      baud = 6;
      break;
    case 96:  /*  9600 */
      baud = 7;
      break;
    /* 14400 ??? */
    case 19:  /* 19200 */
    case 192:
      baud = 8; /* from here on we have to use the new API */
      break;
    /* 28800 ??? */
    case 38:  /* 38400 (supported?) */
    case 384:
      baud = 9;
      break;
    case 57:  /* 57600 (supported?) */
    case 576:
      baud = 10;
      break;
    case 115: /* 115200 (supported?) */
    case 1152:
    case 0xc200: /* 115200 & 0xffff */
      baud = 11;
      break;
    default:
      printf("BIOS-Unsupported baud rate, sorry.\r\n");
      printf("Please use BAUDHARD=value, with value=baud rate / 100\r\n");
      printf("e.g. BAUDHARD=1152 for 115200 baud, to program baud rate\r\n");
      printf("directly into UART hardware. BAUDHARD=1 reads UART config!\r\n");
      return 1;
  } /* switch */
  if ((baud > 8) && (baud < 12)) {
    printf("If your BIOS fails to set this baud rate properly, try\r\n");
    printf("using BAUDHARD=%d instead to program UART hardware directly.\r\n",
      (baud>9) ? ((baud==11) ? 1152 : 576) : 384);
  }

  /* (*) MS MODE only allows full values or 2 or 3 digit abbreviations, */
  /*  while we also allow "omit trailing zeroes" style 1 digit abbrev.! */

  if (data==0)
    data = 8; /* default number of data bits */

  r.x.dx = snum; /* port number */
  if ( (baud > 7) || (xparity > 2) ) { /* need new API? */
    r.x.ax = 0x0401; /* extended setup, no break */
    r.h.bh = xparity;
    r.h.bl = stop-1; /* 0 means 1, 1 means 2 (1.5 if 5 data bits) stop bits */
    r.h.ch = data-5; /* 0..3 means 5..8 data bits */
    r.h.cl = baud;   /* baud rate selector, values 0..8 same for all BIOSes */
    if (xbaud!=1)   /* do not SET before READING config! */
      int86(0x14, &r, &r);
    if (r.x.ax == 0x1954) { /* we got "FOSSIL init successful" */
                            /* which means that BX / CX had no effect */
      printf("FOSSIL driver detected, MODE could not configure port!\r\n");
      printf("You can use MODE with FOSSIL for max 9600 baud / normal parity only.\r\n");
    }
    if (r.x.ax == 0xaa55) {
      printf("MBBIOS detected, please use only 110-9600 baud and set\r\n");
      printf("the MBBIOS high speed option to translate to 9600-330400 baud.\r\n");
      r.x.ax = 0x1954;
    }
  } else {
    if (xparity==2)
      xparity=3; /* translate to old style value */
    r.h.ah = 0; /* initialize port */
    r.h.al = (baud<<5) | (xparity<<3) | ((stop-1)<<1) | (data-5);
    if (xbaud!=1)   /* do not SET before READING config! */
      int86(0x14, &r, &r);
  }
  /* returns status in AX */

  if (xbaud) {
    unsigned int sport = peek(0x40, snum+snum);
    unsigned int scratch;

    if (!sport) {
      printf("This serial port has no UART, sorry.\r\n");
      return 1;
    }

    disable();
    scratch = inportb(sport+7);
    outportb(sport+7, 0xea);
    printf("UART is ");
    if (inportb(sport+7) != 0xea) {
      if (xbaud!=1) {
        printf("8250, reading defaults!");
        xbaud = 1;
      } else
        printf("8250.");
    } else {
      outportb(sport+7, scratch);
      printf("16450 or newer."); /* or 8250A */
    }
    enable();

    printf(" %s:\r\n",
      (xbaud==1) ? "Reading parameters" : "Programming baud rate");
    if (xbaud>1152) { /* limit for 1.8432 MHz clock */
      printf("Maximum baud rate is 115200 (BAUDHARD=1152).\r\n");
      xbaud = 1;
    }

    if (xbaud==1) { /* parameter reading */

      /* line control: DLAB break PPP stop WW */
      /* PPP = 1x1 for sticky ~x parity, ??0 for no parity, 0x1 for */
      /* 0 odd / 1 even parity. stop 0 for 1 stop bit, 1 for 1.5 (WW=0) / 2 */
      /* WW 0..3 for 5..8 data bits */
      scratch = inportb(sport+3);
      if (scratch & 0x40) printf(" break,");
      if (scratch & 0x08) {
        printf(" %s parity,", (scratch & 0x20) ?
            ( (scratch & 0x10) ? "sticky low" : "sticky high" ) :
            ( (scratch & 0x10) ? "even" : "odd" ) );
      } else
        printf(" no parity,");
      printf(" %s, %d bit,", ( (scratch & 0x04) ?
          ( (scratch & 0x03) ? "2 stop bits" : "1.5 stop bits" ) :
          "1 stop bit" ),
        5 + (scratch & 0x03) );

      disable();
      outportb(sport+3, inportb(sport+3) | 0x80); /* divisor access */
#ifdef DIVISOR_WORD
      scratch = inport(sport);	/* divisor word */
#else
      scratch = inportb(sport+1); /* high byte */
      scratch <<= 8;
      scratch |= inportb(sport); /* low byte */ /* better read LO before HI? */
#endif
      outportb(sport+3, inportb(sport+3) & 0x7f);
      enable();

      if (scratch) {
        printf(" %ld baud,", 115200UL / scratch);
        xbaud = 1152 / scratch;
      } else {
        printf(" infinite baud rate!?,");
        xbaud = 9999;
      }
      scratch = inportb(sport+4);
      printf(" RTS %s, DTR %s.\r\n",
        ( (scratch & 2) ? "on" : "auto" ),
        ( (scratch & 1) ? "on" : "auto" ) );
      /* Modem control: ? ? (out0) loopback out2=irqenable out1=0 RTS DTR */
      /* (RTS / DTR set means "forced on" instead of "use for handshake") */

    } else {

      /* registers: +0/+1 divisor latch low/high if DLAB=1, +7 scratch...   */
      /* +3 line control +4 modem control +5 line status +6 modem status... */
      scratch = 1152 / xbaud; /* convert into divisor */
      printf(" %ld %s(divisor ", 115200UL / scratch,
        ((115200UL / scratch) != (100UL * xbaud)) ? "(had to round up) " : "" );

      disable();
      outportb(sport+3, inportb(sport+3) | 0x80); /* divisor access */
#ifdef DIVISOR_WORD
      outport(sport, scratch);	/* divisor word */
#else
      outportb(sport, scratch & 0xff); /* low byte */
      outportb(sport+1, scratch >> 8); /* high byte */
#endif
      outportb(sport+3, inportb(sport+3) & 0x7f);
      enable();

      printf("%d).\r\n", scratch);

#if SET_RTS_DTR
      /* to force RTS / DTR to on: */
      outportb(sport+4, inportb(sport+4) | 3);
      printf("Forcing RTS/DTR on.\r\n");
      /* should be done by drivers, e.g. raising on mouse init */
#endif

    } /* baud rate writing */

  } /* direct UART programming: BAUDHARD argument given */

#ifdef DMODE
  printf("MODE COM%d (x)baud=%ld parity=%c data=%d stop=%d (ignored: retry=%c)\r\n",
    snum+1, (xbaud) ? ((long int)xbaud * 100UL) : baudlist[baud],
    parity, data, stop, retry ? retry : '-');
#endif

  /* TODO: send RETRY setting to TSR part */

  return (r.x.ax==0x1954) ? 1 : 0;
} /* serial */

