/* $Id: sel_list.c,v 1.4 2002/08/30 21:32:21 perditionc Exp $ */

/* A function to present a list to the user, and allow the user to
 select items from that list.  We use a very simple interface here,
 only allowing up/down arrow keys, and Enter.  A single char indicator
 replaces a "highlight". */

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


/* Returns the index in optv chosen by the user. */

/* No screen bounds checking is done here.  It is the responsibility
 of the programmer to ensure that the list does not exceed the screen
 boundaries.  If you want to have a pretty border around this list,
 you have to do that yourself before calling this function. */

/* The programmer must position the cursor before calling select_list */

/* We will turn off the cursor here, and will set to normal cursor
 when we exit.  This may or may not affect the appearance of the rest
 of your program. */

#ifdef __WATCOMC__
#include <screen.h>
#else
#include <conio.h>
#endif
#include <ctype.h>
#include <stdlib.h>         /* NULL */
#include <string.h>         /* strlen() */
#include <dos.h>
#include "getkey.h"
#include "box.h"                /* box() */

static int xord, xord1;

#ifndef __WATCOMC__
#undef _NOCURSOR
#undef _NORMALCURSOR
#undef _SOLIDCURSOR
#undef _HALFCURSOR
#define _NOCURSOR     3
#define _HALFCURSOR   2
#define _NORMALCURSOR 0
#define _SOLIDCURSOR  1

static void my_setcursortype( unsigned short state )
{
   struct REGPACK regs;
   int cur_mode;

   regs.r_ax = 0x0F00;
   intr( 0x21, &regs );
   cur_mode = regs.r_ax & 0xFF;
   regs.r_ax = 0x0100;
   /* ch == start line. cl == end line */

   switch (state)
   {
#if 0
          case _SOLIDCURSOR :
             regs.r_cx = (cur_mode == 7) ? 0x010C : 0x0107;
             break;
#endif
          case _NORMALCURSOR:
             regs.r_cx = (cur_mode == 7) ? 0x0B0C : 0x067;
             break;
/*#if 0*/ /* Unused */
          case _NOCURSOR    :
             regs.r_cx = 0x2020;
             break;
#if 0
          case _HALFCURSOR  :
             regs.r_cx = (cur_mode == 7) ? 0x070C :  0x0407;
             break;
          default           :
             regs.r_cx = state;
             break;
#endif
   }
   intr( 0x10, &regs );
}

#define _setcursortype my_setcursortype
#endif

/* Additional key definitions */

#define KEY_ENTER 13

/* Additional character defs */

#define ACS_RARROW 26  /* '>' */
#define SPACE      ' '


int
select_list (int optc, char *optv[])
{
  int start_y;         /* starting coordinates */
  int i, j, k;
  key_t key;

  /* Initialize starting coordinates */

  start_y = wherey();

  /* Print the options, indented by 2 */

  /* Position the pointer, and allow up/down until Enter */

  _setcursortype (_NOCURSOR);

  i = 0;
  do {
    for (j = 0; j < optc; j++)
      {
          if(j == i) {
            textbackground(WHITE);
            textcolor(BLUE);
          } else {
            textbackground(BLUE);
            textcolor(WHITE);
          }
          gotoxy (xord + 1, start_y + j);
          for(k = 0; k < (xord1 - xord - 1); k++) {
            int centre = (((xord1 - xord) / 2) - (strlen(optv[j]) / 2) - 1);
            if(k == centre) {
                cputs(optv[j]);
                k += strlen(optv[j]) - 1;
            } else putch(' ');
        }
      }
      textcolor(WHITE);

    key = getkey();

    switch (key.extended)
      {
      case KEY_UP:
    i = ( i == 0 ? optc - 1 : i-1 );
    break;

      case KEY_DOWN:
    i = ( i == optc - 1 ? 0 : i+1 );
    break;
      default:
        for(j = 0; j < optc; j++) {
          if(toupper(key.key & 0xFF) == toupper(optv[j][0])) i = j;
        }
      }
  } while (key.key != KEY_ENTER);

  _setcursortype (_NORMALCURSOR);

  /* Return the selection */

  return (i);
}

/* If yesToAll and/or noToAll are NULL, then those prompts not displayed */
int
select_yn (char *prompt, char *yes, char *no, char *yesToAll, char *noToAll)
{
  int len;
  int ret;
  int xPrompt;
  int listSize;
  char *yesno[4];
  char screenbuf[512];

  /* fill in yes/no/yestoall/notoall struct & get its size */

  listSize = 2;
  yesno[0] = yes;
  yesno[1] = no;
  if (yesToAll != NULL)
  {
    yesno[listSize] = yesToAll;
    listSize++;
  }
  if (noToAll != NULL)
  {
    yesno[listSize] = noToAll;
    listSize++;
  }


  /* Draw a box */

  len = strlen (prompt);

  xord = 40 - (len / 2) - 1;
  if (xord < 1)
    {
      xord = 1;
    }

  xord1 = xord + len + 1;
  if (xord1 > 80)
    {
      xord1 = 80;
    }

  gettext(xord, 22-listSize, xord1 + 1, 25, screenbuf);
  /* at 20 for yes/no, 19 for yes/no/yestoall or yes/no/notoall, 18 for all 4 */
  box (xord, 22-listSize, xord1, 25);

  /* Display the prompt, and do the y/n selection */

  gotoxy (xord + 1, 23-listSize);
  cputs (prompt);


  gotoxy (35, 24-listSize);
  ret = select_list (listSize, yesno);

  puttext(xord, 22-listSize, xord1 + 1, 25, screenbuf);

  return (ret);
}
