

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


// ERROR.C - error routines for 4dos
//   (c) 1988 - 2002  Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

// force definition of the global variables
#define DEFINE_GLOBALS 1

#include "4all.h"



// display batch file name & line number
void BatchErrorMsg( void )
{
	if (( cv.bn >= 0 ) && (( bframe[ cv.bn ].nFlags & BATCH_REXX ) == 0 ))
		qprintf( STDERR, _TEXT("%s [%d]  "), bframe[ cv.bn ].pszBatchName, bframe[ cv.bn ].uBatchLine );
}


// display proper command usage
int _fastcall Usage( LPTSTR pszMessage )
{
	TCHAR szBuffer[256];

	gnInternalErrorLevel = USAGE_ERR;
	CheckOnError();

	BatchErrorMsg();

	sprintf( szBuffer, FMT_DOUBLE_STR, USAGE_MSG, pszMessage );
	pszMessage = szBuffer + strlen( USAGE_MSG );
	for ( ; *pszMessage; pszMessage++ ) {

		// expand ? to [d:][path]filename
		if ( *pszMessage == _TEXT('?') )
			strins( pszMessage + 1, FILE_SPEC );

		// expand ~ to [d:]pathname
		else if ( *pszMessage == _TEXT('~') )
			strins( pszMessage + 1, PATH_SPEC );

		// expand # to [bri] [bli] fg ON bg
		else if ( *pszMessage == _TEXT('#') )
			strins( pszMessage + 1, COLOR_SPEC );
		else
			continue;

		strcpy( pszMessage, pszMessage + 1 );
	}

	qprintf( STDERR, FMT_STR_CRLF, szBuffer );

	if ( gpIniptr->LogErrors )
		_log_entry( szBuffer, 0 );

	CheckOnErrorMsg();

	return USAGE_ERR;
}


// display an OS or 4xxx error message w/optional argument
int _fastcall error( int nErrorCode, LPTSTR pszArg )
{
	char szBuffer[300];

	gnInternalErrorLevel = ERROR_EXIT;
	CheckOnError();

	// print the error message, optionally labeling it as belonging to 4xxx
	if ( gpIniptr->INIDebug & INI_DB_ERRLABEL )
		qprintf( STDERR, _TEXT("%s: %s"), NAME_HEADER, (( nErrorCode < OFFSET_4DOS_MSG ) ? _TEXT("(Sys) ") : NULLSTR ));

	// display batch file name & line number
	BatchErrorMsg();
	szBuffer[0] = _TEXT('\0');

	if ( nErrorCode <= 0 )
		nErrorCode = _doserrno;
	if ( nErrorCode < OFFSET_4DOS_MSG ) {
		gnSysError = nErrorCode;
		GetError( nErrorCode, szBuffer );
	} else
		strcpy( szBuffer, int_4dos_errors[ nErrorCode-OFFSET_4DOS_MSG ] );

	// if s != NULL, print it (probably a filename) in quotes
	if ( pszArg != NULL )
		sprintf( strend(szBuffer), (( *pszArg == '"' ) ? " %.260s": " \"%.260s\"" ), pszArg );

	qprintf( STDERR, FMT_STR_CRLF, szBuffer );

	if ( gpIniptr->LogErrors )
		_log_entry( szBuffer, 0 );

	CheckOnErrorMsg();

	return ERROR_EXIT;
}


// check for "ON ERROR" in batch files
void PASCAL CheckOnError( void )
{
	if (( cv.bn >= 0 ) && ( bframe[cv.bn].pszOnError != NULL )) {
		cv.fException |= BREAK_ON_ERROR;
		BreakOut();
	}
}


// check for "ON ERRORMSG" in batch files
void CheckOnErrorMsg( void )
{
	if (( cv.bn >= 0 ) && ( bframe[cv.bn].pszOnErrorMsg != NULL )) {
		cv.fException |= BREAK_ON_ERROR;
		BreakOut();
	}
}
