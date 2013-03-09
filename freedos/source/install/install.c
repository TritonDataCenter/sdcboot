/* install.c */

/* FreeDOS INSTALL program */

/* Copyright (c) 2011 Jim Hall <jhall@freedos.org> */

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
#include <stdlib.h>
#include <string.h>

#include <dos.h>
#include <direct.h>

#include <conio.h>
#include <graph.h>

#include "colors.h"

#include "pkginst.h"
#include "progress.h"
#include "window.h"
#include "yesno.h"

struct path_t
{
  int dirindex;
  char filename[_MAX_NAME];
};

#define MALLOC_INCR 100

int install (char *dest, int do_all, int do_source);
void fail (const char *error);


int
main (int argc, char **argv)
{
  char cwd[_MAX_PATH];
  char dest[_MAX_PATH];
  char buf[_MAX_PATH+3];				/* TODO: match up with code, below */

  int do_all;
  int do_source;

  int install_errs;

  short fg;
  short bg;

  /* command line */

  /* error if ANY command line arguments */

  if (argc > 1)
    {
      fprintf (stderr, "FreeDOS INSTALL\n");
      fprintf (stderr, "Copyright (c) 2011 Jim Hall <jhall@freedos.org>\n\n");
      fprintf (stderr, "Usage: INSTALL\n");
      exit (1);
    }

  /* set up console screen */

  fg = _gettextcolor ();
  bg = _getbkcolor ();

  _settextcolor (_WHITE_);
  _setbkcolor (_BLUE_);
  _clearscreen (_GCLEARSCREEN);

  titlebar ("FreeDOS INSTALL");
  statusbar (" ");

  _settextwindow (2,1 , 24,80);			/* y1,x1 , y2,x2 */

#if 0
  /* show install source */

  getcwd (&cwd, _MAX_PATH);

  _settextposition (X,Y);			/* relative to window */
  cputs ("Install from: ");
  cputs (cwd);
#endif

  /*
    REMINDER:

    buf[0] must contain the maximum length in characters of the string
    to be read.

    The array must be big enough to hold the string, a terminating
    null character, and two additional bytes.

    The actual length of the string read is placed in buf[1].

    The string is stored in the array starting at buf[2]. The newline
    character, if read, is replaced by a null character.
  */

  buf[0] = 60;					/* width of text-input window */

  /* get install destination */

  _settextposition (5,5);			/* relative to window */
  cputs ("Install to:");

  _settextcolor (_BRIGHTWHITE_);
  _setbkcolor (_CYAN_);

  _settextwindow (7,10 , 7,70);			/* y1,x1 , y2,x2 */
  _clearscreen (_GWINDOW);

  _settextposition (1,1);			/* relative to window */
  cgets (buf);

  /* check input, and copy dest */

  if (buf[1] < 1)
    {
      fail ("empty dest string");
    }
  else if (buf[1] > _MAX_PATH)
    {
      fail ("dest string too long");
    }

  strncpy (dest, buf+2, buf[1]);
  dest[ buf[1] ] = '\0';			/* terminate the string */

  _settextcolor (_WHITE_);
  _setbkcolor (_BLUE_);

  _clearscreen (_GWINDOW);

  _settextposition (1,1);			/* relative to window */
  cputs (dest);					/* TODO: display uppercase */

  /* ask if installing all packages, and source code */

  _settextwindow (2,1 , 24,80);			/* y1,x1 , y2,x2 */

  _settextposition (10,5);
  cputs ("Install all packages, in all disk sets?");
  cputs (" : ");

  do_all = yesno ("YN");

  if (do_all)
    {
      cputs ("YES");
    }
  else
    {
      cputs ("NO");
    }

  /* ask if installing source code too */

  _settextposition (11,5);
  cputs ("Install source code, too?");
  cputs (" : ");

  do_source = yesno ("YN");

  if (do_source)
    {
      cputs ("YES");
    }
  else
    {
      cputs ("NO");
    }

  /* install */

  statusbar ("Press any key to begin . . .");
  getch ();

  install_errs = install (dest, do_all, do_source);

  statusbar ("Done!");
  getch ();

  /* reset colors, and exit */

  _settextcolor (fg);
  _setbkcolor (bg);
  _clearscreen (_GCLEARSCREEN);

  if (install_errs > 0)
    {
      printf ("WARNING: %d errors encountered during install\n");
      exit (1);
    }

  /* else */

  exit (0);
}


/*
  install()

  INPUT:
  dest = destination directory
  do_all = non-zero: install everything, zero: only install BASE
  do_source = non-zero: install source code, zero: do not install source

  RETURN:
  number of errors during installation
*/

int
install (char *dest, int do_all, int do_source)
{
  char *pkgsdirs[] = {
    "BASE",
    "BOOT",
    "DEVEL",
    "EDIT",
    "GUI",
    "NET",
    "SOUND",
    "UTIL"
  };

  int pkgsdirs_count = 8;

  char path[_MAX_PATH];

  int done;
  int dir;
  int file;

  int install_ok;
  int install_count;
  int install_errs;

  struct find_t ff;

  struct path_t *files;
  int files_size;
  int files_count;

  /* if NOT installing everything (! do_all), set pkgsdirs_count to 1,
     so we only include BASE */

  if (! do_all)
    {
      pkgsdirs_count = 1;
    }

  /* allocate memory for the file list */

  files = (struct path_t *) malloc (MALLOC_INCR * sizeof (struct path_t));

  if (! files)
    {
      fail ("malloc files");
    }

  files_size = MALLOC_INCR;
  files_count = 0;

  /* build package install list */

  /* read each dir in the PACKAGES subdirs for list of packages to install */

  statusbar ("Building package list . . .");

  for (dir = 0; dir < pkgsdirs_count; dir++)
    {
      sprintf (path, "PACKAGES\\%s\\*.ZIP", pkgsdirs[dir]);
      done = _dos_findfirst (path, _A_NORMAL, &ff);

      while (! done)
	{
	  /* enough memory for a new string? */

	  if (files_count == files_size)
	    {
	      files_size += MALLOC_INCR;
	      files = (struct path_t *) realloc (files, files_size * sizeof (struct path_t));

	      if (! files)
		{
		  free (files);
		  fail ("realloc files");
		}
	    }

	  /* read filename into files list */

	  files[files_count].dirindex = dir;
	  strncpy (files[files_count].filename, ff.name, _MAX_NAME);

	  files_count++;
	  done = _dos_findnext (&ff);
	} /* while (! done) */
    } /* for (dir) */

  /* install the packages */

  statusbar ("Installing . . .");
  progressbar (0, 1);

  install_count = 0;
  install_errs = 0;

  for (file = 0; file < files_count; file++)
  /* for (file = 0; file < 4; file++) */
    {
      statusbar (files[file].filename);
      progressbar (file+1, files_count);

      /* re-set the window, in case errors get printed by pkginstall() */

      _settextwindow (2,1 , 24,80);			/* y1,x1, y2,x2 */

      /*
      cprintf ("DEBUG: %d/%d", files_count, files_size);
      */

      /* install file */

      sprintf (path, "PACKAGES\\%s\\%s", pkgsdirs[ files[file].dirindex ], files[file].filename);

      install_ok = pkginstall (path, dest, do_source);

      if (install_ok)
	{
	  install_count++;
	}
      else
	{
	  install_errs++;
	}
    }

  /* cprintf ("DEBUG: %d files installed, %d errors", install_count, install_errs); */

  /* done */

  progressbar (1, 1);

  free (files);
  return (install_errs);
}


/*
  fail()

  INPUT:
  error = text string to display to the user (probably used in debugging)

  RETURN:
  nothing
*/

void
fail (const char *error)
{
  /* display an error message, then quit */

  _settextcolor (_BRIGHTWHITE_);
  _setbkcolor (_RED_);

  _settextwindow (10,15 , 15,65);			/* y1,x1 , y2,x2 */
  _clearscreen (_GWINDOW);

  _settextposition (2,2);				/* relative to window */
  cprintf ("FAIL: %s\n", error);

  _settextposition (4,2);
  cputs ("Press any key to quit . . .");

  getch();
  
  /* reset colors, and quit */

  _settextcolor (_WHITE_);
  _setbkcolor (_BLACK_);
  _clearscreen (_GCLEARSCREEN);

  exit (1);
}
