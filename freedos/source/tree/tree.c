/*
 * TREE command for MS-DOS
 *
 * This command is virtually identical to the MS-DOS TREE command,
 * except that it expands the current directory path instead of
 * showing '.' in the display.
 *
 * Copyright 1995 Dave Dunfield
 * Freely distributable.
 */

/** May 2000, release under GPL, refer to copying for details. **/

/**
 * FreeDOS tree v3.7
 *
 * Modified: refer to history.txt for details
 * Feb 2000 Jeremy Davis <jeremyd@computer.org>
 * Feb 2000 Imre Leber
 * May 2000 Steve Nickolas <dosius@sdf.lonestar.org> Dosius Software Co.
 * June 2000 Joe Cosentino <jayc17@mediaone.net>
 * July 2000 Joe Cosentino <jayc17@mediaone.net>
 * Jan 2001 Jeremy Davis <jeremyd@computer.org>
 * June 2001 Jeremy Davis <jeremyd@computer.org>
 */


/*** Includes **************************************************************/

#define FF_INCLUDE_FUNCTION_DEFS
#include "findfile.h"
#ifndef MICROC           /* Can not include stdio.h multiple times under   */
#include <stdio.h>       /* Micro-C/PC, already included by FileFind.h     */
#include <string.h>      /* Doesn't exist under Micro-C/PC                 */
#include <ctype.h>
#endif

#ifdef USE_CATGETS
#include "catgets.h"     /* Cats message catalog support                   */
#endif

#include "getopt.h"


/*** Defines ***************************************************************/

#define DIRS 500                            /* Depth of directory stacking */
#define DEPTH 50                            /* Depth of scanner recursion  */


/* Note: Requires a catgets implementation that handles C escape codes,
   which means for standard FreeDOS version, Jim Hall's Cats 3.9.4 or
   higher should be used.
*/
#ifdef USE_CATGETS
#define CATOPEN() cat = catopen ("tree", MCLoadAll)
#define CATS(catset, catnum, msg) catgets(cat, catset, catnum, msg)
#define CATCLOSE() catclose(cat)
#else
#define CATOPEN()
#define CATS(catset, catnum, msg) msg
#define CATCLOSE()
#endif

/* Define SHOW_ATTR to include option /DA to enable displaying attributes */
#define SHOW_ATTR

/* Define SHOW_FSIZE to include option /DS to enable displaying file size */
#ifndef MICROC
#define SHOW_FSIZE   /* Displaying file size not supported with Micro-C   */
#endif


/*** Messages and Message Defines ******************************************/

/* main [Set 1] */
#define SET_MAIN 1
#define MSG_NEWLINE 1
#define MSGTXT_NEWLINE "\n"
#define MSG_PATHLISTINGNOLABEL 2
#define MSGTXT_PATHLISTINGNOLABEL "Directory PATH listing\n"
#define MSG_PATHLISTINGWITHLABEL 3
#define MSGTXT_PATHLISTINGWITHLABEL "Directory PATH listing for Volume %s\n"
#define MSG_SERIALNUMBER 4
#define MSGTXT_SERIALNUMBER "Volume serial number is %s\n"
#define MSG_NOSUBDIRS 5
#define MSGTXT_NOSUBDIRS "No subdirectories exist\n\n"

/* Usage [Set 2] */
#define SET_USAGE 2
#define MSG_DESCRIPTION 1
#define MSGTXT_DESCRIPTION "Graphically displays the directory structure of a drive or path.\n"
#define MSG_USAGE 2
#define MSGTXT_USAGE "TREE [drive:][path] [%c%c] [%c%c]\n"
#define MSG_FOPTION 3
#define MSGTXT_FOPTION "   %c%c   Display the names of the files in each directory.\n"
#define MSG_AOPTION 4
#define MSGTXT_AOPTION "   %c%c   Use ASCII instead of extended characters.\n"

/* InvalidUsage [Set 3] */
#define SET_INVALIDUSAGE 3
#define MSG_INVALIDOPTION 1
#define MSGTXT_INVALIDOPTION "Invalid switch - %s\n"
#define MSG_USETREEHELP 2
#define MSGTXT_USETREEHELP "Use TREE %c? for usage information.\n"
#define MSG_TOOMANYOPTIONS 3
#define MSGTXT_TOOMANYOPTIONS "Too many parameters - %s\n"

/* VersionInfo [Set 4] */
#define SET_VERSIONINFO 4
/* MSG 1-7 reserved for pdTree */
#define MSG_CATSCOPYRIGHT 8
#define MSGTXT_CATSCOPYRIGHT "Uses Jim Hall's <jhall@freedos.org> Cats Library\n  version 3.9 Copyright (C) 1999,2000,2001 Jim Hall\n"
/* Dave Dunfield's copyright */
#define MSG_DDS 20
#define MSGTXT_DDS "Copyright 1995, 2000 Dave Dunfield - Freely distributable (2000 released GPL).\n"

/* InvalidDrive [Set 5] */
#define SET_INVALIDDRIVE 5
#define MSG_INVALIDDRIVE 1
#define MSGTXT_INVALIDDRIVE "Invalid drive specification\n"

/* InvalidPath [Set 6] */
#define SET_INVALIDPATH 6
#define MSG_INVALIDPATH 1
#define MSGTXT_INVALIDPATH "Invalid path - %s\n"

/* Set 7 is reserved for pdTree's misc error messages.    */
/* Set 8 is reserved for pdTree's command line arguments. */
/* Set 9 is reserved for pdTree's future use.             */


/*** Globals ***************************************************************/

char
      *parg = 0,                            /* Path argument               */
      path[FF_MAXPATH],                     /* Final path specification    */
      full = 0,                             /* Full listing specified      */
      dirstack[DIRS][MAXFILENAME],          /* Stack of directory names    */
#ifdef SHOW_ATTR
      dirattrstack[DIRS][8],                /* Stack of directory attr     */
#endif
      *actstack[DEPTH];                     /* Stack of active levels      */

char
      *Vline = "\xB3\x20\x20",              /* Vertical line               */
      *Vtee  = "\xC3\xC4\xC4",              /* Vertical/Horizonal tee      */
      *Corn  = "\xC0\xC4\xC4",              /* Vertical/Horizontal corner  */
      *Hline = "\xC4\xC4\xC4";              /* Horizontal line             */

unsigned
      dirptr = 0,                           /* Directory stacking level    */
      level = 0;                            /* Function recursion level    */
      
#ifdef USE_CATGETS
      nl_catd cat;                          /* Catalog descriptor          */
#endif

#ifdef SHOW_ATTR
      int showAttr = 0;                     /* Default to not display attr */
#endif

#ifdef SHOW_FSIZE
      int showFSize = 0;                    /* Default to not display fsize*/
#endif

/*** Functions *************************************************************/

 /**
 * Handle a single directory, recurse to do others
 */
void tree_path(void)
{
      FFDATA ffdata;
      unsigned plen, dirbase, i, j;
      char *ptr;

      /* Get all subdirectory names in this dir */
      dirbase = dirptr;
      plen = strlen(path);
      strcpy(path+plen, "*.*");
      if (!findFirst(path, ADDRESSOF(ffdata)))
      {
        do 
        {
          if (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_DIRECTORY)
          {
            if ( (strcmp(FF_GetFileName(ADDRESSOF(ffdata)), ".") != 0) && 
                 (strcmp(FF_GetFileName(ADDRESSOF(ffdata)), "..") != 0) )
            {
#ifdef SHOW_ATTR
                  if (showAttr)
                  {
                    sprintf(dirattrstack[dirptr], "%c%c%c%c%c ",
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_DIRECTORY) ? 'D' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_ARCHIVE) ? 'A' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_READONLY) ? 'R' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_HIDDEN) ? 'H' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_SYSTEM) ? 'S' : ' '
                    );
                  }
#endif
                  strcpy(dirstack[dirptr++], FF_GetFileName(ADDRESSOF(ffdata)));
            }
          }
        } while(!findNext(ADDRESSOF(ffdata)));
        findClose(ADDRESSOF(ffdata));
      }

      /* Display files in this dir if required */
      actstack[level++] = (dirbase == dirptr) ? "   " : Vline;
      if(full) 
      {
            i = 0;
		if(!findFirst(path, ADDRESSOF(ffdata)))
            {
              do 
              {
                  if(FF_GetAttributes(ADDRESSOF(ffdata)) & (FF_A_DIRECTORY|FF_A_LABEL))
                        continue;
#ifdef SHOW_ATTR
                  if (showAttr)
                  {
                    printf("%c%c%c%c%c ",
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_DIRECTORY) ? 'D' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_ARCHIVE) ? 'A' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_READONLY) ? 'R' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_HIDDEN) ? 'H' : ' ',
                      (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_SYSTEM) ? 'S' : ' '
                    );
                    fflush(stdout);
                  }
#endif
#ifdef SHOW_FSIZE
                  if (showFSize)
                  {
                    printf("%10lu ", FF_GetFileSize(ADDRESSOF(ffdata)));
                    fflush(stdout);
                  }
#endif
                  for(j=0; j < level; ++j)
                        fputs(actstack[j], stdout);
                  i = -1;
                  printf("%s\n", FF_GetFileName(ADDRESSOF(ffdata)));
              } while(!findNext(ADDRESSOF(ffdata)));
              findClose(ADDRESSOF(ffdata));
            }

            if(i)
            {
#ifdef SHOW_ATTR
                  if (showAttr)
                  {
                    printf("      ");
                    fflush(stdout);
                  }
#endif
#ifdef SHOW_FSIZE
                  if (showFSize)
                  {
                    printf("           ");
                    fflush(stdout);
                  }
#endif
                  for(j=0; j < level; ++j)
                        fputs(actstack[j], stdout);
                  putc('\n', stdout); 
            }
      }

      /* Report of no subdirectories exist */
      if((dirbase == dirptr) && (level == 1))
	      printf(CATS(SET_MAIN, MSG_NOSUBDIRS, MSGTXT_NOSUBDIRS));

      /* Recurse into subdirectories */
      for(i=dirbase; i < dirptr; ++i) 
      {
#ifdef SHOW_ATTR
            if (showAttr)
            {
              printf("%s", dirattrstack[i]);
              fflush(stdout);
            }
#endif
#ifdef SHOW_FSIZE
            if (showFSize)
            {
              printf("           ");
              fflush(stdout);
            }
#endif
            actstack[level-1] = ((i+1) != dirptr) ? Vtee : Corn;
            for(j=0; j < level; ++j)
                  fputs(actstack[j], stdout);
            actstack[level-1] = ((i+1) != dirptr) ? Vline : "   ";
            printf("%s\n", ptr = dirstack[i]);
            strcpy(path+plen, ptr);
            strcat(path, "\\");
            tree_path(); 
      }

      /* Restore entry conditions and exit */
      path[plen] = 0;
      dirptr = dirbase;
      --level;
}

/**
 * Displays serial # if exists
 * "Volume serial number is %s\n"
 */
void showSerialNum(CONST char *rootpath)
{
  union media_serial serial;
  char buffer[10];

  if (!getSerialNumber(rootpath, ADDRESSOF(serial)))
  {
#ifdef MICROC
    sprintf(buffer, "%04x:%04x", serial.wSerial.hw, serial.wSerial.lw);
#else
    sprintf(buffer, "%04X:%04X", serial.wSerial.hw, serial.wSerial.lw);
#endif
    printf(CATS(SET_MAIN, MSG_SERIALNUMBER, MSGTXT_SERIALNUMBER), buffer);
  }
  /* else don't print anything if failed to get serial # */
}


/**
 * Main program - parse arguments & start recursive procedure
 */
main(int argc, char *argv[])
{
      int i;
      char *ptr;
      int optionChar;
      char curdir[FF_MAXPATH]; /* If FF_MAXPATH is changed, change getVolumeLabel as well. */
      char rootpath[4];

      CATOPEN();

      /* Parse the command line using getopt */

      while ((optionChar = getopt (argc, argv, "AaFfD:d:")) != EOF)
	{
	  switch(optionChar) 
	    {
	    case 'A' :        /* Ascii - switch BOX characters */
	    case 'a' :
	      Vline = "|  ";
	      Vtee  = "+--";
	      Corn  = "\\--";
	      Hline = "---";
	      break;

	    case 'F' :        /* Select FULL mode */
	    case 'f' :
	      full = -1;
	      break;

	    case 'D' :        /* extra Display mode - show attributes*/
	    case 'd' :
#ifdef SHOW_ATTR
	      showAttr = 1;
	      full = -1;      /* use FULL mode */
	      break;
#endif
#ifdef SHOW_FSIZE
	    case 'S' :
	    case 's' :
	      showFSize = 1;
	      full = -1;      /* use FULL mode */
	      break;
#endif

	    case '?' :        /* Help request */
	    default:
	      printf(CATS(SET_USAGE, MSG_DESCRIPTION, MSGTXT_DESCRIPTION));

	      printf ("\n");
	      printf(CATS(SET_USAGE, MSG_USAGE, MSGTXT_USAGE), '/','F','/','A');
	      printf(CATS(SET_USAGE, MSG_FOPTION, MSGTXT_FOPTION), '/','F');
	      printf(CATS(SET_USAGE, MSG_AOPTION, MSGTXT_AOPTION), '/','A');

	      printf ("\n");
	      printf(CATS(SET_VERSIONINFO, MSG_DDS, MSGTXT_DDS));

#ifdef USE_CATGETS
	      printf ("\n");
	      printf(CATS(SET_VERSIONINFO, MSG_CATSCOPYRIGHT, MSGTXT_CATSCOPYRIGHT));
#endif
	      exit(0);
	      break;
	    } /* switch */
	} /* while */

      /* If no path specified, default to current */
      if(!parg) 
      {
            *(parg = curdir) = '\\';
            getCurrentDirectoryEx(curdir, FF_MAXPATH); 
      }

      /* If no drive name specified, obtain current drive */
      if(parg[1] == ':')
            strcpy(path, parg);
      else 
      {
            *path = getCurrentDrive() + 'A';
            path[1] = ':';
            strcpy(path+2, parg); 
      }

      /* Setup drive and root path, used to get volume label */
      rootpath[0] = *path;
      rootpath[1] = *(path+1);
      rootpath[2] = '\\';
      rootpath[3] = '\0';

      /* Validate drive and get volume label */
      if (getVolumeLabel(rootpath, curdir, FF_MAXPATH))
      {
            /* rootpath[2] = '\0'; */ /* strip off \ before displaying */
            printf(CATS(SET_INVALIDDRIVE, MSG_INVALIDDRIVE, MSGTXT_INVALIDDRIVE));
            CATCLOSE();
            exit(1);
      }

      /* Display volume label of disk */
      if (*curdir) /* valid path, but check for label */
            printf(CATS(SET_MAIN, MSG_PATHLISTINGWITHLABEL, MSGTXT_PATHLISTINGWITHLABEL), curdir);
      else
            printf(CATS(SET_MAIN, MSG_PATHLISTINGNOLABEL, MSGTXT_PATHLISTINGNOLABEL));

      /* Display serial # if exists */
      showSerialNum(rootpath);

      /* Display path */
#ifdef SHOW_ATTR
      if (showAttr)
      {
        printf("      ");
        fflush(stdout);
      }
#endif
#ifdef SHOW_FSIZE
      if (showFSize)
      {
        printf("           ");
        fflush(stdout);
      }
#endif
      printf("%s\n", path);

      /* if no directory, ie want current directory on drive x */
      if (path[2] == '\0')
             strcat(path, ".\\");

      /* Append backslash if not given */
      if (path[strlen(path)-1] != '\\')
            strcat(path, "\\");

      /* Perform recursive function */
      tree_path();

      CATCLOSE();
      return 0;
}
