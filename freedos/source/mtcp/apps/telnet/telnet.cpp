
/*

   mTCP Telnet.cpp
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


   Description: Telnet client

   Changes:

   2011-05-27: Initial release as open source software
   2011-10-01: Fix telnet options bug; was not checking to see
               if any free packets were available
   2011-10-25: Fix the above bug, again, correctly this time ...
               Check some more return codes, especially when
               allocating memory at startup.
               Minor restructuring of code.
               Ability to abort connection by pressing Esc,
               Ctrl-C or Ctrl-Break while waiting for it.


   Todo:

     Interpret Unicode characters
     Perf improvement: Minimize use of ScOffset in insline and delline
*/



#include <bios.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"

#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "tcp.h"
#include "tcpsockm.h"
#include "udp.h"
#include "dns.h"
#include "telnet.h"
#include "telnetsc.h"



// Buffer lengths

#define SERVER_NAME_MAXLEN   (80)
#define TCP_RECV_BUF_SIZE  (4096)
#define RECV_BUF_SIZE      (1024)
#define TERMTYPE_MAXLEN      (30)






// Globals: Server information

TcpSocket *mySocket;

char     serverAddrName[SERVER_NAME_MAXLEN];   // Target server name
IpAddr_t serverAddr;                           // Target server Ip address
uint16_t serverPort = 23;                      // Target server port (default is telnet)




// Globals: Toggles and options

uint8_t DebugTelnet = 0;       // Are we spitting out messages for telnet?
uint8_t DebugAnsi = 0;         // Are we spitting out messages for ANSI codes?
uint8_t RawOrTelnet = 1;       // Are we doing telnet or just raw?
uint8_t InitWrapMode = 1;      // Normally we wrap
uint8_t SendBsAsDel  = 1;
uint8_t LocalEcho    = 0;      // Is local echoing enabled?
uint8_t NewLineMode  = 0;      // 0 is CR/LF, 1 is CR, 2 is LF
uint8_t BackScrollPages = 4;

uint32_t ConnectTimeout = TCP_CONNECT_TIMEOUT; // How many ms to wait for a connection

char TermType[TERMTYPE_MAXLEN] = "ANSI";




// Function prototypes

static uint16_t processSocket( uint8_t *recvBuffer, uint16_t len );
static uint16_t processEscSeq( uint8_t *buffer, uint16_t len );

static int16_t  processTelnetCmds( uint8_t *cmdStr, uint8_t cmdSize );
static int16_t  processSingleTelnetCmd( uint8_t *cmdStr, uint8_t inputBufLen, uint8_t *outputBuf, uint16_t *outputBufLen );

static void parseArgs( int argc, char *argv[] );
static void getCfgOpts( void );

static void resolveAndConnect( void );
static void sendInitialTelnetOpts( void );
static void shutdown( int rc );

static void doHelp( void );







// Used for outgoing data - should clean up the 1460 number.
// In practice we never send more than three or four bytes and
// 1460 assumes MTU 1500 with 40 bytes of IP and TCP header.
// Real MTU might be smaller.

typedef struct {
  TcpBuffer b;
  uint8_t data[1460];
} DataBuf;




// Trap Ctrl-Break and Ctrl-C so that we can unhook the timer interrupt
// and shutdown cleanly.

// Check this flag once in a while to see if the user wants out.
volatile uint8_t CtrlBreakDetected = 0;

void ( __interrupt __far *oldCtrlBreakHandler)( );

void __interrupt __far ctrlBreakHandler( ) {
  CtrlBreakDetected = 1;
}

void __interrupt __far ctrlCHandler( ) {
  // Do Nothing
}




// Telnet options negotiation variables

TelnetOpts MyTelnetOpts;


// Telnet opts for this program
//
//    Option             Remote   Local
//  0 Binary             on       on
//  1 Echo               on       off
//  3 SGA                on       on
//  5 Status             off      off
//  6 Timing mark        off      off
// 24 Terminal type      off      on
// 31 Window Size        off      on
// 32 Terminal speed     off      off
// 33 Remote Flow Ctrl   off      off
// 34 Linemode           off      off
// 35 X Display          off      off
// 36 Environment vars   off      off
// 39 New environment    off      off





// Screen handling and emulation

Screen s;

enum StreamStates { Normal = 0, ESC_SEEN, CSI_SEEN, IAC_SEEN };
StreamStates StreamState;


// Ansi code handling globals

#define CSI_ARGS (16)
#define CSI_DEFAULT_ARG (-1)

int16_t parms[ CSI_ARGS ];
uint8_t parmsFound = 0;

uint8_t traceBuffer[60];
uint8_t traceBufferLen = 0;

uint8_t fg=7;
uint8_t bg=0;
uint8_t bold=0;
uint8_t blink=0;
uint8_t underline=0;
uint8_t reverse=0;

int16_t saved_cursor_x = 0, saved_cursor_y = 0;


uint8_t *fgColorMap;
uint8_t *bgColorMap;

// Input is ANSI, output is CGA Attribute
uint8_t fgColorMap_CGA[] = {
  0, // 0 - Black
  4, // 1 - Red,
  2, // 2 - Green,
  6, // 3 - Yellow
  1, // 4 - Blue
  5, // 5 - Magenta
  3, // 6 - Cyan
  7, // 7 - White
  7, // 8 - (undefined)
  7  // 9 - (reset to default)
};

uint8_t bgColorMap_CGA[] = {
  0, // 0 - Black
  4, // 1 - Red,
  2, // 2 - Green,
  6, // 3 - Yellow
  1, // 4 - Blue
  5, // 5 - Magenta
  3, // 6 - Cyan
  7, // 7 - White
  0, // 8 - (undefined)
  0  // 9 - (reset to default)
};

uint8_t fgColorMap_Mono[] = {
  0, // 0 - Black
  7, // 1 - Red,
  7, // 2 - Green,
  7, // 3 - Yellow
  7, // 4 - Blue
  7, // 5 - Magenta
  7, // 6 - Cyan
  7, // 7 - White
  7, // 8 - (undefined)
  7  // 9 - (reset to default)
};

uint8_t bgColorMap_Mono[] = {
  0, // 0 - Black
  0, // 1 - Red,
  0, // 2 - Green,
  0, // 3 - Yellow
  0, // 4 - Blue
  0, // 5 - Magenta
  0, // 6 - Cyan
  7, // 7 - White
  0, // 8 - (undefined)
  0  // 9 - (reset to default)
};


uint8_t scNormal;       // Normal text
uint8_t scBright;       // Bright/Bold
uint8_t scTitle;        // Title - used only at startup
uint8_t scBorder;       // Border lines on help window
uint8_t scCommandKey;   // Used in the help menu
uint8_t scToggleStatus; // Used in the help menu




static char tmpBuf[160];


static char CopyrightMsg1[] = "mTCP Telnet by M Brutman (mbbrutman@gmail.com) (C)opyright 2009-2011\r\n";
static char CopyrightMsg2[] = "Version: " __DATE__ "\r\n\r\n";



uint8_t EatOneKeystroke = 0;

// Five bits for up to 32 special keys

#define K_NoKey        (0)
#define K_NormalKey    (1)
#define K_CursorUp     (2)
#define K_CursorDown   (3)
#define K_CursorLeft   (4)
#define K_CursorRight  (5)
#define K_PageUp       (6)
#define K_PageDown     (7)
#define K_Home         (8)
#define K_Insert       (9)
#define K_Backtab     (10)
#define K_Alt_R       (11)
#define K_Alt_W       (12)
#define K_Alt_H       (13)
#define K_Alt_X       (14)
#define K_Alt_B       (15)
#define K_Enter       (16)
#define K_Alt_E       (17)
#define K_Alt_N       (18)


typedef struct {
  uint8_t specialKey;
  uint8_t local;
  uint8_t normalKey;
} Key_t;


static Key_t getKey( void ) {

  Key_t rc;
  rc.specialKey = K_NoKey;
  rc.local = 0;
  rc.normalKey = 0;

  uint16_t c = bioskey(0);

  // Special key?
  if ( (c & 0xff) == 0 ) {

    uint8_t fkey = c>>8;

    switch ( fkey ) {

      case 15: { rc.specialKey = K_Backtab;     break; }
      case 17: { rc.specialKey = K_Alt_W;       rc.local = 1; break; }
      case 18: { rc.specialKey = K_Alt_E;       rc.local = 1; break; }
      case 19: { rc.specialKey = K_Alt_R;       rc.local = 1; break; }
      case 35: { rc.specialKey = K_Alt_H;       rc.local = 1; break; }
      case 45: { rc.specialKey = K_Alt_X;       rc.local = 1; break; }
      case 48: { rc.specialKey = K_Alt_B;       rc.local = 1; break; }
      case 49: { rc.specialKey = K_Alt_N;       rc.local = 1; break; }
      case 71: { rc.specialKey = K_Home;        break; }
      case 72: { rc.specialKey = K_CursorUp;    break; }
      case 73: { rc.specialKey = K_PageUp;      rc.local = 1; break; }
      case 75: { rc.specialKey = K_CursorLeft;  break; }
      case 77: { rc.specialKey = K_CursorRight; break; }
      case 80: { rc.specialKey = K_CursorDown;  break; }
      case 81: { rc.specialKey = K_PageDown;    rc.local = 1; break; }
      case 82: { rc.specialKey = K_Insert;      break; }

    }

  }
  else {

    rc.specialKey = K_NormalKey;
    rc.normalKey = ( c & 0xff );

    // Special enter key processing - we want to be able to tell the
    // difference between Enter and Ctrl-M.
    if ( rc.normalKey == 13 ) {
      if ( (c>>8) == 0x1c ) { // This was the Enter key.
	rc.specialKey = K_Enter;
	rc.normalKey = 0;
      }
    }

  }

  return rc;
}








int main( int argc, char *argv[] ) {

  printf( "%s  %s", CopyrightMsg1, CopyrightMsg2 );

  parseArgs( argc, argv );

  // Initialize TCP/IP

  if ( Utils::parseEnv( ) != 0 ) {
    exit(-1);
  }

  getCfgOpts( );


  if ( Utils::initStack( 1, TCP_SOCKET_RING_SIZE ) ) {
    printf( "\nFailed to initialize TCP/IP - exiting\n" );
    exit(-1);
  }

  // From this point forward you have to call the shutdown( ) routine to
  // exit because we have the timer interrupt hooked.


  // Save off the oldCtrlBreakHander and put our own in.  Shutdown( ) will
  // restore the original handler for us.
  oldCtrlBreakHandler = getvect( 0x1b );
  setvect( 0x1b, ctrlBreakHandler);

  // Get the Ctrl-C interrupt too, but do nothing.  We actually want Ctrl-C
  // to be a legal character to send when in interactive mode.
  setvect( 0x23, ctrlCHandler);



  // Allocate memory for a receive buffer.  This is in addition to the normal
  // socket receive buffer.  Do this early so that we don't get too far into
  // the code before failing.

  uint8_t *recvBuffer = (uint8_t *)malloc( RECV_BUF_SIZE );

  if ( (recvBuffer == NULL) || s.init( BackScrollPages, InitWrapMode ) ) {
    puts( "\nNot enough memory - exiting\n" );
    shutdown( -1 );
  }


  if ( s.colorCard == 0 ) {
    fgColorMap = fgColorMap_Mono;
    bgColorMap = bgColorMap_Mono;
  }
  else {
    fgColorMap = fgColorMap_CGA;
    bgColorMap = bgColorMap_CGA;
  }



  // Set color palette up
  if ( s.colorCard ) {
    scNormal       = 0x07; // White on black
    scBright       = 0x0F; // Bright White on black
    scTitle        = 0x1F; // Bright White on blue
    scBorder       = 0x0C; // Bright Red on black
    scCommandKey   = 0x09; // Bright Blue on black
    scToggleStatus = 0x0E; // Yellow on black
  }
  else {
    scNormal       = 0x02; // Normal
    scBright       = 0x0F; // Bright
    scTitle        = 0x0F; // Bright
    scBorder       = 0x0F; // Bright
    scCommandKey   = 0x01; // Underlined
    scToggleStatus = 0x01; // Underlined
  }



  s.curAttr = scTitle;
  s.add( CopyrightMsg1 );
  s.curAttr = scNormal;
  s.add( "  " );
  s.curAttr = scTitle;
  s.add( CopyrightMsg2 );
  s.curAttr = scNormal;


  resolveAndConnect( );

  s.add ( "Remember to use " );
  s.curAttr = scBright; s.add( "Alt-H" ); s.curAttr = scNormal;
  s.add( " for help!\r\n\r\n" );


  sprintf( tmpBuf, "Connected to %s (%d.%d.%d.%d) on port %d\r\n\r\n",
	   serverAddrName, serverAddr[0], serverAddr[1],
	   serverAddr[2], serverAddr[3], serverPort );
  s.add( tmpBuf );


  sendInitialTelnetOpts( );


  uint8_t done = 0;
  uint8_t remoteDone = 0;

  uint16_t bytesToRead = RECV_BUF_SIZE;
  uint16_t bytesInBuffer = 0;

  while ( !done && !remoteDone ) {

    if ( CtrlBreakDetected ) {
      done = 1;
      break;
    }

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );


    // Process incoming packets first.

    if ( remoteDone == 0 ) {

      while ( 1 ) {

	int16_t recvRc = mySocket->recv( recvBuffer+bytesInBuffer, bytesToRead-bytesInBuffer );

	PACKET_PROCESS_SINGLE;
	Arp::driveArp( );
	Tcp::drivePackets( );

	if ( recvRc > 0 ) {
	  bytesInBuffer += recvRc;
          bytesInBuffer = processSocket( recvBuffer, bytesInBuffer );
	}
	else {
	  break;
	}

      } // end while


      // We might have had bytes to process even though we have not received new
      // data.  Take care of them.  (This only should happen if we are processing
      // telnet options and we use up too many outgoing buffers with small payloads.)

      if ( bytesInBuffer ) {
        bytesInBuffer = processSocket( recvBuffer, bytesInBuffer );
      }

      remoteDone = mySocket->isRemoteClosed( );
    }


    if ( s.virtualUpdated ) {
      s.paint( );
      s.updateVidBufPtr( );
      EatOneKeystroke = 0;
    }

    gotoxy( s.cursor_x+1, s.cursor_y+1 );


    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );




    if ( bioskey(1) ) {

      Key_t key=getKey( );

      if ( EatOneKeystroke ) {key.specialKey = K_NoKey; EatOneKeystroke = 0; s.paint( ); }

      if ( key.specialKey != K_NoKey ) {

	if ( key.local ) {

	  if ( key.specialKey == K_PageUp ) {
	    s.paint( s.terminalLines );
	  }
	  else if ( key.specialKey == K_PageDown ) {
	    s.paint( 0 - s.terminalLines );
	  }
	  else if ( key.specialKey == K_Alt_R ) {
	    s.clearConsole( ); // Flash the screen so they know we did something
	    s.paint( );
	  }
	  else if ( key.specialKey == K_Alt_W ) {
	    s.wrapMode = !s.wrapMode;
	    if ( s.wrapMode ) {
	      sound(500); delay(50); sound(750); delay(50); nosound( );
	    }
	    else {
	      sound(500); delay(50); nosound( );
	    }
	  }
	  else if ( key.specialKey == K_Alt_E ) {
	    LocalEcho = !LocalEcho;
	    if ( LocalEcho ) {
	      sound(500); delay(50); sound(750); delay(50); nosound( );
	    }
	    else {
	      sound(500); delay(50); nosound( );
	    }
	  }
          else if ( key.specialKey == K_Alt_N ) {
            NewLineMode++;
            if ( NewLineMode == 3 ) NewLineMode = 0;
	    sound(500); delay(50); sound(750); delay(50); nosound( );
          }
	  else if ( key.specialKey == K_Alt_B ) {
	    SendBsAsDel = !SendBsAsDel;
	    if ( SendBsAsDel ) {
	      sound(500); delay(50); sound(750); delay(50); nosound( );
	    }
	    else {
	      sound(500); delay(50); nosound( );
	    }
	  }
	  else if ( key.specialKey == K_Alt_H ) {
	    doHelp( );
	  }
	  else if ( key.specialKey == K_Alt_X ) {
	    done = 1;
	  }

	}

	else {

	  DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
	  if ( buf != NULL ) {

	    buf->b.dataLen = 0;

	    uint8_t sk = key.specialKey;

	    switch ( sk ) {

	      case K_NormalKey: {

		if ( SendBsAsDel ) {
		  if ( key.normalKey == 8 ) {
		    key.normalKey = 127;
		  }
		  else if ( key.normalKey == 127 ) {
		    key.normalKey = 8;
		  }
		}

		buf->b.dataLen = 1;
		buf->data[0] = key.normalKey;
		break;
	      }

	      case K_Enter: {
                switch ( NewLineMode ) {
                  case 0: {
		    buf->b.dataLen = 2;
		    buf->data[0] = 0x0D; buf->data[1] = 0x0A;
		    break;
                  }
                  case 1: {
                    buf->b.dataLen = 1;
                    buf->data[0] = 0x0D;
                    break;
                  }
                  case 2: {
                    buf->b.dataLen = 1;
                    buf->data[0] = 0x0A;
                    break;
                  }
                }
                break;
	      }

	      case K_Backtab: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'Z';
		break;
	      }

	      case K_Home: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'H';
		break;
	      }

	      case K_CursorUp: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'A';
		break;
	      }

	      case K_CursorDown: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'B';
		break;
	      }

	      case K_CursorLeft: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'D';
		break;
	      }

	      case K_CursorRight: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'C';
		break;
	      }

	      case K_Insert: {
		buf->b.dataLen = 3;
		buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'L';
		break;
	      }

	    }

	    if ( LocalEcho ) {

	      // Update screen first before we give the buffer away.
	      // We don't do local echo ANSI strings.  Right now all of
	      // our ANSI strings (and only them) are three bytes long,
	      // so this is an easy way to detect them.
	      if ( buf->b.dataLen != 3 ) {
		s.add( (char *)buf->data, buf->b.dataLen );
	      }

	    }

	    if ( buf->b.dataLen ) {
              // Fixme: check return code
	      mySocket->enqueue( &buf->b );
	    }

	  } // End if able to get outgoing buffer

	} // end if else local key

      } // end if key we could process

    } // end if keyboard pressed

  }

  s.curAttr = 0x07;

  s.add( "\r\nConnection closing\r\n" );

  mySocket->close( );

  TcpSocketMgr::freeSocket( mySocket );

  shutdown( 0 );

  return 0;
}





void resolveAndConnect( void ) {

  s.add( "Resolving server address - press [ESC] to abort\r\n\r\n" );


  int8_t rc;

  // Resolve the name and definitely send the request
  int8_t rc2 = Dns::resolve( serverAddrName, serverAddr, 1 );
  if ( rc2 < 0 ) {
    s.add( "Error resolving server: " );
    s.add( serverAddrName );
    s.add( "\r\n" );
    shutdown( -1 );
  }

  uint8_t done = 0;

  while ( !done ) {

    if ( CtrlBreakDetected ) {
      break;
    }

    if ( kbhit( ) ) {
      char c = getch( );
      if ( c == 27 ) {
        s.add( "[Esc] pressed - quitting.\r\n" );
        shutdown( -1 );
      }
    }

    if ( !Dns::isQueryPending( ) ) break;

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );
    Dns::drivePendingQuery( );

  }

  // Query is no longer pending or we bailed out of the loop.
  rc2 = Dns::resolve( serverAddrName, serverAddr, 0 );

  if ( rc2 != 0 ) {
    s.add( "Error resolving server: " );
    s.add( serverAddrName );
    s.add( "\r\n" );
    shutdown( -1 );
  }

  sprintf( tmpBuf, "Server %s resolved to %d.%d.%d.%d\r\nConnecting to port %u...\r\n\r\n",
           serverAddrName, serverAddr[0], serverAddr[1],
           serverAddr[2], serverAddr[3], serverPort );
  s.add( tmpBuf );


  // Make the socket connection

  mySocket = TcpSocketMgr::getSocket( );
  if ( mySocket->setRecvBuffer( TCP_RECV_BUF_SIZE ) ) {
    s.add( "Ouch!  Not enough memory to run!\r\n\r\n" );
    shutdown( -1 );
  }


  rc = mySocket->connectNonBlocking( rand()%2000+2048, serverAddr, serverPort );

  if ( rc == 0 ) {

    clockTicks_t start = TIMER_GET_CURRENT( );

    while ( 1 ) {

      PACKET_PROCESS_SINGLE;
      Tcp::drivePackets( );
      Arp::driveArp( );

      if ( mySocket->isConnectComplete( ) ) { break; }

      if ( bioskey(1) ) {
        char c = getch( );
        if ( c == 3 || c == 27 ) {
          s.add( "[Ctrl-C] or [Esc] pressed - quitting.\r\n" );
          shutdown( -1 );
        }
      }

      if ( CtrlBreakDetected ) {
        s.add( "[Ctrl-Break] pressed - quitting.\r\n" );
          shutdown( -1 );
      }

      if ( mySocket->isClosed( ) || (Timer_diff( start, TIMER_GET_CURRENT( ) ) > TIMER_MS_TO_TICKS( ConnectTimeout )) ) {
        rc = -1;
        break;
      }

      // Sleep for 50 ms just in case we are cutting TRACE records at
      // a furious pace.
      delay(50);
    }

  }

  if ( rc != 0 ) {
    s.add( "Socket connection failed\r\n" );
    shutdown( -1 );
  }

}



void sendInitialTelnetOpts( void ) {

  MyTelnetOpts.setWantRmtOn( TELOPT_BIN );
  MyTelnetOpts.setWantLclOn( TELOPT_BIN );


  MyTelnetOpts.setWantRmtOn( TELOPT_ECHO );
  MyTelnetOpts.setWantRmtOn( TELOPT_SGA );

  MyTelnetOpts.setWantLclOn( TELOPT_SGA );
  MyTelnetOpts.setWantLclOn( TELOPT_TERMTYPE );
  MyTelnetOpts.setWantLclOn( TELOPT_WINDSIZE );


  // Send initial Telnet options
  if ( RawOrTelnet == 1 ) {

    MyTelnetOpts.setDoOrDontPending( TELOPT_ECHO );
    MyTelnetOpts.setDoOrDontPending( TELOPT_SGA );
    MyTelnetOpts.setDoOrDontPending( TELOPT_BIN );

    uint8_t output[] = { TEL_IAC, TELCMD_DO, TELOPT_ECHO,
			 TEL_IAC, TELCMD_DO, TELOPT_SGA,
			 TEL_IAC, TELCMD_DO, TELOPT_BIN };

    mySocket->send( output, sizeof(output) );
    Tcp::drivePackets( );
  }

}





void doHelp( void ) {

  s.updateRealScreen = 0;
  EatOneKeystroke = 1;

  uint8_t i;

  #ifdef __TURBOC__
  textattr( scNormal );

  for ( i=2; i<19; i++ ) {
    gotoxy( 1, i );
    clreol( );
  }
  #else
  uint16_t *start = (uint16_t far *)(s.Screen_base + 2*80);
  fillUsingWord( start, (scNormal<<8|32), 17*80 );
  #endif


  gotoxy( 1, 2 );
  for ( i=0; i < 20; i++ ) { s.myCprintf( scBorder, "%c%c%c%c", 205, 205, 205, 205 ); }


  gotoxy( 1, 3 );
  s.myCprintf( scTitle, "%s", CopyrightMsg1 );
  s.myCprintf( scNormal, "  " );
  s.myCprintf( scTitle, "%s", CopyrightMsg2 );


  s.myCprintf( scNormal, "Commands: " ); s.myCprintf( scCommandKey, "Alt-H" ); s.myCprintf( scNormal, " Help    " ); s.myCprintf( scCommandKey, "Alt-R" ); s.myCprintf( scNormal, " Refresh    " ); s.myCprintf( scCommandKey, "Alt-X" ); s.myCprintf( scNormal, " Exit\r\n" );

  s.myCprintf( scNormal, "Toggles:  " ); s.myCprintf( scCommandKey, "Alt-E" ); s.myCprintf( scNormal, " Local Echo On/Off   " );
  s.myCprintf( scCommandKey, "Alt-W" ); s.myCprintf( scNormal, " Wrap at right margin On/Off\r\n" );
  s.myCprintf( scNormal, "          " ); s.myCprintf( scCommandKey, "Alt-B" ); s.myCprintf( scNormal, " Send Backspace as Delete On/Off\r\n" );
  s.myCprintf( scNormal, "          " ); s.myCprintf( scCommandKey, "Alt-N" ); s.myCprintf( scNormal, " Send [Enter] as CR/LF, CR or LF\r\n\r\n" );

  s.myCprintf( scNormal, "Term Type: " ); s.myCprintf( scToggleStatus, "%s", TermType );
  s.myCprintf( scNormal, "   Virtual buffer pages: " ); s.myCprintf( scToggleStatus, "%d  ", BackScrollPages );
  s.myCprintf( scNormal, "Echo: " ); s.myCprintf( scToggleStatus, "%s", (LocalEcho?"On":"Off") );
  s.myCprintf( scNormal, "   Wrap: " ); s.myCprintf( scToggleStatus, "%s\r\n", (s.wrapMode?"On":"Off") );
  s.myCprintf( scNormal, "Send Backspace as Delete: " ); s.myCprintf( scToggleStatus, "%s", (SendBsAsDel?"On":"Off") );
  s.myCprintf( scNormal, "   Send [Enter] as: " );

  switch ( NewLineMode ) {
    case 0: s.myCprintf( scToggleStatus, "CR/LF\r\n\r\n" ); break;
    case 1: s.myCprintf( scToggleStatus, "CR\r\n\r\n" ); break;
    case 2: s.myCprintf( scToggleStatus, "LF\r\n\r\n" ); break;
  }



  s.myCprintf( scNormal, "Tcp: Sent %lu Rcvd %lu Retrans %lu Seq/Ack errs %lu Dropped %lu\r\n",
               Tcp::Packets_Sent, Tcp::Packets_Received, Tcp::Packets_Retransmitted,
               Tcp::Packets_SeqOrAckError, Tcp::Packets_DroppedNoSpace );
  s.myCprintf( scNormal, "Packets: Sent: %lu Rcvd: %lu Dropped: %lu SendErrs: LowFreeBufs: %u\r\n\r\n",
	   Packets_sent, Packets_received, Packets_dropped, Packets_send_errs, Buffer_lowFreeCount );

  s.myCprintf( scBright, "Press a key to go back to your session ...\r\n" );

  for ( i=0; i < 20; i++ ) { s.myCprintf( scBorder, "%c%c%c%c", 205, 205, 205, 205 ); }
}




char *HelpText[] = {
  "\ntelnet <ipaddr> [port]\n\n",
  "Options:\n",
  "  -help                      Shows this help\n",
  "  -debug_ansi                Turn on debuging for ANSI escape codes\n",
  "  -debug_telnet              Turn on debugging for telnet options\n",
  "  -sessiontype <telnet|raw>  Force telnet mode or raw mode instead\n",
  NULL
};



static void usage( void ) {
  uint8_t i=0;
  while ( HelpText[i] != NULL ) {
    printf( HelpText[i] );
    i++;
  }
  exit( 1 );
}



static void parseArgs( int argc, char *argv[] ) {

  uint8_t RawOrTelnetForced = 0;

  int i=1;
  for ( ; i<argc; i++ ) {

    if ( argv[i][0] != '-' ) break;

    if ( stricmp( argv[i], "-help" ) == 0 ) {
      usage( );
    }
    else if ( stricmp( argv[i], "-debug_telnet" ) == 0 ) {
      strcpy( Utils::LogFile, "telnet.log" );
      Utils::Debugging |= 3;
      DebugTelnet = 1;
    }
    else if ( stricmp( argv[i], "-debug_ansi" ) == 0 ) {
      strcpy( Utils::LogFile, "telnet.log" );
      Utils::Debugging |= 3;
      DebugAnsi = 1;
    }
    else if ( stricmp( argv[i], "-sessiontype" ) == 0 ) {
      i++;
      if ( i == argc ) {
	puts( "Must specify a session type with the -sessiontype option" );
	usage( );
      }
      if ( stricmp( argv[i], "raw" ) == 0 ) {
	RawOrTelnet = 0;
	RawOrTelnetForced = 1;
      }
      else if ( stricmp( argv[i], "telnet" ) == 0 ) {
	RawOrTelnet = 1;
	RawOrTelnetForced = 1;
      }
      else {
	puts( "Unknown session type specified on the -sessiontype option" );
	usage( );
      }
    }
    else {
      printf( "Unknown option %s\n", argv[i] );
      usage( );
    }

  }

  if ( i < argc ) {
    strncpy( serverAddrName, argv[i], SERVER_NAME_MAXLEN-1 );
    serverAddrName[SERVER_NAME_MAXLEN-1] = 0;
    i++;
  }
  else {
    printf( "Need to specify a server name to connect to.\n" );
    usage( );
  }

  if ( i < argc ) {
    serverPort = atoi( argv[i] );
    if (serverPort == 0) {
      printf( "If you specify a port it can't be this: %s\n", argv[i] );
      usage( );
    }
    if ( serverPort != 23 && (RawOrTelnetForced == 0) ) RawOrTelnet = 0;
  }


}


void getCfgOpts( void ) {

  Utils::openCfgFile( );

  char tmp[10];

  if ( Utils::getAppValue( "TELNET_VIRTBUFFER_PAGES", tmp, 10 ) == 0 ) {
    BackScrollPages = atoi( tmp );
    if ( BackScrollPages == 0 ) BackScrollPages = 1;
  }

  if ( Utils::getAppValue( "TELNET_CONNECT_TIMEOUT", tmp, 10 ) == 0 ) {
    ConnectTimeout = ((uint32_t)atoi(tmp)) * 1000ul;
    if ( ConnectTimeout == 0 ) {
      ConnectTimeout = TCP_CONNECT_TIMEOUT;
    }
  }

  if ( Utils::getAppValue( "TELNET_AUTOWRAP", tmp, 10 ) == 0 ) {
    InitWrapMode = atoi(tmp);
    InitWrapMode = InitWrapMode != 0;
  }

  if ( Utils::getAppValue( "TELNET_SENDBSASDEL", tmp, 10 ) == 0 ) {
    SendBsAsDel = atoi(tmp);
    SendBsAsDel = SendBsAsDel != 0;
  }

  if ( Utils::getAppValue( "TELNET_SEND_NEWLINE", tmp, 10 ) == 0 ) {
    if ( stricmp( tmp, "CR/LF" ) == 0 ) {
      NewLineMode = 0;
    }
    else if ( stricmp( tmp, "CR" ) == 0 ) {
      NewLineMode = 1;
    }
    else if ( stricmp( tmp, "LF" ) == 0 ) {
      NewLineMode = 2;
    }
  }

  char tmpTermType[TERMTYPE_MAXLEN];
  if ( Utils::getAppValue( "TELNET_TERMTYPE", tmpTermType, TERMTYPE_MAXLEN ) == 0 ) {
    // Uppercase is the convention
    strupr( tmpTermType );
    strcpy( TermType, tmpTermType );
  }

  Utils::closeCfgFile( );

}





static void shutdown( int rc ) {

  setvect( 0x1b, oldCtrlBreakHandler);

  Utils::endStack( );
  Utils::dumpStats( stderr );
  fclose( TrcStream );
  exit( rc );
}






// processSocket - read and process the data received from the socket
//
// Return code is the number of bytes that were left in the buffer.
// If bytes are left in the buffer they will be moved to the beginning
// of the buffer so that the next socket read will append to them and
// have room to do so.

static uint16_t processSocket( uint8_t *recvBuffer, uint16_t len ) {

  uint16_t i;
  for ( i=0; i < len; i++ ) {

    if ( StreamState == ESC_SEEN ) {
      if ( recvBuffer[i] == '[' ) {
	StreamState = CSI_SEEN;
	parmsFound = 0;
	parms[0] = CSI_DEFAULT_ARG;
      }
      else {
	// Esc char was eaten - return to normal processing.
	StreamState = Normal;
      }
    }

    else if ( StreamState == CSI_SEEN ) {
      uint16_t rc = processEscSeq( recvBuffer+i, (len-i) );
      s.updateVidBufPtr( );
      i = i + rc - 1;
    }

    else if ( StreamState == IAC_SEEN ) {

      if ( MyTelnetOpts.isRmtOn( TELOPT_BIN ) && recvBuffer[i] == TEL_IAC ) {
	// Treat has a normal character.  This is ugly, but should
	// also be rare.
	s.add( (char *)(recvBuffer+i), 1 );
      }
      else {

	// It really is a telnet command ...
	int16_t rc = processTelnetCmds( recvBuffer+i, (len-i) );

        // If a telnet option is processed move 'i' forward the correct
        // number of chars.  Keep in mind that TEL_IAC has already been
        // seen, so we are passing the next character to the telnet
        // options parser.
        //
        // If a zero comes back we either did not have a full telnet command
        // in the buffer to parse or there was a socket error.  If there was
        // not a full telnet command to parse then preemptively slide the buffer
        // down to make more room for future characters from the socket, assuming
        // that we will get more input.
        //
        // The buffer slide doesn't help in our error condition, but eventually
        // we'll read the socket and figure out there is a problem.

	if ( rc > 0 ) {
	  i = i + (rc-1);
	}
	else {
	  // Ran out of data in the buffer!
	  // Move data and break the loop.
	  memmove( recvBuffer, recvBuffer+i, (len-i) );
	  break;
	}

      }

      StreamState = Normal;
    }


    else {

      if ( (RawOrTelnet == 1) && recvBuffer[i] == TEL_IAC ) {
	StreamState = IAC_SEEN;
      }

      else if ( recvBuffer[i] == 27 ) {
	StreamState = ESC_SEEN;
      }

      else {

	// Not a telnet or an ESC code.  Do screen handling here.

	// This is lifted from Screen::add.  I tried to use an inline
	// function to make this common code, but the compiler literally
	// blew up, probably due to nested inlines or the inline for add
	// being too complex.

	uint8_t c = recvBuffer[i];

	if ( c == 0 ) {         // Null char
	  // Do nothing
	}

	else if ( c == '\r' ) { // Carriage Return
	  s.cursor_x = 0;
	  s.updateVidBufPtr( );
	}

	else if ( c == '\n' ) { // Line Feed
	  s.scroll( );
	  s.updateVidBufPtr( );
	}

	else if ( c == '\a' ) { // Attention/Bell
	  sound(1000); delay(200); nosound( );
	}

	else if ( c == '\t' ) { // Tab
	  uint16_t newCursor_x = (s.cursor_x + 8) & 0xF8;
	  if ( newCursor_x < s.terminalCols ) s.cursor_x = newCursor_x;
	  s.updateVidBufPtr( );
	}

	else if ( c == 8 || c == 127 ) { // Backspace or Delete Char
	  // Fixme: If this was delete char we really should blank it out.
	  if ( s.cursor_x > 0 ) {
	    s.cursor_x--;
	    s.updateVidBufPtr( );
	  }
	}

	else {

	  s.lastChar = c;

	  s.buffer[ s.ScrOffset(s.cursor_x, s.cursor_y ) ] = c;
	  s.buffer[ s.ScrOffset(s.cursor_x, s.cursor_y ) + 1 ] = s.curAttr;

	  if ( s.updateRealScreen ) {

	    *s.vidBufPtr = c;
	    s.vidBufPtr++;
	    *s.vidBufPtr = s.curAttr;
	    s.vidBufPtr++;

	  }
	  else {
	    s.virtualUpdated = 1;
	  }

	  s.cursor_x++;
	  if ( s.cursor_x == s.terminalCols ) {
	    if ( s.wrapMode ) {
	      s.cursor_x = 0;
	      s.scroll( );
	    }
	    else {
	      s.cursor_x = s.terminalCols - 1;
	    }
	  }

	}

      // end of lifted code from Screen::add

      }

    } // end of printable char area

  }  // end for

  return (len - i);
}




// Telnet negotiation from Pg 403 of TCP/IP Illustrated Vol 1
//
// Sender    Receiver
// Will      Do        Sender wants, receiver agrees
// Will      Dont      Sender wants, receiver says no
// Do        Will      Sender wants other side to do it, receiver will
// Do        Wont      Sender wants other side to do it, receiver wont
// Wont      Dont      Sender says no way, receiver must agree
// Dont      Wont      Sender says dont do it, receiver must agree
//
// Page 1451 in TCP/IP Guide is good too.


// processTelnetCmds
//
// By the time we get here we have seen TEL_IAC.
//
// Process the first Telnet command, and then enter a loop to process
// any others that might be in the input.  They often come in groups.
// Try to buildup a single packet with out responses to avoid using
// up all of our outgoing packets with small responses.
//
// When we do not find any more options return the number of input
// bytes that we consumed.  Also push out our response packet, and
// ensure that it actually goes out.  (We will sit and wait for all
// of the bytes to be sent.)
//
// Return codes: n is the number of input bytes consumed
//               0 if the input buffer was incomplete (try again later)
//              -1 send error (probably fatal)


uint8_t telnetOptionsOutput[ 100 ];

static int16_t processTelnetCmds( uint8_t *cmdStr, uint8_t cmdSize ) {

  int16_t inputBytesConsumed = 0;
  uint16_t outputBufLen = 0;;

  uint16_t localOutputBufLen;
  int16_t localInputBytesConsumed = processSingleTelnetCmd( cmdStr, cmdSize, telnetOptionsOutput, &localOutputBufLen );

  if (localInputBytesConsumed == 0) {
    // Incomplete input to parse a full Telnet option - return to try again later.
    return 0;
  }


  outputBufLen = localOutputBufLen;
  inputBytesConsumed = localInputBytesConsumed;
  cmdStr += localInputBytesConsumed;
  cmdSize -= localInputBytesConsumed;


  // Ensure a minimum of 50 chars are available for output from processSingleTelnetCmd.
  // It does not do overflow checking, so it had better fit.

  while ( ((100 - outputBufLen) > 50 ) && (cmdSize > 1) && (*cmdStr == TEL_IAC) ) {

    // Another Telnet option!
    //
    // Ensure that if we are in Telnet BINARY mode that we handle two TEL_IACs consecutive correctly.

    if ( MyTelnetOpts.isRmtOn( TELOPT_BIN ) ) {
      if ( *(cmdStr+1) == TEL_IAC ) {
        // Not ours!  Let our caller handle it.
        break;
      }
    }

    localInputBytesConsumed = processSingleTelnetCmd( cmdStr+1, cmdSize-1, telnetOptionsOutput+outputBufLen, &localOutputBufLen );

    if ( localInputBytesConsumed == 0 ) {
      // Incomplete input to parse the option; skip this for now.
      break;
    }

    outputBufLen += localOutputBufLen;

    // Need to skip an extra byte for the initial TEL_IAC

    inputBytesConsumed += localInputBytesConsumed + 1;
    cmdStr += localInputBytesConsumed + 1;
    cmdSize -= localInputBytesConsumed + 1;

  }


  if ( DebugTelnet ) {
    TRACE(( "Consumed %d bytes of telnet options bytes, Sending %d bytes of response data\n", inputBytesConsumed, outputBufLen ));
  }

  // Push the output data out.  This is super paranoid, but do the full loop
  // including processing packets in between to ensure that they get sent.

  int16_t bytesSent = 0;

  while ( outputBufLen ) {

    int16_t rc = mySocket->send( telnetOptionsOutput+bytesSent, outputBufLen );

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );

    if ( rc == -1 ) {
      return -1;
    }
    else if ( rc > 0 ) {
      bytesSent += rc;
      outputBufLen -= rc;
    }
  }


  return inputBytesConsumed;
} 





// Process one telnet command.  The TEL_IAC character is already consumed 
// before this is called so we are dealing with the second character in
// the sequence.
//
// The caller provides the output buffer, and it must have enough available
// space in it.  At the moment we set that to 50 chars which is far more
// than we will ever need for one telnet option response.  That keeps us from
// having to check each time we add a byte to the output.

// Return codes: n is the number of input bytes consumed
//               0 if the input buffer was incomplete (try again later)
//
// The outputBufLen parm is used a secondary return code.

static int16_t processSingleTelnetCmd( uint8_t *cmdStr, uint8_t inputBytes, uint8_t *outputBuf, uint16_t *outputBufLen ) {

  uint8_t localOutputBufLen = 0;

  // Set return parameter to something sane before getting involved.
  *outputBufLen = 0;

  // Not enough input.
  if ( inputBytes < 1 ) return 0;

  char debugMsg[120];
  uint8_t debugMsgLen = 0;

  // Return code is how many bytes to remove from stream.
  uint16_t inputBytesConsumed = 1;

  switch ( cmdStr[0] ) {


    case TELCMD_WILL: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t respCmd;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Received WILL %u, ", cmd );
      }

      if ( MyTelnetOpts.isWantRmtOn( cmdTableIndex ) ) {
	respCmd = TELCMD_DO;
	MyTelnetOpts.setRmtOn( cmdTableIndex );
      }
      else {
	respCmd = TELCMD_DONT;
	MyTelnetOpts.setRmtOff( cmdTableIndex );
      }

      if ( MyTelnetOpts.isDoOrDontPending( cmdTableIndex ) ) {
	MyTelnetOpts.clrDoOrDontPending( cmdTableIndex );
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Was waiting a reply so no resp sent\n" );
	}
      }
      else {
	outputBuf[0] = TEL_IAC;
	outputBuf[1] = respCmd;
	outputBuf[2] = cmd;
	localOutputBufLen = 3;
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent %s\n", (respCmd==TELCMD_DO?"DO":"DONT") );
	}
      }

      break;

    }


    case TELCMD_WONT: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Received WONT %u, ", cmd );
      }

      // Our only valid response is DONT

      MyTelnetOpts.setRmtOff( cmdTableIndex );

      if ( MyTelnetOpts.isDoOrDontPending( cmdTableIndex ) ) {
	MyTelnetOpts.clrDoOrDontPending( cmdTableIndex );
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Was waiting a reply so no resp sent\n" );
	}
      }
      else {
	outputBuf[0] = TEL_IAC;
	outputBuf[1] = TELCMD_DONT;
	outputBuf[2] = cmd;
	localOutputBufLen = 3;
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent DONT\n" );
	}
      }

      break;
    }


    case TELCMD_DO: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t respCmd;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Received DO   %u, ", cmd );
      }

      if ( MyTelnetOpts.isWantLclOn( cmdTableIndex ) ) {
	respCmd = TELCMD_WILL;
	MyTelnetOpts.setLclOn( cmdTableIndex );
      }
      else {
	respCmd = TELCMD_WONT;
	MyTelnetOpts.setLclOff( cmdTableIndex );
      }

      if ( MyTelnetOpts.isWillOrWontPending( cmdTableIndex ) ) {
	MyTelnetOpts.clrWillOrWontPending( cmdTableIndex );
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Was waiting a reply so no resp sent\n" );
	}
      }
      else {
	outputBuf[0] = TEL_IAC;
	outputBuf[1] = respCmd;
	outputBuf[2] = cmd;
	localOutputBufLen = 3;
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent %s\n", (respCmd==TELCMD_WILL?"WILL":"WONT") );
	}
      }

      if ( cmd == TELOPT_WINDSIZE && respCmd == TELCMD_WILL ) {
	outputBuf[3] = TEL_IAC;
	outputBuf[4] = TELCMD_SUBOPT_BEGIN;
	outputBuf[5] = TELOPT_WINDSIZE;
	outputBuf[6] = 0;
	outputBuf[7] = s.terminalCols;
	outputBuf[8] = 0;
	outputBuf[9] = s.terminalLines;
	outputBuf[10] = TEL_IAC;
	outputBuf[11] = TELCMD_SUBOPT_END;
	localOutputBufLen = 12;
      }


      break;

    }


    case TELCMD_DONT: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Received DONT %u, ", cmd );
      }

      // Our only valid response is WONT

      MyTelnetOpts.setLclOff( cmdTableIndex );

      if ( MyTelnetOpts.isWillOrWontPending( cmdTableIndex ) ) {
	MyTelnetOpts.clrWillOrWontPending( cmdTableIndex );
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Was waiting a reply so no resp sent\n" );
	}
      }
      else {
	outputBuf[0] = TEL_IAC;
	outputBuf[1] = TELCMD_WONT;
	outputBuf[2] = cmd;
	localOutputBufLen = 3;
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent WONT\n" );
	}
      }

      break;
    }




    case TELCMD_SUBOPT_BEGIN: { // Suboption begin

      // First thing to do is to find TELCMD_SUBOPT_END

      uint16_t suboptEndIndex = 0;
      for ( uint16_t i=1; i < inputBytes-1; i++ ) {
	if ( cmdStr[i] == TEL_IAC && cmdStr[i+1] == TELCMD_SUBOPT_END ) {
	  inputBytesConsumed = i + 2;
	  suboptEndIndex = i;
	}
      }

      if ( suboptEndIndex < 3 ) return 0;

      if ( (suboptEndIndex == 3) && (cmdStr[1] == TELOPT_TERMTYPE) ) {

	if ( cmdStr[2] == 1 && cmdStr[3] == TEL_IAC && cmdStr[4] == TELCMD_SUBOPT_END ) {
	  outputBuf[0] = TEL_IAC;
	  outputBuf[1] = TELCMD_SUBOPT_BEGIN;
	  outputBuf[2] = TELOPT_TERMTYPE;
	  outputBuf[3] = 0;

	  localOutputBufLen = 4;

	  for ( uint8_t j=0; j<strlen(TermType); j++ ) {
	    outputBuf[localOutputBufLen++] = TermType[j];
	  }

	  outputBuf[localOutputBufLen++] = TEL_IAC;
	  outputBuf[localOutputBufLen++] = TELCMD_SUBOPT_END;
	  if ( DebugTelnet ) {
	    debugMsgLen = sprintf( debugMsg, "Sent termtype %s\n", TermType );
	  }
	}

      }
      else {
	if ( DebugTelnet ) {
	  debugMsgLen += sprintf( debugMsg+debugMsgLen, "Unknown SUBOPT: %u\n", cmdStr[2] );
	}
      }

      break;

    }



    case TELCMD_NOP:   // Nop
    case TELCMD_DM:    // Data Mark
    case TELCMD_BRK:   // Break (break or attention key hit)
    case TELCMD_IP:    // Interrupt process
    case TELCMD_AO:    // Abort output
    {
      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Telnet: Ignored command: %u\n", cmdStr[0] );
      }
      break;
    }

    case TELCMD_AYT: { // Are you there?
      // Send a null command back - that should be sufficent
      outputBuf[0] = TEL_IAC;
      outputBuf[1] = TELCMD_NOP;
      localOutputBufLen = 2;
      break;
    }

    default: {
      if ( DebugTelnet ) {
	debugMsgLen = sprintf( debugMsg, "Telnet: Unprocessed Command: %u\n", cmdStr[0] );
      }
    }

  }

  if ( DebugTelnet ) {
    TRACE(( "%s", debugMsg ));
  }


  *outputBufLen = localOutputBufLen;
  return inputBytesConsumed;

}




// This gets called when we are in state CSI_SEEN, which means that
// we have seen ESC [.
//
// If we run out of bytes before seeing a command we will pick up
// where we left off.  The parms and trace buffer are all globals.

static uint16_t processEscSeq( uint8_t *buffer, uint16_t len ) {

  // Used only for debugging/tracing
  uint16_t start_cursor_x = s.cursor_x;
  uint16_t start_cursor_y = s.cursor_y;


  // Number of bytes consumed by this processing.
  uint16_t bytesProcessed = 0;

  uint8_t ansiCmd = 0;

  uint16_t i = 0;

  while ( i < len ) {

    uint8_t c = buffer[i];

    // Used only for debugging/tracing
    traceBuffer[traceBufferLen++] = c;


    // Is this a numeric parm?
    if ( isdigit( c ) ) {

      if ( parmsFound < CSI_ARGS ) {
	// Ok, we have room for this parm.
	// If this is the first char of this parm, set it to 0.
	if ( parms[ parmsFound ] == CSI_DEFAULT_ARG ) parms[ parmsFound ] = 0;
	// Another char in the parm ..
	parms[ parmsFound ] = parms[ parmsFound ] * 10 + c - '0';
      }

    }
    else if ( c == ';' ) {
      if ( parmsFound < CSI_ARGS ) {
	parmsFound++;
	parms[ parmsFound ] = CSI_DEFAULT_ARG;
      }
    }
    else {
	i++;

	if ( parmsFound < CSI_ARGS ) {
	  if ( parms[ parmsFound ] != CSI_DEFAULT_ARG ) {
	    parmsFound++;
	  }
	}

	ansiCmd = c;
	break;
    }

    i++;

  } // end while

  bytesProcessed = i;

  // Run out of bytes?
  if ( ansiCmd == 0 ) return bytesProcessed;

  traceBuffer[traceBufferLen] = 0;
  traceBufferLen = 0;


  switch ( ansiCmd ) {

    case '@': { // ICH - Insert Character
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      s.insChars( parms[0] );
      break;
    }

    case 'A': { // CUU - Cursor Up
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_y = s.cursor_y - parms[0];
      if ( s.cursor_y < 0 ) s.cursor_y = 0;
      break;
    }

    case 'e':   // VPR -
    case 'B': { // CUD - Cursor Down
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_y = s.cursor_y + parms[0];
      if ( s.cursor_y >= s.terminalLines ) s.cursor_y = s.terminalLines - 1;
      break;
    }

    case 'a':   // HPR -
    case 'C': { // CUF - Cursor Forward
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_x = s.cursor_x + parms[0];
      if ( s.cursor_x >= s.terminalCols ) s.cursor_x = s.terminalCols - 1;
      break;
    }

    case 'D': { // CUB - Cursor Back
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_x = s.cursor_x - parms[0];
      if ( s.cursor_x < 0 ) s.cursor_x = 0;
      break;
    }

    case 'E': { // CNL - Cursor Next Line
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_y = s.cursor_y + parms[0];
      if ( s.cursor_y >= s.terminalLines ) s.cursor_y = s.terminalLines - 1;
      s.cursor_x = 0;
      break;
    }

    case 'F': { // CPL - Cursor Previous Line
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_y = s.cursor_y - parms[0];
      if ( s.cursor_y < 0 ) s.cursor_y = 0;
      s.cursor_x=0;
      break;
    }

    case '`':   // HPA - Set Horizontal Position
    case 'G': { // CHA - Cursor Horizontal Absolute
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_x = parms[0] - 1;
      if ( s.cursor_x < 0 ) s.cursor_x = 0; else if ( s.cursor_x >= s.terminalCols ) s.cursor_x = s.terminalCols-1;
      break;
    }

    case 'f':   // HVP - Horizontal and Vertical Position
    case 'H': { // CUP - Cursor Position

      // First parm would have been initialized to default, but not
      // the second.  Take care that here.
      if ( parmsFound < 2 ) {
	parms[1] = CSI_DEFAULT_ARG;
      }

      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      if ( parms[1] == CSI_DEFAULT_ARG ) parms[1] = 1;
      s.cursor_y = parms[0] - 1;
      if ( s.cursor_y < 0 ) s.cursor_y = 0; else if ( s.cursor_y >= s.terminalLines ) s.cursor_y = s.terminalLines-1;
      s.cursor_x = parms[1] - 1;
      if ( s.cursor_x < 0 ) s.cursor_x = 0; else if ( s.cursor_x >= s.terminalCols ) s.cursor_x = s.terminalCols-1;
      break;
    }

    case 'I': { // CHT - Cursor Horizontal Tabulation
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) {
	int16_t newCursor_x = (s.cursor_x + 8) & 0xF8;
	if ( newCursor_x < s.terminalCols ) s.cursor_x = newCursor_x;
      }
      break;
    }

    case 'Z': { // Backtab

      // Have the capability to do more than one - not sure if this is
      // in the spec.

      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) {


	int16_t newCursor_x;
	if ( ((s.cursor_x & 0xF8) == s.cursor_x) && (s.cursor_x>0) ) {
	  // Already at a tab stop boundary, go back eight
	  newCursor_x = s.cursor_x - 8;
	}
	else {
	  // Not on a tab stop boundary - just round down
	  newCursor_x = s.cursor_x & 0xF8;
	}

	if ( newCursor_x >= 0 ) s.cursor_x = newCursor_x;
      }
      break;
    }

    case 'J': { // ED - Erase Data
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;
      switch ( parms[0] ) {
	case 0: {
	  s.clear( s.cursor_x, s.cursor_y, s.terminalCols-1, s.terminalLines-1 );
	  break;
	}
	case 1: {
	  s.clear( 0, 0, s.cursor_x, s.cursor_y );
	  break;
	}
	case 2: {
	  s.clear( 0, 0, s.terminalCols-1, s.terminalLines-1 );
	  s.cursor_x = 0;
	  s.cursor_y = 0;
	  break;
	}
      }
      break;
    }

    case 'K': { // EL - Erase in Line
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;
      switch ( parms[0] ) {
	case 0: {
	  s.clear( s.cursor_x, s.cursor_y, s.terminalCols-1, s.cursor_y );
	  break;
	}
	case 1: {
	  s.clear( 0, s.cursor_y, s.cursor_x, s.cursor_y );
	  break;
	}
	case 2: {
	  s.clear( 0, s.cursor_y, s.terminalCols-1, s.cursor_y );
	  break;
	}
      }
      break;
    }

    case 'L': { // IL - Insert Lines at current cursor position
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint8_t i=0; i < parms[0]; i++ ) s.insLine( s.cursor_y );
      break;
    }

    case 'M': { // DL - Delete Lines at current cursor position
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint8_t i=0; i < parms[0]; i++ ) s.delLine( s.cursor_y );
      break;
    }

    case 'S': { // SU/INDN - Scroll screen up without changing cursor pos
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint8_t i=0; i < parms[0]; i++ ) s.scrollInternal( );
      break;
    }

    case 'T': { // RIN - Scroll screen down without changing cursor pos
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint8_t i=0; i < parms[0]; i++ ) s.insLine( 0 );
      break;
    }

    case 'm': {

      if ( parmsFound == 0 ) {
	parmsFound = 1;
	parms[0] = 0;
      }

      for ( uint8_t p=0; p < parmsFound; p++ ) {
	if ( parms[p] >= 30 && parms[p] < 40 ) {
	  fg = fgColorMap[ parms[p] - 30 ];
	}
	else if ( parms[p] >= 40 ) {
	  bg = bgColorMap[ parms[p] - 40 ];
	}
	else {
	  switch ( parms[p] ) {

	    case 0: {
	      reverse = underline = bold = blink = bg = 0;
	      fg = 7;
	      break;
	    }

	    case  1: { bold = 1; break; }      // Bold
	    case  2: { bold = 0; break; }      // Faint
	    case  3: { break; }                // Italic
	    case  4: { underline = 1; break; } // Underline
	    case  5: { blink = 1; break; }     // Slow blink
	    case  6: { blink = 1; break; }     // Fast blink
	    case  7: { reverse = 1; break; }   // Reverse
	    case  8: { break; }                // Conceal
	    case 21: { underline = 1; break; } // Double underline
	    case 22: { bold = 0; break; }      // Normal intensity
	    case 24: { underline = 0; break; } // No underline
	    case 25: { blink = 0; break; }     // Blink off
	    case 27: { reverse = 0; break; }   // Reverse off
	    case 28: { break; }                // Conceal off
	  }
	}
      } // end for

      uint8_t newAttr;
      if ( ! reverse ) {
	newAttr = (blink<<7) | (bg<<4) | (bold<<3) | fg;
      }
      else {
	newAttr = (blink<<7) | (fg<<4) | (bold<<3) | bg;
      }

      if ( s.colorCard == 0 && underline ) newAttr = (blink<<7) | (bg<<4) | (bold<<3) | 0x01;
      s.curAttr = newAttr;
      break;
    }

    case 'n': { // DSR - Device Status Report
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;
      switch( parms[0] ) {
	char outBuf[12];
	case 5: { // Terminal Status? Return OK
	  strcpy( outBuf, "\033[0n" );
	  mySocket->send( (uint8_t *)outBuf, 4 );
	  break;
	}
	case 6: { // Return line and col
	  int rcBytes = sprintf( outBuf, "\033[%d;%dR", (s.cursor_y+1), (s.cursor_x+1) );
	  mySocket->send( (uint8_t *)outBuf, rcBytes );
	  break;
	}
      }
      break;
    }

    case 'd': { // VPA - Vertical Position Absolute
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.cursor_y = parms[0] - 1;
      if ( s.cursor_y < 0 ) s.cursor_y = 0; else if ( s.cursor_y >= s.terminalLines ) s.cursor_y = s.terminalLines-1;
      break;
    }

    case 'b': { // REP
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      if ( parms[0] > 80 ) {
	TRACE_WARN(( "Ansi: REP Command: parm (%u) > 80\n", parms[0] ));
	parms[0] = 80;
      }

      uint8_t buf[80];
      memset( buf, s.lastChar, parms[0] );

      s.add( (char *)buf, parms[0] );

      break;
    }

    case 'P': { // DCH - Delete Character
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      s.delChars( parms[0] );
      break;
    }

    case 'X': { // ECH - Erase Character
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      s.eraseChars( parms[0] );
      break;
    }

    case 's': { // Save cursor position
      saved_cursor_x = s.cursor_x;
      saved_cursor_y = s.cursor_y;
      break;
    }


    case 'u': { // Restored saved cursor position
      s.cursor_x = saved_cursor_x;
      s.cursor_y = saved_cursor_y;
      break;
    }




    default: {
      TRACE_WARN(( "Ansi: Unknown cmd: %c %s\n", ansiCmd, traceBuffer ));
      break;
    }


  }


  if ( DebugAnsi ) {
    TRACE(( "Ansi: Old cur: (%02d,%02d) New cur: (%02d,%02d) Attr: %04x Cmd: %s\n",
            start_cursor_x, start_cursor_y, s.cursor_x, s.cursor_y, s.curAttr, traceBuffer ));
  }

  // Set this here instead in the routine that called us.
  // Because we remember state across calls, the routine that called us
  // never knows when to set StreamState back to normal.
  StreamState = Normal;

  return bytesProcessed;
}
