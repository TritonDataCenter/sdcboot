
/*

   mTCP Session.cpp
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


   Description: Session handling code for IRCjr

   Changes:

   2011-05-27: Initial release as open source software

*/



#include <dos.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "session.h"
#include "screen.h"




Session *Session::activeSessionList[MAX_SESSIONS];
uint16_t Session::activeSessions = 0;




int16_t Session::init( const char *name_p, uint16_t bufferRows ) {

  // Add one extra row for the separator line.  Not used if we don't have a
  // backscroll buffer
  bufferRows++;

  // Fixme: Consolidate these two checks.
  if ( bufferRows > 380 ) bufferRows = 380;

  uint16_t bufferAllocSize = bufferRows * BYTES_PER_ROW;
  if ( bufferAllocSize >= 60*1024 ) {
    return -1;
  }

  uint16_t *tmpVirtBuffer = (uint16_t *)malloc( bufferAllocSize );
  if ( tmpVirtBuffer == NULL ) {
    return -1;
  }

  strncpy( name, name_p, 16 );
  name[15] = 0;

  virtBuffer = tmpVirtBuffer;
  virtBufferRows = bufferRows;
  virtBufferSize = bufferAllocSize;

  // Clear the buffer
  fillUsingWord( virtBuffer, (7<<8|32), (bufferAllocSize>>1) );

  output_x = output_y = 0;
  backScrollLines = virtBufferRows - Screen::outputRows - 1;

  backScrollOffset = 0;

  was_updated = 0;

  return 0;
}


void Session::destroy( void ) {
  if ( virtBuffer ) { free( virtBuffer ); }
  memset( this, 0, sizeof( Session ) );
}





// Find a session by pointer
//
int16_t Session::getSessionIndex( Session *target ) {
  for ( uint8_t i=0; i < activeSessions; i++ ) {
    if ( activeSessionList[i] == target ) return i;
  }
  return -1;
}


// Find a session by name
//
int16_t Session::getSessionIndex( const char *name_p ) {
  for ( uint8_t i=0; i < activeSessions; i++ ) {
    if ( stricmp( name_p, activeSessionList[i]->name ) == 0 ) return i;
  }
  return -1;
}



// createAndMakeActive
//
// Ensures a session with the same name doesn't exist already, allocates the
// storage for a new session, and adds it to the active list.  If anything
// goes wrong you get a NULL back.

Session *Session::createAndMakeActive( const char *name_p, uint16_t bufferRows ) {

  if ( activeSessions >= MAX_SESSIONS ) return NULL;

  // Ensure it's not active yet
  if ( getSessionIndex( name_p ) != -1 ) return NULL;

  Session *tmpSession = (Session *)malloc( sizeof(Session) );
  if ( tmpSession == NULL ) return NULL;

  if ( tmpSession->init( name_p, bufferRows ) ) {
    free( tmpSession );
    return NULL;
  }

  activeSessionList[activeSessions] = tmpSession;
  activeSessions++;

  return tmpSession;
}


int16_t Session::removeActiveSession( Session *target ) {

  int8_t index = getSessionIndex( target );
  if ( index == -1 ) return -1;

  // Slide everything down
  for ( int8_t i=index; i < activeSessions-1; i++ ) {
    activeSessionList[i] = activeSessionList[i+1];
  }
  activeSessions--;

  target->destroy( );
  free( target );

  return 0;
}



// Add text to a session
//
// Text is added to a virtual buffer that we maintain.  The buffer wraps
// around when it hits the end.

void Session::puts( uint8_t attr, const char *str ) {

  // Indicate that we need repainting.
  was_updated = 1;

  // We are never going to add more than a few hundred bytes
  // at a time.  Start by normalizing a pointer into the buffer,
  // then convert the normalized pointer back to a far pointer.
  // This avoids the overhead of huge pointer arithmetic on every
  // character.

  uint16_t far *current = virtBuffer;
  uint16_t offset = output_y * BYTES_PER_ROW + (output_x<<1);
  addToPtr( current, offset, uint16_t far * );

  // At this point current pointing at the next location to be written to.
  // (AddToPtr macro normalized the pointer.)


  while ( *str ) {

    if ( *str == '\n' ) {

      // Newline processing - drop to the start of the next line

      str++;
      current += 80-output_x;
      output_y++;
      output_x=0;
    }
    else {

      // Normal character: write it and advance one position

      *current = (attr<<8|*str);
      current++;
      str++;
      output_x++;

      // Did we wrap around the right edge?
      if ( output_x == 80 ) {
        output_y++;
        output_x=0;
      }
    }

    // If after processing that character we are on a new line then
    // we need to check to see if we wrapped around the end of the buffer,
    // clear the new line, and if we have backscroll capability draw the
    // divider.

    // Fix me: the divider should only be drawn once outside of the loop.


    if ( output_x==0 ) {

      if ( output_y == virtBufferRows ) {
        // Wrapped around the end of the buffer.
        output_y = 0;
        current = virtBuffer;
      }

      // Clear new line.  (It was probably used before.)
      fillUsingWord( current, 0, 80 );


      // Put a divider on the next line in case the backscroll buffer
      // is being displayed while it is being modified.

      if ( virtBufferRows > (Screen::outputRows+1) ) {

        // Tmp is already at the start of the line, but we need to be
        // sure that it wasn't supposed to wrap to the start of the buffer.

        // Skip past open line
        uint16_t *tmp = current + 80;

        if ( (output_y + 1) == virtBufferRows ) {
          tmp = (uint16_t far *)virtBuffer;
        }

        fillUsingWord( tmp, 0x0FCD, 80 );

      }

    }

  } // end for


}



static char VirtPrintfBuf[1024];

void Session::printf( uint8_t attr, char *fmt, ... ) {

  va_list ap;
  va_start( ap, fmt );
  vsnprintf( VirtPrintfBuf, 1024, fmt, ap );
  va_end( ap );

  VirtPrintfBuf[1023] = 0;

  puts( attr, VirtPrintfBuf );
}



void Session::draw( void ) {

  was_updated = 0;


  // We start from the bottom of the screen and calculate where the top of
  // the screen is.  If we have a partial line of output on the bottom
  // of the screen we need to make it look like the bottom of the screen
  // is one line lower in the virtual buffer so that we display the
  // partial line.

  int16_t start_y = output_y;
  if ( output_x ) start_y++;


  int16_t topRow = start_y - Screen::outputRows - backScrollOffset;
  if ( topRow < 0 ) topRow += virtBufferRows;

  uint8_t far *startAddr = (uint8_t far *)virtBuffer + (topRow*BYTES_PER_ROW);

  uint8_t far *screenBase = Screen::screenBase;

  for ( uint16_t i=0; i < Screen::outputRows; i++ ) {
    _fmemcpy( screenBase, startAddr, BYTES_PER_ROW );
    screenBase += BYTES_PER_ROW;
    startAddr += BYTES_PER_ROW;

    if ( (topRow + i) == (virtBufferRows - 1) ) {
      // Wrapped
      startAddr = (uint8_t far *)virtBuffer;
    }
  }

}

