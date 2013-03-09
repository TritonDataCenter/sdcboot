/* istrichr.c */

/* Copyright (c) 2010-2011 Jim Hall <jhall@freedos.org> */

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

#include <ctype.h>

/* index_strichr() */

/* case-insensitive strchr, but returns an int (instead of a pointer)
   giving the character's location in the string */

int
index_strichr (const char *str, int ch)
{
  int idx;
  char up_str_char;
  char up_ch;

  /* assume uppercase */

  up_ch = toupper (ch);

  /* find a match for ch in str */

  for (idx = 0; str[idx]; idx++)
    {
      up_str_char = toupper (str[idx]);

      if (up_str_char == up_ch)
	{
	  return (idx);
	}
    }

  /* if failed to find a match, return -1 */

  return (-1);
}
