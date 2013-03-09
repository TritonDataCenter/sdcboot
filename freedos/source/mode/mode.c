/* Partial implementation of the DOS MODE command by Eric Auer 2003 ..., */
/* for inclusion in MODECON (by Aitor Merino), which does the codepages! */
/* This code is primarily for Aitor, but you may use it as is under the  */
/* terms of the GNU GPL (v2) license if you want to, see www.gnu.org ... */
/* If you have questions, mail me: eric%coli.uni-sb.de (replace % by @). */

/* This file: MAIN file */

#define MAIN 1
#include "mode.h"

	/* TODO: use KITTEN i18n library! */

/* find main keyword and jump to matching subroutine(s) */
int main(void) /* (int argc, char ** argv) */
{
  static char * topics[] = { "/?" /* 0 */ , "LPT", "COM", "CON" /* 3 */,
    "BW40", "CO40", "BW80", "CO80", "MONO", "40", "80", "," /* 11 */, NULL
  }; /* topic array: topic string numbering is hardcoded below! */
  int i,j;
  char far * pspargs = (char far *)MK_FP(_psp, 0x81);
  char argstr[128];
  char * args;
  int far * serports = (int far *)MK_FP(0x40, 0);
  int far * parports = (int far *)MK_FP(0x40, 8);
  /* useful: n = atol(string) */
  
  j = 0;
  for (i=0; (pspargs[i]!=0) && (pspargs[i]!=13); i++) { /* until zero/CR */
    argstr[i] = toupper(pspargs[i]);   /* convert to upper case */
    if (argstr[i] == 9) argstr[i]=' '; /* convert tab to space  */
    if (isalpha(argstr[i])) j++;      /* count alphabetic chars */
    if ( (argstr[i] == ':') && (j<7) )
      argstr[i] = ' '; /* convert : to space but only in first args */
      /* strip : in "lpt1:=com1:" but not in "con cp prep=((,1) c:...)" */
  }
  argstr[i] = 0; /* use zero to terminate string */

  args = skipspace(argstr);
  if ( (args == NULL) || (!strncmp(args, "/STA", 4)) ) {
      int n=0;
      for (i=0; i<4; i++) {
        if (serports[i] != 0)
	  n |= serial(i, NULL); /* SERIAL PORT INFORMATION */

      }
      for (i=0; i<4; i++) {
        if (parports[i] != 0) {
	  n |= printer(i, NULL); /* PRINTER PORT INFORMATION */
	  /* (includes redirection information display) */
	} else {
	  printf("*** VIRTUAL PRINTER PORT %d INFORMATION ***\r\n", i+1);
	  if (!DescribeTSR(i)) /* GHOST PRINTER PORT INFORMATION */
	    printf("Free for redirection to serial ports.\r\n");
	  /* (redirection information (and retry information) */
	}

      }
      n |= console(NULL); /* CONSOLE INFORMATION */
      exit(n); /* success only if all information requests succeeded */
  }

  for (i=0; topics[i]!=NULL; i++) { /* find topic selecting first word */
    if ( !strncmp(args, topics[i], strlen(topics[i])) ) { /* THAT topic? */
      int n;
#ifdef DMODE2
      printf("Topic: %s\r\n",topics[i]);
#endif
      if (i != 11) { /* 11 is the number of the "," pseudo-keyword */
        for (n=0; n < (int)strlen(topics[i]); n++) {
	  args++; /* skip over topic keyword itself */
        }
      } /* skip keyword unless "," topic which is used for screen setup */

      switch (i) {
        case 0: /* /? */
	  help(); /* SHOW HELP SCREEN */
	  i=0; /* return success */
          break;
        case 1: /* LPTx */
	  if ((args[0]<'1') || (args[0]>'4')) {
	    printf("Invalid printer port number %c\r\n", args[0]);
	    /* actually LPT4 is allowed only as redirection endpoint */
	    exit(1);
          }
	  i = args[0] - '1';
          args++;
          args = skipspace(args);
          if ((args == NULL) || (args[0] == '=')
            || (!strncmp(args, "/STA",4)) ) {
            /* no sanity check of printer number for redirection */
            /* neither for status display */
          } else {
	    if ((i == ('4'-'1')) || (parports[i] == 0)) {
	      /* LPT4 is no real printer port, it is not configurable */
	      printf("LPT%d not existing in this PC (but free for redirection).\r\n", i+1);
	      exit(1); /* return error - do not check what real command was */
	    }
	  }
	  i = printer(i, args); /* PRINTER PORT COMMAND */
          break;
        case 2: /* COMx */
	  if ((args[0]<'1') || (args[0]>'4')) {
	    printf("Invalid serial port number %c\r\n", args[0]);
            exit(1);
          }
	  i = args[0] - '1';
	  args++;
	  if (serports[i] == 0) {
	    printf("COM%d not existing in this PC.\r\n", i+1);
	    exit(1);
	  }
	  i = serial(i, skipspace(args)); /* SERIAL PORT COMMAND */
          break;
        case 3: /* CON */
	  i = console(skipspace(args)); /* CONSOLE COMMAND */
          break;
        case 4: /* BW40 */
        case 5: /* C040 */
        case 6: /* BW80 */
        case 7: /* CO80 */
        case 8: /* MONO */
	  i = screen(i-4, skipspace(args)); /* VIDEO MODE COMMAND */
          /* 0..3 for BIOS modes 0..3, 4 for BIOS mode 7 */
          break;
        case 9:  /* 40 */
          i = screen(10+1,  skipspace(args)); /* VIDEO MODE COMMAND */
          /* 10..13 for BIOS modes 0..3 with auto-select mono/color */
          break;
        case 10: /* 80 */
          i = screen(10+3,  skipspace(args)); /* VIDEO MODE COMMAND */
          break;
        case 11: /* just keep mode ("," pseudo topic */
          i = screen(10+4,  skipspace(args)); /* VIDEO MODE COMMAND */
          break;
#ifdef DMODE
        default:
	  printf("Internal error %d\r\n", i);
#endif
      } /* case */
      exit(i); /* errorlevel from topic handler */
    } /* found topic */
  }

  printf("Unknown device or syntax error! 'MODE /?' shows help.\r\n");
  exit(1);
  return 1;

} /* main */

