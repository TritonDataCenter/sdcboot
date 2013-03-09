/* $Id: getkey.h,v 1.2 2002/08/30 21:32:21 perditionc Exp $ */

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


#ifndef _GETKEY_H
#define _GETKEY_H

typedef struct {
  int key;
  int extended;
} key_t;


/* Function prototypes: */

key_t getkey (void);


/* 'extended' return values */

/* The key symbolic names have been lifted from the curs_getch man
 page on UNIX.  See the following for why I only define these few.

   Historically, the set of keypad macros was largely defined
   by  the  extremely  function-key-rich keyboard of the AT&T
   7300, aka 3B1, aka Safari 4.   Modern  personal  computers
   usually  have  only a small subset of these.  IBM PC-style
   consoles  typically  support  little  more  than   KEY_UP,
   KEY_DOWN,    KEY_LEFT,   KEY_RIGHT,   KEY_HOME,   KEY_END,
   KEY_NPAGE, KEY_PPAGE, and function keys 1 through 12.  The
   Ins key is usually mapped to KEY_IC.
*/

/* We haven't defined the Shift, Control, or Alt keysets in this
 file, but you can get those, too.  I don't need them.  -jh */

#define KEY_F1  59
#define KEY_F2  60
#define KEY_F3  61
#define KEY_F4  62
#define KEY_F5  63
#define KEY_F6  64
#define KEY_F7  65
#define KEY_F8  66
#define KEY_F9  67
#define KEY_F10 68

#define KEY_F11 133
#define KEY_F12 134

/* Keypad is arranged like this:
 +-----+------+-------+
 | A1  |  up  |  A3   |
 +-----+------+-------+
 |left |  B2  | right |
 +-----+------+-------+
 | C1  | down |  C3   |
 +-----+------+-------+
*/

#define KEY_HOME  71 /* A1 */
#define KEY_A1    71
#define KEY_UP    72
#define KEY_PPAGE 73 /* A3 */
#define KEY_A3    73

#define KEY_LEFT  75
#define KEY_B2    76
#define KEY_RIGHT 77

#define KEY_END   79 /* C1 */
#define KEY_C1    79
#define KEY_DOWN  80
#define KEY_NPAGE 81 /* C3 */
#define KEY_C3    81

#define KEY_IC    82
#define KEY_DC    83

#endif /* _GETKEY_H */
