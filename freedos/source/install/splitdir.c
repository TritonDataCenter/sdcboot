/* splitdir.c */

/* Copyright (c) 2010 Jim Hall <jhall@freedos.org> */

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

#include <stdlib.h>
#include <string.h>

/*
  input: fulldir
  output: drive, dir

  !! drive & dir need to be fully allocated strings
*/

void
splitdir (char *fulldir, char *drive, char *dir)
{
  /*
    splitdir() - Simple function to split a directory path into a
    drive and directory component. Similar to _splitpath but only
    works for dirs like "C:\DIR" and "\DIR" and "DIR\".
  */

  if (fulldir[1] == ':')
    {
      /* fulldir has a drive component, looks like "C:\DIR" */

      drive[0] = fulldir[0];
      drive[1] = '\0';

      strncpy (dir, fulldir + 2, _MAX_DIR);
    }
  else
    {
      /* fulldir is just a dir, looks like "\DIR" or "DIR\" */

      drive[0] = '\0';
      strncpy (dir, fulldir, _MAX_DIR);
    }
}
