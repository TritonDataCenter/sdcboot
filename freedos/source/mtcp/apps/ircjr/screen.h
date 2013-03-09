
/*

   mTCP Screen.h
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


   Description: Screen handling data structures for IRCjr

   Changes:

   2011-05-27: Initial release as open source software

*/


#ifndef _SCREEN_H
#define _SCREEN_H

#include <bios.h>

#include "types.h"
#include "ircjr.h"


// The Screen represents the display device.  It paints the sessions on the
// screen and manages the status line and the user input area.

class Session;

class Screen {

  public:

    enum InputActions {
      NoAction=0,
      EndProgram,
      CloseWindow,
      InputReady,
      BackScroll,
      ForwardScroll,
      Stats,
      BeepToggle,
      Help,
      TimestampToggle,
      LoggingToggle,
      SwitchSession,
      ShowRawToggle
    };


    static uint8_t colorCard;
    static uint8_t far *screenBase;
    static uint16_t rows;

    static uint16_t separatorRow;
    static uint16_t outputRows;

    static uint8_t far *separatorRowAddr;
    static uint8_t far *inputAreaStart;

    static int8_t init( char *userInputBuffer_p, uint8_t *switchToSession );

    static void clearInputArea( void );
    static void repaintInputArea( uint16_t offset, char far *buffer, uint16_t len );


    static void writeOnConsole( uint8_t attr, uint8_t x, uint8_t y, char *msg );



    static inline InputActions getInput( void ) {
      if ( bioskey(1) ) return getInput2( ); else return NoAction;
    }

    static void cursorBack( void );
    static void cursorForward( void );

    static inline uint8_t isCursorHome( void ) {
      return (cur_x == 0) && (cur_y == 0);
    }



  private:

    static uint16_t cur_x, cur_y, cur_y2;
    static uint16_t input_len;
    static char *userInputBuffer;
    static uint8_t *switchToSession;

    static uint8_t insertMode;

    static char tmpInputBuffer[SCBUFFER_MAX_INPUT_LEN];

    static InputActions getInput2( void );



};



#endif
