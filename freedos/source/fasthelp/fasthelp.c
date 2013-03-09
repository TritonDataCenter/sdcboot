/* FASTHELP : Displays a help page, similar to UNIX 'man' */

/* Copyright (C) 1998-2002 Jim Hall, jhall@freedos.org */

/* Internationalization LANG support added by Abrahan Sanjuas Fontan */

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
#include <stdlib.h>			/* Borland: exit */
#include <string.h>			/* strchr, strncmp */

#include <dir.h>			/* fnmerge */
#include <process.h>			/* system */

#include "catgets.h"			/* diropen/catgets */


/* Symbolic constants */

#define DEF_PAGER	"MORE"
#define DEF_HELPPATH	"C:\\FDOS\\HELP"
#define DEF_LANG "EN"


/* Functions */

void usage (nl_catd cat);
char *helpdirname(const char *program);


/* The program starts here */

int
main (int argc, char **argv)
{
  char *s;
  char *pager;
  char *helppath;
  char helpfile[MAXPATH];
  char bigcmd[128];
  char *lang;
  int i;
  nl_catd cat;

  /* International support */

  cat = catopen ("help", 0);

  /* Scan command line for "/?". */

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "/?", 2) == 0)
	{
	  usage (cat);
	  catclose (cat);
	  exit (0);
	}
    } /* for */

  if (argc > 2)
    {
      usage (cat);
      catclose (cat);
      exit (1);
    }

  /* Build the exec string */

  pager = getenv ("PAGER");
  if (!pager)
    {
      pager = DEF_PAGER;		/* pointer to a string */
    }

  helppath = getenv ("HELPPATH");
  if (!helppath)
    {
      /* helppath = DEF_HELPPATH;		/* pointer to a string */
      helppath = helpdirname (argv[0]);
    }

  lang = getenv ("LANG");
  if (!lang)
   {  
     lang = DEF_LANG;
   }
  
  /* Add the topic to view */

  if (argc == 1)
    {
      /* no help topic to view */

      fnmerge (helpfile, "", helppath, "INDEX", lang);
    }

  else
    {
      /* build a path to the help file */

      /* is this a literal file to view? (does it contain '\\'?) */

      /* literal files will not have a LANG value added, but other
         help topics will. */

      if (strchr (argv[1], '\\'))
	{
	  fnmerge (helpfile, "", "", argv[1], "");
	}

      else
	{
	  fnmerge (helpfile, "", helppath, argv[1], lang);
	}
    }

  /* Exec the pager */

  /* this is safer than exec, because the pager might be a shell
     command or PAGER might include arguments */

  sprintf (bigcmd, "%s %s", pager, helpfile);
  i = system (bigcmd);

  if (i)
    {
      /* if you get here, something broke */

      s = catgets (cat, 1, 0, "Cannot execute pager program.");
      printf ("HELP: %s: %s\n", pager, s);
    }

  catclose (cat);
  exit (2);
}

void
usage (nl_catd cat)
{
  char *s;

  fprintf (stderr, "FreeDOS FASTHELP (version 3.5)\n");
  fprintf (stderr, "Copyright (c) 1998-2002 Jim Hall, jhall@freedos.org\n\n");

  s = catgets (cat, 0, 0, "Display a help topic");
  fprintf (stderr, "FASTHELP: %s\n", s);

  s = catgets (cat, 0, 1, "Usage:");
  fprintf (stderr, "%s\n", s);

  s = catgets (cat, 0, 2, "topic");
  fprintf (stderr, "  FASTHELP [ %s ]\n", s);

  s = catgets (cat, 0, 3, "Environment:");
  fprintf (stderr, "%s\n", s);

  s = catgets (cat, 0, 4, "Pager program to display a text file");
  fprintf (stderr, "  PAGER: %s\n", s);

  s = catgets (cat, 0, 5, "Directory that contains your help files");
  fprintf (stderr, "  HELPPATH: %s\n", s);

  s = catgets (cat, 0, 6, "Preferred language for your help files");
  fprintf (stderr, "  LANG: %s\n", s);
}

char *
helpdirname(const char *program)
{
  static char ret_path[MAXPATH];
  char drive[MAXDRIVE], dir[MAXPATH], name[MAXFILE], ext[MAXEXT];

  if ((program == NULL) || (program[0] == '\0'))
    {
      /* If program path not given (NULL or empty string) use hard
	 coded value */
      strcpy (ret_path, DEF_HELPPATH);
    }
  else
    {
      /* Split path up into components, then remerge */
      /* Technically, you shouldn't give a path in the 'name' field,
	 but in this case, Borland C / Turbo C just passed it thru */
      fnsplit(program, drive, dir, name, ext);
      fnmerge (ret_path, drive, dir, "..\\HELP\\", "");
    }

#ifndef NDEBUG /* debugging */
  printf("DEBUG: ret_path is [%s]\n", ret_path);
#endif

  /* Return point to static buffer with base help path to use */
  return (ret_path);
}
