/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: drivers for several aspects of CON */
/* We just call modecp functions for codepage things */

#include "mode.h"

#ifndef VIDMAXROW
#define VIDMAXROW peekb(0x40,0x84)
#endif

static union REGS r;

/* ------------------------------------------------ */

/*
 * ...Added 11/2004 (tables from Ralf Browns Interrupt List 61)...
Bitfields for AMI Hi-Flex BIOS and HUNTER 16 keyboard typematic data:
same values for 8042 command f3h, at least * also for all int 16.3:
Also stored at (40:[e]):[6e] in newer BIOSes, low byte is repeat rate:
0*x  1    2x   3    4x   5    6    7
30.0 26.7 24.0 21.8 20.0 18.5 17.1 16.0
8x   9    10x  11   12*x 13   14   15x
15.0 13.3 12.0 10.9 10.0 9.2  8.6  8.0
16   17   18x  19   20   21   22   23
7.5  6.7  6.0  5.5  5.0  4.6  4.3  4.0
24   25   26   27   28   29   30   31*
3.7  3.3  3.0  2.7  2.5  2.3  2.1  2.0
Conclusion: 2 high bits select division by 1/2/4/8.

In AWARD and AMI WinBIOS CMOS: 0..7 = 6,8,10,12,15,20,24,30 cps

HP Vectra (8-25 MHz CPU) ... delay: 1/60sec, 150ms, 1/3.5s, 1/2.4s,
550ms, ... 8 = 1083ms ...15 = 2017 ms. HP Vectra EX-BIOS typematic:
0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
60  30  20  15  12  10  8.6 7.5 6.7 6.0 5.5 5.0 4.6 4.3 4.0 infty
 *
 */

/* translate BIOS code 0..31 to frequency, unit 0.1 chars/sec */
int repeat_xlate(int code);
int repeat_xlate(int code)
{
  int dcps_for_rate[] = { 300, 266, 240, 220, 200, 184, 172, 160 };
  return ( (dcps_for_rate[code & 7]) >> (code >> 3) );
} /* repeat_xlate */

/* ------------------------------------------------ */

/* sub-parser for CODEPAGE commands: REFRESH, SELECT=n,  */
/* PREPARE=((...) file) and (void) which displays status */
/* "..." can be 1 or more "," separated numbers, empty   */
/* numbers are possible: ((123,,456) foo.txt)            */
int modeconcp(char * what)
{
  unsigned int cp = 0xffff;

  if ( what == NULL ) { /* normal "0 is okay" errorlevels - 30nov2004 */
     return (ShowStatus() == 0) ? 1 : 0; /* return 1 if no DISPLAY */
  }

  if ( !strncmp(what, "/STA", 4) ) { /* special errorlevels - 30nov2004 */
     cp = ShowStatus();
     if (cp == 0)
         return 1; /* errorlevel 1 means failure */
     if (cp == 0xffff)
         return 0; /* errorlevel 0 means okay but no codepage */
     return (cp & 255); /* codepage modulo 256 (hope we do not hit 0 or 1) */
     /* ShowStatus returns 0 if no DISPLAY, else codepage, or -1 if none */
  }

  if ( /* (!strncmp(what, "REFRESH", 7)) || */
       (!strncmp(what, "REF", 3))) {	/* short version allowed 20aug2004 */
     cp = GetCodePage();
     if ((cp == 0xffff) || (cp == 0xfffe) || (cp == 0)) {
       printf("No codepage active - refresh impossible\r\n");
       if (cp == 0xfffe)
         printf("No compatible FreeDOS DISPLAY loaded.\r\n");
       return -1;
     }
     printf("Refreshing codepage number %d\n", cp);
     return (CodePageSelect(cp) == 1) ? 0 : 1; /* updated 30nov2004 */
     /* -1 no DISPLAY, 1 success, other values are error numbers */
  }

  cp = grabarg(what, "SELECT=");
  if (!cp) cp = grabarg(what, "SEL="); /* SEL, not SP */
  if (!cp) cp = grabarg(what, "SELECT");
  if (!cp) cp = grabarg(what, "SEL");
  if (cp) {
#ifdef DMODE2
    printf("Calling CodePageSelect(%d);\n", cp);
#endif    
    return (CodePageSelect(cp) == 1) ? 0 : 1; /* updated 30nov2004 */
  }

  if (  (!strncmp(what, "PREPARE=", 8))
     || (!strncmp(what, "PREP=",    5)) 
     || (!strncmp(what, "PREPARE",  7))
     || (!strncmp(what, "PREP",     4)) ) {
    char * numbers = strchr(what, '(') + 1;
    char * fname;
    char * eol;
    if (numbers == NULL) {
      printf("Syntax error: Opening '(' missing!\n");
      return -1;
    }
    numbers = skipspace(numbers);
    fname = skipspace(strchr(numbers,')')+1);
    if (fname == NULL) {
      printf("Syntax error: Closing ')' of (numbers,...) missing!\n");
      return -1;
    }
    eol = strchr(numbers,')') + 1;
    if (eol[0]!=' ') {
      printf("Syntax error: space expected before filename.\n");
      return -1;
    }
    eol[0] = '\0'; /* split string after first ')' */
    eol = strrchr(fname, ')');
    if (eol == NULL) {
      printf("Syntax error: Closing ')' missing after filename!\n");
      return -1;
    }
    eol[0] = '\0'; /* remove trailing ')' */
    /* assumes no spaces between filename and trailing ')' */
    if ( (strchr(fname, ' ') != NULL) || (strchr(fname, '\t') != NULL) ) {
      printf("Syntax error: No spaces allowed in filename!\n");
    }
#ifdef DMODE2
    printf("Calling CodepagePrepare('%s', '%s');\n", numbers, fname);
#endif    
    return CodePagePrepare(numbers, fname);
  }

  printf("Unexpected CODEPAGE command - syntax error?");
  return -1;

} /* modeconcp */

/* ------------------------------------------------ */

/* handle console related commands */
/* show status if no argument or argument "/STA..." */
/* chains to CODEPAGE code if first argument "CP" or "CODEPAGE" */
/* otherwise, keyboard repeat OR screen size can be set: */
/* style 1: cols=... lines=... (40/80 and 25/43/50) */
/* style 2: rate=... delay=... (1..32 and 1..4) */
int console(char * what)
{
  unsigned int cols, lines, rate, delay, i;

  if ( (what == NULL) || (!strncmp(what, "/STA", 4)) ) {
    unsigned int j;
    printf("*** CONSOLE STATUS ***\r\n");
    (void) ShowStatus(); /* show codepage status, if applicable */
    r.x.ax = 0x0306; /* get keyboard repeat settings */
    r.x.bx = 0xffff; /* to notice if function not possible (common!) */
    int86(0x16, &r, &r);
    if (r.x.bx == 0xffff) {
#ifdef DMODE2 /* changed to DMODE2 */
      printf("BIOS did not tell us the current keyboard repeat rate and delay.\r\n");
#endif
    } else {
      i = r.h.bh;
      j = r.h.bl;
      printf("Keyboard repeat after %u msec, rate %u (%d.%d/s).\r\n",
        (i+1)*250, 32-j, repeat_xlate(j)/10, repeat_xlate(j)%10);
        /* yes it is 32-j, because default 20 corresponds to BIOSes 12 */
    }
    lines = VIDMAXROW+1;
    if (lines < 20) /* CGA/Mono does not set this value */
      lines = 25;
    r.h.ah = 0x0f; /* get current video mode */
    int86(0x10, &r, &r);
    cols = r.h.ah;
    i = r.h.al & 0x7f; /* current video mode, without "no clear" flag. */
    printf("Screen size: %u columns, %u rows, mode %u, font height %u.\r\n",
      cols, lines, i,
      (unsigned int)peekb(0x40, 0x85)); /* font height "any" on CGA/Mono */
    r.x.ax = 0x3700; /* get switchar */
    r.x.dx = 0;
    int86(0x21, &r, &r);
    if (r.x.dx & 0xff) { /* could we read it? */
      printf("Switchar is '%c'. ", r.x.dx & 0xff);
    }
    i = peekb(0x40,0x17);
    printf("Numlock is %s. Scrolllock is %s. Capslock is %s.\r\n",
      (i & 0x20) ? "ON" : "off",
      (i & 0x10) ? "ON" : "off",
      (i & 0x40) ? "ON" : "off");
    return 0;
  } /* status display */

  if ( !strncmp(what, "CP", 2) ) {
#ifdef DMODE2
    printf("MODE CON CODEPAGE '%s'\r\n", skipspace(what+2));
#endif
    return modeconcp(skipspace(what+2));
  }

  if ( !strncmp(what, "CODEPAGE", 8) ) {
#ifdef DMODE2
    printf("MODE CON CODEPAGE '%s'\r\n", skipspace(what+8));
#endif
    return modeconcp(skipspace(what+8));
  }

  i = grabarg(what,"SWITCHAR=");
  if (i) {
    r.x.ax = 0x3701; /* set switchar */
    r.x.dx = i; /* actually only DL is used */
    int86(0x21, &r, &r);
    r.x.ax = 0x3700; /* get switchar */
    r.x.dx = 0;
    int86(0x21, &r, &r);
    if ((r.x.dx ^ i) & 0xff) { /* unexpected DL? */
      printf("could not set switchar!\n");
      return 1;
    } else
      printf("switchar set to %c\n", r.x.dx & 0xff);
    return 0;
  }

  i = grabarg(what,"NUMLOCK=");
  if (i) {
    pokeb(0x40, 0x17, (peekb(0x40,0x17) & 0xdf /* ~0x20 */)
      | ((i!='-') ? 0x20 : 0) );
    r.x.ax = 0x0200; /* dummy read of shift flags, to update LEDs */
    int86(0x16, &r, &r);
    return 0;
  }

  i = grabarg(what,"CAPSLOCK=");
  if (i) {
    pokeb(0x40, 0x17, (peekb(0x40,0x17) & 0xbf /* ~0x40 */)
      | ((i!='-') ? 0x40 : 0) );
    r.x.ax = 0x0200; /* dummy read of shift flags, to update LEDs */
    int86(0x16, &r, &r);
    return 0;
  }

  i = grabarg(what,"SCROLLLOCK=");
  if (i) {
    pokeb(0x40, 0x17, (peekb(0x40,0x17) & 0xef /* ~0x10 */)
      | ((i!='-') ? 0x10 : 0) );
    r.x.ax = 0x0200; /* dummy read of shift flags, to update LEDs */
    int86(0x16, &r, &r);
    return 0;
  }

  cols  = grabarg(what, "COLS=");
  lines = grabarg(what, "LINES=");
  rate  = grabarg(what, "RATE=");
  delay = grabarg(what, "DELAY=");

  if ( (cols!=0) && (cols!=40) && (cols!=80) && (cols!=132) ) {
    printf("Columns must be 40 or 80 or 132.\r\n");
    return 1; /* failed */
  }

  if ( (lines!=0) && (lines!=25) && (lines!=43) && (lines!=50) &&
       (lines!=28) && (lines!=8) && (lines!=14) && (lines!=16) &&
       (lines!=30) && (lines!=34) && (lines!=60) ) {
    printf("Lines must be 25, 28, 43 or 50, or font size 8, 14 or 16.\r\n");
    printf("If you have VESA, 30, 34 or 60 lines can be available, too.\r\n");
    return 1; /* failed */
  }

  if ( (rate<0) || (rate>32) ) {
    printf("Repeat rate code must be 1 ... 32.\r\n");
    return 1; /* failed */
  }

  if ( (delay<0) || (delay>4) ) {
    printf("Repeat delay must be 1, 2, 3 or 4 (unit is 0.25 seconds).\r\n");
    return 1; /* failed */
  }

  if ( (delay==0) && (rate==0) && (cols==0) && (lines==0) ) {
    printf("Syntax error in 'MODE CON ...', please check 'MODE /?'.\r\n");
    return 1;
  }

#ifdef DMODE2 /* moved to DMODE2 */
  printf("MODE CON cols=%d lines=%d rate=%d delay=%d\r\n",
    cols, lines, rate, delay);
#endif

  if ( (rate!=0) || (delay!=0) ) {
    if (rate>0) {
      rate = 32-rate; /* turn 1..32 into 31..0 */
    } else {
      rate = 32-20; /* default: rate=20, 10.0 chars per second */
    }
    if (delay==0)
      delay = 2; /* default: 2/4 seconds */
    r.x.ax = 0x0305; /* set keyboard repeat */
    r.h.bh = delay-1; /* unit 250ms, value 0 means 250ms */
    r.h.bl = rate; /* 0 is 30/sec, default 12 is 10/sec, 31 is 2/sec */
    int86(0x16, &r, &r);
    printf("Repeat rate set to %d.%d chars/sec (after %d.%d sec delay).\r\n",
      repeat_xlate(rate)/10, repeat_xlate(rate)%10,
      delay >> 2, (delay & 3) * 25);
  }

  if ( (cols!=0) || (lines!=0) ) { /* change text mode resolution? */
    r.h.ah = 0x0f; /* get video mode */
    int86(0x10, &r, &r);
    if ((cols) && (cols != r.h.ah)) { /* column count not as wanted? */
      i = r.h.al & 0x7f;  /* current video mode */
#if 0
      /* *** r.h.al &= 0x80; <--- preserve "do not clear screen" bit *** */
#else
      r.h.al = 0;
#endif
      if ( (i < 2) && (cols==80) ) { /* 40x... mode but 80x... requested? */
        i = (i==1) ? 3 : 2; /* mode 2/3 work on MONO, too. */
      }
      if ( (i > 1) && (cols==40) ) { /* 80x... mode but 40x... requested? */
        i = ( (i==2) || (i==7) ) ? 0 : 1; /* 0 if mode was 2 or 7, 1 else */
      }
      r.h.ah = 0;  /* set video mode */
      r.h.al |= i; /* the new mode */
      int86(0x10, &r, &r);
    } /* mode change needed: column count had to change */

    r.h.ah = 0x0f; /* get video mode */
    int86(0x10, &r, &r);
    i = 1;

    if ( ( (cols) && (cols != r.h.ah) ) /* column count still not okay? */ ||
         (cols==132) || (lines==30) || (lines==34) || (lines==60) )
      i = set_vesa(cols, lines); /* SELECT VESA TEXT MODE */
    if (!i)
      return 0; /* already as desired */
    return set_lines(lines); /* SET NUMBER OF TEXT ROWS */
  }

  return 0;
} /* console */

/* ------------------------------------------------ */

/* handle video mode related commands */
/* optional argument: ",rows" (25/43/50) */
/* mode is the BIOS mode number, +10 for "autopick mono/color" */
/* ... and finally 14 for "keep mode" */
int screen(int mode, char * what)
{
  static volatile unsigned char far * sysinfop =
    (unsigned char far *)MK_FP(0x40, 0x10);
  unsigned int lines = 0;
  /* unsigned int flags = SYSINFO & 0x30; ... */
  unsigned int havevga = 0;

  if (mode>=10) { /* auto-select bw?0/co?0 */
    mode -= 10;
    r.h.ah = 0x0f; /* get mode */
    int86(0x10, &r, &r);
    /* AH is the column count, AL the mode, BH the page now */

    if (mode == 4)
      mode = r.h.al; /* keep mode as-is but force to TEXT */

    switch (r.h.al) {
      case 0x07: /* mono text */
      case 0x0f: /* mono graphics */
        mode = 7; /* MONO */
        break;
      case 0:    /* below VGA: bw40 (else identical to co40) */
      case 2:    /* below VGA: bw80 (else identical to co80) */
        mode = (mode & 2) ? 2 : 0; /* BW80 or BW40 */
        break;
      default:
        mode = (mode & 2) ? 3 : 1; /* CO80 or CO40 */
    } /* switch */

  } /* auto-select bw?0/co?0/mono */

  if (mode==4) /* not 0..3? */
    mode=7; /* adjust MONO mode number to match mode.c calling style */

  if ((mode == 5) || (mode == 6) || (mode < 0) || (mode > 7))
    return -1; /* invalid mode */

  if (mode == 7) {
    if ( testCRTC(0x3b4) && testCRTC(0x3d4) ) { /* 2 active CRTCs found? */
      sysinfop[0] |= 0x30; /* force MONO mode (if mono CARD installed!) */
      /* 0x00: card-with-BIOS, 0x10 CGA40, 0x20 CGA80, 0x30 MONO */
      /* the sysinfop trick is needed to switch if TWO cards exist... */
    } /* dual monitor */
    /* other special MONO preparations needed if single monitor? */
  } else {
    if ( testCRTC(0x3b4) && testCRTC(0x3d4) ) { /* 2 active CRTCs found? */
      r.x.ax = 0x1a00; /* get VGA display combination - VGA install check */
      int86(0x10, &r, &r);
      /* returned active / alternate display in BL / BH (use al=1 to SET...) */
      /* 0 none, 1 mono, 2 CGA, 4..5 EGA 6 PGA 7..8 VGA a..c MCGA */
      if (r.h.al == 0x1a)
        havevga = 1; /* VGA found */
      sysinfop[0] = (sysinfop[0] & ~0x30) |
        ( (havevga==1) ? 0 : 0x20 ); /* switch to EGA/VGA BIOS or CGA80. */
        /* EGA seems to work fine in CGA80 mode - we only check for VGA. */
    } /* dual monitor */
    /* other special COLOR preparations needed if single monitor? */
  }

  /* how do we detect cases where ONLY mono or ONLY color works? */
  /* single monitor VGA can be in either mode but still can switch! */

  r.h.ah = 0; /* set mode */
  r.h.al = mode;
  int86(0x10, &r, &r);

#ifdef DMODE2 /* moved to DMODE2 */
  printf("MODE: screen mode %d", mode);
  if (lines) printf(" lines %d", lines);
  printf("\r\n");
#endif
#ifdef VMODE  
  if ( testCRTC(0x3b4) && testCRTC(0x3d4) ) { /* 2 active CRTCs found? */
    printf("Dual monitor system detected (MONO plus CGA-or-newer).\r\n");
  } /* dual monitor */
#endif

  if ((what!=NULL) && (what[0]==',')) { /* start with comma? */
    what++;

    if ((what[0] == 'R') || (what[0] == 'L')) { /* CGA shifting */
      int crtc = peek(0x40, 0x63); /* CRTC base - if dual, current */
      int direction = (what[0] == 'R') ? -1 : 1;
      int interactive = 0;

      what++;
      what = skipspace(what);
      if ((what != NULL) && (what[1] == 'T')) /* is the remainder ",T" ? */
        interactive = 1;

      r.h.ah = 0x12; /* ah=0x12, bl=0x10: get EGA info */
      r.x.bx = 0xff10; /* init bh to -1 */
      int86(0x10, &r, &r);
      if (!(r.x.bx & 0xf0f0)) /* on EGA, returned BH and BL are both < 0x10 */
        havevga = 1; /* EGA is as bad as VGA for shifting */
      if (havevga) {
        printf("< You cannot shift the display on EGA, >");
        if (mode < 2)
          printf("\r\n");
        printf("< VGA or newer systems with this MODE! >\r\n");
#ifdef DMODE2
        printf("Highest debugging mode -> trying to shift anyway.\r\n");
#else
        return -1;
#endif
      }

      if (interactive) {
        int i, pos;
        pos = (mode >= 2) ? 90 : 45; /* center position */
        do {
          pos += direction;
          printf("<234567890<<<<<<<<<<");
          if (mode >= 2) /* 80 columns? */
            for (i = 0; i < (40/2); i++) printf("<>");
          printf(">>>>>>>>>>123456789>\r\n");
          if ( (pos > ((mode >= 2) ? 103 : 46)) ||
               (pos < ((mode >= 2) ? 81 : 41)) ) {
            printf("You tried to shift beyond the\r\n");
            printf("allowed range. Centering again.\r\n");
            pos = (mode >= 2) ? 90 : 45;
          }
          outp(crtc+4, inp(crtc+4) & ~8); /* disable video signal */
          outp(crtc, 2); /* hsync position */
          outp(crtc+1, pos);             /* update position */
          outp(crtc+4, inp(crtc+4) | 8); /* enable video signal */
#ifdef DMODE
          printf("Current hsync pulse position: %d\r\n", pos);
#endif
          printf("Shift more? Y(es), N(o), C(enter)? ");
          do {
            i = toupper(getchar()); /* get 1 char string, with echo */
            /* this actually waits for ENTER after the char  */
            /* alternatively, you would use conio.h getche() */
          } while ((i != 'C') && (i != 'Y') && (i != 'N'));
          /* printf("\r\n"); */
          if (i == 'C') /* abort shifting attempt? */
            pos = ((mode >= 2) ? 90 : 45) /* then jump back to center */
              - direction;
          if (i == 'N')
            interactive = 0; /* done with shifting */
        } while (interactive);
      } else {
        outp(crtc+4, inp(crtc+4) & ~8); /* disable video signal */
        outp(crtc, 2); /* hsync position */
        outp(crtc+1, ((mode >= 2) ? 90 : 45) + direction); /* 1 from center */
        outp(crtc+4, inp(crtc+4) | 8); /* enable video signal */
      }

      return 0; /* worked, kind of ;-) */
    } /* CGA shifting (21 feb 2004) thanks to mc6848 information found on */
      /* http://andercheran.aiind.upv.es/~amstrad/docs/mc6845/mc6845.htm  */

    lines = atoi(what);
    if ( (lines!=0) && (lines!=25) && (lines!=43) && (lines!=50) &&
         (lines!=28) && (lines!=8) && (lines!=14) && (lines!=16) &&
         (lines!=30) && (lines!=34) && (lines!=60) ) {
      printf("Lines must be 25, 28, 43 or 50, or font size 8, 14 or 16.\r\n");
      printf("If you have VESA, 30, 34 or 60 lines can be available, too.\r\n");
      return 1; /* failed */
    }

    return set_lines(lines); /* SET NUMBER OF TEXT ROWS */
    /* this is triggered by "MODE co80,43" syntax, no column setting */
  } /* argument given */

  return 0;
} /* screen */

