/* A function to draw a box on the screen. */

/*
   Copyright (C) 2000 Jim Hall <jhall@freedos.org>

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/* No screen bounds checking is done here.  It is the responsibility
 of the programmer to ensure that the box does not exceed the screen
 boundaries. */

#ifdef __WATCOMC__
#include <screen.h>
#else
#include <conio.h>
#endif


/* Symbolic names for drawing a box */

#define ACS_HLINE    196 /* '-' */
#define ACS_VLINE    179 /* '|' */

#define ACS_LRCORNER 217 /* '+' */
#define ACS_ULCORNER 218 /* '+' */

#define ACS_URCORNER 191 /* '+' */
#define ACS_LLCORNER 192 /* '+' */


void
box (int x0, int y0, int x1, int y1)
{
  int start_x, start_y;			/* starting coordinates */
  int i, j;

  /* Draw top corners and border */
  textbackground(BLUE);
  textcolor(CYAN);

  gotoxy (x0, y0);
  putch (ACS_ULCORNER);

  for (i = x0; i <= x1-2; i++)
    {
      putch (ACS_HLINE);
    }

  putch (ACS_URCORNER);

  /* Draw sides */

  for (i = y0+1; i < y1; i++)
    {
      gotoxy (x0, i);
      putch (ACS_VLINE);

      textbackground(BLUE);
      for (j = x0+1; j < x1; j++)
	{
	  putch (' ');
	}
      textbackground(BLUE);

      putch (ACS_VLINE);
    }

  textbackground(DARKGRAY);

  if(x1 != 80 ) for (i = y0+1; i < y1 + 1; i++)
    {
      gotoxy (x1 + 1, i);
      putch (' ');
    }

  textbackground(BLUE);

  /* Draw bottom corner and border */

  gotoxy (x0, y1);
  putch (ACS_LLCORNER);

  for (i = x0; i <= x1-2; i++)
    {
      putch (ACS_HLINE);
    }

  putch (ACS_LRCORNER);

  if (y1 < 25) {
    gotoxy (x0 + 1, y1 + 1);
    textbackground(DARKGRAY);

    for (i = x0; i <= x1 - 2; i++)
    {
      putch (' ');
    }
    if(x1 != 80) {
      putch(' ');
      putch(' ');
    } else clreol();
  }

  textbackground(BLUE);
  textcolor(WHITE);
  gotoxy (x1 - 1, y1);
}
