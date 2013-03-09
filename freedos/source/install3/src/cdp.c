/* Copyright (C) 1998,1999,2000,2001 Jim Hall <jhall@freedos.org> */

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

#include <string.h> /* strchr() */
#include <stdlib.h>
#include <stdio.h>
#ifdef __WATCOMC__
#include <dos.h>
#include <direct.h>
#include <ctype.h>
#endif
#include "dir.h"    /* DIR_CHAR and mkdir() */

#ifndef ALT_DIR_CHAR
#define STRCHR(str) strchr(destdir, DIR_CHAR)
#else
/* Return next occurance of either directory separator character. */
char *STRCHR(char *str)
{
  char *p1, *p2;
  p1 = strchr(str, DIR_CHAR);
  p2 = strchr(str, ALT_DIR_CHAR);
  if (p1 != NULL & p2 != NULL)
    return (p1 < p2)?p1:p2;
  else
    return (p1 == NULL)?p2:p1;
}
#endif

/* Make sure destination directory exists (or create if not)       */
/* This fixes bug, where if path does not exist then install fails */
void createdestpath(char *destdir)
{
  char dirChar;
  char *p = STRCHR(destdir); /* search for 1st dir separator */
  char envbuffer[MAXPATH+7];

  while (p)
  {
    dirChar = *p;
    *p = '\0';       /* mark as temp end of string */
    mkdir(destdir);  /* create, ignore errors, as dir may exist */
    *p = dirChar;    /* restore dir separator */
    p = STRCHR(p+1); /* look for next dir separator */
  }
  mkdir(destdir);    /* create last portion of dir if didn't end in slash */
  if( (*p = destdir[strlen(destdir) - 1]) == dirChar ) *p = '\0';
  sprintf (envbuffer, "DOSDIR=%s", destdir);
#ifdef __WATCOMC__
  if (destdir[1] == ':') {
    unsigned total;
    _dos_setdrive (tolower(destdir[0]) - 'a' + 1, &total);
  }
#else
  if (destdir[1] == ':') setdisk (toupper(destdir[0]) - 'A');
#endif
  chdir (destdir);
}
