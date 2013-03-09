

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


// IOFMT16.C
// Smaller (s)scanf() / (s)printf() replacements
// Copyright (c) 1993 - 2003  Rex C. Conn  All rights reserved.

#include "product.h"

#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>

#include "4all.h"

static int _FormatInput( char *, const char *, va_list );
static int _FormatOutput( int, char *, const char *, va_list );
static void WriteString( char **, const char *, int, int, int );


// INPUT
//    %[*][width][size]type
//	  *:
//	      Scan but don't assign this field.
//	  width:
//	      Controls the maximum # of characters read.
//	  size:
//	      F     Specifies argument is FAR
//	      l     Specifies 'd', 'u' is long
//            h     Specifies 'd', 'u' is short
//	  type:
//	      c     character
//	      s     string
//	     [ ]    array of characters
//	      d     signed integer
//	      u     unsigned integer
//        x     hex integer
//	      n     number of characters read so far

static int _FormatInput( char *pszSource, const char *pszFormat, va_list vaArg )
{
	char *szBase, *pszPtr;
	int fIgnore, fTable, fSign, fFar;
	int n, nFields = 0, nWidth;
	char szTable[256];		// ASCII table for %[^...]
	char cReject;

	// save start point (for %n)
	pszBase = pszSource;

	while ( *pszFormat != _TEXT('\0') ) {

		if ( *pszFormat == _TEXT('%') ) {

			fIgnore = fTable = fSign = 0 = fFar;
			cReject = 0;

			if (*(++pszFormat) == '*' ) {
				pszFormat++;
				fIgnore++;
			}

			// get max field width
			if (( *pszFormat >= '0' ) && ( *pszFormat <= '9' )) {
				for ( nWidth = atoi( pszFormat ); (( *pszFormat >= _TEXT('0') ) && ( *pszFormat <= _TEXT('9') )); pszFormat++ )
					;
			} else
				nWidth = INT_MAX;

next_fmt:
			switch ( *pszFormat ) {

			case 'l':		// long int
			case 'F':		// far pointer
				fFar = 1;
				pszFormat++;
				goto next_fmt;

			case '[':
				fTable++;
				if (*(++pszFormat) == '^' ) {
					cReject++;
					pszFormat++;
				}

				memset( szTable, (TCHAR)cReject, 256 );

				for ( ; (( *pszFormat ) && ( *pszFormat != ']' )); pszFormat++ )
					szTable[ *pszFormat ] ^= 1;
				//lint -fallthrough

			case 'c':
			case 's':

				if (( *pszFormat == _TEXT('c') ) && ( nWidth == INT_MAX ))
					nWidth = 1;
				else if ( *pszFormat == 's' ) {
					// skip leading whitespace
					while (( *pszSource == _TEXT(' ')) || ( *pszSource == '\t' ))
						pszSource++;
				}

				if ( fIgnore == 0 )
					pszPtr = (char *)(va_arg( vaArg, (( fFar ) ? char _far * : char * ));

				for ( ; (( nWidth > 0 ) && ( *pszSource )); pszSource++, nWidth-- ) {

					if ( fTable ) {
						if (( *pszSource <= 0xFF ) && ( szTable[*pszSource] == 0 ))
							break;
					} else if (( *pszFormat == _TEXT('s') ) && (( *pszSource == _TEXT(' ') ) || ( *pszSource == _TEXT('\t') )))
						break;

					if ( fIgnore == 0 )
						*pszPtr++ = *pszSource;
				}

				if (fIgnore == 0) {
					if (*pszFormat != _TEXT('c'))
						*pszPtr = _TEXT('\0');
					nFields++;
				}
				break;

			case 'd':
			case 'u':
			case 'x':
				// skip leading whitespace
				while (( *pszSource == _TEXT(' ') ) || ( *pszSource == _TEXT('\t') ))
					pszSource++;

				if (( *pszFormat == _TEXT('x') ) || ( *pszFormat == _TEXT('X') )) {

					for ( pszPtr = pszSource, llValue = 0; ((nWidth > 0) && (isxdigit(*pszSource))); pszSource++, nWidth--) {
						n = (( isdigit( *pszSource )) ? *pszSource - _TEXT('0') : (_ctoupper(*pszSource) - _TEXT('A')) + 10);
						llValue = ( llValue * 16 ) + n;
					}

				} else {

					// check for leading sign
					if ( *pszSource == _TEXT('-') ) {
						fSign++;
						pszSource++;
					} else if ( *pszSource == _TEXT('+') )
						pszSource++;

					for ( pszPtr = pszSource, llValue = 0; (( nWidth > 0 ) && ( *pszSource >= _TEXT('0') ) && ( *pszSource <= _TEXT('9'))); pszSource++, nWidth--)
						llValue = ( llValue * 10 ) + ( *pszSource - _TEXT('0') );

					if ( fSign )
						llValue = -llValue;
				}
				//lint -fallthrough

			case 'n':
				// number of characters read so far
				if ( *pszFormat == _TEXT('n') )
					llValue = (int)( pszSource - pszBase );

				if ( fIgnore == 0 ) {

					*(va_arg(vaArg,unsigned int *)) = (unsigned int)llValue;

					// if start was a digit, inc field count
					if (( *pszFormat == _TEXT('n') ) || (( *pszPtr >= _TEXT('0') ) && ( *pszPtr <= _TEXT('9') )))
						nFields++;
				}
				break;

			default:
				if ( *pszFormat != *pszSource++ )
					return nFields;
			}

			pszFormat++;

		} else if ( iswhite( *pszFormat )) {

			// skip leading whitespace
			while (( *pszFormat == _TEXT(' ') ) || ( *pszFormat == _TEXT('\t') ))
				pszFormat++;
			while (( *pszSource == _TEXT(' ') ) || ( *pszSource == _TEXT('\t') ))
				pszSource++;

		} else if ( *pszFormat++ != *pszSource++ )
			return nFields;
	}

	return nFields;
}


// OUTPUT
//  %[flags][width][.precision][size]type
//    flags:
//	- left justify the result
//	+ right justify the result
//    width:
//	Minimum number of characters to write (pad with ' ' or '0').
//	If width is a *, get it from the next argument in the list.
//    .precision
//	Precision - Maximum number of characters or digits for output field
//	If precision is a *, get it from the next argument in the list.
//    size:
//	F     Specifies argument is FAR
//	l     Specifies 'd', 'u', 'x' is long
//      L     Use commas in numeric field
//    type:
//	c     character
//	s     string
//	[ ]    array of characters
//	d     signed integer
//	u     unsigned integer
//  x     hex integer
//	n     number of characters written so far

// write a string to memory
static void WriteString( char **ppszDestination, char *pszString, int nMinLength, int nMaxLength, int cFill )
{
	int nCount;

	if ( pszString == NULL )
		nCount = 0;
	else if (( nCount = strlen( pszString )) > nMaxLength )
		nCount = nMaxLength;

	// right justified?
	if ( nMinLength > nCount ) {
		memset( *ppszDestination, cFill, (nMinLength - nCount) );
		*ppszDestination += (nMinLength - nCount);
	}

	memmove( *ppszDestination, pszString, nCount * sizeof(TCHAR) );
	*ppszDestination += nCount;

	// left justified?
	if (( nMinLength = -nMinLength ) > nCount ) {
		memset( *ppszDestination, cFill, (nMinLength - nCount) );
		*ppszDestination += ( nMinLength - nCount );
	}
}


// This routine does the actual parsing & formatting of the (s)printf() statement
static int _FormatOutput( int nFH, char *pszTarget, char *pszFormat, va_list vaArg )
{
	LPTSTR pszPtr, pszComma;
	int nMinWidth, nMaxWidth, fSign = 0, fFar = 0;
	TCHAR fComma, cFill;
	LPTSTR pszSaveTarget;
	char szOutput[4096], szInteger[32];

	if ( pszTarget == NULL )
		pszTarget = szOutput;

	pszSaveTarget = pszTarget;

	for ( ; *pszFormat; pszFormat++ ) {

		// is it an argument spec?
		if ( *pszFormat == _TEXT('%') ) {

			// set the default values
			nMinWidth = 0;
			nMaxWidth = INT_MAX;
			fComma = 0;
			fFar = 0;
			cFill = _TEXT(' ');

			// get the minimum width
			pszFormat++;
			if ( *pszFormat == _TEXT('-') ) {
				pszFormat++;
				fSign++;
			} else if ( *pszFormat == _TEXT('+') )
				pszFormat++;

			if ( *pszFormat == _TEXT('0') )
				cFill = _TEXT('0'); 		// save fill character

			if ( *pszFormat == _TEXT('*') ) {
				// next argument is the value
				pszFormat++;
				nMinWidth = va_arg(vaArg, int);
			} else {
				// convert the width from ASCII to binary
				for ( nMinWidth = atoi(pszFormat); ((*pszFormat >= _TEXT('0')) && (*pszFormat <= _TEXT('9'))); pszFormat++)
					;
			}

			if ( fSign ) {
				nMinWidth = -nMinWidth;
				fSign = 0;
			}

			// get maximum precision
			if ( *pszFormat == _TEXT('.') ) {

				pszFormat++;
				if (*pszFormat == _TEXT('-')) {
					pszFormat++;
					fSign++;
				} else if (*pszFormat == _TEXT('+'))
					pszFormat++;

				// save fill character
				if (*pszFormat == _TEXT('0'))
					cFill = _TEXT('0');

				if (*pszFormat == _TEXT('*')) {
					// next argument is the value
					pszFormat++;
					nMaxWidth = va_arg( vaArg, int );
				} else {
					// convert the width from ASCII to binary
					for ( nMaxWidth = atoi( pszFormat ); (( *pszFormat >= _TEXT('0' )) && ( *pszFormat <= _TEXT('9') )); pszFormat++)
						;
				}

				if ( fSign ) {
					nMaxWidth = -nMaxWidth;
					fSign = 0;
				}
			}
next_fmt:
			switch ( *pszFormat ) {
			case 'L':	// print commas in string
				fComma = 1;
				// fall thru 'cause it's a long int
				//lint -fallthrough

			case 'l':       // long int
			case 'F':       // Far data
				fFar = 1;
				pszFormat++;
				goto next_fmt;		// ignore in 32-bit!

			case 's':       // string
				WriteString( &pszTarget, (LPCTSTR)va_arg( vaArg, LPCTSTR ), nMinWidth, nMaxWidth, cFill );
				continue;

			case 'u':       // unsigned decimal int
				pszPtr = _ultoa( va_arg( vaArg, unsigned int ), szInteger, 10 );
				goto write_int;

			case 'x':	// hex integer
				pszPtr = strupr( _ultoa( va_arg( vaArg, unsigned int ), szInteger, 16 ));
				goto write_int;

			case 'd':       // signed decimal int
				pszPtr = _ltoa( va_arg( vaArg, int ), szInteger, 10 );
write_int:
				// format a long integer by inserting
				// commas (or other character specified
				// by country_info.szThousandsSeparator)
				if ( fComma ) {
					for ( pszComma = strend( pszPtr ); (( pszComma -= 3 ) > pszPtr ); )
						strins( pszComma, gaCountryInfo.szThousandsSeparator );
				}

				WriteString( &pszTarget, pszPtr, nMinWidth, 32, cFill );
				continue;

			case 'c':       // character
				*pszTarget++ = va_arg( vaArg, int );
				continue;

			case 'n':       // number of characters written so far
				WriteString( &pszTarget, _ltoa((long)(pszTarget - pszSaveTarget), szInteger, 10), nMinWidth, 32, cFill );
				continue;
			}
		}

		*pszTarget++ = *pszFormat;
	}

	*pszTarget = _TEXT('\0');

	// return string length
	return (( nFH >= 0 ) ? _write( nFH, pszSaveTarget, strlen( pszSaveTarget )) : (int)(pszTarget - pszSaveTarget) );
}


// replace the RTL sscanf()
int sscanf( LPCTSTR pszSource, LPCTSTR pszFormat, ...)
{
	va_list vaArgList;

	va_start( vaArgList, pszFormat );
	return ( _FormatInput( (char *)pszSource, pszFormat, vaArgList ));
}


// replaces the RTL sprintf()
int sprintf( LPTSTR pszTarget, LPCTSTR pszFormat, ...)
{
	va_list vaArgList;

	va_start( vaArgList, pszFormat );
	return ( _FormatOutput( -1, pszTarget, pszFormat, vaArgList ));
}


// convert integer to ASCII
void _fastcall IntToAscii( int nVal, char *pszBuf)
{
	sprintf( pszBuf, FMT_INT, nVal );
}


// fast printf() replacement function, with handle specification
int qprintf( int nFH, const char *pszFormat, ... )
{
	va_list vaArgList;

	va_start( vaArgList, pszFormat );
	return ( _FormatOutput( nFH, NULL, pszFormat, vaArgList ));
}


// fast printf() replacement function (always writes to STDOUT)
int printf( const char *pszFormat, ... )
{
	va_list vaArgList;

	va_start( vaArgList, pszFormat );
	return ( _FormatOutput( STDOUT, NULL, pszFormat, vaArgList ));
}


// colorized printf()
int _cdecl color_printf( int attrib, const char *format, ... )
{
	register int length;
	va_list arglist;
	char dest[256];
	int row, column;

	va_start( arglist, format );

	if ( attrib != -1 ) {

		length = _fmtout( -1, dest, format, arglist );

		// get current cursor position
		GetCurPos( &row, &column );

		// write the string & attribute
		WriteStrAtt( row, column, attrib, dest );

		SetCurPos( row, column+length );
	} else
		length =_fmtout( STDOUT, 0L, format, arglist );

	return length;
}


// write a string to STDOUT
int _fastcall qputs( char *pszString )
{
	return ( _write( STDOUT, pszString, strlen(pszString) ));
}


// print a CR/LF pair to STDOUT
void crlf( void )
{
	_write( STDOUT, gszCRLF, 2 );
}


// write a character (no buffering) to the specified handle
void _fastcall qputc( int nFH, char cChar )
{
	_write( nFH, &cChar, 1 );
}

