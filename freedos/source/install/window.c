/* window.c */

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
#include <string.h>

#include <conio.h>
#include <graph.h>

#include "colors.h"

#include "window.h"

void
titlebar (const char *title)
{
  /* titlebar() - display a title at the top of the screen */

  int x;

  /* title bar is line 1, from cols 1-80 */

  _settextwindow (1,1 , 1,80);			/* y1,x1 , y2,x2 */

  _settextcolor (_BLACK_);
  _setbkcolor (_WHITE_);
  _clearscreen (_GWINDOW);

  /* center the title */

  x = (80 - strlen (title)) / 2;

  _settextposition (1, x+1);			/* y,x */

  cputs (title);
}

void
statusbar (const char *status)
{
  /* statusbar() - display a status message at the bottom of the screen */

  /* status bar is line 25, from cols 1-80 */

  _settextwindow (25,1 , 25,80);		/* y1,x1 , y2,x2 */

  _settextcolor (_BLACK_);
  _setbkcolor (_WHITE_);
  _clearscreen (_GWINDOW);

  _settextposition (1, 1);			/* y,x */

  cputs (status);
}
