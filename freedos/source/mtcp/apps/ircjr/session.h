
/*

   mTCP Session.h
   Copyright (C) 2011-2011 Michael B. Brutman (mbbrutman@gmail.com)
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


   Description: Session handling data structures for IRCjr

   Changes:

   2011-05-27: Initial release as open source software

*/





#ifndef _SESSION_H
#define _SESSION_H

#include "types.h"
#include "ircjr.h"
#include "irc.h"


#define MAX_SESSIONS (10)



// A Session represents an open channel, a private conversation with another
// user, or the server messages channel.

class Session {

  public:

    int16_t init( const char *name_p, uint16_t bufferRows );
    void    destroy( void );

    void    puts( uint8_t attr, const char *str );
    void    printf( uint8_t attr, char *fmt, ... );

    void    draw( void );         // Unconditional screen draw

    inline void drawIfUpdated( void ) {
      if ( was_updated ) { draw( ); was_updated = 0; }
    }



    char name[IRCNICK_MAX_LEN]; // Session name or identifier

    uint16_t *virtBuffer;       // Pointer to virtual buffer
    uint16_t  virtBufferRows;   // How many rows does it have
    uint16_t  virtBufferSize;   // How many bytes allocated?

    uint16_t output_x;          // Current position to write to (x,y)
    uint16_t output_y;

    uint16_t backScrollLines;   // How many backscroll lines are there
    int16_t  backScrollOffset;  // Current backscroll offset (0 = none)

    uint8_t was_updated;        // Has the virtual buffer been modified since last draw?
    uint8_t padding;            // Reserved padding



    // Class variables and functions

    static Session *activeSessionList[ MAX_SESSIONS ];
    static uint16_t activeSessions;

    static int16_t getSessionIndex( Session *target );
    static int16_t getSessionIndex( const char *name_p );

    static Session * createAndMakeActive( const char *name_p, uint16_t rows );
    static int16_t removeActiveSession( Session *target );

};


#endif
