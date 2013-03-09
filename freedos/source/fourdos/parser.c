

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


// PARSER.C - parsing routines for 4xxx / TCMD family
//   (c) 1988 - 2002  Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"


int _near ProcessCommands( TCHAR *, TCHAR *, int );
int _near InternalCommands( TCHAR *, int, int * );
int _near ExternalCommands( TCHAR *, TCHAR *, int );
static int _near _fastcall CommandGroups( TCHAR *, int );
static void _fastcall ParsePrompt( TCHAR _far *, LPTSTR pszBuffer );


// INT 2Eh "back door" to 4DOS; also used by %@EXEC[] in all platforms
int PASCAL DoINT2E( TCHAR _far *pszCommand )
{
	register int nReturn;
	CRITICAL_VARS save_cv;

	// save the critical variables
	memmove( &save_cv, &cv, sizeof(cv) );

	// ( re)initialize the critical vars
	nReturn = cv.bn;
	memset( &cv, '\0', sizeof(cv) );
	cv.bn = nReturn;

	// we have to force the CALL flag to 1 to keep from clobbering the
	//   current batch file if the program calls another batch file
	//   from within an INT 2Eh
	cv.fCall |= 1;

	sprintf( gszCmdline, FMT_FAR_PREC_STR, (int)(*pszCommand), pszCommand + 1 );

	nReturn = command( gszCmdline, 0 );

	// restore the critical vars
	memmove( &cv, &save_cv, sizeof(cv) );

	return nReturn;
}


// find 4START & 4EXIT
void _near _fastcall find_4files( LPTSTR lpszFileName )
{
	register LPTSTR pszSearch;
	int nSaveErrorLevel, nSaveCallFlag;
	TCHAR _far *lpszComspec;

	// first, check .INI data for FSPath (no fallback - the file
	//   HAS to be there or it will be ignored)

	if ( gpIniptr->FSPath != INI_EMPTYSTR ) {
		strcpy( gszCmdline, (LPTSTR)( gpIniptr->StrData + gpIniptr->FSPath ));

		// force trailing backslash on path name
		mkdirname( gszCmdline, lpszFileName );

	} else {

		// look in the COMSPEC directory
		if (( lpszComspec = get_variable( COMSPEC )) == 0L )
			lpszComspec = _pgmptr;

		// remove "4xxx.xxx" & replace w/ 4START or 4EXIT
		sprintf( gszCmdline, FMT_FAR_PREC_STR, MAXFILENAME-1, lpszComspec );
		insert_path( gszCmdline, lpszFileName, gszCmdline );
	}

	// if not found in COMSPEC, look in the boot root directory

	if ((( pszSearch = searchpaths( gszCmdline, NULL, TRUE, NULL )) == NULL ) && ( gpIniptr->FSPath == INI_EMPTYSTR )) {

		// get startup disk for 4START and 4EXIT
		sprintf( gszCmdline, _TEXT("%c:\\%s"), gpIniptr->BootDrive, lpszFileName );
		pszSearch = searchpaths( gszCmdline, NULL, TRUE, NULL );
	}

	if ( pszSearch != NULL ) {

		// check for LFN name
		AddQuotes( pszSearch );

		if (( pszCmdLineOpts != NULL ) && ( *pszCmdLineOpts ))
			sprintf( gszCmdline, _TEXT("%s %s"), pszSearch, pszCmdLineOpts );
		else
			strcpy( gszCmdline, pszSearch );

		nSaveErrorLevel = gnErrorLevel;

		// treat it as a CALL to keep batch_cmd() from overwriting batch
		//   arguments inside a pipe.  Turn off REXX and CALL :SUB flags.
		nSaveCallFlag = cv.fCall;
		cv.fCall = 1;

		// save pipe shared memory so it's not closed yet

		ProcessCommands( gszCmdline, NULLSTR, 0 );

		cv.fCall = nSaveCallFlag;
		gnErrorLevel = nSaveErrorLevel;
	}
}


// batch file processor
int _near BatchCLI( void )
{
	int ch, nReturn = 0;

	// if offset == -1, we processed a CANCEL or QUIT
	for ( cv.fException = 0; ( bframe[cv.bn].lOffset >= 0L ); ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {

		    // check for stack overflow
		    if ( cv.fException & BREAK_STACK_OVERFLOW )
			ch = ALL_CHAR;

		    // check for "ON ERROR / ERRORMSG" request
		    else if ( cv.fException & BREAK_ON_ERROR ) {

			cv.fException = 0;
			strcpy( gszCmdline, (( bframe[cv.bn].pszOnError != NULL ) ? bframe[cv.bn].pszOnError : bframe[cv.bn].pszOnErrorMsg ));
			goto BatchExecCmd;

		    } else {

			// Reset ^C so we come back here if the user types
			//   ^C in response to the (Y/N)? prompt
			cv.fException = 0;

			// Got ^C internally or from an external program.

			// check for "ON BREAK" request
			if ( bframe[ cv.bn ].pszOnBreak != NULL ) {
				strcpy( gszCmdline, bframe[cv.bn].pszOnBreak );
				goto BatchExecCmd;
			}

			EnableSignals();

			if ( QueryIsConsole( STDERR )) {

			    qprintf( STDERR, CANCEL_BATCH_JOB, bframe[cv.bn].pszBatchName );

			    for ( ; ; ) {

				ch = GetKeystroke((( gfCTTY == 0 ) ? EDIT_NO_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT : EDIT_NO_ECHO | EDIT_UC_SHIFT ));
				if ( isprint( ch )) {
					qputc( STDERR, (TCHAR)ch );
					if (( ch == YES_CHAR) || ( ch == NO_CHAR) || ( ch == ALL_CHAR))
						break;
					qputc( STDERR, BS );
				}
				honk();
			    }

			    qprintf( STDERR, gszCRLF );

			} else {

			    // console is redirected - write to display &
			    //   read from keyboard
			    sprintf( gszCmdline, CANCEL_BATCH_JOB, bframe[cv.bn].pszBatchName );
			    WriteTTY( gszCmdline );

			    for ( ; ; ) {

				ch = GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT | EDIT_SCROLL_CONSOLE );

				if ( isprint( ch )) {
					WriteTTY( (LPTSTR)&ch );
					if (( ch == YES_CHAR ) || ( ch == NO_CHAR ) || ( ch == ALL_CHAR ))
						break;
					WriteTTY( _TEXT("\b") );
				}

				honk();
			    }

			    WriteTTY( gszCRLF );

			}
		    }

		    cv.fException = 0;

		    if ( ch != NO_CHAR ) {

			// CANCEL ALL nested batch files
			//   by setting the file pointers to EOF
			if ( ch == ALL_CHAR ) {

				for ( ch = 0; ( ch < cv.bn ); ch++ )
					bframe[ch].lOffset = -1L;

				// force abort in things like GLOBAL
				cv.fException = CTRLC;
			}

			// cancel current batch file
			bframe[ cv.bn ].lOffset = -1L;
			nReturn = CTRLC;

			// clear DO / IFF parsing flags
			cv.f.lFlags = 0L;
			break;
		    }

		    continue;
		}

		// Reset ^C handling
		EnableSignals();

		if ( open_batch_file() == 0 )
			break;

		// get file input

		if ( getline( bframe[cv.bn].bfd, gszCmdline, MAXLINESIZ-1, EDIT_COMMAND ) == 0 )
			break;

		bframe[cv.bn].uBatchLine++;

BatchExecCmd:

		// parse & execute the command.  BATCH_RETURN is a special
		//   code sent by functions like GOSUB/RETURN to end recursion
		if ( command( gszCmdline, 2 ) == BATCH_RETURN )
			break;
	}

	if ( setjmp( cv.env ) == -1 )
		nReturn = CTRLC;

	close_batch_file();

	return nReturn;
}


// open the batch file and seek to the saved offset
int open_batch_file( void )
{
	if (( bframe[ cv.bn ].bfd != IN_MEMORY_FILE ) && ( bframe[ cv.bn ].bfd >= 0 )) {

		// we need to check if something (like Netware!!) has
		//   closed the batch file.

		int nFH = bframe[ cv.bn ].bfd;
_asm {
		mov	ax, 04400h	; get device/file information
		mov	bx, nFH
		int	21h
		jc	handle_err

		and	dx, 080h	; check bit 7 - 1 indicates a device
		jz	not_device
		and	dx, 3		; check bits 0 & 1
		jz	not_device	;  (0 = STDIN, 1 = STDOUT)
handle_err:
		mov	nFH, -1		; error - force a reopen
not_device:
}
		bframe[ cv.bn ].bfd = nFH;
	}

	if ( bframe[ cv.bn ].bfd < 0 ) {

		// Batch file was closed (either by us or Netware)
		while (( bframe[ cv.bn ].bfd = _sopen( bframe[ cv.bn ].pszBatchName, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 ) {

			// if missing file is on fixed disk, assume worst & give up
			if ( bframe[ cv.bn ].pszBatchName[0] > _TEXT('B') ) {
				error( ERROR_4DOS_MISSING_BATCH, bframe[ cv.bn ].pszBatchName );
				return 0;
			}

			qprintf( STDERR, INSERT_DISK, bframe[ cv.bn ].pszBatchName );
			GetKeystroke( EDIT_NO_ECHO | EDIT_ECHO_CRLF | EDIT_SCROLL_CONSOLE );
		}


		// seek to saved position
		_lseek( bframe[cv.bn].bfd, bframe[cv.bn].lOffset, SEEK_SET );
	}

	return 1;
}


// close the current batch file
void close_batch_file( void )
{
	// if it's not an in-memory batch file, & it's still open, close it
	if (( cv.bn >= 0 ) && ( bframe[cv.bn].bfd != IN_MEMORY_FILE ) && ( bframe[cv.bn].bfd >= 0 )) {
		_close( bframe[cv.bn].bfd );
		bframe[cv.bn].bfd = -1;
	}
}



// get command line from console or file
int PASCAL getline( int nFH, LPTSTR pszLine, int nMaxSize, int nEditFlag )
{
	int i;

	// get STDIN input (if not redirected or CTTY'd) from egets()
	if (( nFH == STDIN ) && ( QueryIsConsole( STDIN ))) {

		if (( gpIniptr->LineIn ) || ( gfCTTY )) {

			// if inputmode, then get input using INT 21h 0Ah
			pszLine[0] = (( nMaxSize > 255 ) ? 255 : nMaxSize );
			pszLine[1] = '\0';
_asm {
			mov	ah, 0Ah
			mov	dx, pszLine
			int	21h
}
			sscanf( pszLine+2, SCAN_NOCR, pszLine );
			crlf();
			return ( strlen( pszLine ));

		} else
			return ( egets( pszLine, nMaxSize, nEditFlag ));		// already Unicode
	}

	// if it's an in-memory batch file, parse the buffer
	if ( nFH == IN_MEMORY_FILE ) {

		LPBYTE lpASCII;

		lpASCII = bframe[cv.bn].lpszBTMBuffer + bframe[cv.bn].lOffset;

		// get a line and set the file pointer to the next line
		for ( i = 0; ; i++, pszLine++ ) {

			*pszLine = (TCHAR)*lpASCII++;

			if (( i >= nMaxSize ) || ( *pszLine == EoF ))
				break;

			if ( *pszLine == _TEXT('\r') ) {

				// skip a LF following a CR or LF
				if ( ++i < nMaxSize ) {

					if ( *lpASCII == _TEXT('\n') )
						i++;
				}

				break;

			} else if ( *pszLine == _TEXT('\n') ) {
				i++;
				break;
			}
		}

		// truncate the line
		*pszLine = _TEXT('\0');

		bframe[cv.bn].lOffset += i;

	} else {

		nMaxSize = _read( nFH, pszLine, nMaxSize );

		// get a line and set the file pointer to the next line
		for ( i = 0; ; i++, pszLine++ ) {

			if (( i >= nMaxSize ) || ( *pszLine == EoF ))
				break;

			if ( *pszLine == _TEXT('\r') ) {

				// skip the CR
				i++;

				// check for nitwit MS programmers writing a CR/CR/LF
				if (( pszLine[1] == _TEXT('\r')) && ( pszLine[2] == _TEXT('\n') ))
					i += 2;
				// skip a LF following a CR
				else if ( pszLine[1] == _TEXT('\n') )
					i++;

				break;

			} else if ( *pszLine == _TEXT('\n') ) {
				i++;
				break;
			}
		}

		// truncate the line
		*pszLine = _TEXT('\0');

		if ( i >= 0 ) {

			// save the next line's position
			if (( cv.bn >= 0 ) && ( nFH == bframe[cv.bn].bfd )) {

				bframe[cv.bn].lOffset += i;

			}

			if ( _isatty( nFH ) == 0 )
				_lseek( nFH, (LONG)( i - nMaxSize ), SEEK_CUR );
		}
	}

	return i;
}


static int _fastcall CheckForNoEcho( register LPTSTR pszStartLine )
{
	// check for leading '@' (no echo)
	if (( *pszStartLine == _TEXT('@') ) || ( strnicmp( pszStartLine, _TEXT("*@"), 2 ) == 0 )) {

		if ( *pszStartLine == _TEXT('*') )
			strcpy( pszStartLine+1, skipspace( pszStartLine+2 ));
		else
			strcpy( pszStartLine, skipspace( pszStartLine+1 ));

		return 1;
	}

	return 0;
}


#define AND_CONDITION 1
#define OR_CONDITION 2

// Parse the command line, perform alias & variable expansion & redirection,
//   and execute it.
int _near _fastcall command( register LPTSTR pszLine, int fOptions )
{
	extern LPTSTR pszBatchDebugLine;

	static int fStepOver = 0;
	register LPTSTR pszStartLine;
	int i, nReturn = 0, fEcho = 0, fStep, fSetCMDLINE;
	char cCondition;
	TCHAR cEoL;
	REDIR_IO redirect;

	if (( pszLine == NULL ) || ( *pszLine == _TEXT('\0') ) || ( *pszLine == _TEXT(':') ))
		return 0;

	// clear stdin, stdout, stderr, & pipe flags
	memset( &redirect, '\0', sizeof(REDIR_IO) );

	// echo the line if at the beginning of the line, and either:
	//   1. In a batch file and ECHO is on
	//   2. VERBOSE is on
	if ( pszLine == gszCmdline )
		fEcho = (( cv.bn >= 0 ) ? bframe[cv.bn].uEcho : cv.fVerbose );

	// history logging
	if (( gpIniptr->HistLogOn ) && ( fOptions & 1 ))
		_log_entry( pszLine, 1 );

	// turn off unsafe APPENDs
	if (( _osmajor >= 4 ) && ( pszLine == gszCmdline ))
		safe_append();

	for ( ; ; ) {

		fStep = 0;

		// check for ^C or stack overflow
		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			nReturn = CTRLC;
			break;
		}

		// ( re)enable ^C and ^BREAK
		EnableSignals();

		// reset the default switch character (things like IF kill it!)

		gpIniptr->SwChr = QuerySwitchChar();

		if (( nReturn == ABORT_LINE) || ( nReturn == BATCH_RETURN ) || (( cv.bn >= 0 ) && ( bframe[cv.bn].lOffset < 0L )))
			break;

		// kludge for WordPerfect Office bug ("COMMAND /C= ...")
		strip_leading( pszLine, _TEXT("= \t\r\n,;") );
		if ( *pszLine == _TEXT('\0') )
			break;

		// check for a minimum amount of free stack
		CheckFreeStack( MIN_STACK );

		// get the international format chars
		QueryCountryInfo();
		QueryCodePage();

		// kludge to check for a leading `
		//   (for example, "if "%a" == "%b" `a1 ^ a2`)
		if ( *pszLine == _TEXT('`') ) {
			if (( pszStartLine = scan( ++pszLine, _TEXT("`"), QUOTES + 1 )) == BADQUOTES )
				break;
			// strip trailing `
			if ( *pszStartLine != _TEXT('\0') )
				strcpy( pszStartLine, pszStartLine + 1 );
		}

		pszStartLine = strcpy( gszCmdline, pszLine );

		// perform alias expansion
		if (( nReturn = alias_expand( pszStartLine )) != 0 )
			break;

		// check for leading '@' (no echo)
		if ( CheckForNoEcho( pszStartLine ) == 1 ) {

			// check for "persistent echo" flag
			fEcho &= 2;
			fSetCMDLINE = 0;

		} else
			fSetCMDLINE = 1;

		// skip blank lines or those beginning with REM or a label
		if (( *pszStartLine == _TEXT(':') ) || (( pszLine = first_arg( pszStartLine )) == NULL ))
			break;

		// check for IFF / THEN / ELSEIFF / ENDIFF condition
		//   (if waiting for ENDIFF or ELSEIFF, ignore the command)
		if (( iff_parsing( pszLine, pszStartLine ) != 0 ) || (( nReturn = do_parsing( pszLine )) != 0 )) {

			// check for DO / END condition
			if ( nReturn == BATCH_RETURN )
				break;

			// skip this command - look for next compound command
			//  (ignoring pipes & conditionals)
			i = gpIniptr->CmdSep;

			if (( pszLine = scan( pszStartLine, (LPTSTR)&i, QUOTES_PARENS )) == BADQUOTES )
				break;
			if ( *pszLine )
				pszLine++;
			continue;
		}

		// check for ?"prompt" command single-stepping (Novell DOS 7)
		if (( *pszStartLine == _TEXT('?') ) && ( QueryIsConsole( STDIN ))) {

		    pszLine = skipspace( pszStartLine + 1 );

		    // "? > ..." a kludge for Mike B.!
		    if (( *pszLine != _TEXT('\0') ) && ( *pszLine != _TEXT('>') ) && ( *pszLine != _TEXT('|') ) && ( *pszLine != gpIniptr->CmdSep )) {

				LPTSTR pszPrompt = pszLine;

				if ( *pszLine == _TEXT('"') ) {
					// get prompt text enclosed in quotes
					pszPrompt++;
					pszLine++;
					while (( *pszLine != _TEXT('\0') ) && ( *pszLine != _TEXT('"') ))
						pszLine++;
					if ( *pszLine == _TEXT('"') )
						*pszLine++ = _TEXT('\0');
				}

				if ( QueryInputChar( pszPrompt, YES_NO ) != YES_CHAR )
					break;
				pszLine = skipspace( pszLine );
				strcpy( pszStartLine, pszLine );
			}
		}

		// IFF may have removed command
		if (( pszLine = first_arg( pszStartLine )) == NULL )
			break;

		// skip a REM
		// kludge for dumdums who do "rem > file"
		if (( _stricmp( _TEXT("REM"), pszLine ) == 0 ) && (*(skipspace( pszStartLine+3 )) != _TEXT('>') )) {

			// echo the REM lines if ECHO is ON
			if ( fEcho )
				printf( FMT_STR_CRLF, pszStartLine );
			break;
		}

		// get the current disk (because Netware bombs redirection
		//   if we get it anyplace else)
		if (( gnCurrentDisk = _getdrive()) < 0 )
			gnCurrentDisk = 0;

		// check last argument for an escape char (line continuation)
		if (( nReturn = ContinueLine( pszStartLine )) != 0 )
			break;

		// Do variable expansion (except for certain internal commands)
		//   var_expand() will ignore variables inside command groups
		if ((( i = findcmd( pszStartLine, 0 )) < 0 ) || ( commands[i].fParse & CMD_EXPAND_VARS )) {
		
			if ((( nReturn = var_expand( pszStartLine, 0 )) != 0 ) || (*(pszStartLine = skipspace( pszStartLine )) == _TEXT('\0') ))
				break;

			// check for leading '@' (no echo)
			if ( CheckForNoEcho( pszStartLine ) == 1 ) {
				fEcho &= 2;
				fSetCMDLINE = 0;
			}
		}

		pszLine = (( *(skipspace( pszStartLine )) == _TEXT('(') ) ? QUOTES_PARENS : QUOTES );

		// Trailing command group in internal command?  ("IF a==b (")
		if (( i >= 0 ) && ( commands[i].fParse & CMD_GROUPS )) {

			// check last argument for a " ("
			do {
				pszLine = strlast( pszStartLine );
				while ((( *pszLine == _TEXT(' ') ) || ( *pszLine == _TEXT('\t') )) && ( pszLine > pszStartLine ))
					pszLine--;

				if (( *pszLine != _TEXT('(') ) || ( isdelim( pszLine[ -1 ] ) == 0 ))
					break;
			} while (( nReturn = CommandGroups( pszLine, 0 )) == 0 );

			// for commands like IF, IFF, FOR, etc. watch for ( )'s
			pszLine = QUOTES_PARENS;
		}

		// check for batch single-stepping
		if (( fRuntime == 0 ) && ( fOptions & 2 ) && ( cv.bn >= 0 ) && (( bframe[cv.bn].nFlags & BATCH_COMPRESSED ) == 0 ) && ( gpIniptr->SingleStep ) && ( fStepOver == 0 ) && ( QueryIsConsole( STDIN ))) {

			// default is Trace into

			extern int _near BatchDebugger( void );

			pszBatchDebugLine = pszStartLine;
			nReturn = BatchDebugger( );

			if (( nReturn == ERROR_EXIT ) || ( nReturn == _TEXT('Q') )) {
				for ( nReturn = cv.bn; ; nReturn-- ) {
					bframe[ nReturn ].lOffset = -1L;
					if ( nReturn == 0 )
						break;
				}
				break;
			} else if ( nReturn == _TEXT('J') ) {
				// 'Jump'?
// Future enhancement - jump to any line in batch file
				nReturn = 0;
				break;
			} else if ( nReturn == _TEXT('S') ) {
				// 'S'(tep over)
				fStep = fStepOver = 1;
			} else if (( nReturn == _TEXT('O') ) || ( nReturn == ESCAPE ))
				gpIniptr->SingleStep = 0;

			// default is 'T'(race into)

		}

		// get compound command char, pipe, conditional, or EOL
		if (( pszLine = scan( pszStartLine, NULL, pszLine )) == BADQUOTES )
			break;

		nReturn = 0;
		cEoL = cCondition = 0;

		// terminate command & save special char (if any)
		if ( *pszLine ) {
			cEoL = *pszLine;
			*pszLine++ = _TEXT('\0');
		}

		// command ECHOing
		if (( fEcho ) && ( *pszStartLine ))
			printf( FMT_STR_CRLF, pszStartLine );

		// command logging
		if ( gpIniptr->LogOn )
			_log_entry( pszStartLine, 0 );

		// process compound command char, pipe, or conditional
		if ( cEoL == _TEXT('|') ) {

			if ( *pszLine == _TEXT('|') ) {

				// conditional OR (a || b)
				cCondition |= OR_CONDITION;
				pszLine++;

			} else {

				// must be a pipe - try to open it
				if (( nReturn = open_pipe( &redirect )) != 0 )
					break;

				if ( *pszLine == _TEXT('&') ) {
					// |& means pipe STDERR too
					redirect.naStd[ STDERR ] = _dup( STDERR );
					_dup2( STDOUT, STDERR );
					pszLine++;
				}
			}

		} else if (( cEoL == _TEXT('&') ) && ( *pszLine == _TEXT('&') )) {

			// conditional AND (a && b)
			cCondition |= AND_CONDITION;
			pszLine++;
		}

		// do I/O redirection except for some internal commands
		//   ( redir() will check for command grouping)
		if ((( gpIniptr->Expansion & EXPAND_NO_REDIR ) == 0 ) && (( i < 0 ) || ( commands[i].fParse & CMD_EXPAND_REDIR ))) {

			if ( redir( pszStartLine, &redirect )) {
				nReturn = ERROR_EXIT;
				break;
			}
		}

		// strip leading delimiters & trailing whitespace
		strip_leading( pszStartLine, _TEXT(", \t\n\f\r") );
		strip_trailing( pszStartLine, _TEXT(" \t\r\n") );

		nReturn = ProcessCommands( pszStartLine, pszLine, fSetCMDLINE );

		// change icon label back to the default

		// check for ^C or stack overflow
		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			nReturn = CTRLC;
			break;
		}

		if (( fRuntime == 0 ) && ( fStep )) {
			// reenable batch debugger after stepping over
			//   a CALL, GOSUB, etc.
			fStep = fStepOver = 0;
		}

		// clean everything up
		pszLine = gszCmdline;

		// check for funny return value from IFF  (this kludge allows
		//   you to pipe to a command following the IFF; otherwise
		//   IFF would eat the pipe itself)
		if ( nReturn == 0x666 ) {
			nReturn = 0;
			continue;
		}

		// if conditional command failed, skip next command(s)
		while ((( nReturn != 0 ) && ( cCondition & AND_CONDITION )) || (( nReturn == 0 ) && ( cCondition & OR_CONDITION ))) {

			// check for command grouping
			pszLine = skipspace( pszLine );
			if (( pszLine = scan( pszLine, NULL, ((*pszLine == _TEXT('(') ) ? QUOTES_PARENS : QUOTES) )) == BADQUOTES)
				break;

			if (( *pszLine == _TEXT('&') ) && ( pszLine[1] == _TEXT('&') )) {
				cCondition = AND_CONDITION;
				pszLine += 2;
			} else if (( *pszLine == _TEXT('|') ) && ( pszLine[1] == _TEXT('|') )) {
				cCondition = OR_CONDITION;
				pszLine += 2;
			} else
				break;
		}

		// clean up redirection & check pipes from last command
		unredir( &redirect, &nReturn );
	}

	// clean up I/O redirection
	redirect.fPipeOpen = 0;

	unredir( &redirect, &nReturn );

	// delete any leftover temporary pipes (DOS only)
	killpipes( &redirect );

	if ( fOptions & 1 )
		crlf();

	if ( fStep )
		fStepOver = 0;

	return nReturn;
}


int _near ProcessCommands( register LPTSTR pszStartLine, LPTSTR pszNextLine, int fSetCMDLINE )
{
	register LPTSTR pszCmdName;
	int i, nReturn = 0;

	if ( *pszStartLine != _TEXT('\0') ) {

	    // save the line continuation (if any) onto the stack
	    pszNextLine = strcpy( (LPTSTR)_alloca( ( strlen( pszNextLine ) + 1 ) * sizeof(TCHAR) ), pszNextLine );

	    if ( *pszStartLine == _TEXT('(') ) { 

		pszStartLine = strcpy( gszCmdline, pszStartLine );

		// Process command groups
		if (( nReturn = CommandGroups( pszStartLine, 1 )) == 0 ) {

		    nReturn = command( pszStartLine, 0 );

		}

	    } else if (( pszStartLine[1] == _TEXT(':') ) && ( isdelim( pszStartLine[2] ))) {

		// a disk change request
		nReturn = __cd( pszStartLine, CD_CHANGEDRIVE | CD_SAVELASTDIR | CD_NOFUZZY );

	    // search for installed command (INT 2F function AE00h)
	    // then try internal commands
	    } else if ((( nReturn = installable_cmd( pszStartLine )) == -1 ) && ( InternalCommands( pszStartLine, fSetCMDLINE, &nReturn ) == -1 )) {

		// if it's a directory name, do a fast CDD

			// Have to break on first '/' (annoying UNIX users) because
			//   too many people do things like "program/a"
			if (( gpIniptr->UnixPaths ) || ( strnicmp( pszStartLine, _TEXT("../"), 3 ) == 0 ))
				pszCmdName = ntharg( pszStartLine, 0x800 );
			else
				pszCmdName = first_arg( pszStartLine );

			StripQuotes( pszCmdName );
			i = ( strlen( pszCmdName ) - 1 );


		// check for implicit directory change (trailing '\')
		// call "ParseLine" because we need to process any
		//   escaped characters
		if (( i >= 0 ) && (( pszCmdName[i] == _TEXT('\\') ) || ( pszCmdName[i] == _TEXT('/') )))
		    nReturn = ParseLine( _TEXT("cdd"), pszStartLine, Cdd_Cmd, CMD_STRIP_QUOTES | CMD_BACKSLASH_OK, fSetCMDLINE );
		else {
		    // search for external command
		    nReturn = ExternalCommands( pszStartLine, pszCmdName, fSetCMDLINE );
		}
	    }
	}

	// restore remainder of line
	strcpy( gszCmdline, pszNextLine );

	return nReturn;
}


// search for an internal command, & execute it if found
int _near InternalCommands( LPTSTR pszStartLine, int fSetCMDLINE, int *pnReturn )
{
	register int i;
	register LPTSTR pszOpts;

	// search for an internal command
	if (( i = findcmd( pszStartLine, 0 )) < 0 )
		return -1;

	*pnReturn = 0;

	// save the command name (for things like USAGE)
	gpInternalName = commands[i].szCommand;

	// check for "command /?" & display help if found
	//   (with a kludge to except "ECHO /? blah blah blah")
	if (( strnicmp( gpInternalName, ECHO_IS, 4 ) != 0 ) || ( strlen( pszStartLine + strlen( gpInternalName )) <= 3 )) {
		
		if ( QueryIsSwitch( pszStartLine + strlen( gpInternalName ), _TEXT('?'), ( commands[i].fParse & CMD_HELP ))) {

			*pnReturn = ( _help( gpInternalName, skipspace( pszStartLine + strlen( gpInternalName )) ));

			return 0;
		}
	}

	// check if command only allowed in a batch file
	if (( commands[i].fParse & CMD_ONLY_BATCH ) && ( cv.bn == -1 )) {
		*pnReturn = error( ERROR_4DOS_ONLY_BATCH, gpInternalName );
		return 0;
	}

	pszOpts = pszStartLine + strlen( gpInternalName );

	// kludge for "ECHO."
	if ( strnicmp( pszStartLine, _TEXT("echo"), 4 ) != 0 )
		pszOpts = skipspace( pszOpts );

	*pnReturn = ParseLine( pszStartLine, pszOpts, commands[i].pFunc, commands[i].fParse, fSetCMDLINE );

	// ON ERROR usually returns a CTRLC, which isn't really accurate
	if (( *pnReturn == CTRLC ) && ( cv.fException & BREAK_ON_ERROR ))
		*pnReturn = gnInternalErrorLevel;
	else if ( *pnReturn == BATCH_RETURN_RETURN )
		*pnReturn = BATCH_RETURN;
	else if (( *pnReturn == BATCH_RETURN ) || (( gnInternalErrorLevel = *pnReturn ) == ABORT_LINE ) || ( gnInternalErrorLevel == 0x666 )) {
		// clear internal error level flag
		gnInternalErrorLevel = 0;
	}

	// force disk buffer flush on certain commands
	//   (for nitwits like FASTOPEN & dumb caches)
	if (( commands[i].fParse & CMD_RESET_DISKS ) && ( gpIniptr->DiskReset ))
		reset_disks();

	return 0;
}


// search for an external command or batch file, & execute it if found
int _near ExternalCommands( LPTSTR pszStartLine, register LPTSTR pszCmdName, int fSetCMDLINE )
{
	register LPTSTR pszArgs, pszSave;
	PCH pszEE;

	int nReturn = 0;
	int fEExt = 0;

	pszSave = pszStartLine;

	// search for external command, & set batch name pointer
	*(scan( pszCmdName + 1, _TEXT("`="), QUOTES + 1 )) = _TEXT('\0');

	pszArgs = (LPTSTR)_alloca( ( strlen( pszCmdName ) + 1 ) * sizeof(TCHAR) );
	gpBatchName = pszCmdName = strcpy( pszArgs, pszCmdName );

	// if not a COM, EXE, BTM, or BAT (or CMD), or
	//   a user-defined "executable extension", return w/error
	if (( strpbrk( pszCmdName, _TEXT("*?") ) == NULL ) && (( pszArgs = searchpaths( pszCmdName, NULL, TRUE, &fEExt )) != NULL )) {

		// DOS only allows a 127 character command line, so save the
		//   full command line to var CMDLINE.  If command line preceded
		//   by a '@', remove CMDLINE from the environment
		if ( fSetCMDLINE ) {
		    strins( pszStartLine, CMDLINE_VAR );
		    add_variable( pszStartLine );
		    strcpy( pszStartLine, pszStartLine + strlen( CMDLINE_VAR ));
		} else
		    add_variable( CMDLINE_VAR );

		// adjust for quotes around command name
		if ( *pszStartLine == '"' )
			pszStartLine += 2;

		// preserve leading whitespace for DOS externals
		pszStartLine += strlen( pszCmdName );

		pszCmdName = mkfname( pszArgs, 0 );
		if (( pszArgs = ext_part( pszCmdName )) == NULL )
			pszArgs = NULLSTR;

		// check for executable extension (might be .COM, .EXE, .BAT, .CMD, .BTM disabled via PathExt)
		if ( fEExt )
			goto ExecutableExtension;

		if (( _stricmp( pszArgs, COM ) == 0 ) || ( _stricmp( pszArgs, EXE ) == 0 ))
		{

		    gnErrorLevel = nReturn = ParseLine( pszCmdName, pszStartLine, NULL, CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_UCASE_CMD | CMD_EXTERNAL, fSetCMDLINE );

		}
		else if ( _stricmp( pszArgs, BAT ) == 0 ) {
		    // check .BAT files for REXX (PC-DOS 7)
		    nReturn = process_rexx( pszCmdName, pszStartLine, FALSE );
		} else if ( _stricmp( pszArgs, BTM ) == 0 )
		    nReturn = ParseLine( pszCmdName, pszStartLine, NULL, CMD_STRIP_QUOTES | CMD_ADD_NULLS | CMD_CLOSE_BATCH | CMD_UCASE_CMD, fSetCMDLINE );

		else {

ExecutableExtension:
		    // is it an executable extension?
		    if ( *(pszEE = executable_ext( pszArgs )) != _TEXT('\0') ) {

				static int nEELoop = 0;

				pszStartLine = skipspace( pszStartLine );

				AddQuotes( pszCmdName );

				// check for length & save remainder of command line
				if (( _fstrlen( pszEE ) + strlen( pszCmdName ) + 1 + strlen( pszStartLine )) >= MAXLINESIZ )
					return ( error( ERROR_4DOS_COMMAND_TOO_LONG, NULL ));

				// save arguments
				pszArgs = (LPTSTR)_alloca( ( strlen( pszStartLine ) + 1 ) * sizeof(TCHAR) );
				strcpy( pszArgs, skipspace( pszStartLine ));

				sprintf( gszCmdline, _TEXT("%Fs %s %s"), pszEE, pszCmdName, pszArgs );

				// go back so we can support aliases, internal and
				//   external commands
				if ( ++nEELoop > 10 ) {
					nEELoop = 0;
					return ( error( ERROR_4DOS_EE_LOOP, gszCmdline ));
				}

				nReturn = command( gszCmdline, 0 );
				nEELoop = 0;

		    } else {

			goto cmd_err;

		    }
		}

	} else {

cmd_err:
		// check for UNKNOWN_CMD alias
		if ( get_alias( UNKNOWN_COMMAND ) != 0L ) {

			static int nUCLoop = 0;

			if ( ++nUCLoop > 10 )
				nReturn = error( ERROR_4DOS_UNKNOWN_CMD_LOOP, pszSave );

			else {

				if (( strlen( pszSave ) + 14 ) > MAXLINESIZ )
					return ( error( ERROR_4DOS_COMMAND_TOO_LONG, pszSave ));
				strcpy( gszCmdline, pszSave );
				strins( gszCmdline, _TEXT(" ") );
				strins( gszCmdline, UNKNOWN_COMMAND );

				// go back so we can support aliases, internal
				//   and external commands
				nReturn = command( gszCmdline, 0 );
			}

			nUCLoop = 0;

		} else {
			// unknown command
			gnErrorLevel = nReturn = error( ERROR_4DOS_UNKNOWN_COMMAND, pszCmdName );
		}
	}

	return nReturn;
}


// process command grouping & get additional lines if necessary
static int _near _fastcall CommandGroups( LPTSTR pszStartLine, int fRemoveParens )
{
	register int nReturn;
	register LPTSTR pszGroup;
	LPTSTR pszCmd;

	// first get the command group; then remove leading & trailing parens
	//   (except when retrieving continuation lines)
	for ( ; ; ) {

		if (( pszGroup = scan( pszStartLine, _TEXT(")"), QUOTES_PARENS )) == BADQUOTES)
			return ERROR_EXIT;

		if ( *pszGroup == _TEXT('\0') ) {

			// go back & get some more input
			//   ("line" has to be empty to get here!)

			// don't remove ( ) 's if a line continuation is found
			fRemoveParens = 0;

			strcat( pszStartLine, _TEXT(" ") );
			pszGroup = strend( pszStartLine );

			if (( nReturn = ( CMDBUFSIZ - (UINT)(( pszGroup + 0x10 ) - gszCmdline ))) <= 0 ) {
				error( ERROR_4DOS_COMMAND_TOO_LONG, NULL );
				return BATCH_RETURN;
			}

			if ( nReturn > ( MAXLINESIZ - 1 ))
				nReturn = MAXLINESIZ - 1;

			// if not a batch file, prompt for more input
			if (( fRuntime == 0 ) && ( cv.bn < 0 )) {

				printf( COMMAND_GROUP_MORE );
				nReturn = getline( STDIN, pszGroup, nReturn, EDIT_COMMAND | EDIT_SCROLL_CONSOLE );

				// add it to history list
				addhist( pszGroup );

				// history logging
				if ( gpIniptr->HistLogOn )
					_log_entry( pszGroup, 1 );

			} else if ( cv.bn >= 0 ) {

				// kludge for Netware bug
				if ( open_batch_file() == 0 )
					return BATCH_RETURN;

				nReturn = getline( bframe[ cv.bn ].bfd, pszGroup, nReturn, EDIT_COMMAND );
				bframe[cv.bn].uBatchLine++;
			}

			if ( nReturn == 0 ) {
				error( ERROR_4DOS_UNBALANCED_PARENS, pszStartLine );
				return BATCH_RETURN;
			}

			// ignore blank and comment lines
			if (( pszCmd = first_arg( pszGroup )) != NULL ) {
				for ( ; (( *pszCmd ) && ( *pszCmd == _TEXT('@') ) || ( *pszCmd == _TEXT('*') )); pszCmd++ )
					;
			}

			if (( pszCmd == NULL ) || ( _stricmp( pszCmd, _TEXT("REM") ) == 0 )) {
				pszGroup[-1] = _TEXT('\0');
				continue;
			}

			// skip leading whitespace
			strcpy( pszGroup, skipspace( pszGroup ) );
			if (( pszGroup[-2] != _TEXT('(') ) && ( *pszGroup != _TEXT(')') )) {
				strins( pszGroup, _TEXT("  ") );
				*pszGroup = gpIniptr->CmdSep;
			}

		} else {

			// remove the first ( ) set
			if ( fRemoveParens ) {
				strcpy( pszGroup, pszGroup + 1 );
				strcpy( pszStartLine, pszStartLine + 1 );
			}

			return 0;
		}
	}
}


// get additional line(s) when previous line ends in an escape
int _near _fastcall ContinueLine( register LPTSTR pszStartLine )
{
	register int nLength;

	if (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 ) {

	    for ( ; ; ) {

			if (( nLength = strlen( pszStartLine )) == 0 )
				break;
			pszStartLine += ( nLength - 1 );

			// check for single trailing escape character
			//  (plus a kludge for trailing %=)
			if ((( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 ) && ( *pszStartLine == _TEXT('=') ) && ( nLength > 1 ) && ( pszStartLine[-1] == _TEXT('%') ))
				pszStartLine--;
			else if (( *pszStartLine != gpIniptr->EscChr ) || (( nLength > 1 ) && ( pszStartLine[1] == gpIniptr->EscChr )))
				break;

			// get some more input
			*pszStartLine = _TEXT(' ');

			if (( nLength = ( CMDBUFSIZ - (UINT)((pszStartLine + 2) - gszCmdline ))) <= 0 ) {
				error( ERROR_4DOS_COMMAND_TOO_LONG, NULL );
				return BATCH_RETURN;
			}

			if ( nLength > ( MAXLINESIZ - 1 ))
				nLength = MAXLINESIZ - 1;

			// if not a batch file, prompt for more input
			if (( fRuntime == 0 ) && ( cv.bn < 0 )) {

				printf( COMMAND_GROUP_MORE );
				getline( STDIN, pszStartLine, nLength, EDIT_COMMAND | EDIT_SCROLL_CONSOLE );

				// add it to history list
				addhist( pszStartLine );

			} else if ( cv.bn >= 0 ) {
				getline( bframe[cv.bn].bfd, pszStartLine, nLength, EDIT_COMMAND );
				bframe[cv.bn].uBatchLine++;
			}
		}
	}

	return 0;
}


// parse the command line into Argc, Argv format
int _near ParseLine( LPTSTR pszCmdName, register LPTSTR pszLine, int (_near *pfnCmd)(LPTSTR), unsigned int fParse, int fSetCMDLINE )
{
	static int Argc;		// argument count
	static TCHAR *Argv[ARGMAX];	// argument pointers
	static LPTSTR pszTemp;
	static TCHAR cQuote;
	LPTSTR pszArgs = pszLine;

	// fParse bit arguments are:
	//   CMD_STRIP_QUOTES - strip quoting
	//   CMD_ADD_NULLS - add terminators to each argument (batch files)
	//   CMD_CLOSE_BATCH - close open batch file before executing command
	//   CMD_EXTERNAL - external command

	// if it's an external or special case internal, close the batch file
	if ( fParse & CMD_CLOSE_BATCH )
		close_batch_file();

	// set command / program name
	if ( fParse & CMD_UCASE_CMD )
		strupr( pszCmdName );
	Argv[0] = pszCmdName;

	pszLine = skipspace( pszLine );

	// loop through arguments
	for ( Argc = 1; (( Argc < ARGMAX ) && ( *pszLine )); Argc++ )  {

		pszTemp = pszLine;		// save start of argument

		while ( isdelim( *pszLine ) == 0 ) {

		    if ((( gpIniptr->Expansion & EXPAND_NO_QUOTES) == 0 ) && ((*pszLine == SINGLE_QUOTE) || (*pszLine == DOUBLE_QUOTE))) {

			cQuote = *pszLine;

			// strip a single quote
			if (( *pszLine == SINGLE_QUOTE ) && ( fParse & CMD_STRIP_QUOTES ))
				strcpy( pszLine, pszLine + 1 );
			else
				pszLine++;

			// arg ends after matching close quote
			while (( *pszLine ) && ( *pszLine != cQuote )) {

				// collapse any "escaped" characters
				if ( fParse & CMD_STRIP_QUOTES ) {
					escape( pszLine );
				}

				if ( *pszLine )
					pszLine++;
			}

			// strip a single quote
			if (( *pszLine == SINGLE_QUOTE ) && ( fParse & CMD_STRIP_QUOTES ))
				strcpy( pszLine, pszLine + 1 );
			else if ( *pszLine )
				pszLine++;

		    } else {

			// collapse any "escaped" characters
			if ( fParse & CMD_STRIP_QUOTES )
				escape( pszLine );
			if ( *pszLine )
				pszLine++;
		    }
		}

		// check flag for terminators
		if ((( fParse & CMD_ADD_NULLS ) != 0 ) && ( *pszLine != _TEXT('\0') ))
			*pszLine++ = _TEXT('\0');

		// update the Argv pointer
		Argv[ Argc ] = pszTemp;

		// skip delimiters for Argv[2] onwards
		while (( *pszLine ) && ( isdelim( *pszLine )))
			pszLine++;
	}

	Argv[Argc] = NULL;

	// test if only processing line, not calling func
	if ( fParse == CMD_STRIP_QUOTES )
		return 0;

	// update title bar (unless inside a batch file)

	// call the requested function (indirectly)
	if ( fParse & CMD_ADD_NULLS )
		return ( Batch( Argc, Argv ));		// Batch
	if ( fParse & CMD_EXTERNAL )
		return ( External( pszCmdName, pszArgs ));	// External

	return (*pfnCmd)( (( Argc > 1 ) ? pszArgs : NULL ));	// Internal
}


// search paths for file -- return pathname if found, NULL otherwise
LPTSTR PASCAL searchpaths( LPTSTR pszFile, LPTSTR pszSearchPath, int fSearchCurrentDir, int *pfEExt )
{
	static TCHAR szFileName[MAXFILENAME + 32];	// expanded filename
	register LPTSTR pszOldExe, pszNewExe;
	int i, n, nPass = 0;
	PCH pszArg, pszPATH = 0L, lpPathExt = 0L, lpExt;

	FILESEARCH dir;
	int fLFN;

	// build a legal filename ( right filename & extension size)
	insert_path( szFileName, fname_part( pszFile ), pszFile );

	// check for existing extension
	pszOldExe = ext_part( szFileName );

	if (( pszSearchPath == NULL ) && ( gpIniptr->PathExt ))
		lpPathExt = get_variable( PATHEXT );

	fLFN = ifs_type( szFileName );

	// check for relative path spec
	if ( strstr( szFileName, _TEXT("...") ) != NULL ) {

		if ( mkfname( szFileName, 0 ) == NULL )
			return NULL;

		// no path searches if path already specified!
	} else if ( path_part( pszFile ) != NULL )
		nPass = -1;

	else {

		if ( pszSearchPath != NULL )
			pszPATH = pszSearchPath;

		else if (( pszPATH = get_variable( PATH_VAR )) != 0L ) {

			// if path has a ";.", DON'T search the current directory first!
			for ( pszArg = pszPATH; (( pszArg = _fstrchr( pszArg, _TEXT(';') )) != 0L ) ; pszArg++ ) {
				if (( pszArg[1] == _TEXT('.') ) && (( pszArg[2] == _TEXT(';') ) || ( pszArg[2] == _TEXT('\0') )))
					goto next_path;
			}
		}

		// don't look in the current directory?
		if ( fSearchCurrentDir == FALSE )
			goto next_path;
	}

	// search the PATH for the file
	for ( ; ; ) {

		if ( pszOldExe != NULL ) {
			if ( is_file( szFileName ))
				return szFileName;
		}

		// if LFN/NTFS, support odd names like "file1.comp.exe"
		if (( pszOldExe == NULL ) || ( fLFN )) {

			// if no extension, try an executable one
			pszNewExe = strend( szFileName );

			// first check with "filename.*" for any possible match
			strcpy( pszNewExe, WILD_EXT );
			i = 0;

			// kludge for 1DIR+ (it's looking for "________.COM")
			if (( *szFileName == '_' ) || ( is_file( szFileName ))) {

				lpExt = lpPathExt;
				for ( i = 0; ( nPass <= 0 ); i++ ) {

					if ( lpExt ) {

						if ( *lpExt == _TEXT('\0') )
							break;

						// get next PATHEXT argument
						sscanf_far( lpExt, _TEXT("%*[;,=]%31[^;,=]%n"), pszNewExe, &n );
						lpExt += n;

					} else {
						if ( executables[i] == NULL )
							break;
						strcpy( pszNewExe, executables[i] );
					}

					// look for the file
					if ( is_file( szFileName ))
						return szFileName;
				}

				// check for user defined executable extensions
				pszArg = glpEnvironment;

				for ( ; (( pszArg != 0L ) && ( *pszArg != _TEXT('\0') )); pszArg = next_env( pszArg )) {

					for ( ; ( *pszArg == _TEXT('.') ); pszArg++ ) {

						// add extension & look for filename
						sscanf_far( pszArg, _TEXT("%13[^;=]%n"), pszNewExe, &n );

						// search directory for possible wildcard match
						if ( find_file( FIND_FIRST, szFileName, 0x07 | FIND_CLOSE | FIND_NO_ERRORS | FIND_NO_FILELISTS, &dir, szFileName ) != NULL ) {
							if ( pfEExt )
								*pfEExt = 1;
							return szFileName;
						}

						// look for another extension
						pszArg += n;
						if ( *pszArg != _TEXT(';') )
							break;
					}
				}
			}
		}

next_path:
		if ( pszPATH == 0L )
			return NULL;

			// get next PATH argument, skipping delimiters
			sscanf_far( pszPATH, _TEXT("%*[;,=]%260[^;,=]%n"), szFileName, &n );

			// check for end of path
			if ( szFileName[0] == _TEXT('\0') ) {

				if ( pszSearchPath == NULL ) {

				}

				return NULL;
			}

			pszPATH += n;

		// make new directory search name
		mkdirname( szFileName, fname_part( pszFile ));

		// make sure specified drive is ready
		if (( is_net_drive( szFileName ) == 0 ) && ( QueryDriveReady( gcdisk( szFileName )) == 0 ))
			goto next_path;
	}
}


// display the DOS / OS2 command line prompt
void ShowPrompt( void )
{
	TCHAR _far *lpszPrompt;
	TCHAR szTitle[MAXLINESIZ];

	// odd kludge for UNC names
	if (( gnCurrentDisk = _getdrive()) < 0 )
		gnCurrentDisk = 0;

	// Decrement command counter for restricted usage (actually increment,
	// since it is negative)
	if ( glExpDate < 0L )
		glExpDate += SW_RESTRICT_CMDMUL;

	// get prompt from environment, or use default if none
	if (( lpszPrompt = get_variable( PROMPT_NAME )) != 0L ) {
		// expand any environment variables in the PROMPT string
		_fstrcpy( gszCmdline, lpszPrompt );
		var_expand( gszCmdline, 1 );
		lpszPrompt = gszCmdline;

	} else {

		lpszPrompt = (( gnCurrentDisk > 2 ) ? "$p$g" : "$n$g" );

	}

	ParsePrompt( lpszPrompt, szTitle );
	qputs( szTitle );

	// get prompt from environment, or use default if none
	if (( lpszPrompt = get_variable( TITLE_PROMPT )) != 0L ) {
		// expand any environment variables in the PROMPT string
		_fstrcpy( gszCmdline, lpszPrompt );
		var_expand( gszCmdline, 1 );
		lpszPrompt = gszCmdline;

		ParsePrompt( lpszPrompt, szTitle );

		szTitle[ 79 ] = '\0';
		Win95SetTitle( szTitle );
	}
}


// parse the prompt string and store the output
static void _fastcall ParsePrompt( TCHAR _far *pszPrompt, LPTSTR pszBuffer )
{
	extern TCHAR gaPushdQueue[];

	TCHAR * pszStart = pszBuffer;
	LPTSTR pszTemp;
	DATETIME sysDateTime;
	char _far *lpTemp;
	char szDrive[8];

	*pszBuffer = _TEXT('\0');
	for ( ; ( *pszPrompt != _TEXT('\0') ); pszPrompt++ ) {

		if (( *pszPrompt == _TEXT('$') ) && ( pszPrompt[1] )) {

			// special character follows a '$'
			switch ( _ctoupper( *(++pszPrompt ) )) {

			case _TEXT('B'):	// vertical bar
				*pszBuffer++ = _TEXT('|');
				break;

			case _TEXT('C'):	// open paren
				*pszBuffer++ = _TEXT('(');
				break;

			case 'D':	// current date
				if ( *pszPrompt == _TEXT('d') ) {
					// "Sat 01-01-00"
					pszBuffer += sprintf( pszBuffer, _TEXT("%.4s%s"), gdate(0 ), gdate(1) );
				} else {
					// "Sat  Jan 1, 2000"
					pszBuffer += sprintf( pszBuffer, FMT_STR, gdate( 0 ));
				}
				break;

			case _TEXT('E'):	// ESCAPE (for ANSI sequences)
				*pszBuffer++ = ESC;
				break;

			case _TEXT('F'):	// close paren
				*pszBuffer++ = _TEXT(')');
				break;

			case _TEXT('G'):
				*pszBuffer++ = _TEXT('>');
				break;

			case _TEXT('H'):	// destructive backspace
				if ( pszBuffer > pszStart )
					pszBuffer--;
				break;

			case _TEXT('I'):	// display Program Selector help

				// in OS/2 DOS box?
				if ( _osmajor >= 10 ) {

					int nRow, nColumn;

					// save & restore cursor position
					GetCurPos( &nRow, &nColumn );
					WriteStrAtt( 0, 0, ((( GetVideoMode() % 2) == 0 ) ? 0x70 : 0x1F), DOS_HDR );
					SetCurPos( nRow, nColumn );
				}

				// ignore $i in TCMD & 4NT
				break;

			case _TEXT('J'):	// current date, 4-year ISO format (2002-07-04)
				QueryDateTime( &sysDateTime );
				pszBuffer += sprintf( pszBuffer, FMT_STR, FormatDate( sysDateTime.month, sysDateTime.day, sysDateTime.year, 4 ));
				break;

			case _TEXT('L'):
				*pszBuffer++ = _TEXT('<');
				break;

			case _TEXT('M'):	// current time w/o seconds
				pszTemp = gtime( *pszPrompt == _TEXT('m') );

				// remove seconds (but preserve "a" or "p")
				strcpy( pszTemp + 5, pszTemp + 8 );
				pszBuffer += sprintf( pszBuffer, FMT_STR, pszTemp );
				break;

			case _TEXT('N'):	// default drive letter
				if ( gnCurrentDisk != 0 )
					*pszBuffer++ = (TCHAR)( gnCurrentDisk + 64 );
				break;

			case _TEXT('P'):	// current directory
				if (( pszTemp = gcdir( NULL, 1 )) != NULL ) {

DisplayDirectory:
				    if ( isupper( *pszPrompt )) {

						// LFNs preserve case in names,
						//   so just leave it alone
						if ( ifs_type( pszTemp ) != 0 ) {
							pszBuffer += sprintf( pszBuffer, FMT_STR, pszTemp );
							break;
						}
				    }

				    pszBuffer += sprintf( pszBuffer, FMT_STR, (( isupper( *pszPrompt )) ? strupr( pszTemp ) : strlwr( pszTemp )));
				}
				break;

			case _TEXT('Q'):	// Equals sign
				*pszBuffer++ = _TEXT('=');
				break;

			case _TEXT('R'):	// errorlevel
				pszBuffer += sprintf( pszBuffer, FMT_INT, gnErrorLevel );
				break;

			case _TEXT('S'):	// print space
				*pszBuffer++ = _TEXT(' ');
				break;

			case _TEXT('T'):	// current time
				pszBuffer += sprintf( pszBuffer, FMT_STR, gtime( *pszPrompt == _TEXT('t') ) );
				break;

			case _TEXT('U'):	// current user

				if (( lpTemp = get_variable( "LOGINNAME" )) != NULL )
					pszBuffer += sprintf( pszBuffer, FMT_FAR_STR, lpTemp );

				break;

			case _TEXT('V'):	// OS version number
				pszBuffer += sprintf( pszBuffer, FMT_STR, gszOsVersion );
				break;

			case _TEXT('W'):	// working directory
				if (( pszTemp = gcdir( NULL, 1 )) != NULL ) {

					// for now, we don't support this with UNC names
					if ( pszTemp[1] == _TEXT(':') ) {

						LPTSTR pszName;

						pszTemp = strcpy( (LPTSTR)_alloca( ( strlen( pszTemp ) + 4 ) * sizeof(TCHAR) ), pszTemp );

						// strip everything but last path
						if ((( pszName = fname_part( pszTemp )) != NULL ) && ( *pszName )) {
							if ( stricmp( pszTemp + 3, pszName ) != 0 )
								sprintf( pszTemp+3, _TEXT("...\\%s"), pszName );
						}
					}

					goto DisplayDirectory;
				}
				break;

			case _TEXT('X'):	// default dir for specified drive

				sprintf( szDrive, FMT_FAR_PREC_STR, 7, pszPrompt );
				if (( pszTemp = gcdir( (( szDrive[1] ) ? szDrive + 1 : NULL ), 1 )) != NULL )
					pszBuffer += sprintf( pszBuffer, FMT_STR, (( *pszPrompt == _TEXT('x') ) ? pszTemp : strupr( pszTemp )));

				if ( pszPrompt[1] ) {
					pszPrompt++;
					if ( pszPrompt[1] == _TEXT(':') )
						pszPrompt++;
				}
				break;

			case _TEXT('Z'):	// shell nesting level
				pszBuffer += sprintf( pszBuffer, FMT_UINT, gpIniptr->ShellNum );
				break;

			case _TEXT('_'):	// CR/LF
				*pszBuffer++ = _TEXT('\r');
				*pszBuffer++ = _TEXT('\n');
				break;

			case _TEXT('$'):	// just print a $
				*pszBuffer++ = *pszPrompt;
				break;

			case _TEXT('+'):	// print a + for each PUSHD

				for ( lpTemp = (char _far *)gaPushdQueue; ( *lpTemp != _TEXT('\0') ); lpTemp = next_env( lpTemp ) )

					*pszBuffer++ = _TEXT('+');
				break;

			default:	// unknown metachar - ignore it!
				*pszBuffer++ = _TEXT('$');
				*pszBuffer++ = *pszPrompt;
			}

		} else
			*pszBuffer++ = *pszPrompt;
	}

	*pszBuffer = _TEXT('\0');
}
