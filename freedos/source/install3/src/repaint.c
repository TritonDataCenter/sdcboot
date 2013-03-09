/* $Id: repaint.c,v 1.4 2001/05/14 02:15:21 jhall1 Exp $ */

/* Copyright (C) 1999,2000 Jim Hall <jhall@freedos.org> */

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
#ifdef __WATCOMC__
#include <screen.h>
#else
#include <conio.h>
#endif

#include "box.h"
#include "globals.h"	/* for cat - catalog id - and #include "catgets.h" */
#include "text.h"       /* for strings printed */

void
repaint_empty (void)
{
  char *s = catgets (cat, SET_GENERAL, MSG_TITLE, MSG_TITLE_STR);

  /* Clear the screen and repaint an empty screen for the installer */
  if(!mono) textbackground(LIGHTGRAY);
  clrscr();

  gotoxy (2, 1);
  textcolor(BLUE);
  cputs (s);
  textcolor(WHITE);

  box (1, 2, 80, 24);
}
