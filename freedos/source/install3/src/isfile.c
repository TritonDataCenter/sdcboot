/* isfile.c */

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


#include <stdio.h>


int
isfile (const char *filename)
{
  /* Returns a true value if filename exists as a file, or false if not. */

  FILE *stream;

  /* Open the file */

  stream = fopen (filename, "r");
  if (stream == NULL)
    {
      /* Failed */
      return (0);
    }

  /* Close the file, and return success */

  fclose (stream);
  return (1);
}
