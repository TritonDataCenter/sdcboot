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

#include <stdio.h>
#ifdef __WATCOMC__
#include <screen.h>
#else
#include <conio.h>
#endif
#include <string.h>
#include "getkey.h"         /* call getkey() instead of getch() directly */

#include "globals.h"    /* for cat - catalog id - and #include "catgets.h" */
/* #include "catgets.h"         * DOS catopen(), catgets() */
#include "text.h"                   /* All strings displayed */
#include "pause.h"          /* pause() prototype */
#include "box.h"


/* pauses execution with a pretty message */
void pause(void)
{
  char *s = catgets (cat, SET_PROMPT_LOC, MSG_PRESSKEY, MSG_PRESSKEY_STR),
        screenbuf[512];
  unsigned centre = 40 - (strlen(s) / 2);

  gettext(centre, 20, centre + strlen(s) + 2, 23, screenbuf);
  box(centre, 20, centre + strlen(s) + 1, 22);
  gotoxy (centre + 1, 21);
  textbackground(BLUE);
  cputs (catgets (cat, SET_PROMPT_LOC, MSG_PRESSKEY, MSG_PRESSKEY_STR));
  _setcursortype(_NOCURSOR);
  if (!nopauseflag) getkey();
  _setcursortype(_NORMALCURSOR);
  puttext(centre, 20, centre + strlen(s) + 2, 23, screenbuf);
}
