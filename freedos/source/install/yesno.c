/* yesno.c */

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

#include <conio.h>

#include "istrichr.h"

int
yesno (const char *yn)
{
  /* yesno() - Only accept Y or N */

  int c;
  int idx;

  /* query for string */

  /* index_strichr() is a case-insensitive version of strchr() that
     returns an index instead of a pointer, or -1 if not found */

  c = getch();

  while ( (idx = index_strichr (yn, c)) < 0 )
    {
      c = getch();
    }

  /* returns 0 for 'N', 1 for 'Y' */

  /* note that the index in the "yn" string has 'Y' at 0, 'N' at 1,
     but we need to return the opposite */

  if (idx)
    {
      /* "NO" */
      return (0);
    }

  /* else, "YES" */

  return (1);
}
