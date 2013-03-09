

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


// MISC.C - Miscellaneous support routines for 4xxx / TCMD family
//   (c) 1988 - 2002 Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <direct.h>
#include <dos.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <share.h>
#include <string.h>

#include "4all.h"

static LPTSTR _fastcall NextRange( TCHAR * );
static int _fastcall GetRangeArgs( TCHAR *, RANGES * );
static int _fastcall GetStrTime( TCHAR *, int *, int * );

static void GetStrSize( TCHAR *, long * );

static int _fastcall QueryIsRelative( TCHAR * );


int fNoComma = 0;
static TCHAR szNthArgBuffer[ MAXARGSIZ+1 ];


// return the CRC32 for the specified file
unsigned long _fastcall CRC32( LPTSTR pszFileName )
{
	unsigned long ulTable[256];
    unsigned long ulCRC, ulPoly;
    int nFH;
	unsigned int i, j, uBytesRead, uReadSize = 4096;
	char _far *lpszBuffer;

	if (( nFH = _sopen( pszFileName, _O_RDONLY | _O_BINARY, _SH_DENYWR )) < 0 ) {
CRC32_Error:
		error( _doserrno, pszFileName );
		return 0;
	}

	if (( lpszBuffer = (char _far *)AllocMem( &uReadSize )) == NULL ) {
		_close( nFH );
		goto CRC32_Error;
	}

	// generate the table first
    ulPoly = 0xEDB88320L;
    for ( i = 0; i < 256; i++ ) {

        ulCRC = i;
        for ( j = 8; j > 0; j-- ) {
            if ( ulCRC & 1 )
                ulCRC = ( ulCRC >> 1 ) ^ ulPoly;
            else
                ulCRC >>= 1;
        }

		ulTable[i] = ulCRC;
	}

    ulCRC = 0xFFFFFFFF;

	do {

		if ( FileRead( nFH, lpszBuffer, uReadSize, &uBytesRead ) != 0 ) {
			_close( nFH );
			FreeMem( lpszBuffer );
			goto CRC32_Error;
		}

		for ( i = 0; ( i < uBytesRead ); i++ )
	        ulCRC = (( ulCRC >> 8 ) & 0x00FFFFFF ) ^ ulTable[ ( ulCRC ^ (int)lpszBuffer[i] ) & 0xFF ];

	} while ( uBytesRead != 0 );

	_close( nFH );
	FreeMem( lpszBuffer );

    return( ulCRC ^ 0xFFFFFFFF );
}


// read a single line from the file (for @filename)
int PASCAL GetFileLine( TCHAR *pszFileName, long *plOffset, TCHAR *pszLine )
{
	register int nFH, i;
	int nEditFlags = EDIT_DATA;

	*pszLine = _TEXT('\0');

	HoldSignals();
	if ( QueryIsCON( pszFileName ))
		nFH = STDIN;

	else {

		// can only match CLIP: the first time through!
		if ( _stricmp( pszFileName, CLIP ) == 0 ) {
			RedirToClip( pszFileName, 0 );
			CopyFromClipboard( pszFileName );
		}

		if (( nFH = _sopen( pszFileName, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 )
			return _doserrno;

		// read a line from the file

		_lseek( nFH, *plOffset, SEEK_SET );
	}

	i = getline( nFH, pszLine, MAXFILENAME, nEditFlags );
	if ( nFH != STDIN )
		_close( nFH );
	*plOffset += i;
	EnableSignals();

	return 0;
}


int _fastcall QueryIsLFN( TCHAR *pszFilename )
{
	int n, fExt;

	// check for LFN/NTFS name w/spaces or other invalid chars
	if ( strpbrk( pszFilename, _TEXT(" ,\t|=<>") ) != NULL )
		return 1;

	// check name for multiple extensions or
	//   > 8 char filename & 3 char extension
	for ( n = 0, fExt = 0; ( pszFilename[n] != _TEXT('\0') ); n++ ) {
		if ( pszFilename[n] == _TEXT('.') ) {
			if (( fExt ) || ( n > 8 ))
				return 1;
			fExt = strlen( pszFilename + n );
		}
	}

	return (( n > 12 ) || (( fExt == 0 ) && ( n > 8 )) || ( fExt > 4 ));
}


// Get a file's size via the handle

long _fastcall QuerySeekSize( int nFH )

{
	return ( _lseek( nFH, 0L, SEEK_END ));
}


// Rewind a file

long _fastcall RewindFile( int nFH )

{
	return ( _lseek( nFH, 0L, SEEK_SET ));
}


// convert keystrokes
int _fastcall cvtkey( unsigned int uKeyCode, unsigned int uContextBits )
{
	register unsigned int i, uSearchKey;
	unsigned int uKeyCount = gpIniptr->KeyUsed;
	unsigned int *puScan = gpIniptr->Keys;
	unsigned int *puSubstitute = gpIniptr->Keys + uKeyCount;

	uSearchKey = _ctoupper( uKeyCode );

	// scan the key mapping array
	for ( i = 0; ( i < uKeyCount ); i++ ) {

		// if key and context bits match, return the mapped key
		if (( uSearchKey == puScan[i]) && (( puSubstitute[i] & uContextBits) != 0 )) {

			// if forced normal key return original code with force normal bit set
			if (( puSubstitute[i] & NORMAL_KEY ) != 0 )
				return ( uKeyCode | NORMAL_KEY );

			// otherwise return new key

			return ( puSubstitute[i] & ( EXTENDED_KEY | 0xFF ));

		}
	}

	// no match, return original code
	return uKeyCode;
}



#pragma alloc_text( _TEXT, iswhite, isdelim, skipspace, first_arg, next_arg, last_arg, ntharg )


// replace RTL isspace() - we only want to check for spaces & tabs
int _fastcall iswhite( TCHAR c )
{
	return (( c == _TEXT(' ') ) || ( c == _TEXT('\t') ));
}


// test for delimiter (" \t,")
int _fastcall isdelim( TCHAR c )
{
	return (( c == _TEXT('\0') ) || ( iswhite( c )) || ( c == _TEXT(',') ));
}


// skip past leading white space and return pointer to first non-space char
LPTSTR _fastcall skipspace( LPTSTR pszLine )
{
	while (( *pszLine == _TEXT(' ') ) || ( *pszLine == _TEXT('\t') ))
		pszLine++;

	return pszLine;
}



#pragma alloc_text( ASM_TEXT, GetToken )


// return token x to token y without quote processing (for compatibility
//   with nitwit CMD.EXE syntax)
LPTSTR GetToken( TCHAR *pszLine, TCHAR *pszDelims, int nStart, int nEnd )
{
	register int i, n;
	BOOL fReverse;
	TCHAR *pszArg, *pszStartLine;
	TCHAR *pszStart = NULL, *pszEnd = NULL;

	// change to 0 offset!
	fReverse = ( nStart < 0 );
	nStart += (( fReverse ) ? 1 : -1 );
	nEnd += (( fReverse ) ? 1 : -1 );

	pszStartLine = pszLine;
	if ( fReverse )
		pszLine = strlast( pszLine );

	for ( i = nStart, n = 0; ; ) {

		// find start of pszArg[i]
		while (( *pszLine != _TEXT('\0') ) && ( pszLine >= pszStartLine ) && ( strchr( pszDelims, *pszLine ) != NULL ))
			pszLine += (( fReverse ) ? -1 : 1 );

		// search for next delimiter character
		for ( pszArg = pszLine; (( *pszLine != _TEXT('\0') ) && ( pszLine >= pszStartLine )); ) {

			if ( strchr( pszDelims, *pszLine ) != NULL )
				break;

			pszLine += (( fReverse ) ? -1 : 1 );
		}

		if ( i == 0 ) {

			// this is the argument I want - copy it & return
			if (( n = (int)( pszLine - pszArg )) < 0 ) {
				n = -n;
				pszLine++;
			} else
				pszLine = pszArg;

			if ( pszStart == NULL ) {

				// located first argument - save pointer
				pszStart = pszLine;
				if ( nEnd == nStart )
					goto GotArg;

				// reset & get last argument
				i = nEnd;
				pszLine = pszStartLine;
				continue;
			} else {
GotArg:
				// located last argument - save pointer
				pszEnd = pszLine + n;
				break;
			}
		}

		if (( *pszLine == _TEXT('\0') ) || ( pszLine <= pszStartLine ))
			break;

		i += (( fReverse ) ? 1 : -1 );
	}

	if ( i != 0 ) {
		// failed to find at start and/or end
		if ( pszStart == NULL ) {
			pszEnd = pszStart = pszStartLine;
		} else
			pszEnd = strend( pszStart );
	}

	sprintf( pszStartLine, FMT_PREC_STR, ( pszEnd - pszStart ), pszStart );

	return pszStartLine;
}


// return the first argument in the line
LPTSTR _fastcall first_arg( LPTSTR pszLine )
{
	return ( ntharg( pszLine, 0 ));
}


// remove everything up to the "nArgument" argument(s)
LPTSTR _fastcall next_arg( LPTSTR pszLine, int nArgument )
{
	ntharg( pszLine, nArgument | 0x8000 );
	return ( strcpy( pszLine, (( gpNthptr != NULL ) ? gpNthptr : NULLSTR )));
}


// return the last argument in the line (skipping the first arg)
LPTSTR _fastcall last_arg( LPTSTR pszLine, register int *pnLast )
{
	for ( *pnLast = 1; ( ntharg( pszLine, *pnLast | 0x8000 ) != NULL ); (*pnLast)++ )
		;

	return (( *pnLast == 1 ) ? NULL : ntharg( pszLine, --(*pnLast) ));
}


// NTHARG returns the nth argument in the command line (parsing by whitespace
//   & switches) or NULL if no nth argument exists
LPTSTR _fastcall ntharg( LPTSTR pszLine, int nIndex )
{
	static TCHAR szDelims[] = _TEXT("  \t,"), *pszQuotes;
	TCHAR *pszArg;
	register int fCopy = 1;

	gpNthptr = NULL;
	if (( pszLine == NULL ) || ( *pszLine == _TEXT('\0') ))
		return NULL;

	pszQuotes = QUOTES;
	szDelims[3] = (TCHAR)(( fNoComma == 0 ) ? _TEXT(',') : _TEXT('\0') );

	if ( nIndex & 0x800 ) {
		// trick to disable switch character as delimiter
		szDelims[0] = _TEXT(' ');
	} else
		szDelims[0] = gpIniptr->SwChr;

	if ( nIndex & 0x2000 ) {
		// trick to allow []'s as quote chars
		pszQuotes = _TEXT("`\"[");
	}

	if ( nIndex & 0x4000 ) {
		// trick to allow ( )'s as quote characters
		pszQuotes = _TEXT("`\"([");
	}

	if ( nIndex & 0x1000 ) {
		// trick to disable ` as quote character
		pszQuotes++;
	}

	// trick to disable copy - all we want is "gpNthptr"
	if ( nIndex & 0x8000 )
		fCopy = 0;

	nIndex &= 0x7FF;

	for ( ; ; nIndex-- ) {

		// find start of arg[i]
		pszLine += strspn( pszLine, szDelims+1 );
		if (( *pszLine == _TEXT('\0') ) || ( nIndex < 0 ))
			break;

		// search for next delimiter or switch character
		if (( pszArg = scan(( *pszLine == gpIniptr->SwChr ) ? pszLine + 1 : pszLine, szDelims, pszQuotes )) == BADQUOTES)
			break;

		if ( nIndex == 0 ) {

			// this is the argument I want - copy it & return
			gpNthptr = pszLine;
			if ( fCopy == 0)
				return gpNthptr;

			if (( nIndex = (int)( pszArg - pszLine )) > MAXARGSIZ )
				nIndex = MAXARGSIZ;
			sprintf( szNthArgBuffer, FMT_PREC_STR, nIndex, pszLine );
			return szNthArgBuffer;
		}

		pszLine = pszArg;
	}

	return NULL;
}


// Find the first character in LINE that is also in SEARCH.
//   Return a pointer to the matched character or EOS if no match
//   is found.  If SEARCH == NULL, just look for end of line ((which
//   could be a pipe, compound command character, or conditional)
LPTSTR scan( TCHAR *pszLine, TCHAR *pszSearch, TCHAR *pszQuotes )
{
	register TCHAR cQuote;
	TCHAR *pSaveLine, cOpenParen, cCloseParen;
	int fTerm, fError;

	// kludge to suppress error message on unmatched backquote
	if (( pszSearch != NULL ) && ( *pszSearch == _TEXT('\0') )) {
		pszSearch = NULL;
		fError = 0;
	} else
		fError = 1;

	// kludge to allow detection of "%+" when looking for command separator
	fTerm = (( pszSearch == NULL ) || ( strchr( pszSearch, gpIniptr->CmdSep ) != NULL ));

	if ( pszLine != NULL ) {

		while ( *pszLine != _TEXT('\0') ) {

TestForQuotes:
			// test for a quote character or a () or [] expression
			if ((( *pszLine == _TEXT('(') ) || ( *pszLine == _TEXT('[') ) || (( gpIniptr->Expansion & EXPAND_NO_QUOTES ) == 0 )) && ( strchr( pszQuotes, *pszLine ) != NULL ) && (( pszSearch == NULL ) || ( strchr( pszSearch, *pszLine ) == NULL ))) {

				if (( *pszLine == _TEXT('(') ) || ( *pszLine == _TEXT('[') )) {

					cOpenParen = *pszLine;
					cCloseParen = (TCHAR)(( cOpenParen == _TEXT('(') ) ? _TEXT(')') : _TEXT(']') );

					pSaveLine = pszLine;

					// ignore characters within parentheses / brackets
					for ( cQuote = 1; (( cQuote > 0 ) && ( *pszLine != _TEXT('\0') )); ) {

						// nested parentheses?
						if ( *(++pszLine) == cOpenParen )
							cQuote++;
						else if ( *pszLine == cCloseParen )
							cQuote--;
						else if ( *pszLine == _TEXT('\"')) {

							TCHAR tcQuote;

							// ignore characters within double quotes
							for ( tcQuote = *pszLine; ( *(++pszLine) != tcQuote ); ) {

								// skip escape characters
								if (( *pszLine == gpIniptr->EscChr ) && (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 ))
									pszLine++;
								else if (( *pszLine == _TEXT('%') ) && ( pszLine[1] == _TEXT('=') )) {
									if (( tcQuote == _TEXT('"') ) && (( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 )) {
										// convert %= to default escape char
										strcpy( pszLine, pszLine + 1 );
										*pszLine++ = gpIniptr->EscChr;
									} else
										pszLine += 2;
								}

								if ( *pszLine == _TEXT('\0') ) {

									// unclosed double quotes OK?
									if (( tcQuote == _TEXT('"') ) || ( fError == 0 ))
										return pszLine;

									error( ERROR_4DOS_NO_CLOSE_QUOTE, NULL );
									return BADQUOTES;
								}
							}

						// skip escape characters
						} else if (( *pszLine == gpIniptr->EscChr ) && (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 ))
							pszLine++;
						else if (( *pszLine == _TEXT('%') ) && ( pszLine[1] == _TEXT('=') ) && (( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 )) {
							// convert %= to default escape char
							strcpy( pszLine, pszLine + 1 );
							*pszLine++ = gpIniptr->EscChr;
						}
					}

					// if no trailing ], assume [ isn't a quote char!
					if ( *pszLine == _TEXT('\0') ) {

						if ( cOpenParen != _TEXT('[') )
							break;
						pszLine = pSaveLine;

						// if only looking for the trailing ) or ], return
					} else if (( pszSearch != NULL ) && ( *pszSearch == cCloseParen ))
						break;

				} else {

					// ignore characters within quotes
					for ( cQuote = *pszLine; ( *(++pszLine) != cQuote ); ) {

						// skip escape characters
						if (( *pszLine == gpIniptr->EscChr ) && (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 ))
							pszLine++;
						else if (( *pszLine == _TEXT('%') ) && ( pszLine[1] == _TEXT('=') )) {
							if (( cQuote == _TEXT('"') ) && (( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 )) {
								// convert %= to default escape char
								strcpy( pszLine, pszLine + 1 );
								*pszLine++ = gpIniptr->EscChr;
							} else
								pszLine += 2;
						}

						if ( *pszLine == _TEXT('\0') ) {

							// unclosed double quotes OK?
							if (( cQuote == _TEXT('"') ) || ( fError == 0 ))
								return pszLine;

							error( ERROR_4DOS_NO_CLOSE_QUOTE, NULL );
							return BADQUOTES;
						}
					}
				}

			} else {

				// skip escaped characters
				for ( ; ; ) {

					if (( *pszLine == gpIniptr->EscChr ) && (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 )) {
						if ( *(++pszLine) != _TEXT('\0') ) {
							pszLine++;
							goto TestForQuotes;
						} else
							break;
					} else if (( *pszLine == _TEXT('%') ) && ( pszLine[1] == _TEXT('=') )) {
						if (( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 ) {
							// convert %= to default escape char
							strcpy( pszLine, pszLine + 1 );
							*pszLine = gpIniptr->EscChr;
						} else {
							pszLine += 2;
							break;
						}
					} else
						break;
				}

				if ( *pszLine == _TEXT('\0') )
					break;

				if ( pszSearch == NULL ) {

					if (( gpIniptr->Expansion & EXPAND_NO_COMPOUNDS ) == 0 ) {

						// check for pipes, compounds, or conditionals
						if (( *pszLine == _TEXT('|') ) || (( *pszLine == _TEXT('&') ) && ( _strnicmp( pszLine-1, _TEXT(" && "), 4 ) == 0 )))
							break;

						// strange kludge when using & as a command
						//   separator - this fouls up things like
						//   |& and >&
						if (( *pszLine == gpIniptr->CmdSep ) && ( pszLine[-1] != _TEXT('|') ) && ( pszLine[-1] != _TEXT('>') ))
							break;
					}

				} else if ( strchr( pszSearch, *pszLine ) != NULL ) {

					// make sure switch character has something
					//  following it (kludge for "copy *.* a:/")
					if (( *pszLine != gpIniptr->SwChr ) || ( isdelim( pszLine[1] ) == 0 ))
						break;
				}

				// check for %+ when looking for terminator
				if (( fTerm ) && ( *pszLine == _TEXT('%') ) && ( pszLine[1] == _TEXT('+') )) {

					if (( gpIniptr->Expansion & EXPAND_NO_VARIABLES ) == 0 ) {
						// FIXME - Add kludge for %var%+%var%!
						strcpy( pszLine, pszLine + 1 );
						*pszLine = gpIniptr->CmdSep;
						if (( gpIniptr->Expansion & EXPAND_NO_COMPOUNDS ) == 0 )
							break;
					}

					pszLine++;
				}
			}

			if ( *pszLine )
				pszLine++;
		}
	}

	return pszLine;
}


int PASCAL GetMultiCharSwitch( TCHAR *pszLine, TCHAR *pszSwitch, TCHAR *pszOutput, int nMaxLength )
{
	register TCHAR *pszArg;
	register int i;

	// check for and remove switches anywhere in the line
	*pszOutput = _TEXT('\0');
	for ( i = 0; (( pszArg = ntharg( pszLine, i )) != NULL ); i++ ) {

		if (( *pszArg == gpIniptr->SwChr ) && ( _ctoupper( pszArg[1] ) == (int)*pszSwitch )) {

			// save the switch argument
			sprintf( pszOutput, FMT_PREC_STR, nMaxLength, pszArg + 2 );

			// remove the switch(es)
			strcpy( gpNthptr, gpNthptr + strlen( pszArg ) );
			return 1;
		}
	}

	return 0;
}


#define SWITCH_ONLY_FIRST 1

// Scan the line for matching arguments, set the flags, and remove the switches
int PASCAL GetSwitches( LPTSTR pszLine, LPTSTR pszSwitches, long *pfFlags, int fOptions )
{
	LPTSTR pszArg;
	register int i;
	long lSwitch;

	if ( *pszSwitches == _TEXT('*') )
		gnExclusiveMode = gnInclusiveMode = gnOrMode = 0;

	// check for and remove switches anywhere in the line
	for ( i = 0, *pfFlags = 0L; (( pszArg = ntharg( pszLine, i )) != NULL ); ) {

		if ( *pszArg == gpIniptr->SwChr ) {

			if (( lSwitch = switch_arg( pszArg, pszSwitches )) == -1L )
				return ERROR_EXIT;

			if ( lSwitch == 0 ) {
				i++;
				continue;
			}

			*pfFlags |= lSwitch;

			// remove the switch(es)
			if ( fOptions & SWITCH_ONLY_FIRST )
				strcpy( gpNthptr, skipspace( gpNthptr + strlen( pszArg )));
			else
				strcpy( gpNthptr, gpNthptr + strlen( pszArg ) );

		} else if ( fOptions & SWITCH_ONLY_FIRST ) {
			// only want opts at beginning!
			break;
		} else
			i++;
	}

	return 0;
}


// Scan the line for a matching switch
int PASCAL QueryIsSwitch( LPTSTR pszLine, TCHAR cSwitch, int fAnywhere )
{
	register LPTSTR pszArg;
	register int i;

	for ( i = 0x6000; (( pszArg = ntharg( pszLine, i )) != NULL ); i++ ) {

		if ( *pszArg == gpIniptr->SwChr ) {

			if ( cSwitch == _ctoupper( pszArg[1] ))
				return 1;
		}

		// only match first argument?
		if ( fAnywhere == 0 )
			return 0;
	}

	return 0;
}


// check arg for the switch arguments - return 0 if not a switch, -1 if
//   a bad switch argument, else ORed the matching arguments (1 2 4 8 etc.)
long _fastcall switch_arg( LPTSTR pszArg, LPTSTR pszMatch )
{
	register int i;
	long lVal = 0L, lTemp;
	LPTSTR pszPtr;

	if (( pszArg != NULL ) && ( *pszArg++ == gpIniptr->SwChr )) {

		// check for /A:rhsda
		if (( _ctoupper( *pszArg ) == _TEXT('A') ) && ( pszArg[1] == _TEXT(':') ) && ( *pszMatch == _TEXT('*') )) {
			// set gnInclusiveMode and gnExclusiveMode
			GetSearchAttributes( pszArg + 1 );
			return 1;
		}

		for ( i = 0; ( pszArg[i] != _TEXT('\0') ); i++ ) {

			// skip '*' denoting /A:xxx field
			if ((( pszPtr = strchr( pszMatch, (TCHAR)_ctoupper( pszArg[i] ))) == NULL ) || ( *pszPtr == _TEXT('*') )) {

				if ( isalpha( pszArg[i] )) {
					error( ERROR_INVALID_PARAMETER, pszArg );
					return -1L;
				}

				break;

			} else {
				lTemp = (long)( pszPtr - pszMatch );
				lVal |= ( 1L << lTemp );
			}
		}
	}

	return lVal;
}


// returns 1 if the argument is numeric
int _fastcall QueryIsNumeric( LPTSTR pszNum )
{
	if (( pszNum == NULL ) || ( *pszNum == _TEXT('\0') ))
		return 0;

	// a - or + by itself doesn't constitute a numeric argument!
	if ((( *pszNum == _TEXT('-') ) || ( *pszNum == _TEXT('+') )) && ( isdigit( pszNum[1] ) == 0 ))
		return 0;

	if (( is_signed_digit( *pszNum ) != 0 ) || (( *pszNum == gaCountryInfo.szDecimal[0] ) && ( isdigit ( pszNum[1] )) && ( strchr( pszNum+1, gaCountryInfo.szDecimal[0] ) == NULL ))) {
		for ( ++pszNum; (( *pszNum != _TEXT('\0') ) && ( isdigit( *pszNum ) || ( *pszNum == gaCountryInfo.szThousandsSeparator[0] ) || ( *pszNum == gaCountryInfo.szDecimal[0] ))); pszNum++ )
			;
	}

	return ( *pszNum == _TEXT('\0') );
}


// is the filename "CON"
int _fastcall QueryIsCON( LPTSTR pszFileName )
{
	return (( _stricmp( pszFileName, CONSOLE ) == 0 ) || ( _stricmp( pszFileName, DEV_CONSOLE ) == 0 ));
}


// test if relative range spec is composed entirely of digits
static int _fastcall QueryIsRelative( LPTSTR pszRange )
{
	for ( ; ( *pszRange != _TEXT('\0') ); pszRange++ ) {
		if (( *pszRange < _TEXT('0') ) || ( *pszRange > _TEXT('9') )) {
			if (( *pszRange == _TEXT(',') ) || ( *pszRange == _TEXT('@') ) || ( *pszRange == _TEXT(']') ))
				break;
			return 0;
		}
	}

	return 1;
}


// set the date / time / size range info for file searches
// also gets any /Ixxx switches
int PASCAL GetRange( register LPTSTR pszLine, RANGES *aRanges, int fOnlyFirst )
{
	static TCHAR szExclude[MAXLINESIZ];
	static TCHAR szDescription[256];

	register LPTSTR pszArg;
	LPTSTR pSaveNthptr;
	TCHAR szBuffer[MAXLINESIZ];
	int i, fExclude = 0, nReturn = 0;

	// initialize start & end values

	szExclude[0] = _TEXT('\0');
	aRanges->pszExclude = NULL;
	aRanges->pszDescription = NULL;

	// date & inclusive time
	aRanges->DTRS.ulDTStart = 0L;
	aRanges->DTRE.ulDTEnd = (ULONG)-1L;

	// exclusive time
	aRanges->usTimeStart = 0;
	aRanges->usTimeEnd = 0xFFFF;

	// size
	aRanges->ulSizeMin = 0L;

	aRanges->ulSizeMax = (ULONG)-1L;

	// default to "last write" on LFN / NTFS
	aRanges->usDateType = aRanges->usTimeType = 0;
	if ( pszLine == NULL )
		return 0;

	// check for range switches in the line
	for ( i = 0; (( pszArg = ntharg( pszLine, i )) != NULL ); ) {

		// check for a /I"description" argument
		if (( *pszArg == gpIniptr->SwChr ) && ( _ctoupper( pszArg[1] ) == _TEXT('I') ) && ( pszArg[2] != _TEXT('\0') )) {

			szDescription[0] = _TEXT('\0');
			if ( pszArg[2] == _TEXT('"') )
				sscanf( pszArg+3, _TEXT("%255[^\"]"), szDescription );
			else
				sprintf( szDescription, FMT_PREC_STR, 255, pszArg+2 );

			aRanges->pszDescription = szDescription;

			// remove the switch(es)
			strcpy( gpNthptr, gpNthptr + strlen( pszArg ) );
			continue;
		}

		// if not a range argument, get next switch
		if (( *pszArg != gpIniptr->SwChr ) || ( pszArg[1] != _TEXT('[') )) {
			if ( fOnlyFirst )
				return 0;
			i++;
			continue;
		}

		// get start & end of switch
		pSaveNthptr = pszArg = gpNthptr + 1;

		// get start & end of switch
		if (( pszArg = scan( pszArg, _TEXT("]"), _TEXT("`\"[") )) == BADQUOTES )
			return ERROR_EXIT;
		if ( *pszArg == _TEXT(']') )
			*pszArg++ = _TEXT('\0');

		// check for filename exclusions
		pSaveNthptr++;
		if ( *pSaveNthptr == _TEXT('!') ) {
			fExclude = 1;
			pSaveNthptr++;
		} else
			fExclude = 0;

		pSaveNthptr = strcpy( gpNthptr, pSaveNthptr );

		// kludge to allow variable expansion on range arguments
		//   for commands like EXCEPT, FOR, and SELECT
		strcpy( szBuffer, pSaveNthptr );

		if ( var_expand( szBuffer, 1 ) != 0 )
			return ERROR_EXIT;

		if ( fExclude ) {

			if ( szExclude[0] != _TEXT('\0') )
				return ( error( ERROR_4DOS_ALREADYEXCLUDED, szBuffer ));
			sprintf( szExclude, FMT_PREC_STR, MAXLINESIZ - 1, szBuffer );
			aRanges->pszExclude = szExclude;

		} else if (( nReturn = GetRangeArgs( szBuffer, aRanges )) != 0 )
			return ( error( nReturn, pszLine ));

		// remove the switch
		if ( fOnlyFirst )
			pszArg = skipspace( pszArg );
		strcpy( pSaveNthptr, pszArg );
	}

	return 0;
}



#pragma alloc_text( ASM_TEXT, NextRange )


// skip to the next range argument
static LPTSTR _fastcall NextRange( LPTSTR pszArg )
{
	while (( *pszArg ) && ( *pszArg != _TEXT(']') ) && ( *pszArg++ != _TEXT(',') ))
		;

	return ( skipspace( pszArg ));
}



#pragma alloc_text( ASM_TEXT, GetRangeArgs )


static int _fastcall GetRangeArgs( LPTSTR pszRange, RANGES *aRanges )
{
	TCHAR cChar;
	BOOL bReversed = 0, bStartTime = 0;
	int nHours, nMinutes;
	unsigned int uYear, uMonth, uDay;
	long lStart, lEnd, lOffset;
	DATETIME sysDateTime;

	long lSize;


	switch ( _ctoupper( *pszRange++ )) {
	case 'D':		// date range
		// check for Last Access or Created request
		if (( cChar = _ctoupper( *pszRange )) == _TEXT('A') ) {
			aRanges->usDateType = 1;
			pszRange++;
		} else if ( cChar == _TEXT('C') ) {
			aRanges->usDateType = 2;
			pszRange++;
		} else if ( cChar == _TEXT('W') )
			pszRange++;

		// get current date & time
		MakeDaysFromDate( &lStart, NULLSTR );
		lEnd = lStart;

		// get first arg
		if ( is_signed_digit( *pszRange )) {

			if ( MakeDaysFromDate( &lOffset, pszRange ) != 0 )
				return ERROR_4DOS_INVALID_DATE;

			if ( *pszRange == _TEXT('-') )
				lStart += lOffset;
			else if ( *pszRange == _TEXT('+') )
				lEnd += lOffset;
			else
				lStart = lOffset;
		}

		// skip past separator
		while (( *pszRange ) && ( *pszRange != _TEXT(']') )) {

			// get time spec
			if ( *pszRange == _TEXT('@') ) {
				if ( GetStrTime( ++pszRange, &nHours, &nMinutes ) != 0 )
					return ERROR_4DOS_INVALID_TIME;
				aRanges->DTRS.DTS.usTStart = (unsigned int)(( nHours << 11 ) + ( nMinutes << 5 ));
				bStartTime = 1;
			}

			if ( *pszRange++ == _TEXT(',') )
				break;
		}

		pszRange = skipspace( pszRange );

		// get second arg
		if ( is_signed_digit( *pszRange )) {

			if ( MakeDaysFromDate( &lOffset, pszRange ) != 0 )
				return ERROR_4DOS_INVALID_DATE;

			// reversed arguments?
			if ( *pszRange == _TEXT('-') ) {
				lEnd = lStart;
				lStart += lOffset;
				bReversed = 1;
			} else if ( *pszRange == _TEXT('+') )
				lEnd = lStart + lOffset;
			else if (( lEnd = lOffset ) < lStart ) {
				lEnd = lStart;
				lStart = lOffset;
				bReversed = 1;
			}
		}

		// skip past date
		while (( *pszRange ) && ( *pszRange != _TEXT(']') )) {

			// get time spec
			if ( *pszRange++ == _TEXT('@') ) {
				if ( GetStrTime( pszRange, &nHours, &nMinutes ) != 0 )
					return ERROR_4DOS_INVALID_TIME;
				// reverse start/end time?
				if ( bReversed ) {
					// if we had a start time, it should become the end time
					if ( bStartTime )
						aRanges->DTRE.DTE.usTEnd = aRanges->DTRS.DTS.usTStart;
					aRanges->DTRS.DTS.usTStart = (unsigned int)(( nHours << 11 ) + ( nMinutes << 5 ));
				} else
					aRanges->DTRE.DTE.usTEnd = (unsigned int)(( nHours << 11 ) + ( nMinutes << 5 ));
			}
		}

		MakeDateFromDays( lStart, &uYear, &uMonth, &uDay );
		aRanges->DTRS.DTS.usDStart = (( uYear - 80 ) << 9 ) + ( uMonth << 5 ) + uDay;

		MakeDateFromDays( lEnd, &uYear, &uMonth, &uDay );
		aRanges->DTRE.DTE.usDEnd = (( uYear - 80 ) << 9 ) + ( uMonth << 5 ) + uDay;

		break;

	case 'S':		// size range
		// get first arg
		GetStrSize( pszRange, &lSize );
		aRanges->ulSizeMin = lSize;

		// skip past separator
		pszRange = NextRange( pszRange );

		// get second arg
		if ( is_signed_digit( *pszRange )) {

			GetStrSize( pszRange, &lSize );

			if ( *pszRange == _TEXT('-') ) {
				aRanges->ulSizeMax = aRanges->ulSizeMin;
				aRanges->ulSizeMin += lSize;
			} else if ( *pszRange == _TEXT('+') )
				aRanges->ulSizeMax = aRanges->ulSizeMin + lSize;
			else
				aRanges->ulSizeMax = lSize;

			if ( aRanges->ulSizeMin > aRanges->ulSizeMax ) {
				lSize = aRanges->ulSizeMax;
				aRanges->ulSizeMax = aRanges->ulSizeMin;
				aRanges->ulSizeMin = lSize;
			}
		}

		break;

	case 'T':		// time range
		// check for Last Access or Created request
		if (( cChar = _ctoupper( *pszRange )) == _TEXT('A') ) {
			aRanges->usTimeType = 1;
			pszRange++;
		} else if ( cChar == _TEXT('C') ) {
			aRanges->usTimeType = 2;
			pszRange++;
		} else if ( cChar == _TEXT('W') )
			pszRange++;

		// get current date & time
		QueryDateTime( &sysDateTime );
		lStart = ( sysDateTime.hours * 60 ) + sysDateTime.minutes;
		lEnd = lStart;

		// get first arg
		if ( GetStrTime( pszRange, &nHours, &nMinutes ) != 0 )
			return ERROR_4DOS_INVALID_TIME;

		if ( *pszRange == _TEXT('-') )
			lStart += (long)nHours;
		else if ( *pszRange == _TEXT('+') )
			lEnd += (long)nHours;
		else
			lStart = ( nHours * 60 ) + nMinutes;

		// skip past separator
		pszRange = NextRange( pszRange );

		// get second arg
		if ( is_signed_digit( *pszRange )) {

			if ( GetStrTime( pszRange, &nHours, &nMinutes ) != 0 )
				return ERROR_4DOS_INVALID_TIME;

			if ( *pszRange == _TEXT('-') ) {
				lEnd = lStart;
				lStart += (long)nHours;
			} else if ( *pszRange == _TEXT('+') )
				lEnd = lStart + (long)nHours;
			else if (( lEnd = ( nHours * 60 ) + nMinutes ) < lStart ) {
				lOffset = lEnd;
				lEnd = lStart;
				lStart = lOffset;
			}
		}

		// check for lStart < 0 or lEnd > 23:59, & adjust time
		if ( lStart < 0L )
			lStart += 1440L;
		if ( lEnd >= 1440L )
			lEnd -= 1440L;

		aRanges->usTimeStart = (unsigned int)((( lStart / 60 ) << 11) + (( lStart % 60 ) << 5));
		aRanges->usTimeEnd = (unsigned int)((( lEnd / 60 ) << 11) + (( lEnd % 60 ) << 5));
	}

	return 0;
}



#pragma alloc_text( ASM_TEXT, GetStrTime )


// get time
static int _fastcall GetStrTime( LPTSTR pszTime, register int *pnHours, int *pnMinutes )
{
	TCHAR szAmPm[2];

	*pnHours = *pnMinutes = 0;
	szAmPm[0] = _TEXT('\0');

	if ( is_signed_digit( *pszTime ) == 0 )
		return ERROR_EXIT;

	if (( *pszTime == _TEXT('+') ) || ( *pszTime == _TEXT('-') ) || ( QueryIsRelative( pszTime ))) {

		*pnHours = atoi( pszTime );
		if ( is_unsigned_digit( *pszTime ))
			*pszTime = _TEXT('+');

	} else if ( sscanf( pszTime, _TEXT("%u%*1s%u%*u %1[APap]"), pnHours, pnMinutes, szAmPm ) >= 1 ) {

		// check for AM/PM syntax
		if ( *szAmPm != _TEXT('\0') ) {
			if (( *pnHours == 12 ) && ( _ctoupper( *szAmPm ) == _TEXT('A') ))
				*pnHours -= 12;
			else if ((_ctoupper( *szAmPm ) == _TEXT('P') ) && ( *pnHours < 12 ) )
				*pnHours += 12;
		}
	}

	return 0;
}


#pragma alloc_text( ASM_TEXT, GetStrSize )


// get size

static void GetStrSize( register char *pszSize, long *plSize )

{
	*plSize = 0L;
	sscanf( pszSize, FMT_LONG, plSize );
	while ( is_signed_digit( *pszSize ))
		pszSize++;

	// check for Kb or Mb in size range
	if ( *pszSize == _TEXT('k') )
		*plSize *= 1000L;
	else if ( *pszSize == _TEXT('K') )
		*plSize <<= 10;
	else if ( *pszSize == _TEXT('m') )
		*plSize *= 1000000L;
	else if ( *pszSize == _TEXT('M') )
		*plSize <<= 20;
}



#pragma alloc_text( ASM_TEXT, GetStrDate )


// return the date from a string
int GetStrDate( register LPTSTR pszDate, unsigned int *puMonth, unsigned int *puDay, unsigned int *puYear )
{
	register int nReturn;

	// Japan or international format
	nReturn = sscanf( pszDate, DATE_FMT, puYear, puMonth, puDay );

	if (( *puYear < 1900 ) && ( gaCountryInfo.fsDateFmt != 2 )) {

		if ( gaCountryInfo.fsDateFmt == 1 )	// Europe
			nReturn = sscanf( pszDate, DATE_FMT, puDay, puMonth, puYear );
		else 		// USA
			nReturn = sscanf( pszDate, DATE_FMT, puMonth, puDay, puYear );
	}

	return nReturn;
}


// get the number of days since 1/1/80 for the specified date
int _fastcall MakeDaysFromDate( long *plDays, LPTSTR pszDate )
{
	register unsigned int i;
	unsigned int uYear = 80, uMonth = 1, uDay = 1;
	int nReturn = 0;
	DATETIME sysDateTime;

	*plDays = 0L;

	// if arg == NULLSTR, use current date
	if ( *pszDate == _TEXT('\0') ) {

		QueryDateTime( &sysDateTime );
		uYear = sysDateTime.year;
		uMonth = sysDateTime.month;
		uDay = sysDateTime.day;
		nReturn = 3;

	} else if (( *pszDate == _TEXT('+') ) || ( *pszDate == _TEXT('-') ) || ( QueryIsRelative( pszDate ))) {

		sscanf( pszDate, FMT_LONG, plDays );

		// if it's not a date spec, force it to be a "+ relative" spec
		if ( is_unsigned_digit( *pszDate ))
			*pszDate = _TEXT('+');
		return 0;

	} else
		nReturn = GetStrDate( pszDate, &uMonth, &uDay, &uYear );

	if ( uYear < 80 )
		uYear += 2000;
	else if ( uYear < 100 )
		uYear += 1900;

	// don't allow anything before 1/1/1980
	if (( nReturn < 3 ) || ( uMonth > 12 ) || ( uDay > 31 ) || ( uYear > 2099 ) || ( uYear < 1980 ))
		return ERROR_4DOS_INVALID_DATE;

	// get days for previous years
	for ( i = 1980; ( i < uYear ); i++ )
		*plDays += ((( i % 4 ) == 0 ) ? 366 : 365 );

	// get days for previous months
	for ( i = 1; ( i < uMonth ); i++ ) {
		if ( i == 2 )
			*plDays += ((( uYear % 4 ) == 0 ) ? 29 : 28 );
		else
			*plDays += ((( i == 4 ) || ( i == 6 ) || ( i == 9 ) || (i == 11)) ? 30 : 31);
	}

	*plDays += ( uDay - 1 );

	return 0;
}


// get a date from the number of days since 1/1/80
int MakeDateFromDays( long lDays, unsigned int *puYear, register unsigned int *puMonth, register unsigned int *puDay )
{
	// don't allow anything past 12/31/2099
	if (( lDays > 43829L ) || ( lDays < 0L ))
		return ERROR_INVALID_PARAMETER;

	for ( *puYear = 80; ; (*puYear)++ ) {

		for ( *puMonth = 1; (*puMonth <= 12); (*puMonth)++ ) {

			if (*puMonth == 2)
				*puDay = (((*puYear % 4) == 0 ) ? 29 : 28);
			else
				*puDay = (((*puMonth == 4) || (*puMonth == 6) || (*puMonth == 9) || (*puMonth == 11)) ? 30 : 31);

			if ( (LONG)*puDay <= lDays ) 
				lDays -= *puDay;
			else {
				*puDay = (unsigned int)( lDays + 1 );
				return 0;
			}
		}
	}
}


// remove whitespace on both sides of the token
void _fastcall collapse_whitespace( register LPTSTR pszArg, LPTSTR pszToken )
{
	// collapse any whitespace around the token
	for ( ; ; ) {

		if (( pszArg == NULL ) || (( pszArg = scan( pszArg, pszToken, QUOTES )) == BADQUOTES ) || ( *pszArg == _TEXT('\0') ))
			return;

		// collapse leading whitespace
		while ( iswhite( pszArg[-1] )) {
			pszArg--;
			strcpy( pszArg, pszArg+1 );
		}

		// collapse following whitespace
		pszArg++;
		while ( iswhite( *pszArg ))
			strcpy( pszArg, pszArg+1 );
	}
}


// strip the specified leading characters
void _fastcall strip_leading( LPTSTR pszArg, LPTSTR pszDelimiters )
{
	while (( *pszArg != _TEXT('\0') ) && ( strchr( pszDelimiters, *pszArg ) != NULL ))
		strcpy( pszArg, pszArg+1 );
}


// strip the specified trailing characters
void _fastcall strip_trailing( LPTSTR pszString, LPTSTR pszDelimiters )
{
	register int i;

	for ( i = strlen( pszString ); ((--i >= 0 ) && ( strchr( pszDelimiters, pszString[i] ) != NULL )); )
		pszString[i] = _TEXT('\0');
}


// strip the specified leading & trailing characters
void _fastcall trim( register LPTSTR pszString, LPTSTR pszDelimiters )
{
	strip_leading( pszString, pszDelimiters );
	strip_trailing( pszString, pszDelimiters );
}


// look for TMP / TEMP / TEMP4DOS disk area defined in environment
TCHAR _far * _fastcall GetTempDirectory( LPTSTR pszName )
{

	char _far *pszTEMP;

	if ((( pszTEMP = get_variable( TEMP4DOS_DISK )) != 0L ) || (( pszTEMP = get_variable( TEMP_DISK )) != 0L ) || (( pszTEMP = get_variable( TMP_DISK )) != 0L )) {

		_fstrcpy( pszName, pszTEMP );
		if ( is_dir( pszName ) == 0 )
			pszTEMP = 0L;
	}

	return pszTEMP;
}

// check to see if user wants filenames in upper case
LPTSTR _fastcall filecase( LPTSTR pszFileName )
{
	// NTFS preserves case in filenames, so just leave it alone
	if ( ifs_type( pszFileName ) != 0 )
		return pszFileName;

	return (( gpIniptr->Upper == 0 ) ? strlwr( pszFileName ) : strupr( pszFileName ));
}


void _fastcall SetDriveString( LPTSTR pszDrives )
{
	register int i, n;

	// get all of the valid hard disks from C -> Z
	wcmemset( pszDrives, _TEXT('\0'), 30 );
	for ( i = 0, n = 3; ( n <= 26 ); n++ ) {
		pszDrives[i] = (TCHAR)( n + 64 );
		if (( QueryDriveExists( n )) && ( QueryDriveReady( n )) && ( QueryIsCDROM( pszDrives+i ) == 0 ) && ( QueryDriveRemote( n ) == 0 ) && ( QueryDriveRemovable( n ) == 0 ))
			i++;
	}

	pszDrives[i] = _TEXT('\0');
}


// get current directory, with leading drive specifier (or NULL if error)
LPTSTR _fastcall gcdir( LPTSTR pszDrive, int fNoError )
{
	static TCHAR szCurrent[MAXFILENAME+1];
	int nDisk, nReturn;

	nDisk = gcdisk( pszDrive );
	sprintf( szCurrent, FMT_ROOT, nDisk+64 );

	nReturn = (unsigned int)(szCurrent + 3);

	_asm {
		push	si

			mov	dx, nDisk
			mov	si, nReturn
			mov	bx, gpIniptr	  ; skip Win95 call if disabled
			cmp	[bx].Win95LFN,0
			je	gcd_old
			mov	ax, 7147h	  ; fcn 7147: get current directory (long name version)
			stc				 ; set carry in case not supported
			int	21h
			jnc	success		  ; skip if call succeed in long-name form

			cmp	ax, 7100h	  ; check for bona fide error
			jne	error_exit	  ; skip if so

gcd_old:
		mov	ah,047h		  ; reissue as 8.3 call
			int	21h
			jc	error_exit
success:
		xor	ax,ax
error_exit:
		mov	nReturn,ax

			pop	si
	}

	GetLongName( szCurrent );



	if ( nReturn ) {

		if ( fNoError == 0 ) {
			// bad drive - format error message & return NULL
			if ( nDisk ) {
				sprintf( szCurrent, FMT_DISK, nDisk+64 );
				error( nReturn, szCurrent );
			} else
				error( nReturn, NULL );
		}

		return NULL;
	}

	return ( filecase( szCurrent ));
}


// return the specified (or default) drive as 1=A, 2=B, etc.
int _fastcall gcdisk( LPTSTR pszDrive )
{
	if (( pszDrive != NULL ) && ( pszDrive[1] == _TEXT(':') ))
		return ( _ctoupper( *pszDrive ) - 64);

	// we have to save the current disk in DOS to keep Netware from
	//   closing all the files on the system!
	return gnCurrentDisk;

}


// return 1 if the specified name is "." or ".."
int _fastcall QueryIsDotName( LPTSTR pszFileName )
{
	return (( *pszFileName == _TEXT('.') ) && (( pszFileName[1] == _TEXT('\0') ) || (( pszFileName[1] == _TEXT('.') ) && ( pszFileName[2] == _TEXT('\0') ))));
}


// return the path stripped of the filename (or NULL if no path)
LPTSTR _fastcall path_part( LPTSTR pszName )
{
	static TCHAR szBuffer[MAXFILENAME+1];

	copy_filename( szBuffer, pszName );
	StripQuotes( szBuffer );

	// search path backwards for beginning of filename
	for ( pszName = strend( szBuffer ); ( --pszName >= szBuffer ); ) {

		// special case for device name; i.e. "d:\path\con:"
		if ( *pszName == _TEXT(':') ) {
			LPTSTR pszArg;
			if ((( pszArg = strpbrk( szBuffer, _TEXT("/\\") )) != NULL ) && ( pszArg < pszName ))
				continue;
		}

		// accept either forward or backslashes as path delimiters
		if (( *pszName == _TEXT('\\') ) || ( *pszName == _TEXT('/') ) || ( *pszName == _TEXT(':') )) {
			// take care of arguments like "d:.." & "..\.."
			if ( _stricmp( pszName+1, _TEXT("..") ) != 0 )
				pszName[1] = _TEXT('\0');
			return szBuffer;
		}
	}

	return NULL;
}


// return the filename stripped of path & disk spec
LPTSTR _fastcall fname_part( LPTSTR pszFileName )
{
	static TCHAR szBuffer[MAXFILENAME+1];

	register int i;
	int nLength = 0, nMax = 8, fWildCard = 0;

	register LPTSTR pszName;

	// search path backwards for beginning of filename
	for ( pszName = strend( pszFileName ); ( --pszName >= pszFileName ); ) {

		// special case for device name; i.e. "d:\path\con:"
		if ( *pszName == _TEXT(':') ) {
			register LPTSTR pszArg;
			if ((( pszArg = strpbrk( pszFileName, _TEXT("/\\") )) != NULL ) && ( pszArg < pszName ))
				continue;
		}

		// accept either forward or backslashes as path delimiters
		if (( *pszName == _TEXT('\\') ) || ( *pszName == _TEXT('/') ) || ( *pszName == _TEXT(':') )) {

			// take care of arguments like "d:.." & "..\.."
			if ( _stricmp( pszName+1, _TEXT("..") ) == 0 )
				pszName += 2;
			break;
		}
	}

	// step past the delimiter char
	pszName++;

	// allow LFNs
	if ( ifs_type( pszFileName ) != 0 )
		copy_filename( szBuffer, pszName );

	else {

		// only allow 8 chars for filename and 3 for extension
		//   (but don't count * and [] wildcards)
		for ( i = 0; ((*pszName) && (*pszName != _TEXT(';') ) && (i < (MAXFILENAME-1))); pszName++ ) {

			szBuffer[i++] = *pszName;

			if ( fWildCard == 0 ) {

				if ( *pszName == _TEXT('.') ) {
					nMax = 3;
					nLength = 0;

				} else if ( *pszName != _TEXT('*') ) {

					if ( nLength < nMax ) {
						nLength++;

						// start of a regular expression?
						if ( *pszName == _TEXT('[') )
							fWildCard = 1;
					} else
						i--;
				}

			} else if ( *pszName == _TEXT(']') ) {
				// end of regular expression
				fWildCard = 0;
			}
		}

		szBuffer[i] = _TEXT('\0');
	}

	StripQuotes( szBuffer );
	return szBuffer;
}


// return the file extension only
LPTSTR _fastcall ext_part( LPTSTR pszFileName )
{
	static TCHAR szBuffer[65];
	LPTSTR pszExtension;

	// make sure extension is for filename, not a directory name
	if (( pszFileName == NULL ) || (( pszExtension = strrchr( pszFileName, _TEXT('.') )) == NULL ) || ( strpbrk( pszExtension, _TEXT("\\/:") ) != NULL ))
		return NULL;

	// don't read the next element in an include list

	if ( fWin95LFN == 0 )
		sscanf( pszExtension, "%4[^;\"]", szBuffer );
	else
		sscanf( pszExtension, _TEXT("%64[^;\"]"), szBuffer );

	return szBuffer;
}



#pragma alloc_text( _TEXT, copy_filename )


// copy a filename, max of 260/2047 characters
void _fastcall copy_filename( LPTSTR pszTarget, LPTSTR pszSource )
{
	sprintf( pszTarget, FMT_PREC_STR, MAXFILENAME-1, pszSource );
}


// check for LFN/NTFS long filenames delineated by double quotes
//   and strip leading / trailing "
void _fastcall StripQuotes( LPTSTR pszFileName )
{
	LPTSTR pszQuote;

	while (( pszQuote = strchr( pszFileName, _TEXT('"') )) != NULL )
		strcpy( pszQuote, pszQuote + 1 );

	// remove trailing whitespace
	strip_trailing( pszFileName, WHITESPACE );
}


void _fastcall AddCommas( register LPTSTR pszNumber )
{
	register LPTSTR pszThousands;

	// format a long integer by inserting commas (or other
	// character specified by country_info.szThousandsSeparator)
	if (( *pszNumber == _TEXT('-') ) || ( *pszNumber == _TEXT('+') ))
		pszNumber++;
	for ( pszThousands = pszNumber; (( isdigit( *pszThousands )) && ( *pszThousands != _TEXT('\0') ) && ( *pszThousands != gaCountryInfo.szDecimal[0] )); pszThousands++ )
		;
	while (( pszThousands -= 3 ) > pszNumber )
		strins( pszThousands, gaCountryInfo.szThousandsSeparator );
}


int _fastcall AddQuotes( LPTSTR pszFileName )
{
	// adjust for an LFN name with embedded whitespace

	if (( *pszFileName != _TEXT('"') ) && ( strpbrk( pszFileName, " \t,=+<>|`" ) != NULL )) {
		strins( pszFileName, _TEXT("\"") );
		strcat( pszFileName, _TEXT("\"") );
		return 1;
	}

	return 0;
}


#pragma alloc_text( _TEXT, mkdirname )


// make a file name from a directory name by appending '\' (if necessary)
//   and then appending the filename
int _fastcall mkdirname( LPTSTR pszDirectoryName, LPTSTR pszFileName )
{
	register int nLength;

	nLength = strlen( pszDirectoryName );

	if (( nLength + strlen( pszFileName )) >= ( MAXFILENAME - 2 ))
		return ERROR_EXIT;

	if ( *pszDirectoryName ) {

		if (( strchr( _TEXT("/\\:"), pszDirectoryName[nLength-1] ) == NULL ) && ( *pszFileName != _TEXT('\\') ) && ( *pszFileName != _TEXT('/') )) {
			strcat( pszDirectoryName, _TEXT("\\") );
			nLength++;
		}
	}

	sprintf( strend( pszDirectoryName ), FMT_PREC_STR, (( MAXFILENAME - 1 ) - nLength ), pszFileName );

	return 0;
}


// make a full file name including disk drive & path
LPTSTR _fastcall mkfname( LPTSTR pszFileName, int fFlags )
{

	LPTSTR pszArg;
	TCHAR szTemp[MAXFILENAME+1], *pszSource, *pszCurrentDir = NULLSTR, *pszName;
	unsigned int uMode;

	if ( pszFileName != NULL )
		StripQuotes( pszFileName );

	if (( pszFileName == NULL ) || ( *pszFileName == _TEXT('\0') )) {
		if (( fFlags & MKFNAME_NOERROR ) == 0 )
			error( ERROR_FILE_NOT_FOUND, NULLSTR );

		return NULL;
	}

	// skip filename list spec
	pszName = pszFileName;
	if ( *pszName == _TEXT('@') ) {
		// if the file exists with the @ prefix, leave it alone!
		if ( QueryFileMode( pszName, &uMode ) != 0 )
			pszName++;
	}

	// check for clipboard pseudo-device
	if ( stricmp( pszName, CLIP ) == 0 )
		return pszFileName;

	pszSource = pszName;

	// check for network server names (Novell Netware or Lan Manager)
	//   or a named pipe ("\pipe\name...")

	if (( is_net_drive( pszName )) || (( _osmajor >= 10 ) && ( QueryIsPipeName( pszName )))) {

		// don't do anything if it's an oddball network name - just
		//   return the filename, shifted to upper or lower case
		return ( filecase( pszFileName ));
	}

	// get the current directory, & save to the temp buffer
	if (( pszCurrentDir = gcdir( pszName, ( fFlags & MKFNAME_NOERROR ))) == NULL )
		return NULL;

	copy_filename( szTemp, pszCurrentDir );

	// skip the disk specification
	if (( *pszName ) && ( pszName[1] == _TEXT(':') ))
		pszName += 2;
	// if it's an include list, keep the '@' in its original location!
	else if (( pszName != pszFileName ) && (( strpbrk( pszName, _TEXT("/\\") )) == NULL )) {
		pszName--;
		pszSource--;
	}

	// skip the disk spec for the default directory
	if (( szTemp[0] == _TEXT('\\') ) && ( szTemp[1] == _TEXT('\\') )) {

		// if it's a UNC name, skip the machine & sharename
		pszCurrentDir = strend( szTemp );
		if (( pszArg = strchr( szTemp+2, _TEXT('\\') )) != NULL ) {
			if (( pszArg = strchr( pszArg + 1, _TEXT('\\') )) != NULL )
				pszCurrentDir = pszArg + 1;
		}
	} else
		pszCurrentDir = szTemp + 3;

	// if filespec defined from root, there's no default pathname
	if (( *pszName == _TEXT('\\') ) || ( *pszName == _TEXT('/') )) {
		pszName++;
		*pszCurrentDir = _TEXT('\0');
	}

	// handle Netware-like CD changes (..., ...., etc.)
	while (( pszArg = strstr( pszName, _TEXT("...") )) != NULL ) {

		register LPTSTR ptr2;

		// LFN drives allow names like "abc...def"
		if ( ifs_type( szTemp ) != 0 ) {

			// check start
			if (( pszArg > pszName ) && ( pszArg[-1] != _TEXT('/') ) && ( pszArg[-1] != _TEXT('\\') ))
				break;

			// check end
			for ( ptr2 = pszArg; ( *ptr2 == _TEXT('.') ); ptr2++ )
				;
			if (( *ptr2 != _TEXT('\0') ) && ( *ptr2 != _TEXT('\\') ) && ( *ptr2 != _TEXT('/') ))
				break;
		}

		// make sure we're not over the max length
		if (( strlen( pszName ) + 4 ) >= MAXFILENAME ) {
Mkfname_Error:
			if (( fFlags & MKFNAME_NOERROR ) == 0 )
				error( ERROR_FILE_NOT_FOUND, pszName );
			return NULL;
		}

		strins( pszArg+2, _TEXT("\\.") );
	}

	while (( pszName != NULL ) && ( *pszName )) {

		// don't strip trailing \ in directory name
		if (( pszArg = strpbrk( pszName, _TEXT("/\\") )) != NULL ) {
			if (( pszArg[1] != _TEXT('\0') ) || ( pszArg[-1] == _TEXT('.') ))
				*pszArg = _TEXT('\0');
			pszArg++;
		}

		// handle references to parent directory ("..")
		if ( _stricmp( pszName, _TEXT("..") ) == 0 ) {

			// don't delete root directory!
			if ((( pszName = strrchr( pszCurrentDir, _TEXT('\\') )) == NULL ) && (( pszName = strrchr( pszCurrentDir, _TEXT('/') )) == NULL ))
				pszName = pszCurrentDir;
			*pszName = _TEXT('\0');

		} else if ( _stricmp( pszName, _TEXT(".") ) != 0 ) {
			// append the directory or filename to the temp buffer
			if ( mkdirname( szTemp, pszName ))
				goto Mkfname_Error;
		}

		pszName = pszArg;
	}
	copy_filename( pszSource, szTemp );

	// return expanded filename, shifted to upper or lower case
	return ( filecase( pszFileName ));
}


// make a filename with the supplied path & filename
void PASCAL insert_path( LPTSTR pszTarget, LPTSTR pszSource, LPTSTR pszPath )
{
	// get path part first in case "path" and / or "source" are the same
	//   arg as "target"
	pszPath = path_part( pszPath );
	copy_filename( pszTarget, pszSource );

	if ( pszPath != NULL )
		strins( pszTarget, pszPath );
}


// return non-zero if the specified file exists
int _fastcall is_file( LPTSTR pszFileName )
{
	TCHAR szName[MAXFILENAME+1];
	FILESEARCH dir;

	copy_filename( szName, pszFileName );

	// check for valid drive, then expand filename to catch things
	//   like "\4dos\." and "....\wonky\*.*"
	if ( mkfname( szName, MKFNAME_NOERROR ) != NULL ) {
		if ( find_file( FIND_FIRST, szName, 0x07 | FIND_NO_ERRORS | FIND_CLOSE | FIND_NO_FILELISTS, &dir, NULL ) != NULL )
			return 1;
	}

	return 0;
}


// returns 1 if it's a directory, 0 otherwise
int _fastcall is_dir( LPTSTR pszFileName )
{
	register LPTSTR pszArg;
	TCHAR szDirName[MAXFILENAME+1];
	FILESEARCH dir;

	pszArg = pszFileName;

	// check to see if the drive exists
	if ( pszFileName[1] == _TEXT(':') ) {

		// Another Netware kludge:  skip past "d:" when scanning for
		//   wildcards to allow for wacko drive names like ]:
		pszArg += 2;

		if ( QueryDriveExists( gcdisk( pszFileName )) == 0 )
			return 0;
	}

	// it can't be a directory if it has wildcards or include list
	if ( strpbrk( pszArg, _TEXT("*?") ) != NULL )
		return 0;

	if ((( pszArg = scan( pszArg, _TEXT(";"), _TEXT("\"") )) != BADQUOTES ) && ( *pszArg == _TEXT('\0') )) {

		// build a fully-qualified path name (& fixup Netware stuff)
		copy_filename( szDirName, pszFileName );
		if (( mkfname( szDirName, MKFNAME_NOERROR ) == NULL ) || ( szDirName[0] == _TEXT('\0') ))
			return 0;

		// "d:", "d:\" and "d:/" are assumed valid directories
		// Novell names like "SYS:" are also assumed valid directories
		if (( _stricmp( szDirName+1, _TEXT(":") ) == 0 ) || ( _stricmp( szDirName+1, _TEXT(":\\") ) == 0 ) || ( _stricmp( szDirName+1, _TEXT(":/") ) == 0 ))
			return 1;

		// don't treat something like "@con:" as a directory!
		if (( szDirName[0] == _TEXT('@') ) && ( QueryIsDevice( szDirName+1 )))
			return 0;

		if ((( pszArg = strchr( szDirName+2, _TEXT(':') )) != NULL ) &&
			( pszArg[1] == _TEXT('\0') ) && ((unsigned int)( pszArg - szDirName ) < 6 ) &&
			( strpbrk( szDirName, _TEXT(" ,\t=") ) == NULL ) &&
			( QueryIsDevice( szDirName ) == 0 ))
			return 1;

		// try searching for it & see if it exists

		// remove a trailing "\" or "/"
		strip_trailing( szDirName, SLASHES );


		if ( find_file( FIND_FIRST, szDirName, 0x17 | FIND_CLOSE | FIND_ONLY_DIRS | FIND_NO_ERRORS | FIND_EXACTLY | FIND_NO_FILELISTS, &dir, NULL ) != NULL )
			return (( dir.attrib & 0x10 ) ? 1 : 0 );

		// kludge for "\\server\dir" not working in NT & Netware
		if (( szDirName[0] == _TEXT('\\') ) && ( szDirName[1] == _TEXT('\\') )) {

			// see if there are any subdirectories or files
			mkdirname( szDirName, WILD_FILE );

			if ( find_file( FIND_FIRST, szDirName, 0x17 | FIND_CLOSE | FIND_NO_ERRORS | FIND_EXACTLY | FIND_NO_FILELISTS, &dir, NULL ) != NULL )
				return 1;

		}

		else {
			// silly kludge for JOIN bug in DOS
			mkdirname( szDirName, WILD_FILE );
			if ( find_file( FIND_FIRST, szDirName, 0x17 | FIND_CLOSE | FIND_ONLY_DIRS | FIND_NO_ERRORS | FIND_EXACTLY | FIND_NO_FILELISTS, &dir, NULL ) != NULL )
				return (( dir.attrib & 0x10 ) ? 1 : 0 );
		}

	}

	return 0;
}


// Append * (or *.*) if it's a directory
int _fastcall AddWildcardIfDirectory( LPTSTR pszDirectory )
{

		if ( is_dir( pszDirectory )) {
			mkdirname( pszDirectory, WILD_FILE );
			return 1;
		}


	return 0;
}


// check for network server names (Novell Netware or Lan Manager)
//   (like "\\server\blahblah" or "sys:blahblah")
int _fastcall is_net_drive( LPTSTR pszFileName )
{
	LPTSTR pszStream;


	if (( pszFileName[0] == _TEXT('\\') ) && ( pszFileName[1] == _TEXT('\\') ))
		return TRUE;

	// sys:blahblah could be a file stream
	// but not if it's preceded by a path or extension!
	if (( *pszFileName != _TEXT('@') ) && ( pszFileName[1] != _TEXT('\0')) && (( pszStream = strpbrk( pszFileName+2, _TEXT("\\/.:") )) != NULL ))
		return ( *pszStream == _TEXT(':') );

	return FALSE;
}


// check for user defined executable extension
//   (kinda kludgy in order to support wildcards)
TCHAR _far * _fastcall executable_ext( LPTSTR pszExtension )
{
	TCHAR _far *pszEnv;

	for ( pszExtension++, pszEnv = glpEnvironment; ; pszEnv = next_env( pszEnv )) {

		if ( *pszEnv == _TEXT('\0') )
			break;

NextExtension:
		if ( *pszEnv++ == _TEXT('.') ) {

			if ( wild_cmp( pszEnv, (TCHAR _far *)pszExtension, TRUE, TRUE ) == 0 ) {

				// get the argument
				while (( *pszEnv ) && ( *pszEnv++ != _TEXT('=') ))
					;
				break;
			}

			// look for another extension
			while (( *pszEnv ) && ( *pszEnv != _TEXT(';') ) && ( *pszEnv++ != _TEXT('=') ))
				;
			if ( *pszEnv == _TEXT(';') ) {
				pszEnv++;
				goto NextExtension;
			}
		}
	}

	return pszEnv;
}


// Compare filenames for wildcard matches
//   Returns 0 for match; <> 0 for no match
//   *s  matches any collection of characters in the string
//	   up to (but not including) the s.
//    ?  matches any single character in the other string.
//   [!abc-m] match a character to the set in the brackets; ! means reverse
//	   the test; - means match if included in the range.
int wild_cmp( TCHAR _far *fpWildName, TCHAR _far *fpFileName, int fExtension, int fWildBrackets )
{
	register int fWildStar = 0;
	TCHAR _far *lpszWildStart, _far *lpszFileStart;

	// skip ".." and "." (will only match on *.*)
	//   but add a kludge for LFN names like ".toto"
	//   and another kludge for "[!.]*"
	if (( fpFileName[0] == _TEXT('.') ) && ( *fpWildName != _TEXT('[') ) && ( fExtension )) {
		if (( fpFileName[1] == _TEXT('.') ) && ( fpFileName[2] == _TEXT('\0') ))
			fpFileName += 2;
		else if ( fpFileName[1] == _TEXT('\0') )
			fpFileName++;
	}

	for ( ; ; ) {

		// skip quotes in LFN-style names
		while ( *fpWildName == _TEXT('"') )
			fpWildName++;

		if ( *fpWildName == _TEXT('*') ) {

			// skip past * and advance fpFileName until a match
			//   of the characters following the * is found.
			for ( fWildStar = 1; (( *(++fpWildName) == _TEXT('*') ) || ( *fpWildName == _TEXT('?') )); )
				;

		} else if ( *fpWildName == _TEXT('?') ) {

			// ? matches any single character (or possibly
			//   no character, if at start of extension or EOS)
			if (( *fpFileName == _TEXT('.') ) && ( fExtension )) {

				// beginning of extension - throw away any
				//   more wildcards
				while (( *(++fpWildName ) == _TEXT('?') ) || ( *fpWildName == _TEXT('*') ))
					;
				if ( *fpWildName == _TEXT('.') )
					fpWildName++;
				fpFileName++;

			} else {
				if ( *fpFileName )
					fpFileName++;
				fpWildName++;
			}

		} else if (( fWildBrackets ) && ( *fpWildName == _TEXT('[') ) && ( fWildStar == 0 )) {

			// [ ] checks for a single character (including ranges)
			if ( wild_brackets( fpWildName++, *fpFileName, TRUE ) != 0 )
				break;

			if ( *fpFileName )
				fpFileName++;

			while (( *fpWildName ) && ( *fpWildName++ != _TEXT(']') ))
				;

		} else {

			if ( fWildStar ) {

				// following a '*'; so we need to do a complex
				//   match since there could be any number of
				//   preceding characters
				for ( lpszWildStart = fpWildName, lpszFileStart = fpFileName; (( *fpFileName ) && ( *fpWildName != _TEXT('*') )); ) {

					if (( *fpFileName == _TEXT('.') ) && ( fExtension )) {

						if ( fWin95 == 0 )
							break;

						if ( *fpWildName == _TEXT('.') ) {

							// is this the last "extension" in the wildcard name?
							if (( _fstrchr( fpWildName+1, _TEXT('.') ) == NULL ) && ( _fstrchr( fpWildName+1, _TEXT('*')) == NULL )) {

								// we've matched everything so far, so skip to last "extension" in filename
								TCHAR _far *lpszName;
								for ( lpszName = fpFileName; ( *lpszName ); lpszName++ ) {
									if ( *lpszName == _TEXT('.') )
										fpFileName = lpszName;
								}

								break;
							}
						}
					}

					if ( *fpWildName == _TEXT('[') ) {

						// get the first matching char
						if ( wild_brackets( fpWildName, *fpFileName, TRUE ) == 0 ) {

							while (( *fpWildName ) && ( *fpWildName++ != _TEXT(']') ))
								;
							fpFileName++;
							continue;
						}
					}

					if (( *fpWildName != _TEXT('?') ) && ( _ctoupper( *fpWildName ) != _ctoupper( *fpFileName ))) {
						fpWildName = lpszWildStart;
						fpFileName = ++lpszFileStart;
					} else {
						fpWildName++;
						fpFileName++;
					}
				}

				// if "fpWildName" is still an expression, we failed
				//   to find a match
				if ( *fpWildName == _TEXT('[') )
					break;

				fWildStar = 0;

			} else if (( _ctoupper( *fpWildName ) == _ctoupper( *fpFileName )) && ( *fpWildName != _TEXT('\0') )) {

				fpWildName++;
				fpFileName++;

			} else if (( *fpWildName == _TEXT('.') ) && ( *fpFileName == _TEXT('\0') ) && ( fExtension ))
				fpWildName++;	// no extension

			else
				break;
		}
	}

	// a ';' means we're at the end of a filename in a group list
	// a '=' means we're at the end of an executable extension definition
	// a '"' means we're at the end of a quoted filename
	return ((( *fpWildName == _TEXT(';') ) || ( *fpWildName == _TEXT('=') ) || ( *fpWildName == _TEXT('"') )) ? *fpFileName : *fpWildName - *fpFileName );
}


// Evaluate contents of brackets versus a specified character
// Returns 0 for match, 1 for failure
int wild_brackets( TCHAR _far *pszBrackets, TCHAR cChar, int fIgnoreCase )
{
	register int nInverse = 0;

	if ( fIgnoreCase )
		cChar = _ctoupper( cChar );

	// check for inverse bracket "[!a-c]"
	if ( *(++pszBrackets) == _TEXT('!') ) {
		pszBrackets++;
		nInverse++;
	}

	// check for [] or [!] match
	if (( *pszBrackets == _TEXT(']') ) && ( cChar == 0 ))
		return (( nInverse ) ? 1 : 0 );

	// loop til ending bracket or until compare fails
	for ( ; ; pszBrackets++ ) {

		if (( *pszBrackets == _TEXT(']') ) || ( *pszBrackets == _TEXT('\0') )) {
			nInverse--;
			break;
		}

		if ( pszBrackets[1] == _TEXT('-') ) {		// range test

			if (( _ctoupper( *pszBrackets ) <= _ctoupper( cChar )) && ( _ctoupper( cChar ) <= _ctoupper( pszBrackets[2] )))
				break;
			pszBrackets += 2;

			// single character
		} else if ( *pszBrackets == _TEXT('?') ) {

			// kludge for [!?]
			if ( cChar == 0 )
				nInverse--;
			break;

		} else if ( cChar == (( fIgnoreCase ) ? _ctoupper( *pszBrackets ) : *pszBrackets ))
			break;
	}

	return (( nInverse ) ? 1 : 0 );
}


// exclude the specified file(s) from a directory search
int _fastcall ExcludeFiles( LPTSTR pszFiles, LPTSTR pszFilename )
{
	LPTSTR pszExclude, pszSavedNthptr;
	int i, nReturn = 1;

	// save original ntharg values
	pszSavedNthptr = strcpy( (LPTSTR)_alloca( ( strlen( szNthArgBuffer ) + 1 ) * sizeof(TCHAR)), szNthArgBuffer );

	for ( i = 0; (( pszExclude = ntharg( pszFiles, i )) != NULL ); i++ ) {

		StripQuotes( pszExclude );

		// first, check for a literal match with a '['
		if ( strchr( pszFilename, _TEXT('['))) {
			if ( stricmp( pszFilename, pszExclude ) == 0 ) {
				nReturn = 0;
				break;
			}
		}

		if ( wild_cmp( (TCHAR _far *)pszExclude, (TCHAR _far *)pszFilename, TRUE, TRUE ) == 0 ) {
			nReturn = 0;
			break;
		}
	}

	strcpy( szNthArgBuffer, pszSavedNthptr );
	return nReturn;
}


// return the date, formatted appropriately by country type
// Format types:
//	0 - default
//	1 - US
//	2 - European
//	3 - yy/mm/dd
//	4 - yyyy/mm/dd
//	0x100 - use 4-digit year
LPTSTR FormatDate( int nMonth, int nDay, register int nYear, int nFormat )
{
	static TCHAR szDate[14];
	register int i;

	if ( nFormat & 0x100 ) {
		if ( nYear < 79 )
			nYear += 2000;
		else if ( nYear < 200 )
			nYear += 1900;
		nFormat &= 0xFF;

	} else {
		// make sure year is only 2 digits
		if ( nFormat == 4 ) {
			if ( nYear < 79 )
				nYear += 2000;
			else if ( nYear < 200 )
				nYear += 1900;
		} else
			nYear %= 100;
	}

	if ( nFormat == 4 ) {

		// International = yyyy-mm-dd
		sprintf( szDate, _TEXT("%4u-%02u-%02u"), nYear, nMonth, nDay );
		return szDate;

	} else if ((( nFormat == 0 ) && ( gaCountryInfo.fsDateFmt == 1 )) || ( nFormat == 2 )) {

		// Europe = dd-mm-yy
		i = nDay;			// swap day and month
		nDay = nMonth;
		nMonth = i;

	} else if ((( nFormat == 0 ) && ( gaCountryInfo.fsDateFmt == 2 )) || ( nFormat >= 2 )) {

		// Japan = yy-mm-dd
		// we need a leading 0 for the year 00!
		sprintf( szDate, _TEXT("%02u%c%02u%c%02u"), nYear, gaCountryInfo.szDateSeparator[0], nMonth, gaCountryInfo.szDateSeparator[0], nDay );
		return szDate;
	}

	sprintf( szDate, TIME_FMT, nMonth, gaCountryInfo.szDateSeparator[0], nDay, gaCountryInfo.szDateSeparator[0], nYear, 0 );

	return szDate;
}


// honk the speaker (but shorter & more pleasantly than COMMAND.COM)
void honk( void )
{

	// flush the typeahead buffer before honking
	if ( QueryIsConsole( STDIN )) {

		while ( _kbhit() )
			GetKeystroke( EDIT_NO_ECHO );

	}

	SysBeep( gpIniptr->BeepFreq, gpIniptr->BeepDur );
}



void _fastcall PopupEnvironment( int fAlias )
{
	unsigned int i, n, uSize = 0;
	TCHAR _far *lpszPtr;
	TCHAR _far * _far *lppList = 0L;

	// get the environment or alias list into an array
	lpszPtr = (( fAlias ) ? glpAliasList : glpEnvironment );
	for ( i = 0; ( *lpszPtr ); lpszPtr = next_env( lpszPtr )) {

		// allocate memory for 32 entries at a time
		if (( i % 32 ) == 0 ) {
			uSize += 128;
			lppList = (TCHAR _far * _far *)ReallocMem( lppList, uSize );
		}

		lppList[ i++ ] = lpszPtr;
	}

	// get batch variables into the list
	if (( fAlias == 0 ) && ( cv.bn >= 0 )) {

		for ( n = bframe[cv.bn].Argv_Offset; ( bframe[ cv.bn ].Argv[ n ] != NULL ); n++ ) {

			if (( i % 32 ) == 0 ) {
				uSize += 128;
				lppList = (TCHAR _far * _far *)ReallocMem( lppList, uSize );
			}

			lpszPtr = (TCHAR _far *)_alloca( ( strlen( bframe[ cv.bn ].Argv[ n ] ) + 6 ) * sizeof(TCHAR) );
			sprintf_far( lpszPtr, _TEXT("%%%d=%s"), n - bframe[cv.bn].Argv_Offset, bframe[ cv.bn ].Argv[ n ] );
			lppList[ i++ ] = lpszPtr;
		}
	}

	// no entries or no matches?
	if ( i > 0 ) {
		// display the popup selection list
		wPopSelect( gpIniptr->PWTop, gpIniptr->PWLeft, gpIniptr->PWHeight, gpIniptr->PWWidth, lppList, i, 1, (( fAlias ) ? _TEXT("Alias list") : _TEXT("Environment") ), NULL, NULL, 1 );
		FreeMem( lppList );
	}
}



// return a single character answer matching the input mask
int _fastcall QueryInputChar( LPTSTR pszPrompt, LPTSTR pszMask )
{
	register int c, nFH;
	
	nFH = STDOUT;
	// check for output redirected, but input NOT redirected
	//   (this is for things like "echo y | del /q"
	if (( QueryIsConsole( STDOUT ) == 0 ) && ( QueryIsConsole( STDIN )))
		nFH = STDERR;

	qprintf( nFH, _TEXT("%s (%s)? "), pszPrompt, pszMask );

	for ( ; ; ) {

		// get the character - if printable, display it
		// if it's not a Y or N, backspace, beep & ask again
		c = GetKeystroke( EDIT_NO_ECHO | EDIT_UC_SHIFT );

		if ( c == (TCHAR)EOF )
			break;

		if (( c >= (TCHAR)ESCAPE ) && ( c != _TEXT('/') )) {

			if ( c != (TCHAR)ESCAPE )
				qputc( nFH, (TCHAR)c );

			if (( c == (TCHAR)ESCAPE ) || ( strchr( pszMask, (TCHAR)c ) != NULL ))
					break;

			qputc( nFH, (TCHAR)BS );
		}

		honk();
	}

	qprintf( nFH, _TEXT("\r\n") );

	return c;
}


// do a case-insensitive strstr()
LPTSTR _fastcall stristr( LPTSTR pszStr1, LPTSTR pszStr2 )
{
	register int i, nLength;

	nLength = strlen( pszStr2 );
	for ( i = 0; ( i <= ( (int)strlen( pszStr1 ) - nLength )); i++ ) {
		if ( _strnicmp( pszStr1 + i, pszStr2, nLength ) == 0 )
			return ( pszStr1 + i );
	}

	return NULL;
}


// insert a string inside another one
LPTSTR _fastcall strins( LPTSTR pszString, LPTSTR pszInsert )
{
	register unsigned int uInsertLength;

	// length of insert string
	if (( uInsertLength = strlen( pszInsert )) > 0 ) {

		// move original
		memmove( pszString + uInsertLength, pszString, ( strlen( pszString ) + 1 ) * sizeof(TCHAR));

		// insert the new string into the hole
		memmove( pszString, pszInsert, uInsertLength * sizeof(TCHAR) );
	}

	return pszString;
}


// return a pointer to the end of the string
LPTSTR _fastcall strend( LPTSTR pszString )
{
	return ( pszString + strlen( pszString ));
}


// return a pointer to the end of the string
LPTSTR _fastcall strlast( LPTSTR pszString )
{
	return (( *pszString != _TEXT('\0') ) ? ( pszString + strlen( pszString )) - 1 : pszString );
}


// write a long line to STDOUT & check for screen paging
void _fastcall more_page( TCHAR _far *pszStart, int nColumn )
{
	int nWidth, i, fConsole;
	TCHAR cChar = 0;

	nWidth = GetScrCols();

	if (( fConsole = QueryIsConsole( STDOUT )) != 0 ) {

		for ( i = 0; ( pszStart[i] != _TEXT('\0') ); ) {

			// count up column position
			incr_column( pszStart[i], &nColumn );

			if (( nColumn > nWidth ) || (( cChar = pszStart[i++] ) == _TEXT('\r') ) || ( cChar == _TEXT('\n') )) {
				printf( FMT_FAR_PREC_STR, i, pszStart );
				_page_break();
				if (( cChar == _TEXT('\r') ) && ( pszStart[i] == _TEXT('\n') ))
					i++;
				pszStart += i;
				i = nColumn = 0;
			}
		}
	}

	printf( FMT_FAR_STR, (char _far *)pszStart );
	if ( nColumn != nWidth )
		crlf();

	if ( fConsole )
		_page_break();
}


// increment the column counter
void _fastcall incr_column( TCHAR cChar, int *puColumn )
{
	if ( cChar != TAB )
		(*puColumn)++;
	else
		*puColumn += ( 8 - ( *puColumn & 0x07 ));
}


long _fastcall GetRandom( long lStart, long lEnd )
{
	// return random value
	static unsigned long lRandom = 1L;
	DATETIME sysDateTime;

	// set seed to random value based on initial time
	if ( lRandom == 1L ) {
		QueryDateTime( &sysDateTime );
		lRandom *= (long)( sysDateTime.seconds * sysDateTime.hundredths );
	}

	lEnd++;
	lEnd -= lStart;

	lRandom = (( lRandom * 214013L ) + 2531011L );
	lRandom = ( lRandom << 16 ) | ( lRandom >> 16 );

	return (( lRandom % lEnd ) + lStart );
}


// Return a 0 if the arg == "OFF", 1 if == "ON", -1 otherwise
int _fastcall OffOn( LPTSTR pszToken )
{
	while ( strchr( _TEXT(" \t=,"), *pszToken ) != NULL )
		pszToken++;

	if ( _stricmp( pszToken, OFF ) == 0 )
		return 0;

	return (( _stricmp( pszToken, ON ) == 0 ) ? 1 : -1 );
}


// get cursor position request, adjust if relative, & check for valid range
int PASCAL GetCursorRange( LPTSTR pszCursor, int *puRow, int *puColumn )
{
	int nRow, nCol, nLen;

	GetCurPos( &nRow, &nCol );

	if ( sscanf( pszCursor, _TEXT("%d %*[,] %n%d"), puRow, &nLen, puColumn ) == 3) {

		// if relative range get current position & adjust
		if (( *pszCursor == _TEXT('+') ) || ( *pszCursor == _TEXT('-') ))
			*puRow += nRow;

		pszCursor += nLen;
		if (( *pszCursor == _TEXT('+') ) || ( *pszCursor == _TEXT('-') ))
			*puColumn += nCol;

		return ( verify_row_col( *puRow, *puColumn ));
	}

	return ERROR_EXIT;
}


// scan for screen colors
// returns the attribute, and removes the colors from "pszStart"
int _fastcall GetColors( LPTSTR pszStart, int nBorderFlag )
{
	LPTSTR pszCurrent;
	int nFG = -1, nBG = -1, nColor = -1;

	pszCurrent = pszStart;
	pszCurrent = ParseColors( pszStart, &nFG, &nBG );

	// if foreground & background colors are valid, set attribute
	if (( nFG >= 0 ) && ( nBG >= 0 )) {

		nColor = nFG + ( nBG << 4 );

		// check for border color set
		if (( nBorderFlag ) && ( pszCurrent != NULL ) && ( _strnicmp( first_arg( pszCurrent ),BORDER,3) == 0 )) {

			char *pszArg;

			// skip "BORDER"
			pszArg = ntharg( pszCurrent, 1 );
			if (( pszArg != NULL ) && (( nFG = color_shade( pszArg )) <= 7 )) {

				// Set the border color
				SetBorderColor( nFG );

				// skip the border color name
				ntharg( pszCurrent, 0x8002 );
			}
		}

		// remove the color specs from the line
		pszCurrent = (( gpNthptr != NULL ) ? gpNthptr : NULLSTR );
		strcpy( pszStart, pszCurrent );
	}

	return nColor;
}


// if ANSI, send an ANSI color set sequence to the display; else, twiddle the
//   screen attributes directly
void _fastcall set_colors( int nColor )
{

	if ( QueryIsANSI() )
		printf( _TEXT("\033[0;%s%s%u;%um"), (( nColor & 0x08 ) ? _TEXT("1;") : NULLSTR ), (( nColor & 0x80 ) ? _TEXT("5;") : NULLSTR), colors[nColor & 0x07].uANSI, (colors[(nColor & 0x70 ) >> 4].uANSI) + 10 );
	else
		SetScrColor( GetScrRows(), GetScrCols(), nColor );

}


// get foreground & background attributes from an ASCII string
LPTSTR _fastcall ParseColors( LPTSTR pszColors, int *pnFG, int *pnBG )
{
	LPTSTR pszArg;
	register int i, nIntensity = 0, nAttrib;

	for ( ; ; ) {

		if (( pszArg = first_arg( pszColors )) == NULL )
			return NULL;

		if ( _strnicmp( pszArg, BRIGHT, 3 ) == 0 ) {
			// set intensity bit
			nIntensity |= 0x08;
		} else if ( _strnicmp( pszArg, BLINK, 3 ) == 0 ) {
			// set blinking bit
			nIntensity |= 0x80;
		} else
			break;

		// skip BRIGHT or BLINK
		pszColors = (( ntharg( pszColors, 0x8001 ) != NULL ) ? gpNthptr : NULLSTR);
	}

	// check for foreground color match
	if (( nAttrib = color_shade( pszArg )) <= 15)
		*pnFG = nIntensity + nAttrib;

	// "ON" is optional
	i = 1;
	if ((( pszArg = ntharg( pszColors, 1 )) != NULL ) && ( stricmp( pszArg, ON ) == 0 ))
		i++;

	// check for BRIGHT background
	if ((( pszArg = ntharg( pszColors, i )) != NULL ) && ( _strnicmp( pszArg, BRIGHT, 3 ) == 0 )) {
		nIntensity = 0x08;
		i++;
	} else
		nIntensity = 0;

	// check for background color match
	if (( nAttrib = color_shade( ntharg( pszColors, i ))) <= 15 ) {
		*pnBG = nAttrib + nIntensity;
		ntharg( pszColors, ++i );
	}

	return gpNthptr;
}


// match color against list
int _fastcall color_shade( LPTSTR pszColor )
{
	register int i;

	if ( pszColor != NULL ) {

		// allow 0-15 as well as Blue, Green, etc.
		if ( is_signed_digit( *pszColor ))
			return ( atoi( pszColor ));

		for ( i = 0; ( i <= 7 ); i++ ) {
			// check for color match
			if ( _strnicmp( pszColor, colors[i].szShade, 3 ) == 0 )
				return i;
		}
	}

	return 0xFF;
}


// read / write the description file(s)
int PASCAL process_descriptions( LPTSTR pszReadD, LPTSTR pszWriteD, int fFlags )
{
	register int i;
	register LPTSTR dname_part;
	int hFH, nReturn = 0;
	unsigned int uBytesRead, uBytesWritten, fLFN, uBufferSize;
	unsigned int nMode = (_O_RDWR | _O_BINARY), fTruncate = 0;
	TCHAR *pszArg, szDName[MAXFILENAME+1], *pszNewDescription = NULLSTR;
	TCHAR _far *dptr = 0L, _far *pchRead = 0L, _far *pchWrite = 0L;
	TCHAR _far *fptr, _far *fdesc;
	long lReadOffset, lWriteOffset;
	FILESEARCH dir;


	// check if no description processing requested
	if (( gpIniptr->Descriptions == 0 ) && (( fFlags & DESCRIPTION_PROCESS ) == 0 ))
		return 0;
	fFlags &= ~DESCRIPTION_PROCESS;

	// disable ^C / ^BREAK handling
	HoldSignals();

	// read 4K blocks
	uBufferSize = 4098 * sizeof(TCHAR);

	// read the descriptions ( from path if necessary)
	if (( pszReadD != NULL ) && ( fFlags & DESCRIPTION_READ )) {

		if (( pchRead = (TCHAR _far *)AllocMem( &uBufferSize )) == 0L )
			return ERROR_EXIT;

		insert_path( szDName, DESCRIPTION_FILE, pszReadD );

		// it's not an error to not have a DESCRIPT.ION file!
		if (( hFH = _sopen( szDName, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) >= 0 ) {

			dname_part = fname_part( pszReadD );

			// read 4K blocks of the DESCRIPT.ION file
			// if app is Unicode, read 4K bytes (2K characters) then convert
			for ( lReadOffset = 0L; (( FileRead( hFH, pchRead, 4096, &uBytesRead ) == 0 ) && ( uBytesRead != 0 )); ) {

				// back up to the end of the last line & terminate there
				if (( i = uBytesRead ) == 4096 ) {
					for ( ; ((--i > 0 ) && ( pchRead[i] != _TEXT('\n') ) && ( pchRead[i] != _TEXT('\r') )); )
						;
					i++;
				}
				pchRead[i] = _TEXT('\0');
				lReadOffset += i;

				// read a line & try for a match
				for ( fptr = pchRead; (( *fptr != _TEXT('\0') ) && ( *fptr != EoF )); ) {

					// check for LFNs & strip quotes
					if ( *fptr == _TEXT('"') ) {

						if (( fdesc = _fstrchr( ++fptr, _TEXT('"') )) != 0L )
							*fdesc++ = _TEXT('\0');

					} else {

						// skip to the description part (kinda kludgy
						// to avoid problems if no description
						for ( fdesc = fptr; ; fdesc++ ) {

							if (( *fdesc == _TEXT(' ') ) || ( *fdesc == _TEXT(',') ) || ( *fdesc == _TEXT('\t') ))
								break;
							if (( *fdesc == _TEXT('\r') ) || ( *fdesc == _TEXT('\n') ) || ( *fdesc == _TEXT('\0') )) {
								fdesc = 0L;
								break;
							}
						}
					}

					if ( fdesc != 0L ) {

						// strip trailing '.' (3rd-party bug)
						if (( *fptr != _TEXT('.') ) && ( fdesc[-1] == _TEXT('.') ))
							fdesc[-1] = _TEXT('\0');

						// wipe out space between name & description
						*fdesc++ = _TEXT('\0');

						if ( _fstricmp( dname_part, fptr ) == 0 ) {

							// just return the matching description?
							if (( fFlags & DESCRIPTION_WRITE ) == 0 )
								sscanf_far( fdesc, DESCRIPTION_SCAN, pszWriteD );

							dptr = fdesc;
							break;
						}

						fptr = fdesc;
					}

					// skip the description & get next filename
					for ( ; (( *fptr != _TEXT('\0') ) && ( *fptr++ != _TEXT('\n') )); )
						;
				}

				if (( dptr != 0L ) || ( uBytesRead < 4096 ))
					break;

				// seek to the end of the last line of the current block
				_lseek( hFH, lReadOffset, SEEK_SET );
			}

			_close( hFH );
		}
	}

	if (( fFlags & DESCRIPTION_WRITE ) || ( fFlags & DESCRIPTION_REMOVE )) {

		// if we're writing a new description, it's in the
		//   format "process_description(pszWriteD,description,flags)"
		if ( fFlags & DESCRIPTION_CREATE ) {

			if ( dptr != 0L ) {
				// save extended part of old description
				sscanf_far( dptr, _TEXT("%*[^\004\032\r\n]%n"), &uBytesRead );
				dptr += uBytesRead;
				pszNewDescription = pszWriteD;
			} else
				dptr = (TCHAR _far *)pszWriteD;

			pszWriteD = pszReadD;
		}

		// if no description, and we're not removing descriptions, exit
		if ( dptr == 0L ) {
			if (( fFlags & DESCRIPTION_REMOVE ) == 0 )
				goto descript_bye;
		} else {
			// if we're adding a description, we may need to create a file
			nMode |= _O_CREAT;
		}

		if (( pchWrite = (TCHAR _far *)AllocMem( &uBufferSize )) == 0L ) {
			nReturn = ERROR_EXIT;
			goto descript_bye;
		}

		// open the target DESCRIPT.ION file
		insert_path( szDName, DESCRIPTION_FILE, pszWriteD );

		// Can't use DENYRW in DOS due to SHARE bug

		if (( hFH = _sopen( szDName, nMode, _SH_COMPAT, (_S_IREAD | _S_IWRITE ))) < 0 )

			nReturn = _doserrno;

		else {

			fLFN = ifs_type( szDName );
			lReadOffset = lWriteOffset = 0L;

			// point to the name part
			dname_part = szDName + ((( dname_part = path_part( szDName )) != NULL ) ? strlen( dname_part ) : 0 );



			// read 4k blocks of the DESCRIPT.ION file
			while (( FileRead( hFH, pchWrite, 4096, &uBytesRead ) == 0 ) && ( uBytesRead != 0 )) {

				// back up to the end of the last line, and seek
				//   to the end of the last line of the current block
				for ( i = uBytesRead; (( --i > 0 ) && ( pchWrite[i] != _TEXT('\n') )); )
					;

				pchWrite[++i] = _TEXT('\0');
				lReadOffset += i;

				// read a line & try for a match
				for ( fptr = pchWrite; (( *fptr != _TEXT('\0') ) && ( *fptr != EoF )); ) {

					// look for argument match or file not found
					// if not found, delete this description
					// check for LFNs
					if ( *fptr == _TEXT('"') )
						sscanf_far( fptr, _TEXT("\"%[^\"\n]%*[^\n]\n%n"), dname_part, &nMode );
					else
						sscanf_far( fptr, _TEXT("%[^ ,\t\n]%*c%*[^\n]\n%n"), dname_part, &nMode );

					// don't let non-LFN OS kill LFN descriptions
					if (( fLFN == 0 ) && (( *fptr == _TEXT('"') ) || ( strlen( dname_part ) > 12 ))) {

						// point to beginning of next line
						fptr += nMode;

						// Can't use _dos_getfileattr() because of
						//   Netware bug
					} else if (( _stricmp( pszWriteD, szDName ) == 0 ) || (( fFlags & DESCRIPTION_REMOVE ) && ( find_file( 0x4E, szDName, 0x17 | FIND_CLOSE | FIND_NO_ERRORS | FIND_EXACTLY | FIND_NO_FILELISTS, &dir, NULL ) == NULL ))) {

						fTruncate = 1;

						// collapse matching or missing filename
						_fstrcpy( fptr, fptr + nMode );

					} else
						// point to beginning of next line
						fptr += nMode;
				}

				if ( fTruncate ) {

					if (( i = _fstrlen( pchWrite )) > 0 ) {

						_lseek( hFH, lWriteOffset, SEEK_SET );

						FileWrite( hFH, pchWrite, i * sizeof(TCHAR), (unsigned int *)&uBytesWritten );

						// save current write position &
						//   restore read position
						lWriteOffset += uBytesWritten;
					}

				} else
					lWriteOffset = lReadOffset;

				if ( uBytesRead < 4096 )
					break;

				_lseek( hFH, lReadOffset, SEEK_SET );
			}

			// if truncating, or we have a description, write it out
			if (( fTruncate ) || ( dptr != 0L )) {

				_lseek( hFH, lWriteOffset, SEEK_SET );

				// truncate the file
				if ( fTruncate )
					_chsize( hFH, lWriteOffset );

				// add the new description (if any) to the list
				if ( dptr != 0L ) {

					// get description length
					for ( i = 0; (( dptr[i] ) && ( dptr[i] != _TEXT('\r') ) && ( dptr[i] != _TEXT('\n') )); i++ )
						;

					// format is: filename description[cr][lf]
					if (( i ) || ( *pszNewDescription )) {

						// check for LFN/NTFS name
						if (( pszArg = fname_part( pszWriteD )) != NULL ) {

							if ( QueryIsLFN( pszArg ))
								qprintf( hFH, _TEXT("\"%s\" %s%.*Fs\r\n"), pszArg, pszNewDescription, i, dptr );
							else
								qprintf( hFH, _TEXT("%s %s%.*Fs\r\n"), pszArg, pszNewDescription, i, dptr );
						}

						// make sure we don't delete the list
						lWriteOffset = 1L;
					}
				}

				_close( hFH );

				// restore description file name
				strcpy( dname_part, DESCRIPTION_FILE );

				// if filesize == 0, delete the DESCRIPT.ION file
				if ( lWriteOffset == 0L )
					remove( szDName );

				else if ( lReadOffset == 0 ) {

					// make file hidden & set archive (for BACKUP)
					// but only if we just created it!
					SetFileMode( szDName, (_A_HIDDEN | _A_ARCH) );
				}

			} else
				_close( hFH );
		}
	}

descript_bye:

	FreeMem( pchRead );
	FreeMem( pchWrite );

	// enable ^C / ^BREAK handling
	EnableSignals();

	return nReturn;
}


// search for a file in the installation directory or on the PATH
// and return its full path name in szOutBuf
int _fastcall FindInstalledFile( LPTSTR pszOutBuf, LPTSTR pszFName )
{
	LPTSTR pszFound;

	// Create a full path name

	*pszOutBuf = _TEXT('\0');
	if ( gpIniptr->InstallPath != INI_EMPTYSTR )
		copy_filename( pszOutBuf, (char *)( gpIniptr->StrData + gpIniptr->InstallPath ));

	mkdirname( pszOutBuf, pszFName );

	// If not found by the full name, look on the path
	if ( !is_file( pszOutBuf )) {
		if (( pszFound = searchpaths( pszFName, NULL, TRUE, NULL )) == NULL)
			return ERROR_FILE_NOT_FOUND;
		strcpy( pszOutBuf, pszFound );
	}

	// Make it a fully qualified name
	mkfname( pszOutBuf, MKFNAME_NOERROR );

	return 0;
}
