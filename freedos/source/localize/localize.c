/* localize.c */

/* 2003 by Eric Auer */

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

/* This program prints a message in the current NLS setting: It is
 * simply a small wrapper to the KITTEN toolkit. The first option
 * must be the message number in x.y format. Other options are just
 * printed after the message. If NLS is not set, nothing is printed
 * and errorlevel 1 is returned (errorlevel is 0 if NLS did work).
 *
 * Example usage:
 * LOCALIZE 42.10 %value%
 * IF [%errorlevel%]==[1] ECHO The magic number is %value%
 *
 * localize.xx (where xx is your language code) would contain
 * a line
 */

#include <stdio.h>			/* strtoul */
#include <stdlib.h>			/* borland needs this for 'exit' */
#include <string.h>			/* strchr */

#include "kitten.h"			/* Kitten message library */


/* Functions */

void usage (nl_catd cat);


/* Main program */

int
main (int argc, char **argv)
{
  char *s;
  char *s2;
  int c,x,y;
  nl_catd cat;				/* message catalog */

  /* Message catalog */

  cat = catopen ("localize", 0);

  /* Scan the command line */
  /* argv[0] is the path of the exectutable! */

  if (argc < 2)	/* at least argv[0] and a message number needed */
    {
    	usage (cat);
    	catclose (cat);
    	exit (2);
    }

  s = argv[1];		/* 1st real argument: the message number */
  s2 = strchr(s,'.');	/* first "." in the number */

  if (s2 != NULL)
    {
      s2[0] = '\0';	/* modify argument to split it! */
      s2++;
      x = atoi(s);	/* first number (returns 0 on error) */
      if ((x==0) && (s[0]!='0'))
        x = -1;
      y = atoi(s2);	/* second number (returns 0 on error) */
      if ((y==0) && (s2[0]!='0'))
        y = -1;

      s2--;
      s2[0] = '.';	/* unsplit argument */
      s2 = s;		/* point to message number string */
    }
  else			/* not even a dot provided... */
    {
      x = -1;
      y = -1;
    }

  if ((x < 0) || (y < 0))	/* did the user provide numbers? */
    {
    	usage (cat);		/* else abort with help message */
    	catclose (cat);
    	exit (2);
    }


  if (cat >= 0)				/* catalog found? */
    s = catgets(cat, x, y,    "[Undefined message ");

  if ((cat < 0) || !strcmp(s, "[Undefined message "))
    {		/* message or whole catalog not found: */
      c=2;	/* show ALTERNATIVE message instead */
      while (argc > c)		/* scan to @@@ alternative message marker */
        {
	  if (!strcmp(argv[c],"@@@"))	/* start of alternative message? */
	    break;			/* then leave the printing loop! */
	  c++;				/* else just skip */
        }
      c++;				/* skip the @@@ itself */
      while (argc > c)			/* now print alternative message */
        {
	    write(1 /* stdout */, argv[c], strlen(argv[c]) );
	    write(1, " ",1); /* write has much smaller overhead than printf */
	    c++;
        }
    }

  /* now display localized message followed by further arguments */

  if ((cat >= 0) && !strcmp(s, "[Undefined message "))
    {
      write(1, s, strlen(s));		/* the un-message message */
      write(1, s2, strlen(s2));		/* stored message number string */
      write(1, "]", 1);
      x = 1;				/* remember the problem */
    } else {
      if (cat >= 0)			/* if any catalog at all... */
        {
          write(1,s,strlen(s));		/* ...then print the message */
          x = 0;			/* localization worked */
        } else {
          x = 1;			/* remember the problem */
        }
    }
  
  c=2;			/* if argc > 2, display extra arguments "as is" */
  while (argc > c)
    {
	write(1, " ",1);
	if (!strcmp(argv[c],"@@@"))	/* start of alternative message? */
	  break;			/* then leave the printing loop! */
	write(1, argv[c], strlen(argv[c]) );
	c++;
    }
  
  write(1,"\r\n",2); /* print CR LF */

  if (cat>=0)
    {
      catclose (cat);
    } else {
      exit(1);	/* errorlevel 1 - localization file not found */
    }

  exit (x);	/* errorlevel 0 if localization did work, else 1. */
  return x;

}


#define strWrite(st) write(1,st,strlen(st));

/* Show usage */

void
usage (nl_catd cat)
{
  char *s;

  if (cat != cat) {}; /* avoid unused argument error message in kitten */

  strWrite("FreeDOS LOCALIZE, version 0.1\r\n"); /* NEW VERSION */
  strWrite(
    "GNU GPL - copyright 2003 Eric Auer <eric@coli.uni-sb.de>\r\n\r\n");

  s = catgets (cat, 99, 0, "Prints a localized message (from localize.*)");
  strWrite(s);
  strWrite("\r\n");

  strWrite("LOCALIZE x.y [etc.] [@@@ alternative message]\r\n");
  s = catgets (cat, 99, 1, "Prints message number x.y or the alt. message (and echoes the [etc.])");
  strWrite(s);
  strWrite("\r\n");

  s = catgets (cat, 99, 2, "Errorlevel: 0 if okay, 1 on localization error.");
  strWrite(s);
  strWrite("\r\n");
  
  /* Help screen could be more verbose: errorlevel 2 on syntax error,
   * nothing is printed when localization failed completely, a default
   * message is printed if the requested message was not found, etc.
   * Put longer help screen text in LOCALIZE.* files! Do not include
   * it in the binary as well (to avoid redundancy).
   */
}

