

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


// BATCH.C - Batch file routines for 4xxx / TCMD family
//   Copyright ( c) 1988 - 2002  Rex C. Conn   All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"
#include "batcomp.h"
#include "strmenc.h"


static int _fastcall __echo( LPTSTR, int );
static int _fastcall __echos( LPTSTR, int );
static int _fastcall __if( LPTSTR, int );
static int _fastcall __input( LPTSTR, int );
static int LoadBTM( void );
static int _fastcall WhileTest( LPTSTR );
static void _near UndoGosubVariables( void );
static int _fastcall goto_label( LPTSTR, int );
static LPTSTR _fastcall ExpandArgument( LPTSTR, int );
static int _fastcall __quit( LPTSTR, BOOL );
static int _fastcall __scrput( LPTSTR, int );
static int _fastcall _setlocal( void );
static int _fastcall _endlocal( LPTSTR );

static unsigned int GetNibble( void );
static char DecompressChar( void );
static void _fastcall TripleKey( char * );

static TCHAR DELIMS[] = _TEXT("%9[^  .\"`\\+=:<>|%]");
static unsigned char cEncryptSig[4] = ENCRYPT_SIG;

extern int fNoComma;


// Execute a batch file (.BAT or .BTM or .CMD)
int _near Batch( int argc, TCHAR **argv )
{
	register int i;
	int nReturn = 0;
	unsigned int uSaveEcho = 0;
	long lSaveFlags = 0L;

	// we can only nest batch files up to MAXBATCH levels deep
	if ( cv.bn >= MAXBATCH - 1 )
		return ( error( ERROR_4DOS_EXCEEDED_NEST, NULL ) );

	if ( fRuntime == 0 ) {
		// set ECHO state flag (from parent file or CFGDOS)
		//  (we don't want runtime users to set BatEcho=2 or Setdos /V2
		uSaveEcho = (( cv.bn < 0 ) ? (unsigned int)gpIniptr->BatEcho : bframe[cv.bn].uEcho );
	}

	// disable batch file nesting unless CALL ( fCall == 1 )
	if ( cv.fCall == 0 ) {

		// reset old batch file to the new batch file!
		if ( cv.bn >= 0 )
			ExitBatchFile();
		else		// first batch file counts as nested
			cv.fCall |= 1;
	}

	// to allow nested batch files, create a batch frame containing
	//   all the info necessary for the batch process
	cv.bn++;

	// clear the current frame
	memset( &bframe[cv.bn], '\0', sizeof( bframe[cv.bn]) );
	bframe[cv.bn].bfd = -1;

	// save the fully expanded name & restore the original name
	//   (for COMMAND / CMD compatibility)
	bframe[cv.bn].pszBatchName = filecase( _strdup( argv[0] ));
	argv[0] = gpBatchName;

	// save batch arguments (%0 - %argc) on the heap
	bframe[cv.bn].Argv = (TCHAR **)malloc(( argc + 1 ) * sizeof( TCHAR * ));
	bframe[cv.bn].Argv[argc] = NULL;

	for ( i = 0; ( i < argc ); i++ )
		bframe[cv.bn].Argv[i] = _strdup( argv[i] );

	// set ECHO state (previously saved from parent file or .INI file )
	bframe[cv.bn].uEcho = (char)uSaveEcho;

	// load an "in-memory" batch file?
	if ( _stricmp( BTM, ext_part( bframe[cv.bn].pszBatchName )) == 0 ) {

		if (( setjmp( cv.env) == -1 ) || ( LoadBTM() != 0 )) {
			ExitBatchFile();
			return (( cv.fException ) ? CTRLC : ERROR_EXIT );
		}
	}

	// save the DO & IFF flags when CALLing another batch file
	if ( cv.fCall )
		lSaveFlags = cv.f.lFlags;

	// clear the DO & IFF flags before running another batch file
	cv.f.lFlags = 0L;

	// if nesting, call BatchCLI() recursively...
	if ( cv.fCall ) {

	    cv.fCall = 0;
	    if (( nReturn = BatchCLI()) == 0 )
		nReturn = bframe[ cv.bn ].nReturn;

	    ExitBatchFile();

	    // restore the IFF flags
	    cv.f.lFlags = lSaveFlags;

	} else {
	    nReturn = ABORT_LINE;		// end any FOR processing
	}

	return nReturn;
}


#pragma alloc_text( MISC_TEXT, DecompressChar, GetNibble, LoadBTM )

static int fNibble;			// 0 = first nibble, 1 = second
static LPBYTE pInputText;	// pointer to current character in input
static LPBYTE pTransTab;	// pointer to translation table


// Get the next nibble during decompression
static unsigned int GetNibble( void )
{
	static unsigned char cThis = 0;
	unsigned int uResult;

	uResult = ( fNibble ? ( cThis & 0xF ) : (( cThis = *pInputText++ ) >> 4 ));
	fNibble ^= 1;
	return uResult;
}


// Decompress one character
static char DecompressChar( void )
{
	register unsigned int uNibble;

	//Get next nibble
	uNibble = GetNibble();

	// Determine decompression type
	switch(uNibble) {
	case 0:
		// Return an uncompressed byte
		uNibble = GetNibble();
		return (char)(uNibble | ((unsigned char)GetNibble() << 4));
	case 1:
		// Return a compressed byte from the second half of the table
		uNibble = GetNibble();
		return (pTransTab[uNibble + 14]);
	default:
		// Return a compressed byte from the first part of the table
		return (pTransTab[uNibble - 2]);
	}
}


// Decompress a compressed batch file
void BatchDecompile( CompHead _far *lpHeader, LPBYTE lpszOutputBuf, unsigned int uCount )
{
	// Initialize globals
	pTransTab = &(lpHeader->cCommon[0]);
	pInputText = pTransTab + BC_COM_SIZE;
	fNibble = 0;

	// Check for new format
	if ( lpHeader->usSignature == 0xEBBE ) {

		// is it encrypted?
		if ((( lpHeader->fEncrypted & 0xF ) ^ ( lpHeader->fEncrypted >> 4 )) == 0 ) {
			// skip password
			pInputText += 4;
		}

	} else {
		// old format doesn't have encryption word
		pInputText -= 2;
		pTransTab -= 2;
	}

	// Decompress all characters
	while ( uCount-- )
		*lpszOutputBuf++ = DecompressChar();
}


// load a batch file into memory (as a .BTM)
static int LoadBTM( void )
{
	UINT uSize, uUnCompSize = 0, uBufferSize;
	LONG lOSize;
	LPBYTE lpBuffer;
	LPBYTE lpCompBuffer = NULL;
	CompHead _far *pBTMHdr;
	char cKey[BC_KEY_LEN], cKeyChar;
	char _far *pKey;
	signed char * szKeyKeyOffsets = (signed char *)KKOFFSETS;
	register int i;

	if ( open_batch_file() == 0 )
		return ERROR_EXIT;

	// get size of original file (may be compressed size!)
	lOSize = (LONG)QueryFileSize( bframe[cv.bn].pszBatchName, 0 );

	uBufferSize = (UINT)(lOSize + 8);

	if ((lOSize >= 0xFFF0L ) || (( lpBuffer = AllocMem( &uBufferSize )) == 0L )) {

LoadBTM_error:
		FreeMem( lpBuffer );	// we may get here from further down!
		close_batch_file();
		return (error( ERROR_NOT_ENOUGH_MEMORY, bframe[cv.bn].pszBatchName ));
	}

	// rewind & read the file (max of 64K in 4DOS)
	RewindFile( bframe[cv.bn].bfd );

	FileRead( bframe[cv.bn].bfd, lpBuffer, (UINT)lOSize, &uSize );
	close_batch_file();

	// check for compression
	pBTMHdr = (CompHead _far *)lpBuffer;

	if (( pBTMHdr->usSignature == 0xEBBE ) || ( pBTMHdr->usSignature == 0xBEEB )) {

		// Get size from old or new header structure, as appropriate
	 	uUnCompSize = ( pBTMHdr->usSignature == 0xEBBE ) ? pBTMHdr->usCount : pBTMHdr->fEncrypted;

		bframe[cv.bn].nFlags |= BATCH_COMPRESSED;

		// turn off batch single-stepping!
		gpIniptr->SingleStep = 0;

		uBufferSize = (UINT)(uUnCompSize + 8);

		// allocate decompression buffer
		if (( lpCompBuffer = (LPBYTE)AllocMem( &uBufferSize )) == 0L )
			goto LoadBTM_error;

		// compressed .BTMs can't be Unicode?

		// Check for encryption; if encrypted, decrypt the user key, and
		// use that to decrypt the common character table and the text.
		// Encryption check is strange to confuse hackers, see BATCOMP.C
		// for approach.

   		if ((( pBTMHdr->fEncrypted & 0xF ) ^ ( pBTMHdr->fEncrypted >> 4 )) == 0 ) {

			// build the key to decrypt the user's key
			for ( i = 0, cKeyChar = ' '; i < BC_KK_LEN; i++ ) {
				cKeyChar += (int)szKeyKeyOffsets[i + 4];
				cKey[i] = cKeyChar;
			}

			// Create a 12-character version of the key encryption key
			TripleKey( cKey );

			// Decrypt the user's key, copy it, and convert to 12 characters
			pKey = (char _far *)pBTMHdr + sizeof(CompHead);

			EncryptDecrypt( NULL, (char _far *)cKey, pKey, BC_MAX_USER_KEY );

			_fmemmove( cKey, pKey, BC_MAX_USER_KEY );
			TripleKey( cKey );

			// Decrypt the new common character table
			EncryptDecrypt( NULL, (char _far *)cKey, (char _far *)&(pBTMHdr->cCommon), BC_COM_SIZE );

			// Decrypt the rest of the buffer in one shot
			EncryptDecrypt( NULL, NULL, pKey + BC_MAX_USER_KEY, (char _far *)pBTMHdr + (unsigned int)lOSize - pKey - BC_MAX_USER_KEY - 4 );

			// zap the key with gibberish
			_fmemmove( cKey, pKey + BC_MAX_USER_KEY, BC_KEY_LEN );

			// verify the encryption signature
			EncryptDecrypt( NULL, NULL, (char _far *)pBTMHdr + (unsigned int)lOSize - 4 , 4);

			for ( i = 0; i < 4; i++ ) {
				if ( lpBuffer[uSize - 4 + i] != cEncryptSig[i] )
			  		return ( error( ERROR_4DOS_INVALID_BATCH, NULL ));
			}
			
		} else {

			// not encrypted ...

			// If new format, use quick XOR decryption on new common character table
			if ( pBTMHdr->usSignature == 0xEBBE ) {
				for ( i = 0; i <= BC_COM_SIZE - 2; i++ )
					pBTMHdr->cCommon[i] ^= pBTMHdr->cCommon[i + 1];
			}
		}

		// uncompress the file
		BatchDecompile( pBTMHdr, lpCompBuffer, uUnCompSize );

		FreeMem( lpBuffer );
		lpBuffer = lpCompBuffer;
		uSize = uUnCompSize;
	}

	// uSize is size in characters + EoF
	lpBuffer[uSize] = EoF;

	bframe[cv.bn].lpszBTMBuffer = lpBuffer;

	// change the file type to .BTM
	bframe[cv.bn].bfd = IN_MEMORY_FILE;

	return 0;
}


// clean up & exit a batch file
int _near ExitBatchFile( void )
{
	register int n;

	if ( cv.bn < 0 )
		return ERROR_EXIT;

	// free the batch argument list
	free( bframe[cv.bn].pszBatchName );
	for ( n = 0; ( bframe[cv.bn].Argv[n] != NULL ); n++)
		free( bframe[cv.bn].Argv[n] );
	free( (char *)bframe[cv.bn].Argv );

	// restore saved environment
	while ( bframe[ cv.bn ].nSetLocalOffset > 0 )
		_endlocal( NULL );

	// free the internal buffer for .BTM files
	FreeMem( bframe[cv.bn].lpszBTMBuffer );

	// free ON xxx arguments
	if ( bframe[cv.bn].pszOnBreak != NULL ) {
		free( bframe[cv.bn].pszOnBreak );
		bframe[cv.bn].pszOnBreak = NULL;
	}

	if ( bframe[cv.bn].pszOnErrorMsg != NULL ) {
		free( bframe[cv.bn].pszOnErrorMsg );
		bframe[cv.bn].pszOnErrorMsg = NULL;
	}

	if ( bframe[cv.bn].pszOnError != NULL ) {

		free( bframe[cv.bn].pszOnError );
		bframe[cv.bn].pszOnError = NULL;

		// reset error popups to previous value
		ServCtrl( SERV_AF, bframe[cv.bn].uOnErrorState );
	}

	// point back to previous batch file (if any)
	cv.bn--;

	// reset the window title

	return 0;
}


int _near BatchDebugger( void )
{
	extern int gnPopExitKey;
	int i, nFH, nEditMode = EDIT_DATA;
	unsigned int nLength, uSize;
	unsigned long ulListSize = 0L;
	TCHAR _far * lpszArg;
	TCHAR _far *lpBuf;
	TCHAR szBuffer[MAXFILENAME+12];
	TCHAR _far * _far *lppList = 0L;
	char szLine[MAXLINESIZ];

	if ( fRuntime )
		return 0;

	if (( nFH = _sopen( bframe[ cv.bn ].pszBatchName, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 )
		return ( error( _doserrno, bframe[cv.bn].pszBatchName ));
	
	uSize = (UINT)( QueryFileSize( bframe[ cv.bn ].pszBatchName, 0 ) + 0x10 ) * sizeof(TCHAR);
	lpBuf = (TCHAR _far *)AllocMem( &uSize );

	HoldSignals();

	for ( i = 0, lpszArg = lpBuf; (( nLength = getline( nFH, szLine, MAXLINESIZ-1, nEditMode )) > 0 ); ) {
		_fstrcpy( lpszArg, szLine );

		// allocate memory for 256 lines at a time
		if (( i % 256 ) == 0 ) {
		    ulListSize += ( 256 * sizeof(TCHAR _far *) );
		    if (( lppList = (TCHAR _far * _far *)ReallocMem( lppList, ulListSize )) == 0L )
			return ( error( ERROR_NOT_ENOUGH_MEMORY, NULL ));
		}

		lppList[i++] = lpszArg;

		lpszArg += nLength;
	}

	_close( nFH );

	// call the popup window
	sprintf( szBuffer, _TEXT("%s [%u]"), bframe[cv.bn].pszBatchName, bframe[cv.bn].uBatchLine );

	if ( wPopSelect( 2, 4, 8, 70, lppList, i, bframe[cv.bn].uBatchLine, szBuffer, DEBUGGER_PROMPT, _TEXT("TSJXVAOQL"), 0x12 ) == 0L )
		gnPopExitKey = ESC;

	FreeMem( lpBuf );
	FreeMem( lppList );

	// reenable signal handling after cleanup
	EnableSignals();

	// if we aborted wPopSelect with ^C, bomb after cleanup
	if ( cv.fException )
		longjmp( cv.env, -1 );

	return gnPopExitKey;
}


// send a block of text to the screen
int _near Battext_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszToken;

	// make sure the batch file is still open
	if ( open_batch_file() == 0 )
		return BATCH_RETURN;

	// scan the batch file until we find an ENDTEXT
	while ( getline( bframe[cv.bn].bfd, gszCmdline, CMDBUFSIZ-1, EDIT_DATA ) > 0 ) {

		bframe[cv.bn].uBatchLine++;

		pszToken = first_arg( gszCmdline );
		if (( pszToken != NULL ) && ( _stricmp( pszToken, ENDTEXT ) == 0 ))
			return 0;

		printf( FMT_STR_CRLF, gszCmdline );
	}

	return ( error( ERROR_4DOS_MISSING_ENDTEXT, NULL ));
}


// make a beeping noise (optionally specifying tone & length)
int _near Beep_Cmd( LPTSTR pszCmdLine )
{
	unsigned int uFrequency, uDuration, uLength;

	uFrequency = gpIniptr->BeepFreq;
	uDuration = gpIniptr->BeepDur;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	do {
		if ( *pszCmdLine ) {
			// skip non-numeric args
			sscanf( pszCmdLine, _TEXT("%*[^0123456789] %u %n"), &uFrequency, &uLength );
			pszCmdLine += uLength;
			if ( *pszCmdLine ) {
				sscanf( pszCmdLine, _TEXT("%d %n"), &uDuration, &uLength );
				pszCmdLine += uLength;
			}

			if ( uLength == 0 )
				return ( Usage( BEEP_USAGE ));
		}

		// kludge to abort on a ^C
		if ( SysBeep( uFrequency, uDuration ) == CTRLC )
			_getch();

	} while ( *pszCmdLine );

	return 0;
}


// CALL a nested batch file (or any executable file )
// (because the CALL argument may be a .COM or .EXE, we can't just call BATCH)
int _near Call_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( CALL_USAGE ));

	cv.fCall = 1;

	nReturn = command( pszCmdLine, 0 );
	cv.fCall = 0;

	return nReturn;
}


// wait for the specified number of seconds
int _near Delay_Cmd( LPTSTR pszCmdLine )
{
	register unsigned int i;
	LPTSTR pszToken;
	ULONG ulWait = 1L;
	long lFlags = 0, lTemp;

	if (( pszCmdLine ) && ( *pszCmdLine )) {
		
		for ( i = 0; (( pszToken = ntharg( pszCmdLine, i )) != NULL ); i++ ) {
			if (( lTemp = switch_arg( pszToken, _TEXT("BM") )) != 0L )
				lFlags |= lTemp;
			else if ( isdigit( *pszToken ))
				sscanf( pszToken, FMT_ULONG, &ulWait );
		}
	}

	SysWait( ulWait, (int)lFlags );

	return 0;
}


#define DOTYPE_IN 1
#define DOTYPE_TO 2
#define DOTYPE_REPEAT 4
#define DOTYPE_UNTIL 8
#define DOTYPE_WHILE 0x10

// DO loop
int _near Do_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	LONG lRepetitor = -1L;
	LONG lStart = 0L, lEnd = 0L, lBy = 1L, lOldDoState, lCurrentDoState;
	TCHAR szVar[80], szFileName[MAXFILENAME], *pExpression = NULL, *pszTemp;
	int fDoType = 0, nReturn = 0, n, nFind = FIND_FIRST, hFH = -1;
	int nInclusive = 0, nExclusive = 0, nOr = 0, nEditFlags = EDIT_DATA;
	unsigned int uSavedLine;
	long fDoFlags = 0, lMode = 0x10 | FIND_NO_ERRORS;
	FILESEARCH dir;
	LONG lSaveOffset;

	dir.hdir = INVALID_HANDLE_VALUE;
	szFileName[0] = _TEXT('\0');

	if (( pszArg = first_arg( pszCmdLine )) != NULL ) {

	    if (( _stricmp( pszArg, DO_WHILE ) == 0 ) || ( _stricmp( pszArg, DO_UNTIL ) == 0 )) {

			if ( _ctoupper( *pszArg ) == _TEXT('W') )
				fDoType = DOTYPE_WHILE;
			else
				fDoType = DOTYPE_UNTIL;

			next_arg( pszCmdLine, 1 );
			if ( *pszCmdLine == _TEXT('\0') ) {
do_usage:
				return Usage( DO_USAGE );
			}

			pExpression = pszCmdLine;

	    } else {

		if ( var_expand( pszCmdLine, 1 ) != 0 )
			return ERROR_EXIT;

		pszArg = skipspace( pszCmdLine );
		strip_trailing( pszArg, WHITESPACE );

		if ( _stricmp( pszArg, DO_FOREVER ) == 0 ) {

			// repeat forever
			fDoType = DOTYPE_REPEAT;

		} else if ( is_signed_digit( *pszArg )) {

			// repeat specified # of times
			sscanf( pszArg, FMT_LONG, &lRepetitor );
			fDoType = DOTYPE_REPEAT;

			if ( lRepetitor < 0 )
				goto do_usage;

		} else if ((( pszTemp = ntharg( pszArg, 1 )) != NULL ) && ( stricmp( pszTemp, _TEXT("in") ) == 0 )) {

			// DO x in *.* ...   or   DO x in @filename
			fDoType = DOTYPE_IN;
			lMode = 0x10 | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES;
			szFileName[0] = _TEXT('\0');

			// initialize date/time/size ranges start & end values
			ntharg( pszArg, 0x8002 );
			if (( pExpression = gpNthptr ) == NULL )
				goto do_usage;

			if ( GetRange( pExpression, &(dir.aRanges), 1 ) != 0 )
				goto do_usage;

			if ( dir.aRanges.pszExclude != NULL ) {
				pszTemp = (LPTSTR)_alloca( ( strlen( dir.aRanges.pszExclude ) + 1 ) * sizeof(TCHAR) );
				dir.aRanges.pszExclude = strcpy( pszTemp, dir.aRanges.pszExclude );
			}

			// check for and remove switches
			if ( GetSwitches( pExpression, _TEXT("*L"), &fDoFlags, 1 ) != 0 )
				goto do_usage;

			// save the inclusive/exclusive modes, in case we're
			//   doing a DIR or SELECT
			nInclusive = gnInclusiveMode;
			nExclusive = gnExclusiveMode;
			nOr = gnOrMode;

			if ( fDoFlags & 1 )
				lMode |= FIND_BYATTS | 0x17;

			if ( *pExpression == _TEXT('\0') )
				goto do_usage;

			// save variable name
			sscanf( pszArg, _TEXT("%79[^ \t=,]"), szVar );

		} else {

			// must be "DO x=1 to n [by z]" syntax
			if ( sscanf( pszArg, _TEXT("%79[^ \t=,] = %ld %*[TtOo] %ld %n"), szVar, &lStart, &lEnd, &n ) < 4 )
				goto do_usage;

			// check for optional "BY n" syntax
			pszArg += n;
			if ( *pszArg ) {
				if ( _strnicmp( pszArg, DO_BY, 2 ) == 0 )
					sscanf( pszArg+2, FMT_LONG, &lBy );
				else
					goto do_usage;
			}

			fDoType = DOTYPE_TO;
		}
	    }
	}

	if ( pExpression != NULL )
		pExpression = strcpy( (LPTSTR)_alloca( ( strlen( pExpression ) + 1 ) * sizeof(TCHAR) ), pExpression );

	// save current position to RETURN to
	lSaveOffset = bframe[cv.bn].lOffset;

	// save previous DO flags
	lOldDoState = cv.f.lFlags;
	(cv.f.flag._do)++;

	// save the current line number
	uSavedLine = bframe[ cv.bn ].uBatchLine;

	lCurrentDoState = cv.f.lFlags;

	for ( ; ; ) {

		// we need to do a ^C check here in case somebody puts something
		//   like %_kbhit in the DO statement!
		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			nReturn = CTRLC;
			FindClose( dir.hdir );
			break;
		}

		// check if we've aborted the DO loop (i.e., a batch file
		//   chained to another batch file )
		if ( cv.f.flag._do == 0 )
			break;

		if ( cv.f.flag._do_leave ) {
			cv.f.flag._do_leave--;
			break;
		}

		// if new offset < original offset, we must have GOTO'd
		//   to previous point, so abort this DO loop
		if ( bframe[ cv.bn ].lOffset < lSaveOffset )
			break;

		// restore the saved file read offset
		bframe[ cv.bn ].lOffset = lSaveOffset;

		// restore saved line number
		bframe[ cv.bn ].uBatchLine = uSavedLine;

		// restore DO / IFF flags
		cv.f.lFlags = lCurrentDoState;

		// *.* or @filename loop?
		if ( fDoType == DOTYPE_IN ) {

			gszCmdline[0] = _TEXT('\0');
			if ( fDoFlags & 2 ) {

				// a text argument list, not a file list
				if (( pszTemp = first_arg( pExpression )) != NULL ) {
					strcpy( gszCmdline, pszTemp );
					next_arg( pExpression, 1 );
				} else
					cv.f.flag._do_leave++;

			} else if ( *pExpression == _TEXT('@') ) {

				// get argument from next line in file
				strcpy( gszCmdline, pExpression + 1 );

				if ( QueryIsCON( gszCmdline ))
					hFH = STDIN;

				else if ( hFH < 0 ) {

					if ( _stricmp( gszCmdline, CLIP ) == 0 ) {
						RedirToClip( gszCmdline, 0 );
						CopyFromClipboard( gszCmdline );
					} else
						mkfname( gszCmdline, 0 );

					if (( hFH = _sopen( gszCmdline, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 ) {
						nReturn = error( _doserrno, gszCmdline );
						cv.f.flag._do_leave++;
					}

				}

				if (( hFH >= 0 ) && ( getline( hFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) <= 0 )) {
					// end of file - close & get next arg
					if ( hFH != STDIN )
						_close( hFH );
					hFH = -1;
					cv.f.flag._do_leave++;
				}

			} else {

				// set VAR to each matching filename
				gnInclusiveMode = nInclusive;
				gnExclusiveMode = nExclusive;
				gnOrMode = nOr;

				// kludge for "...\*.*" syntax
NextINName:
				if ( szFileName[0] == _TEXT('\0') ) {
					pszTemp = first_arg( pExpression );
					copy_filename( szFileName, (( pszTemp == NULL ) ? NULLSTR : pszTemp ));
					if ( strstr( szFileName, _TEXT("...\\") ) != NULL )
						mkfname( szFileName, MKFNAME_NOERROR );
				}

				// if no more matches, get next argument
				if ( find_file( nFind, szFileName, ( lMode | FIND_NO_ERRORS ), &dir, gszCmdline ) == NULL ) {
					
					szFileName[0] = _TEXT('\0');
					next_arg( pExpression, 1 );
					if ( *pExpression ) {
						nFind = FIND_FIRST;
						goto NextINName;
					}
					cv.f.flag._do_leave++;
				}

				nFind = FIND_NEXT;
			}

			strins( gszCmdline, _TEXT("=") );
			strins( gszCmdline, szVar );
			add_variable( gszCmdline );

		// Repetitor loop?
		} else if ( fDoType == DOTYPE_REPEAT ) {

			if ( lRepetitor == 0L )
				cv.f.flag._do_leave++;
			else if ( lRepetitor > 0L )
				lRepetitor--;

		// TO loop?
		} else if ( fDoType == DOTYPE_TO ) {

		    sprintf( gszCmdline, _TEXT("%s=%ld"), szVar, lStart );
		    add_variable( gszCmdline );

			// if condition fails, wait for ENDDO & then exit
			if ( lBy < 0L ) {
				if ( lStart < lEnd )
					cv.f.flag._do_leave++;
			} else if ( lStart > lEnd )
				cv.f.flag._do_leave++;

		    lStart += lBy;

		// WHILE loop?
		} else if ( fDoType == DOTYPE_WHILE ) {
			if (( n = WhileTest( pExpression )) < 0 ) {
				nReturn = -n;
				break;
			} else if ( n == 0 ) {
				// if condition fails, wait for ENDDO & exit
				cv.f.flag._do_leave++;
			}
		}

		if ((( nReturn = BatchCLI()) == CTRLC ) || ( cv.fException ) || ( bframe[cv.bn].lOffset == -1L )) {
			bframe[cv.bn].lOffset = -1L;
			break;
		}

		// we need to do a ^C check here in case somebody puts
		//   something like %_kbhit in the UNTIL statement!
		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			nReturn = CTRLC;
			FindClose( dir.hdir );
			break;
		}
		EnableSignals();

		// check if we've aborted the DO loop
		if ( cv.f.flag._do == 0 )
			break;

		// UNTIL loop?
		if ( fDoType == DOTYPE_UNTIL ) {

		    if (( n = WhileTest( pExpression )) < 0 ) {
			nReturn = -n;
			break;

		    } else if ( n == 1 ) {

			cv.f.flag._do_leave++;
			bframe[cv.bn].lOffset = lSaveOffset;
			bframe[cv.bn].uBatchLine = uSavedLine;

			// dummy call just to skip to ENDDO
			BatchCLI();
		    }
		}
	}

	if ( hFH > 0 ) {
		_close( hFH );
		hFH = -1;
	}

	// restore previous DO state (if _do == 0, we've had a GOTO)
	if ( cv.f.flag._do )
		cv.f.lFlags = lOldDoState;

	return nReturn;
}


// evaluate WHILE or UNTIL expression
static int _fastcall WhileTest( LPTSTR pszExpression )
{
	register int nCondition;

	if ( cv.f.flag._do_end != 0 )
		return 0;

	if (( nCondition = TestCondition( strcpy( gszCmdline, pszExpression ), 0 )) == -USAGE_ERR) {
		gpInternalName = _TEXT("DO");
		return -( Usage( DO_USAGE ));
	}

	return nCondition;
}


#pragma alloc_text( MISC_TEXT, do_parsing )

// check for DO processing
int _fastcall do_parsing( register LPTSTR pszCmdName )
{
	// if not processing a DO just return
	if ( cv.f.flag._do == 0 )
	    return 0;

	if ( _stricmp( pszCmdName, DO_END ) == 0 ) {

	    // if not ignoring nested DO's, return to Do_Cmd()
	    if ( cv.f.flag._do_end == 0 )
		return BATCH_RETURN;

	    // drop one "ignored DO loop" level
	    (cv.f.flag._do_end)--;

	    return 1;
	}

	if ( _stricmp( pszCmdName, DO_DO ) == 0 ) {

	    // if waiting for "ENDDO", ignore nested DO
	    if ( cv.f.flag._do_end | cv.f.flag._do_leave )
		( cv.f.flag._do_end)++;

	} else if (( cv.f.flag._do_end | cv.f.flag._do_leave ) == 0 ) {

	    if ( _stricmp( pszCmdName, DO_LEAVE ) == 0 ) {

		// ignore everything until END, then return to "Do_Cmd()" & exit
		cv.f.flag._do_leave++;

	    } else if ( _stricmp( pszCmdName, DO_ITERATE ) == 0 ) {

		// return to top of loop
		return BATCH_RETURN;
	    }
	}

	return ( cv.f.flag._do_end | cv.f.flag._do_leave );
}


// echo arguments to the screen (stdout)
int _near Echo_Cmd( LPTSTR pszCmdLine )
{
	return ( __echo( pszCmdLine, 0 ));
}


// echo arguments to the screen (stderr)
int _near EchoErr_Cmd( LPTSTR pszCmdLine )
{
	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;
	return ( __echo( pszCmdLine, 1 ));
}


// echo arguments to the screen (stdout or stderr)
static int _fastcall __echo( LPTSTR pszCmdLine, int fErr )
{
	int fEcho;

	// ECHOERR ?
	if ( fErr )
		goto EchoErr;

	if ( pszCmdLine == NULL ) {

		// just inquiring about ECHO state
		printf( ECHO_IS, (( cv.bn >= 0 ) ? bframe[cv.bn].uEcho : cv.fVerbose ) ? ON : OFF );

	} else if (( fEcho = OffOn( pszCmdLine )) == 0 ) {

		// disable batch file or command line echoing
		if ( cv.bn >= 0 ) {
			// unless "persistent echo" is enabled
			bframe[cv.bn].uEcho &= 2;
		} else
			cv.fVerbose = 0;

	} else if ( fEcho == 1 ) {

		// enable batch file or command line echoing
		if ( cv.bn >= 0 ) {
			// don't turn off "persistent echo" (uEcho==2)
			bframe[cv.bn].uEcho |= 1;
		} else
			cv.fVerbose = 1;

	} else {	// print the line verbatim
EchoErr:
		qprintf( (( fErr ) ? STDERR : STDOUT ), FMT_STR_CRLF, pszCmdLine+1 );
	}

	return 0;
}


// echo arguments to the screen (stdout) without a trailing CR/LF
int _near Echos_Cmd( LPTSTR pszCmdLine )
{
	return ( __echos( pszCmdLine, 0 ));
}


// echo arguments to the screen (stdout) without a trailing CR/LF
int _near EchosErr_Cmd( LPTSTR pszCmdLine )
{
	return ( __echos( pszCmdLine, 1 ));
}


// echo arguments to the screen (stdout) without a trailing CR/LF
static int _fastcall __echos( LPTSTR pszCmdLine, int fErr )
{
	register int fStd;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		fStd = (( fErr ) ? 2 : 1 );

		if ( setjmp( cv.env ) != -1 )
			qprintf( fStd, FMT_STR, pszCmdLine+1 );
	}

	return 0;
}


#define FOR_ATTRIBUTES 1
#define FOR_NO_ESCAPE 2
#define FOR_FILESET 4
#define FOR_NO_DOTS 8
#define FOR_STEP 0x10
#define FOR_GLOBAL 0x20
#define FOR_GLOBAL_PASS2 0x40

// FOR - allows iterative execution of DOS commands
int _near For_Cmd( LPTSTR pszCmdLine )
{
	register TCHAR *pszArg, *pszLine;
	TCHAR *pszForVar, *pszArgumentSet, *pszCurrentDirectory = NULL, *pszRanges = NULLSTR;
	TCHAR szSource[MAXFILENAME];
	TCHAR *pszTokenArg, szTokens[64], szDelims[64], szVariables[64];
	int nSaveInclusive, nSaveExclusive, nSaveOR;
	int i, n, nReturn = 0, hFH = -1, nFind, argc, nEditFlags = EDIT_DATA;
	int nStart = 0, nStep = 0, nEnd = 0;
	int nSkip = 0, fDelete = 0, nUseBackQ = 0;
	long fForFlags, lMode = FIND_NO_ERRORS | FIND_RANGE | FIND_EXCLUDE;
	FILESEARCH dir;
	TCHAR nEOL = _TEXT('\0');

	dir.hdir = INVALID_HANDLE_VALUE;
	szVariables[0] = _TEXT('\0');

	// initialize date/time/size ranges start & end values

	// save /A:x, range, and /H arguments for FOR /R
	szSource[0] = _TEXT('\0');

	for ( i = 0; ; i++ ) {

		fNoComma = 1;
		pszArg = ntharg( pszCmdLine, i | 0x2000 );
		fNoComma = 0;
		if ( pszArg == NULL )
			break;

		if ( *pszArg++ == gpIniptr->SwChr ) {
		
			if ( _ctoupper( *pszArg ) == _TEXT('H') ) {
				if ( i > 0 )
					strcat( szSource, _TEXT(" ") );
				sprintf( strend(szSource), _TEXT("%c%c"), gpIniptr->SwChr, *pszArg );
			}

			if (( *pszArg == _TEXT('[') ) || ( _ctoupper( *pszArg ) == _TEXT('A') )) {
				if ( i > 0 )
					strcat( szSource, _TEXT(" ") );
				strcat( szSource, pszArg-1 );
			}

		} else
			break;
	}

	if ( szSource[0] )
		pszRanges = strcpy( (LPTSTR)_alloca( ( strlen( szSource ) + 1 ) * sizeof(TCHAR) ), szSource );

	// get date/time/size/exception ranges
	GetRange( pszCmdLine, &(dir.aRanges), 1 );

	// preserve file exception list
	if ( dir.aRanges.pszExclude != NULL ) {
		pszArg = (LPTSTR)_alloca( ( strlen( dir.aRanges.pszExclude ) + 1 ) * sizeof(TCHAR) );
		dir.aRanges.pszExclude = strcpy( pszArg, dir.aRanges.pszExclude );
	}

	// check for and remove switches
	if ( GetSwitches( pszCmdLine, _TEXT("*DFHLRZ"), &fForFlags, 1 ) != 0 ) {
for_usage_error:
		return ( Usage( FOR_USAGE ));
	}

	if ( fForFlags & FOR_FILESET ) {

		// get the /F token arguments
		strcpy( szTokens, _TEXT("1") );
		strcpy( szDelims, _TEXT(" \t,") );

		while ( *pszCmdLine == _TEXT('"') ) {

			pszLine = first_arg( pszCmdLine ); 

			i = strlen( pszLine );
			strcpy( szSource, pszLine );
			
			// check for variable in directory name
			if ( var_expand( szSource, 1 ) != 0 )
				return ( Usage( FOR_USAGE ));

			// horrible kludge for compatibility with CMD.EXE syntax (multiple
			//   arguments in a single quoted string)
			for ( pszArg = szSource + strlen(szSource) - 1; (( pszArg > szSource ) && *pszArg ); pszArg-- ) {

				if ( strnicmp( pszArg, _TEXT("eol="), 4 ) == 0 )
					nEOL = pszArg[4];
				else if ( strnicmp( pszArg, _TEXT("skip="), 5 ) == 0 )
					nSkip = atoi(pszArg+5);
				else if ( strnicmp( pszArg, _TEXT("delims="), 7 ) == 0 )
					sscanf( pszArg+7, _TEXT("%63[^\"]"), szDelims );
				else if ( strnicmp( pszArg, _TEXT("tokens="), 7 ) == 0 )
					sscanf( pszArg+7, _TEXT("%63[^ \t\"]"), szTokens );
				else if ( stricmp( pszArg, _TEXT("usebackq") ) == 0 )
					nUseBackQ = 1;
				else
					continue;

				*pszArg = _TEXT('\0');
				if ( isspace( pszArg[-1] ))
					*--pszArg = _TEXT('\0');
			}

			strcpy( pszCmdLine, skipspace( pszCmdLine + i ));
		}
	}

	if ( fForFlags & FOR_GLOBAL ) {

		// check for path argument
		pszArg = ntharg( pszCmdLine, 1 );

		// they should quote the arg if it's a variable, but just in case, check for the
		//  next argument (either the FOR variable or the IN keyword)
		if (( *pszCmdLine != _TEXT('%') ) || ( *pszArg == _TEXT('%'))) {

			// save the original directory
			if (( pszCurrentDirectory = gcdir( NULL, 0 )) == NULL )
				return ERROR_EXIT;
			pszCurrentDirectory = strdup( pszCurrentDirectory );

			if (( pszArg = first_arg( pszCmdLine )) == NULL )
				goto for_usage_error;
			i = strlen( pszArg );
			pszArg = strcpy( szSource, pszArg );

			// check for variable in directory name
			if ( var_expand( pszArg, 1 ) != 0 )
				return ERROR_EXIT;

			mkfname( pszArg, 0 );
			__cd( pszArg, CD_CHANGEDRIVE );
			strcpy( pszCmdLine, pszCmdLine + i );
		}

		strins( pszCmdLine, _TEXT(" /z ") );
		if ( *pszRanges )
			strins( pszCmdLine, pszRanges );
		strins( pszCmdLine, _TEXT("/iq for ") );
		nReturn = Global_Cmd( pszCmdLine );

		// restore original directory
		if ( pszCurrentDirectory != NULL )
			__cd( pszCurrentDirectory, CD_CHANGEDRIVE );
		return nReturn;
	}

	// save the inclusive/exclusive modes, in case we're
	//   doing a DIR or SELECT
	nSaveInclusive = gnInclusiveMode;
	nSaveExclusive = gnExclusiveMode;
	nSaveOR = gnOrMode;

	if ( fForFlags & 1 )
		lMode |= 0x17 | FIND_BYATTS;

	// check for proper syntax
	pszArg = ntharg( pszCmdLine, 1 );
	if (( pszArg == NULL ) || ( _stricmp( pszArg, FOR_IN ) != 0 ) || (( pszArg = first_arg( pszCmdLine )) == NULL ))
		goto for_usage_error;

	// get variable name & save it on the stack
	for ( ; ( *pszArg == _TEXT('%') ); pszArg++ )
		;
	if ( *pszArg == _TEXT('\0') )
		goto for_usage_error;

	pszForVar = (LPTSTR)_alloca( ( strlen( pszArg ) + 4 ) * sizeof(TCHAR) );
	sprintf( pszForVar, _TEXT("%s="), pszArg );

	// set flag if FOR variable is single char (kludge for COMMAND.COM
	//    & CMD.EXE limitation)
	if ( pszArg[1] == _TEXT('\0') )
		strins( pszForVar, _TEXT("\001") );

	ntharg( pszCmdLine, 0x8002 );
	pszArgumentSet = gpNthptr;
	if (( pszArgumentSet == NULL ) || ( *pszArgumentSet != _TEXT('(') ))
		goto for_usage_error;

	// get the end of the argument list, skipping any escape'd characters
	pszArg = skipspace( pszArgumentSet + 1 );

	// check for /F & parsing output of a command
	if (( fForFlags & FOR_FILESET ) && ( *pszArg == _TEXT('\'') )) {

		for ( pszArg++; (( *pszArg ) && ( *pszArg != _TEXT('\''))); pszArg++ ) {
			if ( *pszArg == gpIniptr->EscChr )
				pszArg++;
		}
	}

	for ( ; (( *pszArg ) && ( *pszArg != _TEXT(')') )); pszArg++ ) {
		if ( *pszArg == gpIniptr->EscChr )
			pszArg++;
	}

	pszArgumentSet++;

	// rub out the ')', & point to the command
	*pszArg++ = _TEXT('\0');
	pszArg = skipspace( pszArg );

	// ignore the DO keyword
	if (( pszLine = first_arg( pszArg )) == NULL )
		goto for_usage_error;

	if ( _stricmp( pszLine, FOR_DO ) == 0 ) {
		next_arg( pszArg, 1 );
		if ( *pszArg == _TEXT('\0') )
			goto for_usage_error;
	}

	// save the command line onto the stack
	pszLine = strcpy( (LPTSTR)_alloca( ( strlen( pszArg ) + 2 ) * sizeof(TCHAR) ), pszArg );

	// do variable expansion on the file spec(s)
	strcpy( gszCmdline, pszArgumentSet );
	if ( var_expand( gszCmdline, 1 ) != 0 )
		return ERROR_EXIT;

	pszArgumentSet = skipspace( gszCmdline );
	if (( fForFlags & FOR_FILESET ) && ( *pszArgumentSet == _TEXT('\'') )) {

		// parse output of command
		pszArgumentSet++;
		strip_trailing( pszArgumentSet, _TEXT(" ") );
		strip_trailing( pszArgumentSet, _TEXT("'") );

		if ( GetTempDirectory( szSource ) == 0L )
		    *szSource = _TEXT('\0');
		UniqueFileName( szSource );
		sprintf( strend(pszArgumentSet), _TEXT(" >! %s"), szSource );
		command( pszArgumentSet, 0 );
		strcpy( pszArgumentSet, szSource );
		fDelete = 1;
	}

	if ( fForFlags & FOR_STEP ) {
		// save the /L arguments
		sscanf( pszArgumentSet, _TEXT("%d,%d,%d"), &nStart, &nStep, &nEnd );
	} else {
		// save the variable set onto the stack
		pszArgumentSet = strcpy( (LPTSTR)_alloca( ( strlen( pszArgumentSet ) + 1 ) * sizeof(TCHAR) ), pszArgumentSet );
	}

	for ( argc = 0; ; argc++ ) {

		if ( fForFlags & FOR_STEP ) {
			sprintf( szSource, FMT_INT, nStart );
			pszArg = szSource;
		} else {

			szSource[0] = _TEXT('\0');

			// if FOR /R, insert current directory
			if ( fForFlags & FOR_GLOBAL_PASS2 ) {
				if (( pszArg = gcdir( NULL, 0 )) != NULL )
					copy_filename( szSource, pszArg );
			}

			if (( pszArg = ntharg( pszArgumentSet, argc )) == NULL )
				break;
			mkdirname( szSource, pszArg );
		}

		for ( nFind = FIND_FIRST; ; ) {

			// check for ^C abort
			if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
				if ( nFind == FIND_NEXT )
					FindClose( dir.hdir );
				nReturn = CTRLC;
				goto for_bye;
			}

			// (re )enable ^C and ^BREAK
			EnableSignals();

			if (( nReturn == ABORT_LINE ) || (( cv.bn >= 0 ) && ( bframe[cv.bn].lOffset == -1L ))) {
				if ( nFind == FIND_NEXT )
					FindClose( dir.hdir );
				goto for_bye;
			}

			if ( szSource[0] == _TEXT('\0') )
				break;

			// close file if not still getting input
			if ( hFH > 0 ) {
				if ((( fForFlags & FOR_FILESET ) == 0 ) && ( szSource[0] != _TEXT('@') )) {
					_close( hFH );
					hFH = -1;
				}
			}

			gszCmdline[0] = _TEXT('\0');

			if ((( fForFlags & FOR_NO_ESCAPE ) == 0 ) && ( szSource[0] == _TEXT('/') )) {

				// kludge for COMMAND.COM undoc'd behavior
				//   (if first char is '/', only return second
				//   char, then return rest of arg )
				gszCmdline[0] = szSource[1];
				gszCmdline[1] = _TEXT('\0');

				// strip / and first character
				strcpy( gpNthptr, (( gpNthptr[1] ) ? gpNthptr+2 : gpNthptr+1 ));
				argc--;
				szSource[0] = _TEXT('\0');

			} else if (( strpbrk( szSource, WILD_CHARS ) != NULL ) || ( nFind == FIND_NEXT )) {

				// if ARG includes wildcards, set VAR to each
				//   matching filename from the disk
				gnInclusiveMode = nSaveInclusive;
				gnExclusiveMode = nSaveExclusive;
				gnOrMode = nSaveOR;

				// kludge for "...\*.*" syntax
				if ( strstr( szSource, _TEXT("...\\") ) != NULL )
					mkfname( szSource, MKFNAME_NOERROR );

				dir.szAlternateFileName[0] = _TEXT('\0');

				// if no more matches, get next argument
				if ( find_file( nFind, szSource, lMode, &dir, gszCmdline ) == NULL )
					break;

				nFind = FIND_NEXT;
				if (( fForFlags & FOR_NO_DOTS ) && ( QueryIsDotName( dir.szFileName )))
					continue;

				if (( cv.fLfnFor == 0 ) && ( dir.szAlternateFileName[0] ))
					strcpy( gszCmdline, dir.szAlternateFileName );

			} else if (( szSource[0] == _TEXT('@') ) && (( hFH >= 0 ) || ( QueryIsDevice( szSource+1 )) || (( is_file( szSource+1 )) && ( is_file( szSource ) == 0 )))) {

GetFileTokens:
				if ( hFH < 0 ) {

				    // get FOR argument from next line in file
				    if ( gszCmdline[0] == _TEXT('\0') )
				        strcpy( gszCmdline, szSource+1 );

				    if ( QueryIsCON( gszCmdline ))
						hFH = STDIN;
					else {

						if ( _stricmp( gszCmdline, CLIP ) == 0 ) {
							RedirToClip( gszCmdline, 0 );
							CopyFromClipboard( gszCmdline );
						} else
							mkfname( gszCmdline, 0 );

						if (( hFH = _sopen( gszCmdline, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 ) {
							nReturn = error( _doserrno, gszCmdline );
							goto for_bye;
						}

				    }
				}

				if ( getline( hFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) <= 0 ) {
					// end of file - close & get next arg
					if ( hFH != STDIN )
						_close( hFH );
					hFH = -1;
					break;
				}

				// skipping first few lines?
				if ( nSkip > 0 ) {
					nSkip--;
					continue;
				}

				// parse the selected tokens
				if ( fForFlags & FOR_FILESET ) {

					int nMax = 0;

GetStringTokens:
				    // check for defined EOL character
				    if (( nEOL != 0 ) && (( pszArg = strchr( gszCmdline, nEOL )) != NULL ))
						*pszArg = _TEXT('\0');

					if (( fForFlags & FOR_FILESET ) && ( gszCmdline[0] == _TEXT('\0')))
						continue;

					nStep = pszForVar[1];
				    for ( i = 0, n = 0; (( pszTokenArg = ntharg( szTokens, i )) != NULL ); i++ ) {

						nStart = 0;
						if (( n = sscanf( pszTokenArg, _TEXT("%d-%d"), &nStart, &nEnd )) == 1 )
							nEnd = nStart;
						else if ( *pszTokenArg == _TEXT('*')) {
							// remainder of arguments
							nEnd = 9999;
							nStart = ++nMax;
						} else if (( n == 0 ) || ( strlen( pszForVar ) > 3 ) || ( nEnd < nStart ))
							goto for_usage_error;
GetRemainder:
						for ( n = nStart; ( n <= nEnd ); n++ ) {
							
							pszArg = (LPTSTR)malloc( ( strlen( gszCmdline ) + strlen(pszForVar) + 3 ) * sizeof(TCHAR) );
							strcpy( pszArg, gszCmdline );

							GetToken( pszArg, szDelims, n, (( nEnd == 9999 ) ? nEnd : n ));

							// create new environment vars to expand
							strins( pszArg, pszForVar );
							pszArg[1] = (TCHAR)nStep;
							nStep++;
							add_variable( pszArg );

							// save the name so we can delete it later
							if ( strchr( szVariables, pszArg[1] ) == NULL )
								sprintf( strend(szVariables), FMT_CHAR, pszArg[1] );
							free( pszArg );

							nMax = n;

							// if we just returned "all remaining arguments", stop now
							if (( nStep > _TEXT('z') ) || ( nEnd == 9999 ))
								break;
						}

						// * means "all remaining args"
						for ( ; ( is_signed_digit( *pszTokenArg )); pszTokenArg++ )
							;
						if ( *pszTokenArg == _TEXT('*') ) {
							pszTokenArg++;
							nStart++;
							nEnd = 9999;
							goto GetRemainder;
						}
					}
				}

			} else if ( fForFlags & FOR_FILESET ) {

				if (( hFH >= 0 ) || ( is_file( szSource ))) {
					strcpy( gszCmdline, szSource );
					goto GetFileTokens;
				} else if ( szSource[0] == _TEXT('"') ) {
					// parse a string, not a file
					// strip leading/trailing "
					strcpy( gszCmdline, szSource );
					StripQuotes( gszCmdline );
					if ( nUseBackQ )
						goto GetFileTokens;
					szSource[0] = _TEXT('\0');
					goto GetStringTokens;
				} else
					break;

			} else {
				// copy from "arg" so we can get the entire
				//   argument if > MAXFILENAME
				strcpy( gszCmdline, pszArg );
				szSource[0] = _TEXT('\0');
			}

			if (( fForFlags & FOR_FILESET ) == 0 ) {
				// create new environment variables to expand
				strins( gszCmdline, pszForVar );
				add_variable( gszCmdline );
			}

			if ( fForFlags & FOR_STEP ) {
				if (( nStep >= 0 ) ? ( nStart > nEnd ) : ( nStart < nEnd ))
					goto for_bye;
				nStart += nStep;
			}

			// execute the command
			nReturn = command( pszLine, 2 );
		}

		// delete temporary FILESET file ('command')
		if ( fDelete ) {
			remove( szSource );
			fDelete = 0;
		}
	}

for_bye:
	if ( hFH > 0 ) {
		_close( hFH );
		hFH = -1;
	}

	// delete the FOR variable(s)
	add_variable( pszForVar );
	for ( i = 0; ( szVariables[i] != _TEXT('\0') ); i++ ) {
		pszForVar[1] = szVariables[i];
		add_variable( pszForVar );
	}

	return nReturn;
}


// Call subroutine (GOSUB labelname )
int _near Gosub_Cmd( LPTSTR pszCmdLine )
{
	LONG lSaveFlags;
	unsigned int uSavedLine;
	int nRet, nSavedReturn;
	LPTSTR pszSavedArgs;
	LONG lOffset;

	// save current position to RETURN to
	lOffset = bframe[ cv.bn ].lOffset;
	uSavedLine = bframe[ cv.bn ].uBatchLine;
	pszSavedArgs = bframe[ cv.bn ].pszGosubArgs;
	bframe[ cv.bn ].pszGosubArgs = NULL;

	// call the subroutine
	if (( nRet = goto_label( pszCmdLine, 3 )) == 0 ) {

		// save the DO / IFF flags state
		lSaveFlags = cv.f.lFlags;

		// save the RETURN value (for nested GOSUBs)
		nSavedReturn = bframe[ cv.bn ].nGosubReturn;

		// clear the DO / IFF flags
		cv.f.lFlags = 0L;

		bframe[cv.bn].nGosubOffset++;
		if (( BatchCLI() == CTRLC ) || ( cv.fException ))
			bframe[ cv.bn ].lOffset = -1L;

		else if ( bframe[ cv.bn ].lOffset >= 0L ) {

			// restore the saved file read offset
			bframe[cv.bn].lOffset = lOffset;
			bframe[cv.bn].uBatchLine = uSavedLine;

			// restore the DO / IFF flags
			cv.f.lFlags = lSaveFlags;
		}

		if ( bframe[ cv.bn ].pszGosubArgs ) {
			UndoGosubVariables();
			bframe[ cv.bn ].uGosubStack--;
			free( bframe[ cv.bn ].pszGosubArgs );
		}

		bframe[ cv.bn ].pszGosubArgs = pszSavedArgs;

		// RETURN may have set an errorlevel
		nRet = gnInternalErrorLevel = bframe[ cv.bn ].nGosubReturn;
		bframe[ cv.bn ].nGosubReturn = nSavedReturn;
	}

	return nRet;
}


// GOTO a label in the batch file
int _near Goto_Cmd( LPTSTR pszCmdLine )
{
	int nReturn = ABORT_LINE;
	long fGoto;

	// check if they want to stay within an IFF or DO
	GetSwitches( pszCmdLine, _TEXT("I"), &fGoto, 1 );

	// position file pointer & force bad return to abort multiple cmds
	if (( goto_label( pszCmdLine, 1 ) == 0 ) && ( fGoto != 1 )) {
		// turn off DO & IFF flag when you do a GOTO unless a /I
		//   was specified
		if ( cv.f.flag._do )
			nReturn = BATCH_RETURN;
		cv.f.lFlags = 0L;
	}

	return nReturn;
}


// remove the Gosub local variables
static void _near UndoGosubVariables( void )
{
	register int i;
	register LPTSTR pszArg;

	for ( i = 0; (( pszArg = ntharg( bframe[ cv.bn ].pszGosubArgs, i )) != NULL ); i ++ ) {

		TCHAR szBuf[2];

		szBuf[0] = (TCHAR)(bframe[ cv.bn ].uGosubStack + 1);
		szBuf[1] = _TEXT('\0');
		strins( pszArg, szBuf );	
		add_variable( pszArg );
	}
}


#pragma alloc_text( MISC_TEXT, goto_label )

// position the file pointer following the label line
static int _fastcall goto_label( register LPTSTR pszCmdLine, int fFlags )
{
	static TCHAR GOTO_DELIMS[] = _TEXT("+=:[/");
	register TCHAR *pszArg, *pszArguments = NULL;
	TCHAR szLabel[MAXARGSIZ], szLine[CMDBUFSIZ];
	int i;

	// GOSUB / GOTO must have an argument!  Look for first whitespace (not switch!) delimited arg
	if (( pszArg = ntharg( pszCmdLine, 0x800 )) == NULL )
		return USAGE_ERR;

	// make sure the file is still open
	if ( open_batch_file() == 0 )
		return BATCH_RETURN;

	// rewind file for search
	RewindFile( bframe[ cv.bn ].bfd );
	bframe[ cv.bn ].lOffset = 0L;
	bframe[ cv.bn ].uBatchLine = 0;

	// skip a leading @
	if ( *pszArg == _TEXT('@') )
		pszArg++;

	// skip a leading ':' in label name
	if ( *pszArg == _TEXT(':') )
		pszArg++;

	// remove a trailing :, =, and/or +
	strip_trailing( pszArg, GOTO_DELIMS );
	strcpy( szLabel, pszArg );


	// scan for the label
	while ( getline( bframe[cv.bn].bfd, szLine, CMDBUFSIZ-1, EDIT_DATA ) > 0 ) {

		bframe[cv.bn].uBatchLine++;

		// strip leading white space
		pszArg = skipspace( szLine );

		if (( *pszArg++ == _TEXT(':') ) && ( *pszArg != _TEXT(':') )) {

			// get first (whitespace delimited) argument
			if (( pszArg = ntharg( pszArg, 0x800 )) == NULL )
				continue;

			if ( stricmp( szLabel, pszArg ) == 0 ) {

				if ( fFlags & 2 ) {

					// it's a GOSUB - check for args
					if (( pszArguments = ntharg( szLine, 0x2001 )) != NULL ) {

						pszArguments = skipspace( pszArguments + 1 );
						strip_trailing( pszArguments, _TEXT(" \t]") );
						if ( *pszArguments ) {

							// we have to limit the GOSUBs with parameters, so we don't
							//   end up with valid variable name characters!
							if ( bframe[ cv.bn ].uGosubStack > 22 )
								return ( error( ERROR_4DOS_EXCEEDED_NEST, NULL ));

							bframe[ cv.bn ].pszGosubArgs = strdup( pszArguments );
							bframe[ cv.bn ].uGosubStack++;

							// add arguments to environment
							for ( i = 0; (( pszArg = ntharg( bframe[ cv.bn ].pszGosubArgs, i )) != NULL ); i ++ ) {
								strcpy( szLabel, pszArg );
								if (( pszArg = ntharg( pszCmdLine, i+1 )) == NULL )
									pszArg = NULLSTR;
								sprintf( szLine, _TEXT("%c%s=%s"), bframe[ cv.bn ].uGosubStack+1, szLabel, pszArg );
								add_variable( szLine );
							}
						}
					}
				}

				return 0;

			} else
				pszArguments = NULL;
		}
	}

	// (for NT) - if label is :EOF, and no matching label is found,
	//   goto end & exit
	if ( stricmp( szLabel, _TEXT("EOF") ) == 0 )
		return 0;

	return (( fFlags & 1 ) ? error( ERROR_4DOS_LABEL_NOT_FOUND, szLabel ) : ERROR_EXIT );
}


// conditional IF command
int _near If_Cmd( LPTSTR pszCmdLine )
{
	return ( __if( pszCmdLine, 0 ));
}


// conditional IFF command
int _near Iff_Cmd( LPTSTR pszCmdLine )
{
	return ( __if( pszCmdLine, 1 ));
}


// conditional IF command (including ELSE/ELSEIFF)
static int _fastcall __if( LPTSTR pszCmdLine, int fIff )
{
	LPTSTR pszLine, pszArg;
	int nCondition;

	if ( ntharg( pszCmdLine, 0x8001 ) == NULL ) {
if_usage:
		return ( Usage( fIff ? IFF_USAGE : IF_USAGE ));
	}

	pszLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );

	// test condition & strip condition text from line
	if ((( nCondition = TestCondition( pszLine, 1 )) == -USAGE_ERR ) || ( *pszLine == _TEXT('\0') ))
		goto if_usage;

	// IFF / THEN / ELSE support
	if (( fIff ) && ( _stricmp( THEN, first_arg( pszLine )) == 0 )) {

		// increment IFF nesting flag
		cv.f.flag._iff++;

		// if condition is false, flag as "waiting for ELSE"
		if ( nCondition == 0 )
			cv.f.flag._else++;

		// skip THEN & see if there's another argument
		next_arg( pszLine, 1 );
		if ( *pszLine == _TEXT('\0') )
			return 0x666;
	}

	// support (undocumented) CMD.EXE "IF condition (...) ELSE (...)" syntax
	if ( *pszLine == _TEXT('(') ) {

		pszArg = scan( pszLine, _TEXT(")"), QUOTES_PARENS );

		if ((( pszArg = first_arg( pszArg + 1 )) != NULL ) && ( _stricmp( pszArg, _TEXT("ELSE") ) == 0 )) {

			// if first case is true, squash the ELSE
			//   otherwise, set condition & remove the first case
			if ( nCondition )
				*gpNthptr = _TEXT('\0');
			else
				strcpy( pszLine, gpNthptr + 5 );

			// force execution of the command
			nCondition = 1;
		}
	}

	// if condition is true, execute the specified command
	if ( nCondition )
		return ( command( pszLine, 2 ));

	// return non-zero for conditional tests (&& / ||)
	return (( fIff ) ? 0x666 : ERROR_EXIT );
}


#pragma alloc_text( MISC_TEXT, ExpandArgument )

// expand the argument
static LPTSTR _fastcall ExpandArgument( LPTSTR pszLine, int nArgument )
{
	TCHAR szBuffer[CMDBUFSIZ];

	fNoComma = 1;
	pszLine = ntharg( pszLine, nArgument | 0x6000 );
	fNoComma = 0;

	if ( pszLine == NULL )
		return NULL;

	// do variable expansion
	strcpy( szBuffer, pszLine );
	if ( var_expand( szBuffer, 1 ) != 0 )
		return NULL;

	// strip escape characters
	EscapeLine( szBuffer );

	// put argument into "pszLine" (static buffer in ntharg())
	strcpy( pszLine, skipspace( szBuffer ) );
	strip_trailing( pszLine, WHITESPACE );

	return pszLine;
}


#pragma alloc_text( MISC_TEXT, TestCondition )

// DO / IF / IFF testing
int _fastcall TestCondition( LPTSTR pszLine, int fIf )
{
	register int nCondition;
	register LPTSTR pszArg;
	LPTSTR pszArg2, pszTest;
	int nArgument = 0, fOR = 0, fAND = 0, fXOR = 0xFF;
	int fNOT;
	TCHAR szBuf[128];

	// KLUDGE for compatibility with things like "if %1/==/ ..."
	//   (DO & IF[F] don't have any switches anyway)
	gpIniptr->SwChr = 0x7F;

	for ( ; ; ) {

		// (re )set result condition
		nCondition = fNOT = 0;

		if (( pszArg = ExpandArgument( pszLine, nArgument )) != NULL ) {

			// check for NOT condition
			if ( _stricmp( pszArg, IF_NOT ) == 0 ) {
				fNOT = 1;
				nArgument++;
				pszArg = ExpandArgument( pszLine, nArgument );
			}
		}

		nArgument++;
		if ( pszArg == NULL )
			return -USAGE_ERR;

		// if ( "a"=="%a" .and. "%b"=="b" ) .or. .....
		if ( *pszArg == _TEXT('(') ) {
			if (( stristr( pszArg, _TEXT(" .or. ") ) != NULL ) || ( stristr( pszArg, _TEXT(" .and. ") ) != NULL ) || (stristr( pszArg, _TEXT(" .xor. ") ) != NULL )) {
				// strip leading & trailing ( )
				pszArg2 = strlast( pszArg );
				if ( *pszArg2 == _TEXT(')') )
					*pszArg2 = _TEXT('\0');
				strcpy( pszArg, pszArg + 1 );
				pszTest = (LPTSTR)_alloca( ( strlen( pszArg ) + 1 ) * sizeof(TCHAR) );
				pszArg = strcpy( pszTest, pszArg );
				nCondition = TestCondition( pszArg, 0 );
				goto Conditionals;
			}
		}

		pszTest = (LPTSTR)_alloca( ( strlen( pszArg ) + 1 ) * sizeof(TCHAR) );
		pszArg = strcpy( pszTest, pszArg );
		if ((( pszArg2 = ExpandArgument( pszLine, nArgument )) == NULL ) && ( fIf & 1 ))
			return -USAGE_ERR;

		if ( pszArg2 == NULL )
			pszArg2 = NULLSTR;

		nArgument++;

		if ( _stricmp( pszArg, IF_EXIST ) == 0 ) {

			// check for existence of specified file
			nCondition = is_file( pszArg2 );

		} else if ( _stricmp( pszArg, IF_ISINTERNAL ) == 0 ) {

			// check for existence of specified internal command
			nCondition = ( findcmd( pszArg2, 0 ) >= 0 );

		} else if ( _stricmp( pszArg, IF_ISALIAS ) == 0 ) {

			// check for existence of specified alias
            nCondition = ( get_alias( pszArg2 ) != 0L );

		} else if (( _stricmp( pszArg, IF_ISDIR ) == 0 ) || ( _stricmp( pszArg, IF_DIREXIST ) == 0 )) {

			// check ready state & existence of specified directory
			//   ("if direxist" is for DR-DOS compatibility)
			nCondition = is_dir( pszArg2 );

		} else if ( _stricmp( pszArg, IF_ISFUNCTION ) == 0 ) {

			// check existence of user-defined function
			extern TCHAR _far * glpFunctionList;

            nCondition = ( get_list( pszArg2, glpFunctionList ) != 0L );

		} else if ( _stricmp( pszArg, IF_ISLABEL ) == 0 ) {

			// check existence of specified batch label
			if ( cv.bn >= 0 ) {

				long lSaveOffset;
				int uSavedLine;

				lSaveOffset = bframe[cv.bn].lOffset;
				uSavedLine = bframe[cv.bn].uBatchLine;
				pszTest = (LPTSTR)_alloca( ( strlen( pszArg2 ) + 1 ) * sizeof(TCHAR) );
				pszArg2 = strcpy( pszTest, pszArg2 );
				nCondition = ( goto_label( pszArg2, 0 ) == 0 );
				bframe[cv.bn].uBatchLine = uSavedLine;
				bframe[cv.bn].lOffset = lSaveOffset;

				// reset file pointer
				if ( bframe[cv.bn].bfd != IN_MEMORY_FILE )
					_lseek( bframe[cv.bn].bfd, lSaveOffset, SEEK_SET );
			}

		} else if ( _stricmp( pszArg, IF_DEFINED ) == 0 ) {

			// check for existence of specified variable
			nCondition = ( get_variable( pszArg2 ) != 0L );

		} else {

			// check for a "=" or "==" in the middle of the first argument
			if (( pszTest = scan( pszArg, _TEXT("="), QUOTES )) == BADQUOTES )
				return -USAGE_ERR;

			if (( *pszArg2 == _TEXT('=') ) || ( *pszTest == _TEXT('\0') )) {

				// DO test with no argument ("DO WHILE a")
				if ( *pszArg2 == _TEXT('\0') )
					return -USAGE_ERR;

				// kludge for STUPID people who do "if "%1==a" ..."
				if (( fIf & 1 ) && ( *pszArg == _TEXT('"') ) && ( *pszArg2 != _TEXT('=') ) && ( pszTest[-1] == _TEXT('"') ) && (( pszTest = strstr( pszArg, _TEXT("==") )) != NULL )) {

					pszArg++;
					strip_trailing( pszArg, _TEXT("\"") );
					pszArg2 = NULLSTR;

				} else {

					// pszArg2 must be test type; save it to pszTest
					pszTest = (LPTSTR)_alloca( ( strlen( pszArg2 ) + 1 ) * sizeof(TCHAR) );
					strcpy( pszTest, pszArg2 );
					pszArg2 = NULL;
				}
			}

			if ( *pszTest == _TEXT('=') ) {

				// change a "==" to a "EQ"
				while ( *pszTest == _TEXT('=') )
					*pszTest++ = _TEXT('\0');

				if (( *pszTest ) || ( pszArg2 == NULLSTR )) {
					if ( pszArg2 != NULL )
						nArgument--;
					pszArg2 = pszTest;
				}
				pszTest = EQ;
			}

			// we have to support various ERRORLEVEL constructs:
			//   if ERRORLEVEL 5 ; if ERRORLEVEL == 5 ; if ERRORLEVEL LT 5
			if (( _stricmp( pszArg, IF_ERRORLEVEL ) == 0 ) && ( is_signed_digit(*pszTest))) {

				pszArg2 = pszTest;
				pszTest = GE;

			} else if ( pszArg2 == NULL ) {

				// have we got the second argument yet?
				if (( pszArg2 = ExpandArgument( pszLine, nArgument )) == NULL )
					return -USAGE_ERR;
				nArgument++;
			}

			// check error return of previous command or do string match
			if ( _stricmp( pszArg, IF_ERRORLEVEL ) == 0 ) {

				// kludge for nitwits who do things like
				//   "if errorlevel -1 ..."
				nCondition = ( gnErrorLevel - (unsigned int)(atoi( pszArg2 ) & 0xFF ));

			} else {

				// do ASCII/numeric comparison
				if (( QueryIsNumeric( pszArg ) != 0 ) && ( QueryIsNumeric( pszArg2 ) != 0 )) {

					// allow decimal comparisons
					sprintf( szBuf, _TEXT("%s-%s=0.10"), pszArg, pszArg2 );
					evaluate( szBuf );

					if ( szBuf[0] == _TEXT('-') )
						nCondition = -1;
					else if ( stricmp( szBuf, _TEXT("0") ) != 0 )
						nCondition = 1;

				} else
					nCondition = fstrcmp( pszArg, pszArg2, 1 );
			}

			// modify result based on equality test
			if ( _stricmp( pszTest, EQ ) == 0 )
				nCondition = ( nCondition == 0 );
			else if ( _stricmp( pszTest, GT ) == 0 )
				nCondition = ( nCondition > 0 );
			else if ( _stricmp( pszTest, GE ) == 0 )
				nCondition = ( nCondition >= 0 );
			else if ( _stricmp( pszTest, LT ) == 0 )
				nCondition = ( nCondition < 0 );
			else if ( _stricmp( pszTest, LE ) == 0 )
				nCondition = ( nCondition <= 0 );

			else if (( _stricmp( pszTest, NE ) == 0 ) || ( _stricmp( pszTest, _TEXT("!=") ) == 0 ))
				nCondition = ( nCondition != 0 );
			else
				return -USAGE_ERR;
		}

Conditionals:
		nCondition ^= fNOT;

		// if fOR != 0, a successful OR test is in progress
		//   force condition to TRUE
		if ( fOR ) {
			nCondition = 1;
			fOR = 0;
		}

		// if fXOR != 0xFF, then an XOR test is in progress
		if ( fXOR != 0xFF ) {
			nCondition = ( nCondition ^ fXOR );
			fXOR = 0xFF;
		}

		// if fAND != 0, an unsuccessful AND test is in progress
		//   force condition to FALSE
		if ( fAND ) {
			nCondition = 0;
			fAND = 1;
		}

		// check for conditionals
		fNoComma = 1;
		pszArg = ntharg( pszLine, nArgument | 0x6000 );
		fNoComma = 0;

		if ( pszArg == NULL )
			return (( fIf & 1 ) ? -USAGE_ERR : nCondition );

		// conditional OR
		if ( _stricmp( IF_OR, pszArg ) == 0 ) {

			// We need to keep parsing the line, even if the last test was
			//   true (for IFF).  Save the condition in fOR.
			fOR = nCondition;
			fAND = fXOR = 0;
			nArgument++;

			// exclusive OR
		} else if ( _stricmp( IF_XOR, pszArg ) == 0 ) {

			// Continue parsing the line to get the second test value
			fXOR = nCondition;
			fAND = fOR = 0;
			nArgument++;

			// conditional AND
		} else if ( _stricmp( IF_AND, pszArg ) == 0 ) {

			// We need to keep parsing the line, even if the last test was
			//   false (for IFF).  Save the inverse condition in fAND.
			fAND = ( nCondition == 0 );
			fOR = fXOR = 0;
			nArgument++;

		} else {

			// remove the conditional test(s) & return the result
			if ( fIf & 1 ) {
				fNoComma = 1;
				next_arg( pszLine, nArgument | 0x6000 );
				fNoComma = 0;
			}

			return nCondition;
		}
	}
}


#pragma alloc_text( MISC_TEXT, iff_parsing )

// check for IFF THEN / ELSE / ENDIFF condition
//   (if waiting for ENDIFF or ELSE, ignore the command)
int _fastcall iff_parsing( LPTSTR pszCmdName, LPTSTR pszLine )
{
	// extract the command name (first argument)
	// do it first so we don't have to duplicate it in do_parsing()
	DELIMS[4] = gpIniptr->CmdSep;
	DELIMS[5] = gpIniptr->SwChr;
	sscanf( pszCmdName, DELIMS, pszCmdName );

	// if not parsing an IFF just return
	// if waiting on a DO/END then don't test for IFF
	if (( cv.f.flag._iff == 0 ) || ( cv.f.flag._do_end | cv.f.flag._do_leave ))
		return 0;

	// terminated IFF tests?
	if ( _stricmp( pszCmdName, ENDIFF ) == 0 ) {

		// drop one ENDIFF wait
		if ( cv.f.flag._endiff == 0 ) {
			cv.f.flag._iff--;
			cv.f.flag._else = 0;
		} else
			cv.f.flag._endiff--;

		return 1;

	} else if ( _stricmp( pszCmdName, IFF ) == 0 ) {

		// if "waiting for ELSE" or "waiting for ENDIFF" flags set,
		//   ignore nested IFFs
		if ( cv.f.flag._else )
			cv.f.flag._endiff++;

	} else if (( cv.f.flag._endiff == 0 ) && (( _stricmp( pszCmdName,ELSE) == 0 ) || ( _stricmp( pszCmdName,ELSEIFF ) == 0 ))) {

		// are we waiting for an ELSE[IFF]?
		if ( cv.f.flag._else & 0x7F ) {

			// remove an ELSE or make an ELSEIFF look like an IFF
			cv.f.flag._else = 0;
			strcpy( pszLine, pszLine+4 );

			// an ELSEIFF is an implicit ENDIFF
			if ( _stricmp( pszCmdName, ELSEIFF ) == 0 )
				cv.f.flag._iff--;

			// if an ELSE doesn't have a ^, add one
			else if (( *(pszLine = skipspace( pszLine )) != gpIniptr->CmdSep) && (*pszLine != _TEXT('\0') )) {
				DELIMS[5] = _TEXT('\0');
				strins( pszLine, DELIMS+4 );
			}

		} else
			cv.f.flag._else = 0x80;	// wait for an ENDIFF
	}

	// waiting for ELSE[IFF] or ENDIFF?
	return ( cv.f.flag._else | cv.f.flag._endiff);
}


#define INKEY_CLEAR 1
#define INKEY_KEYMASK 2
#define INKEY_PASSWORD 4
#define INKEY_WAIT 8
#define INPUT_EDIT 0x10
#define INPUT_NO_CRLF 0x20
#define INPUT_NO_COLOR 0x40
#define INPUT_LENGTH 0x80
#define INPUT_DIGITS 0x100
#define INKEY_MOUSE 0x200


// assign a single-key value to a shell variable (INKEY)
// assign a string to a shell variable (INPUT)
int _near Inkey_Cmd( LPTSTR pszCmdLine )
{
	return ( __input( pszCmdLine, 0 ));
}


// assign a string to a shell variable (INPUT)
int _near Input_Cmd( LPTSTR pszCmdLine )
{
	return ( __input( pszCmdLine, 1 ));
}


static int _fastcall __input( LPTSTR pszCmdLine, int fInput )
{
	register LPTSTR pszVariable;
	TCHAR *pszArg, *pszFirstArg, szKeylist[MAXARGSIZ+1];
	TCHAR _far *lpszValue;
	LONG lDelay = -1L;
	int i, c, nLength, nRow, nColumn, fPrompt, nEditFlag = EDIT_DATA | EDIT_SCROLL_CONSOLE;
	int nMaxLength = MAXARGSIZ;
	long lFlags;

	// get the last argument & remove it from the command line
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {
Input_Bad:
		return ( Usage((( fInput ) ? INPUT_USAGE : INKEY_USAGE )));
	}

	if ( last_arg( pszCmdLine, &i ) == NULL )
		pszVariable = skipspace( pszCmdLine );
	else
		pszVariable = gpNthptr;

	if (( pszVariable == NULL ) || ( *pszVariable != _TEXT('%') ))
		goto Input_Bad;

	// strip single trailing space
	if ( pszVariable[-1] == _TEXT(' ') )
		pszVariable[-1] = _TEXT('\0');

	*pszVariable++ = _TEXT('\0');
	pszVariable = strcpy( (LPTSTR)_alloca( ( strlen( pszVariable ) +1 ) * sizeof(TCHAR) ), pszVariable );

	szKeylist[0] = _TEXT('\0');

	// check for optional input mask & timeout (& max length for INPUT)
	pszArg = pszCmdLine;
	for ( c = 0; ; ) {

		pszArg = skipspace( pszArg );
		if (( pszArg == NULL ) || ( *pszArg != gpIniptr->SwChr ))
			break;

		while ( *pszArg == gpIniptr->SwChr ) {

			pszFirstArg = first_arg( pszArg );
			if (( lFlags = switch_arg( pszFirstArg, (( fInput ) ? _TEXT("C PWEXNLD") : _TEXT("CKPW X  DM")))) == 0 )
				break;

			if ( lFlags == -1L )
				goto Input_Bad;

			// flush the typeahead buffer?
			if ( lFlags & INKEY_CLEAR ) {
				while ( _kbhit() )
					GetKeystroke( EDIT_NO_ECHO );
			}

			if ( lFlags & INKEY_MOUSE ) {
				nEditFlag |= EDIT_MOUSE_BUTTON;

				MouseReset();
			}

			if ( lFlags & INKEY_PASSWORD )
				nEditFlag |= EDIT_PASSWORD;

			if ( lFlags & INPUT_EDIT )
				nEditFlag |= EDIT_ECHO;

			// save the key mask set onto the stack
			if ( lFlags & INKEY_KEYMASK )
				sprintf( szKeylist, FMT_PREC_STR, MAXARGSIZ, pszFirstArg+2 );
			else if ( lFlags & INPUT_LENGTH ) {	// INPUT length
				sscanf( pszArg+2, FMT_UINT, &nMaxLength );

				if ( gpIniptr->LineIn )
					nMaxLength++;

			} else if ( lFlags & INKEY_WAIT )	// timeout delay
				sscanf( pszArg+2, FMT_ULONG, &lDelay );
			else {
				if ( lFlags & INPUT_NO_CRLF )
					nEditFlag |= EDIT_NO_CRLF;
				if ( lFlags & INPUT_NO_COLOR )
					nEditFlag |= EDIT_NO_INPUTCOLOR;
				if ( lFlags & INPUT_DIGITS )
					nEditFlag |= EDIT_DIGITS;
			}

			// skip switch
			strcpy( pszArg, skipspace( pszArg + strlen( pszFirstArg )));
		}
	}

	// print the (optional) prompt string
	if (( pszCmdLine != NULL ) && ( *pszCmdLine )) {
		fPrompt = 1;
		qputs( pszCmdLine );
	} else
		fPrompt = 0;

	nLength = sprintf( gszCmdline, _TEXT("%.80s="), pszVariable );

	// allow editing an existing value?
	if (( fInput ) && ( nEditFlag & EDIT_ECHO )) {
		if (( lpszValue = get_variable( pszVariable )) != 0L )
			sprintf( gszCmdline + nLength, FMT_FAR_PREC_STR, MAXARGSIZ, lpszValue );
		// save cursor position, display value, and restore cursor
		GetCurPos( &nRow, &nColumn );
		if (( i = gpIniptr->InputColor ) == 0 )
			i = -1;
		color_printf( i, FMT_STR, gszCmdline + nLength );
		SetCurPos( nRow, nColumn );
	}

	// wait the (optionally) specified length of time
	if ( lDelay >= 0L ) {

		lDelay *= 18;

		for ( ; ; lDelay-- ) {

continue_wait:
			// is a key waiting?
			if ( _kbhit() )
				break;

			if ( lDelay <= 0L ) {
				// if we displayed a prompt, print a CR/LF
				if (( nEditFlag & EDIT_ECHO ) || ( fPrompt ))
					crlf();
				return 0;
			}

			SysBeep( 0, 1 );	// wait 1/18th second
		}
	}

	// get the variable
	if ( fInput )
		getline( STDIN, gszCmdline + nLength, nMaxLength, nEditFlag );
	else {

		for ( ; ; ) {

			// if EOF, we're redirecting & ran out of input
			if (( c = GetKeystroke( nEditFlag | EDIT_NO_ECHO )) == EOF )
				return ERROR_EXIT;

			// mouse button?
			if  ( c == LEFT_MOUSE_BUTTON ) {
				c = 0xF0;
				break;
			} else if ( c == RIGHT_MOUSE_BUTTON ) {
				c = 0x1F1;
				break;
			} else if ( c == MIDDLE_MOUSE_BUTTON ) {
				c = 0x1F2;
				break;
			}

			// if printable character, display it
			if ((( nEditFlag & EDIT_PASSWORD ) == 0 ) && ( c >= 32 ) && ( c < FBIT ))
				qputc( STDOUT, (TCHAR)c );

			if ( nEditFlag & EDIT_DIGITS ) {
				if ( isdigit( c ))
					break;
			} else if ( szKeylist[0] == _TEXT('\0') )
				break;

			// check the key against the optional mask
			for ( pszArg = szKeylist+1; (( *pszArg != _TEXT('"') ) && ( *pszArg != _TEXT('\0') )); pszArg++ ) {

				// check for extended keys
				if ( *pszArg == _TEXT('[') ) {

					// can't use sscanf() because of []'s
					pszArg++;
					i = (int)( scan( pszArg, _TEXT("]\""), QUOTES ) - pszArg );

					if ( c == keyparse( pszArg, i ))
						goto good_key;

					pszArg += i;

				} else if ( _ctoupper( c ) == _ctoupper( *pszArg ))
					goto good_key;
			}

			// if printable character, back up over it
			if ((( nEditFlag & EDIT_PASSWORD ) == 0 ) && ( c >= 32 ) && ( c < FBIT ))
				qputc( STDOUT, BS );

			honk();

			if ( lDelay > 0L )
				goto continue_wait;
		}
good_key:
		// kludge for ENTER - set to @28
		if ( c == 13 )
			c = FBIT + 28;

		sprintf( gszCmdline + nLength, (( c < FBIT ) ? FMT_CHAR : _TEXT("@%u")), ( c & ~FBIT ));

		if (( nEditFlag & EDIT_NO_CRLF ) == 0 )
			crlf();
	}

	return ( add_variable( gszCmdline ));
}


// enable/disable disk verify
int _near LfnFor_Cmd( LPTSTR pszCmdLine )
{
	register int nLFNFor;

	// inquiring about LFNFOR status
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		printf( LFNFOR_IS, cv.fLfnFor ? ON : OFF );

	else {		// setting new LFNFOR status

		if (( nLFNFor = OffOn( pszCmdLine )) == -1 )
			return ( Usage( LFNFOR_USAGE ));

		cv.fLfnFor = nLFNFor;
	}

	return 0;
}


// switch a file between .BAT and .BTM types
int _near Loadbtm_Cmd( LPTSTR pszCmdLine )
{
	register int nLoad;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {

		// just inquiring about LOADBTM state
		printf( LOADBTM_IS, ( bframe[cv.bn].bfd == IN_MEMORY_FILE ) ? ON : OFF );

	} else if (( nLoad = OffOn( pszCmdLine )) == 1 ) {

		// switch batch file to .BTM mode
		if ( bframe[cv.bn].bfd != IN_MEMORY_FILE )
			return (LoadBTM());

	} else if ( nLoad == 0 ) {

		// switch batch file to .BAT mode (if not compressed)
		if (( bframe[cv.bn].bfd == IN_MEMORY_FILE ) && (( bframe[cv.bn].nFlags & BATCH_COMPRESSED) == 0 )) {

			// turn off the IN_MEMORY_FILE flag
			bframe[cv.bn].bfd = -1;

			// free the internal .BTM buffer
			FreeMem( bframe[cv.bn].lpszBTMBuffer );
			bframe[cv.bn].lpszBTMBuffer = NULL;
		}

	} else
		return ( Usage( LOADBTM_USAGE ));

	return 0;
}


// ON [BREAK | ERROR | ERRORMSG | ...] condition
int _near On_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszToken;

	if (( pszToken = first_arg( pszCmdLine )) != NULL ) {

		if ( _stricmp( pszToken, ON_BREAK ) == 0 ) {

			if ( bframe[cv.bn].pszOnBreak != NULL )
				free( bframe[cv.bn].pszOnBreak );
			next_arg( pszCmdLine, 1 );
			bframe[cv.bn].pszOnBreak = (( *pszCmdLine == _TEXT('\0') ) ? NULL : _strdup( pszCmdLine ));
			return 0;

		} else if ( _stricmp( pszToken, ON_ERROR ) == 0 ) {

			// clear previous condition
			if ( bframe[cv.bn].pszOnError != NULL ) {

				free( bframe[cv.bn].pszOnError );

				// reset previous error popup state
				bframe[ cv.bn ].uOnErrorState = ServCtrl( SERV_AF, bframe[cv.bn].uOnErrorState );
			}

			// set OnError condition & disable error popups
			next_arg( pszCmdLine, 1 );
			if (( bframe[ cv.bn ].pszOnError = (( *pszCmdLine == _TEXT('\0') ) ? NULL : _strdup( pszCmdLine ))) != NULL ) {
				bframe[ cv.bn ].uOnErrorState = ServCtrl( SERV_AF, 1 );
			}

			return 0;

		} else if ( _stricmp( pszToken, ON_ERRORMSG ) == 0 ) {

			// clear previous condition
			if ( bframe[ cv.bn ].pszOnErrorMsg != NULL )
				free( bframe[ cv.bn ].pszOnErrorMsg );

			// set OnErrorMsg condition
			next_arg( pszCmdLine, 1 );
			bframe[ cv.bn ].pszOnErrorMsg = (( *pszCmdLine == _TEXT('\0') ) ? NULL : _strdup( pszCmdLine ));
			return 0;
		}
	}

	return ( Usage( ON_USAGE ) );
}


// wait for a keystroke; display optional message
int _near Pause_Cmd( LPTSTR pszCmdLine )
{
	// make sure STDOUT hasn't been redirected
	qprintf( ( _isatty( STDOUT ) ? STDOUT : STDERR ), FMT_STR, (( pszCmdLine == NULL ) ? PAUSE_PROMPT : pszCmdLine ));
 
	GetKeystroke( EDIT_NO_ECHO | EDIT_ECHO_CRLF | EDIT_SCROLL_CONSOLE );
	return 0;
}


// cancel nested batch files
int _near Cancel_Cmd( register LPTSTR pszCmdLine )
{
	return ( __quit( pszCmdLine, 1 ));
}


// exit batch file
//   NOTE: you can also use this command to set the ERRORLEVEL (even from
//   the command line!)
int _near Quit_Cmd( register LPTSTR pszCmdLine )
{
	return ( __quit( pszCmdLine, 0 ));
}


static int _fastcall __quit( register LPTSTR pszCmdLine, BOOL bCancel )
{
	register int nBatch;

	if ( cv.bn >= 0 ) {

		// QUIT a batch file or CANCEL nested batch file sequence
		//   by positioning the file pointer in each file to EOF
		for ( nBatch = (( bCancel ) ? 0 : cv.bn ); ( nBatch <= cv.bn ); nBatch++ ) {
			UndoGosubVariables();
			bframe[ nBatch ].lOffset = -1L;
		}
	}

	// set ERRORLEVEL
	if (( pszCmdLine ) && ( *pszCmdLine )) {
		gnErrorLevel = atoi( pszCmdLine );
		if ( cv.bn >= 0 )
			bframe[cv.bn].nReturn = gnErrorLevel;
	}

	// Abort the remainder of the batch file (or alias)
	return ABORT_LINE;
}


// REM - do nothing
int _near Remark_Cmd( LPTSTR pszCmdLine )
{
	return 0;
}


// RETURN from a GOSUB
int _near Ret_Cmd( LPTSTR pszCmdLine )
{
	// check for a previous GOSUB
	if ( bframe[cv.bn].nGosubOffset <= 0 )
		return ( error( ERROR_4DOS_BAD_RETURN, NULL ));

	bframe[cv.bn].nGosubOffset--;

	// set (optional) return value
	bframe[ cv.bn ].nGosubReturn = ((( pszCmdLine ) && ( *pszCmdLine )) ? atoi( pszCmdLine ) : 0 );
	gnInternalErrorLevel = bframe[ cv.bn ].nGosubReturn;

	// return a "return from nested BatchCLI()" code
	return BATCH_RETURN_RETURN;
}


// set cursor position
int _near Scr_Cmd( LPTSTR pszCmdLine )
{
	int nRow, nColumn;
	LPTSTR pszText;

	// make sure row & column are displayable on the current screen
	if (( pszCmdLine == NULL ) || ( GetCursorRange( pszCmdLine, &nRow, &nColumn ) != 0 ))
		return ( Usage( SCREEN_USAGE ));

	if (( pszText = ntharg( pszCmdLine, 1 )) != NULL ) {

		pszText = gpNthptr + strlen( pszText );

		if ( *pszText )
			pszText++;

		// center the text?
		if ( nColumn == 999 ) {
			if (( nColumn = (( GetScrCols() - strlen( pszText )) / 2 )) < 0 )
				nColumn = 0;
		}

		if ( nRow == 999 ) {
			if (( nRow = ( GetScrRows() / 2 )) < 0 )
				nRow = 0;
		}
	}

	// move the cursor
	SetCurPos( nRow, nColumn );

	// if the user specified a string, display it
	if ( pszText != NULL )
		qputs( pszText );

	return 0;
}


// set screen position and print a string with the specified attribute
//   directly to the screen (vertically)
int _near VScrput_Cmd( LPTSTR pszCmdLine )
{
	return ( __scrput( pszCmdLine, 1 ));
}


// set screen position and print a string with the specified attribute
//   directly to the screen (vertically)
int _near Scrput_Cmd( LPTSTR pszCmdLine )
{
	return ( __scrput( pszCmdLine, 0 ));
}


// set screen position and print a string with the specified attribute
//   directly to the screen (either horizontally or vertically)
static int _fastcall __scrput( LPTSTR pszCmdLine, int fVscrput )
{
	LPTSTR pszText;
	int nAttribute = -1;
	int nRow = 0, nColumn = 0;

	// make sure row & column are displayable on the current screen
	ntharg( pszCmdLine, 0x8002 );
	pszText = gpNthptr;
	if (( pszText != NULL ) && ( GetCursorRange( pszCmdLine, &nRow, &nColumn ) == 0 ))
		nAttribute = GetColors( pszText, 0 );

	if (( pszText == NULL ) || ( nAttribute == -1 ))
		return ( Usage(( fVscrput ) ? VSCRPUT_USAGE : SCRPUT_USAGE ));

	// strip any quotes & process escape characters
	// We do it here to keep GetColors from removing whitespace around
	//   the text argument!
	ParseLine( NULL, pszText, NULL, CMD_STRIP_QUOTES, 0 );

	if ( nColumn == 999 ) {
		nColumn = ( GetScrCols() - (( fVscrput ) ? 0 : strlen( pszText ))) / 2;
		if ( nColumn < 0 )
			nColumn = 0;
	}

	if ( nRow == 999 ) {
		nRow = ( GetScrRows() - (( fVscrput ) ? strlen( pszText ) : 0 )) / 2;
		if ( nRow < 0 )
			nRow = 0;
	}

	// write the string with the specified attribute
	// first, check for VSCRPUT (vertical write )
	if ( fVscrput )
		WriteVStrAtt( nRow, nColumn, nAttribute, pszText );
	else
		WriteStrAtt( nRow, nColumn, nAttribute, pszText );

#if _WIN
	// restore original position
	SetCurPos( nSavedRow, nSavedColumn );
#endif

	return 0;
}


// save the current environment, alias list, and current drive/directory
//   (restored by ENDLOCAL or end of batch file )
int _near Setlocal_Cmd( LPTSTR pszCmdLine )
{
	return ( _setlocal() );
}


// restore the disk, directory, and environment (saved by SETLOCAL)
int _near Endlocal_Cmd( LPTSTR pszCmdLine )
{
	return ( _endlocal( pszCmdLine ) );
}


static int _fastcall _setlocal( void )
{
	register LPTSTR pszCurrentDirectory;
	int nOffset;

	nOffset = bframe[ cv.bn ].nSetLocalOffset;

	if ( nOffset >= SETLOCAL_NEST )
		return ( error( ERROR_4DOS_EXCEEDED_SETLOCAL_NEST, NULL ));

	// check for environment already saved
	if ( bframe[ cv.bn ].lpszLocalEnv[ nOffset ] != 0L )
		return ( error( ERROR_4DOS_ENV_SAVED, NULL ));

	nOffset = bframe[ cv.bn ].nSetLocalOffset;

	// save parameter & separator & escape characters
	bframe[ cv.bn ].cLocalParameter[ nOffset ] = gpIniptr->ParamChr;
	bframe[ cv.bn ].cLocalEscape[ nOffset ] = gpIniptr->EscChr;
	bframe[ cv.bn ].cLocalSeparator[ nOffset ] = gpIniptr->CmdSep;
	bframe[ cv.bn ].cLocalDecimal[ nOffset ] = gpIniptr->DecimalChar;
	bframe[ cv.bn ].cLocalThousands[ nOffset ] = gpIniptr->ThousandsChar;

	// get environment and alias list sizes
	bframe[ cv.bn ].uLocalEnvSize[ nOffset ] = gpIniptr->EnvSize;
	bframe[ cv.bn ].uLocalAliasSize[ nOffset ] = gpIniptr->AliasSize;

	// save the environment & alias list
	if ((( bframe[ cv.bn ].lpszLocalEnv[ nOffset ] = (TCHAR _far *)AllocMem((UINT *)(&( bframe[cv.bn].uLocalEnvSize[ nOffset ] )))) == 0L ) ||
	  (( bframe[cv.bn].lpszLocalAlias[ nOffset ] = (TCHAR _far *)AllocMem((UINT *)(&( bframe[cv.bn].uLocalAliasSize[ nOffset ] )))) == 0L ))
		return (error( ERROR_NOT_ENOUGH_MEMORY, NULL ));

	_fmemmove( bframe[ cv.bn ].lpszLocalEnv[ nOffset ], glpEnvironment, bframe[cv.bn].uLocalEnvSize[ nOffset ] );
	_fmemmove( bframe[ cv.bn ].lpszLocalAlias[ nOffset ], glpAliasList, bframe[cv.bn].uLocalAliasSize[ nOffset ] );

	pszCurrentDirectory = gcdir( NULL, 1 );
	bframe[ cv.bn ].lpszLocalDir[ nOffset ] = (( pszCurrentDirectory != NULL ) ? _strdup( pszCurrentDirectory ) : 0 );

	bframe[ cv.bn ].nSetLocalOffset++;
	return 0;
}


static int _fastcall _endlocal( LPTSTR pszCmdLine )
{
	int i, nReturn = 0, fExport, nOffset;
	TCHAR _far *lpszVar;
	TCHAR *pszArg;
	TCHAR szVariable[CMDBUFSIZ];

	if ( bframe[ cv.bn ].nSetLocalOffset == 0 )
		return ( error( ERROR_4DOS_ENV_NOT_SAVED, NULL ));

	bframe[ cv.bn ].nSetLocalOffset--;
	nOffset = bframe[ cv.bn ].nSetLocalOffset;

	// check for missing SETLOCAL (environment not saved)
	if (( bframe[ cv.bn ].lpszLocalEnv[ nOffset ] == 0L ) || ( bframe[ cv.bn ].lpszLocalAlias[ nOffset ] == 0L ))
		return ( error( ERROR_4DOS_ENV_NOT_SAVED, NULL ));

	// remove the SETLOCAL local environment
	for ( lpszVar = glpEnvironment; ( *lpszVar != _TEXT('\0') ); ) {

		// skip "=C:\..." variables
		if ( *lpszVar == _TEXT('=') ) {
			lpszVar = next_env( lpszVar );
			continue;
		}
		
		sscanf_far( lpszVar, _TEXT("%255[^=]"), szVariable );

		// export list?
		fExport = 0;
		if (( pszCmdLine ) && ( *pszCmdLine )) {

			for ( i = 0; (( fExport == 0 ) && (( pszArg = ntharg( pszCmdLine, i )) != NULL )); i++ ) {
				// save this variable?
				if ( stricmp( pszArg, szVariable ) == 0 )
					fExport = 1;
			}
		}

		// remove the existing environment variables unless they're exported
		if ( fExport == 0 ) {
			add_list( szVariable, glpEnvironment );
			lpszVar = glpEnvironment;
		} else
			lpszVar = next_env( lpszVar );
	}

	 // restore the saved variables
	 for ( lpszVar = bframe[ cv.bn ].lpszLocalEnv[ nOffset ]; ( *lpszVar != _TEXT('\0') ); lpszVar = next_env( lpszVar ) ) {

		// skip "=C:\..." variables
		if ( *lpszVar == _TEXT('=') )
			continue;

		sscanf_far( lpszVar, _TEXT("%255[^=]"), szVariable );

		// export list?
		fExport = 0;
		if (( pszCmdLine ) && ( *pszCmdLine )) {

			for ( i = 0; (( fExport == 0 ) && (( pszArg = ntharg( pszCmdLine, i )) != NULL )); i++ ) {
				// don't overwrite this variable?
				if ( stricmp( pszArg, szVariable ) == 0 )
					fExport = 1;
			}
		}

		if ( fExport == 0 ) {
			sprintf( szVariable, FMT_FAR_PREC_STR, CMDBUFSIZ-1, lpszVar );
			add_list( szVariable, glpEnvironment );
		}
	}

	 // restore the alias list
	_fmemmove( glpAliasList, bframe[ cv.bn ].lpszLocalAlias[ nOffset ], bframe[ cv.bn ].uLocalAliasSize[ nOffset ] );

	FreeMem( bframe[ cv.bn ].lpszLocalEnv[ nOffset ] );
	FreeMem( bframe[ cv.bn ].lpszLocalAlias[ nOffset ] );
	bframe[ cv.bn ].lpszLocalEnv[ nOffset ] = bframe[ cv.bn ].lpszLocalAlias[ nOffset ] = NULL;
	bframe[ cv.bn ].uLocalEnvSize[ nOffset ] = bframe[ cv.bn ].uLocalAliasSize[ nOffset ] = 0;

	// restore parameter & separator & escape characters
	gpIniptr->ParamChr = bframe[ cv.bn ].cLocalParameter[ nOffset ];
	gpIniptr->EscChr = bframe[ cv.bn ].cLocalEscape[ nOffset ];
	gpIniptr->CmdSep = bframe[ cv.bn ].cLocalSeparator[ nOffset ];
	gpIniptr->DecimalChar = bframe[ cv.bn ].cLocalDecimal[ nOffset ];
	gpIniptr->ThousandsChar = bframe[ cv.bn ].cLocalThousands[ nOffset ];

	QueryCountryInfo();

	// restore the original drive and directory
	if ( bframe[ cv.bn ].lpszLocalDir[ nOffset ] != NULL ) {
		nReturn = __cd( bframe[ cv.bn ].lpszLocalDir[ nOffset ], CD_CHANGEDRIVE | CD_NOFUZZY | CD_NOERROR );
		free( bframe[ cv.bn ].lpszLocalDir[ nOffset ] );
		bframe[ cv.bn ].lpszLocalDir[ nOffset ] = 0;
	}

	return nReturn;
}


// shift the batch arguments upwards (-n) or downwards (+n)
int _near Shift_Cmd( LPTSTR pszCmdLine )
{
	int i, nShift = 1;

	// get shift count
	if (( pszCmdLine ) && ( *pszCmdLine )) {

		// collapse starting at a particular argument
		if ( *pszCmdLine == _TEXT('/') ) {

			nShift = atoi( pszCmdLine + 1 ) + bframe[aGlobals.bn].Argv_Offset;
			for ( i = 1; ( bframe[ cv.bn ].Argv[i] != NULL ) ; i++ )
				;
			if (( i <= nShift ) || ( nShift < 0 ))
				return 0;

			for ( ; ( bframe[ cv.bn ].Argv[nShift+1] != NULL ); nShift++ ) {
				free( bframe[ cv.bn ].Argv[nShift] );
				bframe[ cv.bn ].Argv[nShift] = _strdup( bframe[ cv.bn].Argv[nShift+1] );
			}
			free( bframe[cv.bn].Argv[nShift] );
			bframe[ cv.bn ].Argv[nShift] = NULL;
			return 0;
		}

		nShift = atoi( pszCmdLine );
	}

	// make sure we don't shift past the Argv[] boundaries
	for ( ; (( nShift < 0 ) && ( bframe[ cv.bn ].Argv_Offset > 0 )); bframe[ cv.bn ].Argv_Offset--, nShift++ )
		;

	for ( ; (( nShift > 0 ) && ( bframe[ cv.bn ].Argv[bframe[ cv.bn ].Argv_Offset] != NULL )); bframe[ cv.bn ].Argv_Offset++, nShift-- )
		;

	return 0;
}


// Switch Case Statement
int _near Switch_Cmd( LPTSTR pszCmdLine )
{
	register TCHAR *pszToken, *pszTest, *pszLine;
	int nNestingLevel = 0;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	// make sure the batch file is still open
	if ( open_batch_file() == 0 )
		return BATCH_RETURN;

	// save SWITCH arguments
	pszTest = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1) * sizeof(TCHAR) ), pszCmdLine );

	// scan for a matching Case, Default, or the EndSwitch line
	while ( getline( bframe[cv.bn].bfd, gszCmdline, CMDBUFSIZ-1, EDIT_DATA ) > 0 ) {

		bframe[cv.bn].uBatchLine++;

		if ( ContinueLine( gszCmdline ) != 0 )
			break;

		trim( gszCmdline, WHITESPACE );

		// check for (illegal syntax) "*case ..."
		if ( gszCmdline[0] == _TEXT('*'))
			strcpy( gszCmdline, gszCmdline+1 );

		// ignore nested SWITCH calls
		if ( strnicmp( gszCmdline, _TEXT("switch "), 7 ) == 0 )
			nNestingLevel++;

		else if ( stricmp( gszCmdline, _TEXT("Default") ) == 0 ) {
			if ( nNestingLevel == 0 )
				return 0;

		} else if (( nNestingLevel == 0 ) && ( strnicmp( gszCmdline, _TEXT("Case "), 5 ) == 0 )) {

			// test condition
			strcpy( gszCmdline, gszCmdline + 5 );
			var_expand( gszCmdline, 1 );

			// reformat line for TestCondition
			strins( gszCmdline, _TEXT(" EQ ") );
			strins( gszCmdline, pszTest );
			pszLine = gszCmdline;
			while (( pszToken = stristr( pszLine, _TEXT(" .or. ") )) != NULL ) {
				pszLine = skipspace( pszToken + 6 );
				strins( pszLine, _TEXT(" EQ ") );
				strins( pszLine, pszTest );
			}

			if ( TestCondition( gszCmdline, 0 ) == 1 )
				return 0;

		} else if ( stricmp( gszCmdline, _TEXT("EndSwitch") ) == 0 ) {
			if ( nNestingLevel <= 0 )
				return 0;
			nNestingLevel--;
		}
	}

	return USAGE_ERR;
}


// found a trailing CASE or DEFAULT - skip everything until EndSwitch
int _near Case_Cmd( LPTSTR pszCmdLine )
{
	extern int _near _fastcall ContinueLine( LPTSTR  );

	int nNestingLevel = 0;

	// make sure the batch file is still open
	if ( open_batch_file() == 0 )
		return BATCH_RETURN;

	// scan for the EndSwitch line
	while ( getline( bframe[cv.bn].bfd, gszCmdline, CMDBUFSIZ-1, EDIT_DATA ) > 0 ) {

		bframe[cv.bn].uBatchLine++;

		if ( ContinueLine( gszCmdline ) != 0 )
			break;

		trim( gszCmdline, WHITESPACE );

		// ignore nested SWITCH calls
		if ( strnicmp( gszCmdline, _TEXT("switch "), 7 ) == 0 )
			nNestingLevel++;

		else if ( stricmp( gszCmdline, _TEXT("EndSwitch") ) == 0 ) {
			if ( nNestingLevel <= 0 )
				return 0;
			nNestingLevel--;
		}
	}

	return USAGE_ERR;
}


#pragma alloc_text( MISC_TEXT, TripleKey )

// triple up a 4-character key to make it 12 characters
// method is slightly obscure to make hacking more confusing
static void _fastcall TripleKey( char *pKey )
{
	static int nIndexes[4] = {4, 2, 1, 3};
	register int i;

	for (i = 0; i < 4; i++)
		pKey[nIndexes[i] + 7] = pKey[nIndexes[i] + 3] = pKey[nIndexes[i] - 1];
}
