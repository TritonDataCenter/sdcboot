/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: Option parsing helpers */

#include "mode.h"

static char * helptext[] = {
  "New FreeDOS MODE by Eric Auer 2003-2005. License: GPL.   (version " VERSION ")",
  "MODE [device] [/STA[TUS]]                (show status of one or all devices)",
  "MODE LPTn[:] cols[,[lines][,retry]]     (cols or cpi, 6/8 lpi, retry p or n)",
  "MODE LPTn[:] [COLS=...] [LINES=...] [RETRY=...] (retry: p infinite / n none)",
  "MODE LPTn[:]=[COMn[:]|NUL]     (redirect printer data to serial port or NUL)",
  "MODE COMn[:] baud,parity,data,stop,retry              (empty values allowed)",
  "MODE COMn[:] [BAUD[HARD]=...] [PARITY=...] [DATA=...] [STOP=...] [RETRY=...]",
  "  Baud can be abbreviated to unique prefix,  parity can be o/e/n/s/m,  the",
  "  latter 2 mean space/mark, data can be 5..8, stop 1..2. Retry is IGNORED!",
#ifdef VMODE
  "  PLANNED: Retry b/e/r -> busy/error/ready if busy, p/n infinite/no retry.",
#endif
  "MODE CON[:] [CP|CODEPAGE] [/STA[TUS]]       (FreeDOS DISPLAY must be loaded)",
  "MODE CON[:] [CP|CODEPAGE] REF[RESH]                          (needs DISPLAY)",
  "MODE CON[:] [CP|CODEPAGE] SEL[ECT]=number                    (needs DISPLAY)",
  "MODE CON[:] [CP|CODEPAGE] PREP[ARE]=((codepage) filename)    (needs DISPLAY)",
  "  Use PREP=((,cp2,cp3,,cp5) ...) to prep codepages in other buffers.",
/* "  '...' can be one or more comma separated items:  numbers or empty strings.", */
/* mode 2003/2004 and display 0.10/0.11 only supported ONE non-empty codepage item */
/* "  Currently only CON device supported for CODEPAGE operations!",               */
  "MODE [40|80|BW40|BW80|CO40|CO80|MONO][,rows]  (rows can be 25, 28, 43 or 50)",
  "  Use 8, 14 or 16 as 'rows' value if you only want to change the font.",
  "MODE [CO40|CO80|...],[R|L][,T] (shift CGA left/right, T is interactive mode)",
  "MODE CON[:] [NUMLOCK|CAPSLOCK|SCROLLLOCK|SWITCHAR]=value",
  "  Value can be: + or - for the locks or a character for switchar.",
  "MODE CON[:] [COLS=...] [LINES=...] (possible values depend on your hardware)",
  "MODE CON[:] [RATE=...] [DELAY=...]        (default rate 20, default delay 1)",
  "  Rate can be 1..32 for 2..30 char/sec,  delay can be 1..4 for 1/4..4/4 sec.",
  NULL
}; /* helptext */

	/* TODO: use KITTEN i18n library! */

/* ------------------------------------------------ */

/* skip over whitespace, return NULL if hitting EOF */
char * skipspace(char * str)
{
  if (str == NULL)
    return str; /* the NULL string is empty, too */

  while (!isgraph(str[0])) { /* skip over leading whitespace */
    if ((str[0] == 13) || (str[0] == 0)) { /* nothing but whitespace? */
      return NULL;
    }
    str++;
  }
  return str;
} /* skipspace */

/* ------------------------------------------------ */

/* find a numerical or one-char value after the label string */
unsigned int grabarg(char * str, const char * label)
{
  str = strstr(str, label);
  if (str == NULL)
    return 0; /* failed */
  str = skipspace(str+strlen(label));
  if (isdigit(str[0])) {
    return (int)(0xffff & atol(str)); /* numerical argument */
  } else {
    return (int)str[0]; /* single char argument */
  }
} /* grabarg */

/* ------------------------------------------------ */

/* find a numerical or one-char value after the nth comma */
unsigned int posarg(char * str, int commas)
{
  char * termp;
  char term;
  unsigned int value;

  while (commas>0) {
    str = strchr(str, ',');
    if (str == NULL)
      return 0; /* failed */
    str++; /* advance past the comma */
    commas--;
  }
  str = skipspace(str);

  if (str==NULL) { /* string ended at the comma that would start the value */
    return 0;
  }
  
  termp = str;
  while ( (termp[0]!=0) && (termp[0]!=' ') && (termp[0]!=',') ) {
    termp++; /* find END of argument (next space or comma) */
  }
  term = termp[0]; /* save char */
  termp[0] = 0; /* temporarily terminate string here */

  if (isdigit(str[0])) {
    value = (int)(0xffff & atol(str)); /* numerical argument */
  } else {
    value = (int)str[0]; /* single char argument */
  }
  
  /* printf("[%s] -> %u\r\n", str, value); */
  termp[0] = term; /* restore string */
  return value;
} /* posarg */

/* ------------------------------------------------ */

/* translate char retry B/E/R/P/N into value 0..4 or -1 */
int xlatretry(char retry)
{
#ifdef DMODE
  static char * retryname[] = {
    "report B_USY", "report E_RROR", "report R_EADY",
    "INFINITE retry (P)", "N_O retry"
  }; /* retryname */
#endif
  int i = -1;

  switch (retry) {
    case 0: /* default: return BUSY when printer is BUSY */
      return 0;
    case 'B':
      i = 0;
      break;
    case 'E':
      i = 1;
      break;
    case 'R':
      i = 2;
      break;
    case 'P':
      i = 3;
      break;
    case 'N':
      i = 4;
      break;
  } /* switch */

#ifdef DMODE
  if (i>=0) printf("User selected reaction on busy port: %s\r\n",
    retryname[i]);
#endif

  return i;
} /* xlatretry */

/* ------------------------------------------------ */

/* show the help screen */
void help(void)
{
  int i;
  for (i=0; helptext[i]!=NULL; i++) {
    printf("%s\r\n", helptext[i]);
  }
  return;
} /* help */

