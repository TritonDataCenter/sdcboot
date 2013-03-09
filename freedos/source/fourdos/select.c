

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


// SELECT.C - SELECT commands for 4xxx / TCMD family
//   Copyright (c) 1993 - 2002 Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <malloc.h>
#include <string.h>

#include "4all.h"


// SELECT arguments
typedef struct {
	unsigned int fFlags;
	unsigned int uMarked;
#if (_WIN == 0)
	int nDirLine;
	int nDirFirst;
	int nDirLast;
	unsigned int uNormal;
	unsigned int uInverse;
	int nLeftMargin;
#endif
	unsigned int uEntries;
	unsigned int uLength;
	unsigned int uMarkedFiles;
	long lTotalSize;
	LPTSTR pszArg;
	LPTSTR pszCmdStart;
	LPTSTR pszCmdTail;
	LPTSTR pszFileVar;
	TCHAR cOpenParen;
	TCHAR cCloseParen;
	TCHAR szPath[MAXPATH+1];
	DIR_ENTRY _huge *sfiles;
	RANGES aSelRanges;
} SELECT_ARGS;

static int InitSelect( SELECT_ARGS *, LPTSTR, long * );
static int _fastcall ProcessSelect( SELECT_ARGS *, LPTSTR );


#define SELECT_ONLY_ONE 1

#include "selectc.c"		// SELECT for character mode


// check for SELECT options
static int InitSelect( SELECT_ARGS *sel, LPTSTR pszStartLine, long *plMode )
{
	register int i;
	register LPTSTR pszOption;
	TCHAR szLine[MAXLINESIZ+4];

	// clear everything to 0
	memset( (char *)sel, _TEXT('\0'), sizeof(SELECT_ARGS) );

	// reset the DIR flags
	init_dir();

	// initialize date/time/size ranges
	if ( GetRange( pszStartLine, &(sel->aSelRanges), 1 ) != 0 )
		return ERROR_EXIT;

	for ( i = 0; (( pszOption = ntharg( pszStartLine, i )) != NULL ) && ( *pszOption == gpIniptr->SwChr ); i++ ) {

		// point past switch character
		for ( pszOption++; ( *pszOption != _TEXT('\0') ); ) {

			// expand variables embedded in switch
			if (( pszOption[1] ) && ( strchr( pszOption, _TEXT('%') ))) {
				strcpy( szLine, pszOption );
				if ( var_expand( szLine, 1 ) != 0 )
					return ERROR_EXIT;
				pszOption = szLine;
			}

			switch ( _ctoupper( *pszOption++ )) {
			case '1':
				// only allow marking 1 file
				sel->fFlags |= SELECT_ONLY_ONE;
				break;

			case 'A':
				// retrieve based on specified atts
				pszOption = GetSearchAttributes( pszOption );

				// default to everything
				*plMode = FIND_BYATTS | 0x17;
				break;

			case 'C':	// show compression ratio
				glDirFlags |= DIRFLAGS_COMPRESSION;

				if ( _ctoupper( *pszOption ) == 'H' ) {
					pszOption++;
					glDirFlags |= DIRFLAGS_HOST_COMPRESSION;
				}
				if ( _ctoupper( *pszOption ) == 'P' ) {
					pszOption++;
					glDirFlags |= DIRFLAGS_PERCENT_COMPRESSION;
				}

				break;

			case 'D':	// don't colorize
				glDirFlags |= DIRFLAGS_NO_COLOR;
				break;

			case 'E':	// display filenames in upper case
				glDirFlags |= DIRFLAGS_UPPER_CASE;
				break;

			case 'H':	// hide "." and ".."
				glDirFlags |= DIRFLAGS_NO_DOTS;
				break;

			case 'J':       // DOS justify filenames
				glDirFlags |= DIRFLAGS_JUSTIFY;
				break;

			case 'L':	// display filenames in lower case
				glDirFlags |= DIRFLAGS_LOWER_CASE;
				break;

			case 'O':	// dir sort order
				// kludges for DOS 5 format
				pszOption = dir_sort_order( pszOption );
				break;

			case 'T':	// display attributes or time field

				if ( *pszOption ) {

					if ( *pszOption == _TEXT(':') )
						pszOption++;

					switch ( _ctoupper( *pszOption )) {
					case 'A':	// last access
						gnDirTimeField = 1;
						break;
					case 'C':	// creation
						gnDirTimeField = 2;
						break;
					case 'W':	// last write (default)
						gnDirTimeField = 0;
						break;
					default:
						goto SelectError;
					}
					pszOption = NULLSTR;

				} else
					glDirFlags |= DIRFLAGS_SHOW_ATTS;
				break;

			case 'X':       // display alternate name

				if ( fWin95 == 0 )
					break;

			case 'Z':
				glDirFlags |= DIRFLAGS_LFN_TO_FAT;
				break;

			default:	// invalid option
SelectError:
				error( ERROR_INVALID_PARAMETER, pszOption-1 );
				return ( Usage( SELECT_USAGE ));
			}
		}
	}

	// skip the switch statements
	strcpy( pszStartLine, ((gpNthptr == NULL ) ? NULLSTR : gpNthptr ));

	return 0;
}


#pragma alloc_text( _TEXT, ProcessSelect )

// process the SELECT'd filenames
static int _fastcall ProcessSelect( register SELECT_ARGS *sel, LPTSTR pszCmdline )
{
	static TCHAR szInvalidChars[] = _TEXT("  \t,+=<>|");
	register unsigned int i, n;
	int nReturn = 0;
	long lSaveDirFlags;
	LPTSTR szFormat;

	szInvalidChars[0] = gpIniptr->CmdSep;

	// process the marked files in order
	for ( *pszCmdline = _TEXT('\0'), n = 0, i = 1; ((nReturn != CTRLC) && (i <= sel->uMarked)); ) {

		if ( setjmp( cv.env ) == -1) {

			Cls_Cmd( NULL );

			nReturn = CTRLC;
			break;
		}

		if ( sel->sfiles[n].uMarked == i ) {

			if ( sel->cOpenParen == _TEXT('(') ) {

				// substitute the filename & run the
				//   specified command

				// kludge to save flags for "Select DIR ..."
				lSaveDirFlags = glDirFlags;

				// if LFN names with whitespace, quote them
				if ( glDirFlags & DIRFLAGS_LFN ) {
					szFormat = ((( _fstrpbrk( sel->sfiles[n].lpszLFN, szInvalidChars ) != 0L ) || ( strpbrk( sel->szPath, szInvalidChars ) != 0L )) ? _TEXT("%s\"%s%Fs\"%s") : _TEXT("%s%s%Fs%s") );
					sprintf( pszCmdline, szFormat, sel->pszCmdStart, sel->szPath, (( glDirFlags & DIRFLAGS_NT_ALT_NAME ) ? sel->sfiles[n].szFATName : sel->sfiles[n].lpszLFN ), sel->pszCmdTail );
				} else {

					// reassemble pre-/J name
					if (( glDirFlags & DIRFLAGS_JUSTIFY ) && ( sel->sfiles[n].szFATName[8] == _TEXT(' ') )) {

						int j;

						sel->sfiles[n].szFATName[8] = _TEXT('.');
						for ( j = 8; (( j > 0 ) && ( sel->sfiles[n].szFATName[j-1] == _TEXT(' ') )); j-- )
							;
						if ( j < 8 )
							_fstrcpy( sel->sfiles[n].szFATName + j, sel->sfiles[n].szFATName + 8 );
					}

					sprintf( pszCmdline, _TEXT("%s%s%Fs%s"), sel->pszCmdStart, sel->szPath, sel->sfiles[n].szFATName, sel->pszCmdTail );
				}

				nReturn = command( pszCmdline, 0 );

				// restore flags (primarily for DIRFLAGS_LFN)
				glDirFlags = lSaveDirFlags;
				*pszCmdline = _TEXT('\0');

			} else {
				// if LFN names with whitespace, quote them
				if (glDirFlags & DIRFLAGS_LFN) {
					szFormat = ((( _fstrpbrk( sel->sfiles[n].lpszLFN, szInvalidChars ) != 0L ) || ( strpbrk( sel->szPath, szInvalidChars ) != 0L )) ? _TEXT("\"%s%Fs\" ") : _TEXT("%s%Fs ") );
					sprintf( strend( pszCmdline ), szFormat, sel->szPath, sel->sfiles[n].lpszLFN );
				} else 
					sprintf( strend( pszCmdline ), _TEXT("%s%Fs "), sel->szPath, sel->sfiles[n].szFATName );
			}

		} else if ( ++n < sel->uEntries )
			continue;

		// reset index & marker
		n = 0;
		i++;
	}

	// check for [ ] (put everything on a single line) use
	if ( pszCmdline[0]) {
		strins( pszCmdline, sel->pszCmdStart );
		strcat( pszCmdline, sel->pszCmdTail );
		nReturn = command( pszCmdline, 0 );
	}


	return nReturn;
}

