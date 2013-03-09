
/*

   mTCP Screen.cpp
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


   Description: Screen handling code for IRCjr

   Changes:

   2011-05-27: Initial release as open source software

*/



#include <bios.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include <malloc.h>
#include <mem.h>
#include <string.h>

#include "screen.h"
#include "session.h"



uint8_t Screen::colorCard;
uint8_t far *Screen::screenBase;
uint16_t Screen::rows;
uint16_t Screen::separatorRow;
uint16_t Screen::outputRows;
uint8_t far *Screen::separatorRowAddr;
uint8_t far *Screen::inputAreaStart;

uint16_t Screen::cur_x, Screen::cur_y, Screen::cur_y2;

uint8_t Screen::insertMode;

uint16_t Screen::input_len;



char Screen::tmpInputBuffer[SCBUFFER_MAX_INPUT_LEN];
char *Screen::userInputBuffer;
uint8_t *Screen::switchToSession;



int8_t Screen::init( char *userInputBuffer_p, uint8_t *switchToSession_p ) {

  // This always works:
  unsigned char mode = *((unsigned char far *)MK_FP( 0x40, 0x49 ));

  if ( mode == 7 ) {
    colorCard = 0;
    screenBase = (uint8_t far *)MK_FP( 0xb000, 0 );
  }
  else {
    colorCard = 1;
    screenBase = (uint8_t far *)MK_FP( 0xb800, 0 );
  }

  // Call int 10, ah=12 for EGA/VGA config

  union REGS inregs, outregs;
  struct SREGS segregs;

  inregs.h.ah = 0x12;
  inregs.h.bl = 0x10;
  int86x( 0x10, &inregs, &outregs, &segregs );

  if ( outregs.h.bl == 0x10 ) {
    // Failed.  Must be MDA or CGA
    rows = 25;
  }
  else {
    rows = *((unsigned char far *)MK_FP( 0x40, 0x84 )) + 1;
  }

  separatorRow = rows - INPUT_ROWS - 1;
  outputRows = rows - (INPUT_ROWS + 1);

  separatorRowAddr = screenBase + ( separatorRow * BYTES_PER_ROW );
  inputAreaStart = separatorRowAddr + BYTES_PER_ROW;

  cur_y = cur_x = 0;
  cur_y2 = separatorRow + 1;

  input_len = 0;

  userInputBuffer = userInputBuffer_p;
  switchToSession = switchToSession_p;

  insertMode = 1;

  // Draw initial screen
  fillUsingWord( screenBase, 7<<8|32, rows * 80 );
  fillUsingWord( separatorRowAddr, 7<<8|196, 80 );

  return 0;
}





void Screen::clearInputArea( void ) {

  fillUsingWord( inputAreaStart, (7<<8|32), INPUT_ROWS * BYTES_PER_ROW );

  input_len = 0;
  cur_y = cur_x = 0;
  cur_y2 = separatorRow + 1;

  // Put the cursor back in the right spot to be pretty.
  gotoxy( cur_x, cur_y2 );

}



void Screen::repaintInputArea( uint16_t offset, char far *buffer, uint16_t len ) {

  uint8_t far *target = inputAreaStart + offset*2;

  for ( uint16_t i=0; i < len; i++ ) {
    *target = *buffer;
    buffer++;
    target += 2;
  }

  fillUsingWord( target, (7<<8|32), (SCBUFFER_MAX_INPUT_LEN-offset)*2 );

}



void Screen::cursorBack( void ) {
  if ( cur_x == 0 ) {
    cur_x = 79;
    cur_y--; cur_y2--;
  }
  else {
    cur_x--;
  }
}

void Screen::cursorForward( void ) {
  cur_x++;
  if ( cur_x == 80 ) {
    cur_x = 0;
    cur_y++; cur_y2++;
  }
}


Screen::InputActions Screen::getInput2( void ) {

  gotoxy( cur_x, cur_y2 );

  uint16_t key = bioskey(0);

  if ( (key & 0xff) == 0 ) {

    // Function key
    uint8_t fkey = key >> 8;

    switch ( fkey ) {

      case 19: return ShowRawToggle;          // Alt-R
      case 20: return TimestampToggle;        // Alt-T
      case 31: return Stats;                  // Alt-S
      case 35: return Help;                   // Alt-H
      case 38: return LoggingToggle;          // Alt-L
      case 45: return EndProgram;             // Alt-X
      case 46: return CloseWindow;            // Alt-C
      case 48: return BeepToggle;             // Alt-B
      case 73: return BackScroll;             // PgUp
      case 81: return ForwardScroll;          // PgDn

      case 71: {                              // Home
        cur_x = 0;
        cur_y = 0; cur_y2 = separatorRow+1;
        break;
      }

      case 72: {                                           // Up
        if ( cur_y ) {
          cur_y--; cur_y2--;
        }
        else {
          ERRBEEP( );
        }
        break;
      }

      case 75: {                                           // Left
        if ( !isCursorHome( ) ) {
          cursorBack( );
        }
        else {
          ERRBEEP( );
        }
        break;
      }

      case 77: {                                           // Right
        if ( cur_x + (cur_y * 80) < input_len ) {
          cursorForward( );
        }
        else {
          ERRBEEP( );
        }

        break;
      }

      case 79: {                                           // End
        cur_x = input_len % 80;
        cur_y = input_len / 80;
        cur_y2 = cur_y + separatorRow + 1;
        break;
      }

      case 80: {                                           // Down
        if ( cur_x + ((cur_y+1)*80) <= input_len ) {
          cur_y++; cur_y2++;
        }
        else {
          ERRBEEP( );
        }
        break;
      }


      case 82: {                                           // Insert

        insertMode = !insertMode;
        if ( insertMode ) {
          sound(500); delay(50); sound(750); delay(50); nosound( );
        }
        else {
          // Not really complaining - just indicating back to overstrike
          sound(750); delay(50); sound(500); delay(50); nosound( );
        }

        break;
      }

      case 83: {                                           // Delete

        // Has to be on an existing char

        uint16_t startIndex = cur_x + cur_y*80;

        if ( startIndex < input_len ) {

          memmove( tmpInputBuffer+startIndex, tmpInputBuffer+startIndex+1, (input_len-startIndex) );
          input_len--;
          tmpInputBuffer[input_len] = 0;

          repaintInputArea( startIndex, tmpInputBuffer+startIndex, (input_len-startIndex) );
        }
        else {
          ERRBEEP( );
        }

        break;
      }

      case 129:
        *switchToSession = 0;
        return SwitchSession;                 // Alt-0 = ServerSession

      default: {

        if ( fkey >= 120 && fkey <= 128 ) {
          *switchToSession = fkey - 119;
          return SwitchSession;
        }

      }

    }



  }
  else {

    // Normal key
    int c = key & 0xff;

    if ( ((c > 31) && (c < 127)) || (c > 127) ) {

      uint16_t startIndex = cur_x + cur_y*80;

      // Are we appending to the end of the buffer?

      if ( startIndex == input_len ) {

        // Is there room in the buffer to add more?
        if ( input_len < (SCBUFFER_MAX_INPUT_LEN-1) ) {
          tmpInputBuffer[ input_len++ ] = c;
          cursorForward( );
          putch( c );
        }
        else {
          ERRBEEP( );
        }

      }
      else {

        // In the middle of the line - insert or replace?

        if ( insertMode ) {

          // Room to insert?
          if ( input_len < (SCBUFFER_MAX_INPUT_LEN-1) ) {

            memmove( tmpInputBuffer+startIndex+1, tmpInputBuffer+startIndex, input_len-startIndex+1 );
            tmpInputBuffer[startIndex] = c;
            input_len++;
            tmpInputBuffer[input_len] = 0;

            cursorForward( );
            repaintInputArea( startIndex, tmpInputBuffer+startIndex, (input_len-startIndex) );

          }
          else {
            ERRBEEP( );
          }

        }
        else {
          tmpInputBuffer[ startIndex ] = c;
          cursorForward( );
          putch( c );
        }

      } // end editing in the middle of the line

    } // end of is it a printable character

    else if ( c == 8 ) {

      // If pressed at the end of the line eat the last char. If
      // pressed in the middle of the line slide remaining chars
      // back.

      if ( input_len ) {

        uint16_t startIndex = cur_x + cur_y*80;

        if ( startIndex == input_len ) {
          input_len--;
          cursorBack( );
          gotoxy( cur_x, cur_y2 ); putch( ' ' ); gotoxy( cur_x, cur_y2 );
        }
        else {

          if ( !isCursorHome( ) ) {
            memmove( tmpInputBuffer+startIndex-1, tmpInputBuffer+startIndex, (input_len-startIndex) );
            input_len--;
            cursorBack( );
            repaintInputArea( startIndex, tmpInputBuffer+startIndex, (input_len-startIndex) );
          }
          else {
            ERRBEEP( );
          }

        }

      }
      else {
        // No input to backspace over!
        ERRBEEP( );
      }


    }

    else if ( c == 13 ) {
      tmpInputBuffer[input_len] = 0;
      strcpy( userInputBuffer, tmpInputBuffer );
      clearInputArea( );
      return InputReady;
    }

    else if ( c == 27 ) {
      clearInputArea( );
    }


  } // end non-function keys

  gotoxy( cur_x, cur_y2 );

  return NoAction;

}



void Screen::writeOnConsole( uint8_t attr, uint8_t x, uint8_t y, char *msg ) {

  uint16_t far *start = (uint16_t far *)(screenBase + (y*80+x)*2);

  uint16_t len = strlen( msg );

  for ( uint16_t i = 0; i < len; i++ ) {
    *start++ = ( attr << 8 | msg[i] );
  }

}


