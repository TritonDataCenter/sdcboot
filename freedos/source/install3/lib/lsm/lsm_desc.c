/* $Id: lsm_desc.c,v 1.1.1.1 2000/12/30 05:27:14 jhall1 Exp $ */

/* Copyright (C) 1998 Jim Hall, jhall1@isd.net */

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

#ifdef __WATCOMC__
#include <screen.h>
#endif
#include <stdio.h>
#include <string.h>				/* for strncmp */
#include <ctype.h>				/* for isspace */

#ifndef __WATCOMC__
#ifdef unix
#include "conio.h"
#else
#include <conio.h>
#endif /* unix */
#endif

#define STRLEN 80

int
lsm_description (int y0, int x0, int maxlines, const char *lsmfile)
{
  /* Displays the Description lines in a Linux Software Map
     file (.lsm file).  Returns the number of lines that were
     printed. */

  /* lsmfile is the name of a '.lsm' file. */

  FILE *stream;
  char s[STRLEN];
  int len;
  int i, n;
  int is_desc_line;
  int is_blank;

  /* Open the file */

  stream = fopen (lsmfile, "r");
  if (stream == NULL) {
    return 0;
  }

  /* Read the lsmfile until done */

  n = 0;
  is_desc_line = 0;

  while (fgets (s, STRLEN, stream) != NULL) {
    /* If the line is empty, this is the end of the desc */

    for (len = 0, is_blank = 1; s[len] != '\0'; len++) {
      if (!isspace(s[len])) {
	is_blank = 0;
      }
    } /* for */

    /* See if another paragraph starts here.  An LSM paragraph is
       ended can be ended with a blank line, or by starting a new
       paragraph.  Always begin with a keyword in column 0. */

    if (is_blank) {
      is_desc_line = 0;
    }

    if (!isspace(s[0])) {
      is_desc_line = 0;
    }

    /* Look for the 'Description:' flag.  Assumes that we are case-
       sensitive, and that the flag appears at the start of the
       string. */

    if (strncmp (s, "Description:", 12) == 0) {
      is_desc_line = 1;
    }

    /* If this is part of the description, print it */

    if (is_desc_line) {
      gotoxy (x0, y0 + n);			/* conio */

      /* I need to use a hack here instead of cputs(s) because we
         appear to be printing a circle (ASCII 09h) instead of blank
         space. It's not a perfect hack, but it works. */

      /* The following should be replaced by cputs(s); */
      /* cputs (s);				/* conio */

      for (i = 0; s[i] != '\0'; i++)
	{
 	  switch (s[i])
 	    {
 	    case '\t':
 	      cputs ("        ");		/* print spaces instead */
 	      break;
 	    default:
 	      putch (s[i]);
 	    } /* switch ch */
	} /* for i */

      /* As a hack, we won't refresh the screen unless we really need to.
	 So we'll refresh the screen with cputs, but not with putch */

#ifdef unix /* hack added 12/27/99 jhall */
      refresh();				/* curses */
#endif /* unix */

      n++;

      if (n >= maxlines)
	{
	  fclose (stream);
	  return (n);
	}
    }

  } /* while */

  /* Close the file */

  fclose (stream);
  return (n);
}
