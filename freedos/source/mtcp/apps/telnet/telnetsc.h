
/*

   mTCP TelnetSc.h
   Copyright (C) 2009-2011 Michael B. Brutman (mbbrutman@gmail.com)
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


   Description: Data structures for telnet screen handling code

   Changes:

   2011-05-27: Initial release as open source software

*/



#ifndef _TELNETSC_H
#define _TELNETSC_H


#include "types.h"



// We assume that we have an 80 column screen that that we devote all
// 80 columns to the terminal window.  The primary reason is that the
// number of columns is less than 80, we'll be able to shrink the virtual
// screen correctly but not the real screen.  Which makes addressing
// the real screen more complicated, and is not worth doing.

#define SCREEN_COLS     (80)
#define BYTES_PER_LINE (160)




// Watcom specifics.

#if defined ( __WATCOMC__ ) || defined ( __WATCOM_CPLUSPLUS__ )

// Inline asm functions
extern void fillUsingWord( uint16_t far * target, uint16_t fillWord, uint16_t len );
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


extern unsigned char wherex( void );
#pragma aux wherex = \
  "push bp"          \
  "mov ah, 3h"       \
  "mov bh, 0h"       \
  "int 10h"          \
  "add dl, 1"        \
  "pop bp"           \
  modify [ ax bx cx dx ] \
  value [ dl ];

extern unsigned char wherey( void );
#pragma aux wherey = \
  "push bp"          \
  "mov ah, 3h"       \
  "mov bh, 0h"       \
  "int 10h"          \
  "add dh, 1"        \
  "pop bp"           \
  modify [ ax bx cx dx ] \
  value [ dh ];


// Go fix usage of gotoxy so that we don't have to sub ...
extern void gotoxy( unsigned char col, unsigned char row );
#pragma aux gotoxy = \
  "push bp"     \
  "mov ah, 2h" \
  "mov bh, 0h"  \
  "sub dh, 1"   \
  "sub dl, 1"   \
  "int 10h" \
  "pop bp"  \
  parm [dl] [dh] \
  modify [ax bh dx];




#endif





class Screen {

  public:

    int8_t init( uint8_t backScrollPages, uint8_t initWrapMode );

    void clearConsole( void );  // Clears physical screen, not virtual
    void myCprintf( uint8_t attr, char *fmt, ... );


    void scroll( void );
    void scrollInternal( void );

    void add( char *buf );
    void add( char *buf, uint16_t len );
    void paint( void );
    void paint( int16_t offsetLines );
    void clear( uint16_t top_x, uint16_t top_y, uint16_t bot_x, uint16_t bot_y );

    void insLine( uint16_t line_y );
    void delLine( uint16_t line_y );

    void delChars( uint16_t len );
    void insChars( uint16_t len );
    void eraseChars( uint16_t len );

    inline uint16_t ScrOffset( uint16_t x, uint16_t y ) {

      uint32_t tmp = topOffset;
      tmp = tmp + (y<<7)+(y<<5)+(x<<1);
      if ( tmp >= bufferSize ) tmp = tmp - bufferSize;

      return tmp;
    }

    inline void updateVidBufPtr( void ) {
      vidBufPtr = Screen_base + ( ((cursor_x<<1) + (cursor_y<<7) + (cursor_y<<5)) );
    }

    uint16_t ScreenRows;        // How many rows on the screen?
    uint16_t ScreenCols;        // How many columns?  (Will be 80)

    uint8_t  colorCard;         // 0=Monochrome, 1=CGA, EGA, or VGA
    uint8_t  padding;

    uint8_t far *Screen_base;

    uint16_t terminalLines;     // How many lines for the terminal window?
    uint16_t terminalCols;      // How many columns in our terminal window?
    uint16_t totalLines;        // How many lines for viewable and backscroll?

    uint8_t far *buffer;        // Start of virtual screen buffer
    uint8_t far *bufferEnd;     // End of virtual screen buffer

    uint16_t bufferSize;        // Size of buffer area in bytes
    uint16_t topOffset;         // Offset in bytes to start of virtual screen

    int16_t cursor_x, cursor_y; // X and Y for cursor

    uint8_t far *vidBufPtr;     // Pointer into the real video buffer

    uint8_t curAttr;            // Current screen attribute
    uint8_t lastChar;           // Last printable char (used by some ANSI functions)

    uint8_t updateRealScreen;   // Should we be updating the live screen?
    uint8_t virtualUpdated;     // Have we updated the virtual screen?

    uint16_t backScrollOffset;  // If backscrolling is active, how far back?

    uint8_t wrapMode;           // Are we wrapping around lines?
    uint8_t  padding2;

};




#endif
