/* Functions to install a disk series, or a package */

/* Copyright (C) 1998-2001 Jim Hall <jhall@freedos.org> */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>                 /* for system(), free() */
#include <string.h>
#ifdef __WATCOMC__
#include <screen.h>
#define fnmerge _makepath
#else
#include <conio.h>
#endif

#include "globals.h"                /* cat, yes, no, "catgets.h" */
#include "bargraph.h"               /* for bargraph() */
#include "box.h"                    /* box() */
#include "dat.h"                    /* data file functions */
#include "cat.h"                    /* for cat_file() */
#include "inst.h"                   /* for this file */
#include "isfile.h"                 /* for isfile() */
#include "lsm.h"                    /* Linux LSM files */
#include "repaint.h"                /* for repaint() */
#include "sel_list.h"               /* select_yn() */
#include "unz.h"                    /* for UzpMain() */
#include "dir.h"
#include "pause.h"                  /* for pause() */
#include "text.h"                   /* All strings displayed */
#include "log.h"                    /* for log() */
#include "cchndlr.h"                /* for reregisterSIGINTHandler() */
#include "list.h"



inst_t
set_install (const char *diskset, char *fromdir, char *destdir)
{
  /* Variables */

  char endfile[MAXPATH];            /* marks end of series */
  char datfile[MAXPATH];            /* current DAT file */
  char ext[MAXPATH];                /* file extension */
  char *s;
  int disknum = 0;                  /* current disk number */
  int ch;
  inst_t ret;                       /* return: no. of errors,warnings */
  inst_t this;                      /* no. of errors,warnings */

  /* Create the filenames */

  fnmerge (endfile, "", fromdir, diskset, ".END");

  /* Print the name of the series we are working on */

  repaint_empty();
  s = catgets (cat, SET_PKG_GENERAL, MSG_INSTSERIES, MSG_INSTSERIES_STR);
  gotoxy (2, 3);
  cputs (s);
  cputs (diskset);
  log("<diskset name=\"%s\" >\n", diskset);


  /* Install while we have disks to work from.  Since we will reach an
     exit condition within the loop, we use an infinite loop here. */

  ret.errors = 0;
  ret.warnings = 0;

  while (1) {
    repaint_empty();

    /* First check that the datfile exists.  If it doesn't, check if
       the endfile was found. */

    sprintf (ext, ".%d", ++disknum);
    fnmerge (datfile, "", fromdir, diskset, ext);
    log("<datfile name=\"%s\" >\n", datfile);

    if (!isfile (datfile)) {
        log("<datfile name no exist=\"%s\" >\n", datfile);
      /* Does the endfile exist? */

      if (isfile (endfile)) {
    s = catgets (cat, SET_PKG_GENERAL, MSG_INSTSERIESDONE, MSG_INSTSERIESDONE_STR);
    gotoxy (2, 10);
    cputs (s);

    s = catgets (cat, SET_PKG_GENERAL, MSG_NEXTSERIESDISK, MSG_NEXTSERIESDISK_STR);
    gotoxy (2, 15);
    cputs (s);

    s = catgets (cat, SET_PKG_GENERAL, MSG_NEXTSERIESDISK2, MSG_NEXTSERIESDISK2_STR);
    gotoxy (2, 16);
    cputs (s);

      log("</diskset>\n\n");
    pause();
    return (ret);
      }

      /* The endfile was not found, so we know there is at least one
         more disk left to do.  Keep asking the user to insert the
         next disk. */

      do {

      /* If this is the 1st disk in the series, then instead of assuming wrong disk
         prompt for them to insert the disk. */
      if (disknum == 1)
      {
        s = catgets (cat, SET_PKG_GENERAL, MSG_INSERT1STDISK, MSG_INSERT1STDISK_STR);
      gotoxy (2, 10);
        cprintf(s, diskset);
      }
      else 
      {
      s = catgets (cat, SET_PKG_GENERAL, MSG_MISSINGDATAFILE, MSG_MISSINGDATAFILE_STR);
      gotoxy (2, 10);
      cputs (s);

      gotoxy (2, 11);
      cputs (datfile);

      s = catgets (cat, SET_PKG_GENERAL, MSG_WRONGFLOPPY, MSG_WRONGFLOPPY_STR);
      gotoxy (2, 15);
      cputs (s);

      s = catgets (cat, SET_PKG_GENERAL, MSG_STILLWRONGFLOPPY, MSG_STILLWRONGFLOPPY_STR);
      gotoxy (2, 16);
      cputs (s);
      }

    s = catgets (cat, SET_PROMPT_YN, MSG_CONTINSTDISK, MSG_CONTINSTDISK_STR);
    ch = select_yn (s, yes, no, NULL, NULL);

    if (ch)
      {
        /* user has decided to quit this series */
          log("</diskset>\n\n");
        return (ret);
      }

      } while (!isfile (datfile));
    } /* if no datfile */

    /* Install files from this disk */

    log("<disk name=\"%s\" number=\"%i\" >\n", diskset, disknum);
    this = disk_install (datfile, fromdir, destdir);
    log("</disk>\n");
    ret.errors += this.errors;
    ret.warnings += this.warnings;
  } /* while (1) */
}

inst_t
disk_install(const char *datfile, char *fromdir, char *destdir)
{
  char lsmfile[MAXPATH];            /* Linux software map file */
  char *s;

  int dat_count;                    /* size of the dat array */
  int ret;
  int ch;
  int i;

  int pkg_yesToAll = 0;             /* Assume default yes to all = not specified */

  dat_t *dat_ary;                   /* the DAT array */
  inst_t this;                      /* return: no. of errors,warnings */

  /* Initialize variables */

  this.errors = 0;
  this.warnings = 0;

  /* Read dat file */

  dat_ary = dat_read (datfile, &dat_count);
  if (dat_ary == NULL)
  {
    s = catgets (cat, SET_ERRORS, MSG_ERROR, MSG_ERROR_STR);
    fprintf (stderr, s);
    log("<error msg=\"%s\" />\n", s);

    if (dat_count > 0)
      s = catgets (cat, SET_ERRORS, MSG_ERRALLOCMEMFDF, MSG_ERRALLOCMEMFDF_STR);
    else /* Either error reading file, eg no file, or file has no entries */
      s = catgets (cat, SET_ERRORS, MSG_ERREMPTYFLOPPYDATAFILE, MSG_ERREMPTYFLOPPYDATAFILE_STR);

    fprintf (stderr, s);
    log("<error msg=\"%s\" />\n", s);

    pause();
    return (this);
  }

  /* Run the install */
  repaint_empty();
  dat_ary = listbox(dat_ary, dat_count, fromdir);

  for (i = 0; i < dat_count; i++) {
    /* Print the screen and progress bargraph */

    repaint_empty();

    box (14, 16, 66, 18);
    gotoxy (15, 17);
    bargraph (i, dat_count, 50 /* width */);

    /* Print the package name */

    gotoxy (2, 5);
    s = catgets (cat, SET_PKG_GENERAL, MSG_PACKAGE, MSG_PACKAGE_STR);
    cputs (s);

    cputs (dat_ary[i].name);

    /* Show the package description */

    /* Generate the lsmfile name */

    fnmerge (lsmfile, "", fromdir, dat_ary[i].name, ".LSM");

    if (isfile (lsmfile))
    {
    lsm_description (8, 2, 10, lsmfile);
    }
    else
    {
    /* no lsm file. try it again with a (localized) plain txt file */
    fnmerge (lsmfile, "", fromdir, dat_ary[i].name, "");

      gotoxy (2, 8);
      cat_file (lsmfile, 10 /* no. lines */);
    }

    /* Find out which ones the user wants to install */

    gotoxy (2, 25);
    switch (dat_ary[i].rank) {
    case 'n':
    case 'N':
      /* Do not install */

      log("<package name=\"%s\" choice=\"n\" />\n", dat_ary[i].name);
      s = catgets (cat, SET_PKG_NEED, MSG_SKIPPED, MSG_SKIPPED_STR);
      cputs (s);
      break;

    case 'y':
    case 'Y':
      /* Always install */

      log("<package name=\"%s\" choice=\"y\" ", dat_ary[i].name);
      textbackground(BLUE);
      gotoxy(2, 23);
      s = catgets (cat, SET_PKG_NEED, MSG_REQUIRED, MSG_REQUIRED_STR);
      cputs (s);

      ret = unzip_file (dat_ary[i].name, fromdir, destdir);

      reregisterSIGINTHandler(); /* unzip installs its own SIGINT handler */

      if (ret != 0 && ret != 1) {
        unsigned centre;
    /* Print an error message */

    s = (ret != 3) ?
        catgets (cat, SET_PKG_GENERAL, MSG_ERRREQPKG, MSG_ERRREQPKG_STR) :
        catgets (cat, SET_PKG_GENERAL, MSG_NODISKSPC, MSG_NODISKSPC_STR);
    centre = (40 - (strlen(s) / 2));
    box(centre, 12, centre + strlen(s) + 1, 14);
    gotoxy(centre + 1, 13);
    cputs (s);
      log(">\n");
      log("<error msg=\"%s\" />\n", s);
      log("</package>\n");

    /* Return an error */

        this.errors++;

    /* Does the user want to continue anyway? */

    s = catgets (cat, SET_PROMPT_YN, MSG_CONTINSTDISK, MSG_CONTINSTDISK_STR);
    ch = select_yn(s, yes, no, NULL, NULL);

        if (ch)
        {
          log("<abort msg=\"User choose not to continue after error.\" />\n");
          return (this);
      }
      }
      else if (ret == 1) this.warnings++;
      else /* ret == 0, ie no errors */
        log("/>\n");
      break;

    default:
      /* Optional */

      textbackground(BLUE);
      s = catgets (cat, SET_PKG_NEED, MSG_OPTIONAL, MSG_OPTIONAL_STR);
      gotoxy(2, 23);
      cputs (s);

      /* Ask the user if you want to install it */

      s = catgets (cat, SET_PROMPT_YN, MSG_INSTALLPKG, MSG_INSTALLPKG_STR);
      if (!pkg_yesToAll)
    {
          ch = select_yn (s, yes, no, 
                            catgets (cat, SET_PROMPT_YN, MSG_YESTOALL, MSG_YESTOALL_STR),
                            NULL);
        if (ch == 2) pkg_yesToAll = 1;
    }

      if (pkg_yesToAll || ch==0) /* Yes or YesToAll */
    {
        log("<package name=\"%s\" choice=\"y\" ", dat_ary[i].name);
      ret = unzip_file (dat_ary[i].name, fromdir, destdir);

        reregisterSIGINTHandler(); /* unzip installs its own SIGINT handler */

      if (ret != 0)
        {
          unsigned centre;
          /* Print a warning message */

          s = catgets (cat, SET_PKG_GENERAL, MSG_WARNOPTPKG, MSG_WARNOPTPKG_STR);
          centre = 40 - (strlen(s) / 2);
          box(centre, 12, centre + strlen(s) + 1, 14);
          gotoxy(centre + 1, 13);
          cputs (s);
            log(">\n");
            log("<warning msg=\"%s\" />\n", s);
            log("</package>\n");

        pause();
          this.warnings++;
        }
          else
            log("/>\n");
    }
      else /* user selected no */
          log("<package name=\"%s\" choice=\"n\" />\n", dat_ary[i].name);
      break;

    } /* switch */
  } /* for */

  /* Print the screen and 100% complete progress bargraph */
  repaint_empty();
  box (14, 16, 66, 18);
  gotoxy (15, 17);
  bargraph (1, 1, 50 /* width */);

  /* Free memory for this disk */

  free (dat_ary);
  return (this);
}
