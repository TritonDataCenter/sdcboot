

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


// MAIN.C - main() for 4dos
//   (c) 1993 - 2002  Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <io.h>
#include <malloc.h>

#include "4all.h"


int _cdecl main( int argc, char **argv )
{

	// initialize the OS-specific stuff
	InitOS( argc, argv );

	if ( fRuntime == 0 ) {

		// kludge for bug in Tom's? Microsoft's? startup code
		if (( QueryIsConsole( STDIN ) == 0 ) && ( _isatty( STDIN ) == 0 ))
			_lseek( STDIN, 0L, SEEK_SET );

		while ( gnTransient == 0 ) {

			// reset ^C and "stack full" handling
			cv.fException = 0;

			// reset DO / IFF parsing flags
			cv.f.lFlags = 0;

			// reset batch single-stepping flag
			gpIniptr->SingleStep = 0;

			if ( setjmp( cv.env ) != -1 ) {

				EnableSignals();

				// do an INT 2F to signal we're displaying the prompt
_asm {
				xor     bx,bx           ; 0 = about to display prompt
				mov     ax, 0D44Eh
				int     2Fh
}
				if ( gpIniptr->SDFlush != 0 )
					SDFlush();

				ShowPrompt();
				EnableSignals();

				// do an INT 2F to signal we're about to accept input
_asm {
				mov     bx, 1           ; 0 = about to accept input
				mov     ax, 0D44Eh
				int     2Fh
}
				gszCmdline[0] = '\0';

				// get command line from STDIN
				argc = getline( STDIN, gszCmdline, MAXLINESIZ-1, EDIT_COMMAND );

				// do an INT 2F to signal that input is complete
_asm {
				mov     bx, 2           ; 2 = input complete
				mov     ax, 0D44Eh
				int     2Fh
}

				if ( gszCmdline[0] != '\0' ) {

					// add line to the history list BEFORE
					//   doing alias & variable expansion
					addhist( gszCmdline );

					// parse and execute command
					command( gszCmdline, 1 );

				} else if (( argc == 0 ) && ( _isatty( STDIN ) == 0 )) {
					// if redirected or piped process, quit when
					//   no more input
					break;
				}
			}
		}
	}

	// reset ^C and "stack full" handling
	HoldSignals();
	cv.fException = 0;

	return ( Exit_Cmd( NULL ) );
}
