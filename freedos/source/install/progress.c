/* progress.c */

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

#include <conio.h>
#include <graph.h>

#include "colors.h"

#include "progress.h"

void
progressbar (int current, int total)
{
  /* display a progress bar on screen */

  char bar[51];
  int percent;
  int width;
  int col;
  int row;

  /* assumes current >= 0, current <= total, and both current & total
     are non-negative, otherwise behavior may not be as expected. */

  if (total > 0)
    {
      percent = 100 * current / total;
    }
  else
    {
      percent = 0;
    }

  if (percent < 0)
    {
      percent = 0;
    }
  else if (percent > 100)
    {
      percent = 100;
    }

  width = percent / 2;

  /* generate a string to display */

  for (col = 0; col < width; col++)
    {
      /* bar[col] = '#'; */
      bar[col] = 219;				/* filled box */
    }

  for ( ; col < 50; col++)
    {
      /* bar[col] = '-'; */
      bar[col] = 176;				/* shaded box */
    }

  bar[50] = 0;

  /* print the string */

  /* bar is 50 cols wide, so offset is 80 - 50 = 30 ; 30 / 2 = 15 */

  _settextwindow (15,1 , 20,80);		/* y1,x1 , y2,x2 */

  _settextcolor (_YELLOW_);
  _setbkcolor (_BLUE_);

  /* only initalize/clear screen if 'current' is 0 */

  if (current == 0)
    {
      _clearscreen (_GWINDOW);
    }

  for (row = 1; row <= 3; row++)
    {
      _settextposition (row, 15);		/* relative to window */
      cputs (bar);
    }

  _settextposition (3, 67);			/* relative to window */
  cprintf ("%d%%", percent);
}
