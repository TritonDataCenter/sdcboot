
/*

   mTCP Ircjr.h
   Copyright (C) 2008-2011 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP


   This file is part of mTCP.

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.


   Description: Some common defines and inline functions for IRCjr

   Changes:

   2011-05-27: Initial release as open source software

*/


#ifndef _IRCJR_H
#define _IRCJR_H



// Configuration

#define INPUT_ROWS (3)
#define SCBUFFER_MAX_INPUT_LEN (240)



// Common defines and utilities

#define BYTES_PER_ROW (160ul)


extern void ERRBEEP( void );



extern void fillUsingWord( void far * target, uint16_t fillWord, uint16_t len );
#pragma aux fillUsingWord = \
  "push es"    \
  "push di"    \
  "mov es, dx" \
  "mov di, bx" \
  "rep stosw"  \
  "pop di"     \
  "pop es"     \
  modify [ax]  \
  parm [dx bx] [ax] [cx]



// This gotoxy is zero based!

extern void gotoxy( unsigned char col, unsigned char row );
#pragma aux gotoxy = \
  "mov ah, 2h" \
  "mov bh, 0h"  \
  "int 10h" \
  parm [dl] [dh] \
  modify [ax bh dx];


#endif

#define normalizePtr( p, t ) {       \
  uint32_t seg = FP_SEG( p );        \
  uint16_t off = FP_OFF( p );        \
  seg = seg + (off/16);              \
  off = off & 0x000F;                \
  p = (t)MK_FP( (uint16_t)seg, off );          \
}

#define addToPtr( p, o, t ) {        \
  uint32_t seg = FP_SEG( p );        \
  uint16_t off = FP_OFF( p );        \
  seg = seg + (off/16);              \
  off = off & 0x000F;                \
  uint32_t p2 = seg << 4 | off ;       \
  p2 = (p2) + (o);                       \
  p = (t)MK_FP( (uint16_t)((p2)>>4), (uint16_t)((p2)&0xf) );          \
}

