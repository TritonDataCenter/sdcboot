

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


// LISTALL.C - file listing routines for 4xxx / TCMD family
//   Copyright (c) 1993 - 2003  Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#if _DOS
#include <dos.h>
#endif

#include "4all.h"

#define MAXLISTLINE 511


static int ListInit( void );
static int _fastcall ListOpenFile( LPTSTR );
static void ListHome( void );
static int _fastcall ListMoveLine( int);
static LONG _fastcall MoveViewPtr( LONG, LONG * );
static LONG _fastcall ComputeLines( LONG, LONG );
static VOID SetRightMargin( void );
static int _fastcall DisplayLine( UINT, LONG );
int FormatLine( TCHAR [], int, LONG, int *, int );
static BOOL _near _fastcall SearchFile( long );
VOID ListPrintFile( void );
void _fastcall ListSetCurrent( LONG );
static void _near _fastcall ListFileRead( LPBYTE, unsigned int );
static int _near GetPrevChar( void );
static int _near GetNextChar( void );
static void _near ListSaveFile( void );


LISTFILESTRUCT LFile;

TCHAR  szListFindWhat[80] = _TEXT("");
TCHAR  szFFindFindWhat[128] = _TEXT("");
static TCHAR szClip[MAXFILENAME];

static BOOL bListSkipLine = 0;
BOOL  fEndSearch = 0;

static int nScreenRows;			// screen size in rows
static int nScreenColumns;		// screen size in columns
static int nRightMargin;		// right margin (column #)

static int nNormal;				// normal video attribute
static int nInverse;			// inverse video attribute

static int nStart;				// start index & current index for
static int nCurrent;			// "previous file" and "next file" support
static int nCodePage;
long lListFlags;				// LIST options (/H, /S, /W, /X)



#include "listc.c"


extern RANGES aRanges;

// FFIND structure
typedef struct
{
	unsigned long fFlags;
	unsigned long ulMode;
	long lFilesFound;
	long lLinesFound;
	TCHAR szDrives[32];
	TCHAR szSource[MAXFILENAME];
	TCHAR szFilename[MAXFILENAME];

	char szText[512];
	unsigned long ulSize;

} FFIND_STRUCT;




static int __ffind( char * );

static int _fastcall __ffindit( FFIND_STRUCT * );

#pragma alloc_text( _TEXT, Ffind_Cmd )


// find files or text
int _near Ffind_Cmd( LPTSTR pszCmdLine )
{

	return ( __ffind( pszCmdLine ));
}


static int __ffind( char *pszCmdLine )
{
	TCHAR *pszArg, *ptr;
	int i, n, argc, nLength, nReturn = 0;
	FFIND_STRUCT Find;
	LISTFILESTRUCT SaveLFile;

	// initialize variables for SearchDirectory()
	init_dir();

	memset( &Find, '\0', sizeof( FFIND_STRUCT) );
	fEndSearch = 0;
	lListFlags = 0L;

	// turn off the error reporting in SearchDirectory()
	glDirFlags |= DIRFLAGS_RECURSE;

	// no "." or ".."
	glDirFlags |= DIRFLAGS_NO_DOTS;

	if (( i = gcdisk( NULL )) != 0 )
		sprintf( Find.szDrives, FMT_CHAR, i + 64 );
	else
		Find.szDrives[0] = _TEXT('C');

		if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
			return ( Usage( FFIND_USAGE ));

		// get date/time/size ranges
		if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
			return ERROR_EXIT;

		init_page_size();

		// check for and remove switches; abort if no filename arguments
		for ( argc = 0; (( pszArg = ntharg( pszCmdLine, argc | 0x1000 )) != NULL ) && ( *pszArg == gpIniptr->SwChr ); argc++ ) {

			// point past switch character
			for ( pszArg++; ( *pszArg != _TEXT('\0') ); ) {

				switch ( _ctoupper( *pszArg++ )) {
			case _TEXT('A'):
				// retrieve based on specified atts
				pszArg = GetSearchAttributes( pszArg );
				Find.ulMode = FIND_BYATTS | 0x17;
				break;

			case _TEXT('B'):	// bare display ( filenames only)
				glDirFlags |= (DIRFLAGS_NAMES_ONLY | DIRFLAGS_NO_HEADER | DIRFLAGS_NO_FOOTER);
				break;

			case _TEXT('C'):	// case-sensitive
				Find.fFlags |= FFIND_CHECK_CASE;
				break;

			case _TEXT('D'):	// search entire disk(s)
				Find.fFlags |= ( FFIND_SUBDIRS | FFIND_ALL );

				// check for additional drives
				if ( *pszArg ) {
					sscanf( pszArg, _TEXT("%31s%n"), Find.szDrives, &i );
					pszArg += i;
					strupr( Find.szDrives );
				}
				break;

			case _TEXT('E'):	// display filenames in upper case
				glDirFlags |= DIRFLAGS_UPPER_CASE;
				break;

			case _TEXT('F'):	// stop after first match
				Find.fFlags |= FFIND_ONLY_FIRST;
				break;

			case _TEXT('I'):
				Find.fFlags |= FFIND_NOWILDCARDS;
				break;

			case _TEXT('K'):	// no header
				glDirFlags |= DIRFLAGS_NO_HEADER;
				break;

			case _TEXT('L'):	// display line numbers
				Find.fFlags |= FFIND_LINE_NUMBERS;
				break;

			case _TEXT('M'):	// no footer
				glDirFlags |= DIRFLAGS_NO_FOOTER;
				break;

			case _TEXT('N'):	// reverse meaning
				Find.fFlags |= FFIND_NOT;
				// for now, only works on a per-file basis
				glDirFlags |= (DIRFLAGS_NAMES_ONLY | DIRFLAGS_NO_HEADER | DIRFLAGS_NO_FOOTER);
				break;

			case _TEXT('O'):	// dir sort order
				// kludges for DOS 5 format
				pszArg = dir_sort_order( pszArg );
				break;

			case _TEXT('P'):	// pause after each page
				gnPageLength = GetScrRows();

				break;

			case _TEXT('R'):	// reverse search (from end)
				Find.fFlags |= FFIND_REVERSE_SEARCH;
				break;

			case _TEXT('S'):	// search subdirectories
				Find.fFlags |= FFIND_SUBDIRS;
				break;

			case _TEXT('U'):	// summary only
				Find.fFlags |= FFIND_SUMMARY;
				break;

			case _TEXT('V'):	// show all matching lines / offsets
				Find.fFlags |= FFIND_SHOWALL;
				break;

			case _TEXT('Y'):	// prompt to continue search
				Find.fFlags |= FFIND_QUERY_CONTINUE;
				break;

			case _TEXT('X'):	// hex search
				Find.fFlags |= FFIND_HEX_DISPLAY;
				if ( _ctoupper( *pszArg ) == _TEXT('T') )
					pszArg++;
				else {
					if ( *pszArg == _TEXT('\0') )
						break;
					Find.fFlags |= FFIND_HEX_SEARCH;
				}

			case _TEXT('T'):	// text search
				if ( *pszArg != _TEXT('\0') ) {
					if ( *pszArg == _TEXT('"') ) {
						ptr = scan( ++pszArg, _TEXT("\""), _TEXT("`") );
						*ptr = _TEXT('\0');
						sprintf( szFFindFindWhat, _TEXT("%.127s"), pszArg );
						EscapeLine( szFFindFindWhat );
					} else
						strcpy( szFFindFindWhat, pszArg );

					pszArg = NULLSTR;
					Find.fFlags |= FFIND_TEXT;
					break;
				}

			default:	// invalid option
				error( ERROR_INVALID_PARAMETER, pszArg-1 );
				return ( Usage( FFIND_USAGE ));
				}
			}
		}

		// skip the switch statements
		if ( gpNthptr == NULL ) {

			if ( _isatty( STDIN ) == 0 )

				pszCmdLine = CONSOLE;
			else
				return ( Usage( FFIND_USAGE ));
		} else
			strcpy( pszCmdLine, gpNthptr );

	Find.fFlags |= FFIND_NOERROR;

	if ( Find.fFlags & FFIND_TEXT ) {

		// save the LIST variables
		memmove( &SaveLFile, &LFile, sizeof(LISTFILESTRUCT) );

		// allocate buffer if searching for text
		ListInit();

		// don't search for directories for text!
		Find.ulMode &= ( FIND_BYATTS | 0x07 );
	}

	if ( setjmp( cv.env ) == -1 ) {
		nReturn = CTRLC;
		goto ffind_bye;
	}

	for ( argc = 0; ( fEndSearch == 0 ); argc++ ) {

		for ( n = 0; (( Find.szDrives[n] != _TEXT('\0') ) && ( fEndSearch == 0 )); n++ ) {

			if (( pszArg = ntharg( pszCmdLine, argc )) == NULL ) {
				fEndSearch = 1;
				break;
			}

			// no multi-drive specs if drive already named!
			if (( Find.fFlags & FFIND_ALL ) && ( pszArg[1] != _TEXT(':') ) && (pszArg[1] != _TEXT('\\') )) {

				// check for drive range (c-f)
				if ( Find.szDrives[n] == _TEXT('-') ) {
					if (( n > 0 ) && ( Find.szDrives[n+1] > (TCHAR)( Find.szDrives[n-1] + 1)))
						Find.szDrives[--n] += 1;
					else
						n++;
				}
				i = Find.szDrives[n];

				strins( pszArg, _TEXT(" :\\") );
				*pszArg = (TCHAR)i;

			} else
				n = strlen( Find.szDrives ) - 1;

			// build the source name
			if (( QueryIsDevice( pszArg )) && ( QueryIsCON( pszArg )))
				lListFlags |= LIST_STDIN;

			else if ( mkfname( pszArg, 0 ) == NULL ) {
				nReturn = ERROR_EXIT;
				goto ffind_bye;

			} else {

				if ( glDirFlags & DIRFLAGS_UPPER_CASE )
					strupr( pszArg );

				// check to see if it's an LFN partition
				if ( ifs_type( pszArg ) != 0 )
					glDirFlags |= DIRFLAGS_LFN;

				// if a directory, append "\*.*"
				// if not a directory, append a ".*" to files with no extension
				if ( is_dir( pszArg ))
					mkdirname( pszArg, (( glDirFlags & DIRFLAGS_LFN ) ? _TEXT("*") : WILD_FILE ));

				else {

					// if no include list & no extension specified, add default one
					//   else if no filename, insert a wildcard filename
					if ( strchr( pszArg, _TEXT(';') ) == NULL ) {

						if ( ext_part( pszArg ) == NULL ) {

							// don't append "*" if name has wildcards
							//   (for compatibility with COMMAND.COM)
							if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( strpbrk( pszArg, WILD_CHARS ) == NULL ))
								strcat( pszArg, WILD_EXT );

						} else {
							// point to the beginning of the filename
							ptr = pszArg + strlen( path_part( pszArg ));
							if ( *ptr == _TEXT('.') )
								strins( ptr, _TEXT("*") );
						}
					}
				}
			}

			copy_filename( Find.szSource, pszArg );

			// save the source filename part ( for recursive calls &
			//   include lists)
			nLength = 0;
			if (( pszArg = path_part( Find.szSource )) != NULL )
				nLength = strlen( pszArg );
			strcpy( Find.szFilename, Find.szSource + nLength );

			if (( i = __ffindit( &Find )) == ABORT_LINE )
				i = 0;
			else if ( i == -ABORT_LINE ) {
				fEndSearch = 1;
				i = 0;
			}

			if (( setjmp( cv.env) == -1 ) || ( i == CTRLC )) {
				nReturn = CTRLC;
				fEndSearch = 1;
				break;
			}

			if ( i != 0 )
				nReturn = ERROR_EXIT;
		}
	}


		if (( glDirFlags & DIRFLAGS_NO_FOOTER ) == 0 ) {
			// display number of lines & files found
			crlf();
			_page_break();
			if ( Find.fFlags & FFIND_TEXT )
				printf( FFIND_TEXT_FOUND, Find.lLinesFound, (( Find.lLinesFound == 1) ? FFIND_ONE_LINE : FFIND_MANY_LINES) );
			FilesProcessed( FFIND_FOUND, Find.lFilesFound );
			crlf();
			_page_break();
		}

ffind_bye:
		if ( Find.fFlags & FFIND_TEXT ) {

			FreeMem( LFile.lpBufferStart );
			if ( LFile.hHandle > 0 )
				_close( LFile.hHandle );

			LFile.hHandle = -1;

			// restore the LIST variables
			memmove( &LFile, &SaveLFile, sizeof(LISTFILESTRUCT) );
		}


		return (( Find.lFilesFound != 0L ) ? nReturn : ERROR_EXIT );
}


static int _fastcall __ffindit( register FFIND_STRUCT *Find )
{
	register unsigned int i;
	LPTSTR pszArg;
	int c, n, nReturn = 0, nLength, fAbort = 0;
	unsigned int entries = 0;
	long lLines;
	DIR_ENTRY _huge *files = 0L;	// file array in system memory


	// trap ^C and clean up
	if ( setjmp( cv.env ) == -1 ) {
		dir_free( files );
		return CTRLC;
	}

	if ( glDirFlags & DIRFLAGS_UPPER_CASE )
		strupr( Find->szSource );
	else if (( glDirFlags & DIRFLAGS_LFN ) == 0 )
		strlwr( Find->szSource );

	// look for matches
	if ((( lListFlags & LIST_STDIN ) == 0 ) && ( SearchDirectory(( Find->ulMode | FIND_RANGE | FIND_EXCLUDE ), Find->szSource, (DIR_ENTRY _huge **)&files, &entries, &aRanges, 0 ) != 0 )) {

		nReturn = ERROR_EXIT;

	} else {

		if ( lListFlags & LIST_STDIN )
			entries = 1;
		else
			pszArg = path_part( Find->szSource );

		for ( i = 0; (( i < entries ) && ( fEndSearch == 0 )); i++ ) {

			if ( lListFlags & LIST_STDIN )
				strcpy( LFile.szName, _TEXT("CON") );
			else
				sprintf( LFile.szName, _TEXT("%s%Fs"), pszArg, (( glDirFlags & DIRFLAGS_LFN ) ? files[i].lpszLFN : files[i].szFATName ));

			if ( Find->fFlags & FFIND_TEXT ) {

				if ( lListFlags & LIST_STDIN ) {

					LFile.hHandle = STDIN;

					// DOS puts pipes into temp files; so we can
					//   get the size via a seek to the end
					if ( _isatty( STDIN ))
						LFile.lpEOF = LFile.lpCurrent;
					else
						LFile.lSize = QuerySeekSize( STDIN );

				} else {

					LFile.lSize = files[i].ulFileSize;

						if (( LFile.hHandle = _sopen( LFile.szName, (_O_BINARY | _O_RDONLY), _SH_DENYNO )) == -1 ) {

								nReturn = error( _doserrno, LFile.szName );
							continue;
						}

				}

				LFile.lpEOF = 0L;

				ListHome();
				lLines = 0L;
				if ( Find->fFlags & FFIND_REVERSE_SEARCH ) {
					// goto the last row
					while ( ListMoveLine( 1 ) != 0 )
						;
				}

				for ( n = 0; (( SearchFile( Find->fFlags ) != 0 ) && ( fEndSearch == 0 )); ) {

					lLines++;

						if (( Find->fFlags & FFIND_SUMMARY ) == 0 ) {

							// display filename
							if ( glDirFlags & DIRFLAGS_NAMES_ONLY ) {
								// if /B, _only_ display name!
								more_page( LFile.szName, 0 );
								break;
							}

							if (( n == 0 ) && (( glDirFlags & DIRFLAGS_NO_HEADER) == 0 )) {
								crlf();
								_page_break();
								sprintf( Find->szText, FFIND_FILE_HEADER, LFile.szName );
								more_page( Find->szText, 0 );
								n++;
							}

							if ( Find->fFlags & FFIND_HEX_DISPLAY ) {
								sprintf( Find->szText, FFIND_OFFSET, LFile.lViewPtr, LFile.lViewPtr );
								more_page( Find->szText, 0 );

							} else {

								// display line number
								nLength = 0;
								if ( Find->fFlags & FFIND_LINE_NUMBERS )
									nLength = sprintf( Find->szText, _TEXT("[%ld] "), LFile.lCurrentLine + gpIniptr->ListRowStart );

								// display the line
								while ((( c = GetNextChar()) != EOF ) && ( c != LF ) && ( c != CR ) && ( nLength < MAXLISTLINE )) {
									if ( c == 0 )
										c = _TEXT(' ');
									Find->szText[ nLength++ ] = (TCHAR)c;
								}
								Find->szText[ nLength ] = _TEXT('\0');
								more_page( Find->szText, 0 );
							}

							// continue the search?
							if (( Find->fFlags & FFIND_QUERY_CONTINUE ) && ( QueryInputChar( FFIND_CONTINUE, YES_NO ) != YES_CHAR )) {
								fAbort = 1;
								break;
							}
						}

					// only show first match?
					if ( Find->fFlags & FFIND_ONLY_FIRST )
						break;

					if (( Find->fFlags & FFIND_SHOWALL ) == 0 )
						break;

					// skip to next line
					if (( Find->fFlags & FFIND_REVERSE_SEARCH ) == 0 ) {
						if ( Find->fFlags & FFIND_HEX_DISPLAY )
							LFile.lViewPtr += LFile.nSearchLen;
						else if ( ListMoveLine( 1 ) == 0 )
							break;
					}

					ListSetCurrent( LFile.lViewPtr );
				}

				if ( LFile.hHandle > 0 )
					_close( LFile.hHandle );
				LFile.hHandle = -1;
				if ( lLines == 0L )
					continue;

				Find->lLinesFound += lLines;

			} else {

				if (( Find->fFlags & FFIND_SUMMARY ) == 0 )
					more_page( LFile.szName, 0 );
			}

			Find->lFilesFound++;

			// only show first match?
			if (( fAbort ) || ( Find->fFlags & FFIND_ONLY_FIRST )) {
				dir_free( files );
				return ABORT_LINE;
			}

			// continue search?
			if ((( Find->fFlags & FFIND_TEXT ) == 0 ) && ( Find->fFlags & FFIND_QUERY_CONTINUE )) {

				if ( QueryInputChar( FFIND_CONTINUE, YES_NO ) != YES_CHAR ) {

					dir_free( files );
					return -ABORT_LINE;
				}
			}

		}

		dir_free( files );
		files = 0L;

		// search subdirectories too?
		if ( Find->fFlags & FFIND_SUBDIRS ) {

			insert_path( Find->szSource, WILD_FILE, path_part( Find->szSource ) );

			// save the current source filename start
			pszArg = strchr( Find->szSource, _TEXT('*') );
			entries = 0;

			// search for subdirectories
			if ( SearchDirectory((( Find->ulMode & 0xF) | 0x10 | FIND_ONLY_DIRS ), Find->szSource, (DIR_ENTRY _huge **)&files, &entries, &aRanges, (( gszSortSequence[0] == _TEXT('\0') ) ? 2 : 0 )) != 0 ) {
				nReturn = ERROR_EXIT;
				goto find_abort;
			}


			for ( i = 0; (( i < entries ) && ( fEndSearch == 0 )); i++ ) {

				// make directory name

				sprintf( pszArg, "%Fs\\%s", (( glDirFlags & DIRFLAGS_LFN ) ? files[i].lpszLFN : files[i].szFATName ), Find->szFilename );

				if (( nReturn = __ffindit( Find )) == ABORT_LINE )
					nReturn = 0;

				// check for ^C and reset ^C trapping
				if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ))
					nReturn = CTRLC;

				// quit on Cancel or error
				if ( nReturn )
					break;

			}

			dir_free( files );
			files = 0L;
		}
	}
find_abort:

	return nReturn;
}


// initialize the LIST buffers & globals
static int ListInit( void )
{
	// allocate 64K
	LFile.uTotalSize = 0xFFF0;

	if (( LFile.lpBufferStart = (LPBYTE)AllocMem( &LFile.uTotalSize )) == 0L )
		return ( error( ERROR_NOT_ENOUGH_MEMORY, NULL ));

	LFile.uBufferSize = ( LFile.uTotalSize / 2 );
	LFile.lpBufferEnd = LFile.lpBufferStart + LFile.uTotalSize;
	LFile.lpCurrent = LFile.lpBufferStart;
	LFile.lpEOF = 0L;

	// initialize LISTFILESTRUCT parms
	LFile.szName[0] = _TEXT('\0');
	LFile.hHandle = -1;
	LFile.lSize = LFile.lCurrentLine = 0L;

	LFile.lViewPtr = 0L;
	LFile.lFileOffset = 0;

	// set screen sizes
	nScreenRows = GetScrRows();
	nScreenColumns = GetScrCols();
	nScreenColumns--;

	return 0;
}


// open the file & initialize buffers
static int _fastcall ListOpenFile( LPTSTR pszFileName )
{
	LFile.lpEOF = 0L;


	if ( lListFlags & LIST_STDIN ) {

		// reading from STDIN
		LFile.hHandle = STDIN;
		strcpy( LFile.szName, LIST_STDIN_MSG );

		// DOS puts pipes into temp files; so we can
		//   get the size via a seek to the end
		if ( _isatty( STDIN ))
			LFile.lpEOF = LFile.lpCurrent;
		else
			LFile.lSize = QuerySeekSize( STDIN );

	} else {


			if ( LFile.hHandle > 0 )
				_close( LFile.hHandle );

			StripQuotes( pszFileName );
			if (( LFile.hHandle = _sopen( (( szClip[0] ) ? szClip : pszFileName ), (_O_BINARY | _O_RDONLY), _SH_DENYNO )) == -1 ) {

				return ( error( _doserrno, pszFileName ));

			}


		strcpy( LFile.szName, pszFileName );
	}

	ListHome();

	ListUpdateScreen();

	return 0;
}


// reset to the beginning of the file
static void ListHome( void )
{
	// Reset the memory buffer to purge old file data
	LFile.lpBufferEnd = LFile.lpBufferStart + LFile.uTotalSize;
	LFile.lpCurrent = LFile.lpBufferStart;
	LFile.lViewPtr = 0L;
	LFile.lFileOffset = 0;
	LFile.lCurrentLine = 0L;
	LFile.nListHorizOffset = 0;

	if (( lListFlags & LIST_STDIN ) == 0 ) {

	}

	ListFileRead( LFile.lpBufferStart, LFile.uTotalSize );

	// determine EOL character for this file
	for ( LFile.fEoL = CR; (( LFile.lpCurrent != LFile.lpEOF ) && ( LFile.lpCurrent < LFile.lpBufferEnd )); ) {

		if ( *LFile.lpCurrent == LF ) {
			LFile.fEoL = LF;
			break;
		}

		LFile.lpCurrent++;
	}

	LFile.lpCurrent = LFile.lpBufferStart;
}


// move up/down "n" lines
static int _fastcall ListMoveLine( int nRows )
{
	long lTemp, lRows;

	lRows = nRows;
	lTemp = MoveViewPtr( LFile.lViewPtr, &lRows );

	if ( lRows == nRows ) {
		LFile.lCurrentLine += lRows;
		LFile.lViewPtr += lTemp;
		return 1;
	}

	return 0;
}


static VOID SetRightMargin( void )
{
	nRightMargin = (( lListFlags & LIST_WRAP ) ? nScreenColumns + 1 : MAXLISTLINE );
}


// read the current file into the specified buffer
static void _near _fastcall ListFileRead( LPBYTE lpbBuffer, register unsigned int uReadSize )
{
	extern BOOL bUnicodeOutput;
	unsigned int uBytesRead = 0;
	unsigned long ulSize = 0;


	// position the read pointer

	if ( _isatty( LFile.hHandle )) {
		LFile.lpEOF = lpbBuffer;
		LFile.lSize = 0L;
		return;
	}

		_lseek( LFile.hHandle, LFile.lFileOffset, SEEK_SET );
		FileRead( LFile.hHandle, lpbBuffer, uReadSize, &uBytesRead );


	// check for file length the same size as the input buffer!
	LFile.lpEOF = ((( LFile.lSize > (LONG)( LFile.lFileOffset + uBytesRead )) && ( uBytesRead == uReadSize )) ? 0L : lpbBuffer + uBytesRead );

}


// set LFile.lpCurrent to the specified file offset
void _fastcall ListSetCurrent( long lOffset )
{
	if ( lOffset < LFile.lFileOffset ) {

		// get previous block
		while (( lOffset < LFile.lFileOffset ) && ( LFile.lFileOffset > 0L )) {
			if (( LFile.lFileOffset -= LFile.uTotalSize ) < 0L )
				LFile.lFileOffset = 0L;
		}

		ListFileRead( LFile.lpBufferStart, LFile.uTotalSize );

	} else if ( lOffset > (long)( LFile.lFileOffset + LFile.uTotalSize )) {

		// get next block
		while ( lOffset > (long)( LFile.lFileOffset + LFile.uTotalSize ))
			LFile.lFileOffset += LFile.uTotalSize;
		ListFileRead( LFile.lpBufferStart, LFile.uTotalSize );
	}

	// it's (now) in the current buffer
	LFile.lpCurrent = LFile.lpBufferStart + ( lOffset - LFile.lFileOffset );
}


// Display Line
static int _fastcall DisplayLine( UINT nRow, LONG lLinePtr )
{
	register int i, n;
	INT nLength, nBytesPrinted = 0, nHOffset = 0, nHexOffset;
	TCHAR *pszArg, cSave;
	TCHAR szBuffer[MAXLISTLINE+1];

	ListSetCurrent( lLinePtr );

	nLength = FormatLine( szBuffer, MAXLISTLINE, lLinePtr, &nBytesPrinted, TRUE );

	if (( lListFlags & LIST_HEX ) == 0 ) {
		nHOffset = LFile.nListHorizOffset;
		nLength -= LFile.nListHorizOffset;
		nHexOffset = 0;
	} else
		nHexOffset = 9;

	// adjust to max screen width
	if ( nLength > ( nScreenColumns + 1 ))
		nLength = nScreenColumns + 1;

	if (( nBytesPrinted > 0 ) && ( nLength > 0 )) {

		WriteCharStrAtt( nRow, 0, nLength, nNormal, szBuffer + nHOffset );

	}

	// if we're displaying a search result, highlight the string
	if (( LFile.fDisplaySearch ) && ( strlen( szBuffer ) > (int)nHOffset )) {

		for ( i = 0; ; ) {

			if ( LFile.fDisplaySearch & 2 )
				pszArg = strstr( szBuffer + nHOffset + i + nHexOffset, szListFindWhat );
			else
				pszArg = stristr( szBuffer + nHOffset + i + nHexOffset, szListFindWhat );
			if ( pszArg == NULL )
				break;

			n = strlen( szListFindWhat );
			cSave = pszArg[ n ];
			pszArg[ n ] = _TEXT('\0');
			i = (int)( pszArg - ( szBuffer + nHOffset ));

			WriteStrAtt( nRow, i, nInverse, pszArg );

			pszArg[ n ] = cSave;
			i += n;
		}
	}

	return nBytesPrinted;
}


// Compute number of lines between the specified points in the file
static LONG _fastcall ComputeLines( LONG lOldPtr, LONG lNewPtr )
{
	register int ch, n, fDirection = 0;
	LONG lRowCount;

	// always count upwards (it makes wrap computations a lot easier!)
	if ( lOldPtr > lNewPtr ) {
		lRowCount = lOldPtr;
		lOldPtr = lNewPtr;
		lNewPtr = lRowCount;
		fDirection++;
	}

	lRowCount = 0L;
	ListSetCurrent( lOldPtr );

	if (( lListFlags & LIST_HEX ) == 0 ) {

		SetRightMargin();

		for ( n = 0; ( lOldPtr < lNewPtr ); ) {

			// don't call GetNextChar unless absolutely necessary!
			if ( LFile.lpCurrent == LFile.lpEOF )
				break;

			if ( LFile.lpCurrent < LFile.lpBufferEnd ) {

				ch = *LFile.lpCurrent;

					LFile.lpCurrent++;

			} else if (( ch = GetNextChar()) == EOF )
				break;

			// do tab expansion
			if ( ch == TAB )
				n = ((( n + TABSIZE ) / TABSIZE ) * TABSIZE );

			else if ((++n >= nRightMargin) || ( ch == LFile.fEoL )) {

				// if EOL increment the row count
				lRowCount++;
				n = 0;
			}

				lOldPtr++;
		}

	} else
		lRowCount = (( lNewPtr - lOldPtr ) / 16 );

	return (( fDirection ) ? -lRowCount : lRowCount );
}


// format the current line in either ASCII or hex mode
int FormatLine( TCHAR szBuffer[], int nBufferLength, long lLinePtr, int *nBytesPrinted, int fTabs )
{
	register int i, n;
	TCHAR szScratch[8];
	int cChar;
	LPBYTE lpbSave;

	*nBytesPrinted = 0;
	lpbSave = LFile.lpCurrent;

	if (( lListFlags & LIST_HEX ) == 0 ) {

		// Print as ASCII until CR/LF or for "nRightMargin" characters.

		SetRightMargin();
		for ( i = 0; ( i < nBufferLength-1 ); ) {

			if ((( cChar = GetNextChar()) == EOF ) || ( cChar == LFile.fEoL ))
				break;

			// ignore CR's
			if ( cChar == (TCHAR)CR )
				continue;

			szBuffer[i] = cChar;

			if ( i >= nRightMargin ) {
				GetPrevChar();
				(*nBytesPrinted)--;
				break;
			}

			// Expand Tab Characters
			if (( szBuffer[i] == TAB ) && ( fTabs )) {

				n = ((( i + TABSIZE ) / TABSIZE ) * TABSIZE );
				for ( ; (( i < n ) && ( i < nRightMargin )); i++ ) {
					// insert spaces until next tab stop
					szBuffer[i] = _TEXT(' ');
				}

			} else {
				if ( lListFlags & LIST_HIBIT )
					szBuffer[i] &= 0x7F;
				i++;
			}
		}

		szBuffer[i] = _TEXT('\0');

	} else {

		// Convert from Hex and print.

		// clear the whole line to blanks
		sprintf( szBuffer, _TEXT("%04lx %04lx%70s"), (long)( lLinePtr / 0x10000L), (long)( lLinePtr & 0xFFFF ), NULLSTR );

		for ( i = 0; ( i < 16 ); i++ ) {

			if (( cChar = GetNextChar()) == EOF )
				break;

			if ( lListFlags & LIST_HIBIT )
				cChar &= 0x7F;

			sprintf( szScratch, _TEXT("%02x"), cChar );
			memmove( szBuffer + 10 + ( i * 3 ) + ( i / 8 ), szScratch, 2 * sizeof(TCHAR) );
			szBuffer[ i + 61 ] = (TCHAR)(((( cChar >= (TCHAR)TAB ) && ( cChar <= (TCHAR)CR )) || ( cChar == 0 ) || ( cChar == (TCHAR)27 )) ? _TEXT('.') : cChar );
		}

		i = 78;
	}

	// check if we've wrapped the buffer!
	if ( LFile.lpCurrent < lpbSave )
		lpbSave -= LFile.uBufferSize;

	*nBytesPrinted = ( LFile.lpCurrent - lpbSave );

	return i;
}


// MoveViewPtr - Update view pointer by scrolling the number of
//   lines specified in ptrRow.  If limited by the top or bottom
//   of the file, modify ptrRow to indicate the actual number of rows.
static LONG _fastcall MoveViewPtr( LONG lFilePtr, LONG *ptrRow )
{
	register int i, n;
	LONG lOffset = 0L, lRow, lRowCount = 0L;

	LONG lSaveOffset;
	char _far *lpTemp;
	char _far *lpSave, _far *lpEnd;

	ListSetCurrent( lFilePtr );

	lRow = *ptrRow;

	if (( lListFlags & LIST_HEX ) == 0 ) {

		SetRightMargin();
		if ( lRow > 0 ) {

			for ( ; ( lRowCount < lRow ); ) {

				lpSave = LFile.lpCurrent;
				lSaveOffset = LFile.lFileOffset;

				for ( n = 0; ; ) {

					if ((( i = GetNextChar()) == EOF ) || ( i == LFile.fEoL ))
						break;

					if ( n >= nRightMargin ) {
						GetPrevChar();
						break;
					}

					// get tab expansion
					if ( i == TAB )
						n = ((( n + TABSIZE ) / TABSIZE ) * TABSIZE );
					else
						n++;
				}

				// check for buffer move
				if ( LFile.lFileOffset != lSaveOffset )
					lpSave -= LFile.uBufferSize;

				lOffset += (long)( LFile.lpCurrent - lpSave );

				if ( i == EOF )
					break;

				lRowCount++;
			}

		} else {

			// move backwards
			for ( ; ( lRowCount > lRow ); lRowCount-- ) {

				lSaveOffset = (long)LFile.lFileOffset;

				// get previous line
				lpEnd = LFile.lpCurrent;
				n = GetPrevChar();
				while ((( i = GetPrevChar()) != EOF ) && ( i != LFile.fEoL ))
					;

				// if not at start of file, move to beginning
				//   of the line
				if ( i == EOF ) {
					if ( lpEnd == LFile.lpCurrent)
						break;
				} else
					GetNextChar();

				lpTemp = lpSave = LFile.lpCurrent;

				// check for buffer move
				if ( LFile.lFileOffset != lSaveOffset )
					lpEnd += LFile.uBufferSize;
				if ( lpEnd > LFile.lpBufferEnd )
					lpEnd = LFile.lpBufferEnd;

				// adjust for long or wrapped lines
				for ( n = 0; ( lpTemp < lpEnd ); ) {

					if ( *((TCHAR _far *)lpTemp ) != (TCHAR)LFile.fEoL ) {

						// check if past right margin but less
						//   than current position
						if (( n >= nRightMargin ) && ( lpTemp + 1 < lpEnd )) {
							n = 0;
							lpSave = lpTemp;
						}

						// kludge for TAB offsets
						if ( *lpTemp == TAB )
							n = ((( n + TABSIZE ) / TABSIZE ) * TABSIZE );
						else
							n++;
					}

						lpTemp++;
				}

				lOffset += (long)( lpSave - lpEnd );
			}
		}

		*ptrRow = lRowCount;

	} else {

		lOffset = ( lRow * 16L );
		*ptrRow = lRow;

		// Limit Checking
		if ((( lFilePtr == 0 ) && ( lRow < 0 )) || (( lFilePtr == LFile.lSize) && ( lRow > 0 ))){
			lOffset = 0L;
			*ptrRow = 0;
		} else if (( lFilePtr + lOffset) < 0 ) {
			lOffset = -lFilePtr;
			*ptrRow = ( lOffset / 16L );
		} else if (( lFilePtr + lOffset) > LFile.lSize ) {
			lOffset = (LONG)(LFile.lSize - lFilePtr );
			*ptrRow = ( lOffset / 16L );
		}
	}

	return lOffset;
}


// SearchFile - Search the file for the value in szSearchStr.
//   If found, the file pointer is modified to the location in the file.
static BOOL _near _fastcall SearchFile( long fFlags )
{
	register int i;
	INT   c, nOffset, fIgnoreCase = 0, fWildCards = 0, fWildStar, nLoopCounter = 0;
	UINT  uLimit;
	LONG  lTemp, lLocalPtr;
	TCHAR szSearchStr[128], *pszSearchStr;
	int fKBHit;

	nCodePage = QueryCodePage();

	lLocalPtr = (( fFlags & FFIND_TOPSEARCH ) ? 0L : LFile.lViewPtr );
	ListSetCurrent( lLocalPtr );

	pszSearchStr = (( fFlags & FFIND_NOERROR ) ? szFFindFindWhat : szListFindWhat);

	if ( fFlags & FFIND_HEX_SEARCH ) {

		// convert ASCII hex string to binary equivalent
		strupr( pszSearchStr );
		for ( i = 0; (( sscanf( pszSearchStr, _TEXT("%x%n"), &uLimit, &LFile.nSearchLen ) != 0 ) && ( LFile.nSearchLen > 0 )); i++ ) {
			sprintf( szSearchStr+i, FMT_CHAR, uLimit );
			pszSearchStr += LFile.nSearchLen;
		}

		if ( i == 0 )
			return 0;
		szSearchStr[i] = _TEXT('\0');
		LFile.nSearchLen = i;

	} else {

		// check for a wildcard search
		// a leading ` means "ignore wildcard characters"
		if ( *pszSearchStr == _TEXT('`') ) {
			pszSearchStr++;
			// strip (optional) trailing `
			if ((( i = strlen( pszSearchStr )) > 0 ) && ( pszSearchStr[--i] == _TEXT('`') ))
				pszSearchStr[i] = _TEXT('\0');
		} else if ((( fFlags & FFIND_NOWILDCARDS ) == 0 ) && ( strpbrk( pszSearchStr, WILD_CHARS ) != NULL ))
			fWildCards = 1;

		strcpy( szSearchStr, pszSearchStr );
		if (( LFile.nSearchLen = strlen( szSearchStr )) == 0 )
			return 0;

		if (( fFlags & FFIND_CHECK_CASE ) == 0 ) {
			strupr( szSearchStr );
			fIgnoreCase = 1;
		}
	}

	if ( fFlags & FFIND_REVERSE_SEARCH )
		_strrev( szSearchStr );

	if ( fWildCards == 0 ) {

		// not a real elegant search algorithm, but real small!
		for ( i = 0; ; ) {

			if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 ) {

				if (( c = GetNextChar()) == EOF )
					break;

			} else if (( c = GetPrevChar()) == EOF )
				break;

			// abort search if ESC key is hit
			if (( nLoopCounter % 1024 ) == 0 ) {

				// kbhit() in DOS tries to read from STDIN, which
				//   screws up a LIST /S pipe
				_asm {
					mov		ah, 1
					int		16h
					mov		fKBHit, 1
					jnz		KBHDone
					mov		fKBHit, 0
KBHDone:
				}
				if ( fKBHit && ( GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY ) == ESC ))
					break;

			}
			nLoopCounter++;

			if (( c >= _TEXT('a') ) && ( fIgnoreCase )) {
				if (( c <= _TEXT('z') ) || ( c >= 0x80 ))
					c = _ctoupper( c );
			}

			if ( c != (int)szSearchStr[i] ) {

				// rewind file pointer
				if ( i > 0 ) {

					if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 )
						LFile.lpCurrent -= i;
					else
						LFile.lpCurrent += i;
					i = 0;
				}

			} else if ( ++i >= LFile.nSearchLen )
				break;
		}

	} else {

		// search with wildcards
RestartWildcardSearch:
		for ( i = 0, fWildStar = 0, nOffset = 0; ( szSearchStr[i] != _TEXT('\0') ); ) {

			if ( szSearchStr[i] == _TEXT('*') ) {

				// skip past * and advance file pointer until a match
				//   of the characters following the * is found.
				for ( fWildStar = 1; (( szSearchStr[++i] == _TEXT('*') ) || ( szSearchStr[i] == _TEXT('?') )); )
					;

			} else {

				if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 ) {

					if (( c = GetNextChar()) == EOF ) {
						i = 0;
						break;
					}

				} else if (( c = GetPrevChar()) == EOF ) {
					i = 0;
					break;
				}

				if (( nLoopCounter % 1024 ) == 0 ) {
					// abort search if ESC key is hit

					// kbhit() in DOS tries to read from STDIN, which
					//   screws up a LIST /S pipe
					_asm {
						mov		ah, 1
						int		16h
						mov		fKBHit, 1
						jnz		KBHDone2
						mov		fKBHit, 0
KBHDone2:
					}
					if ( fKBHit && ( GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY ) == ESC ))
						break;

				}

				nLoopCounter++;

				nOffset++;

				if (( c >= _TEXT('a') ) && ( fIgnoreCase )) {
					if (( c <= _TEXT('z')) || ( c >= 0x80 ))
						c = _ctoupper( c );
				}

				// ? matches any single character
				if ( szSearchStr[i] == _TEXT('?') )
					i++;

				else if (( szSearchStr[i] == _TEXT('[') ) && ( fWildStar == 0 )) {

					// [ ] checks for a single character
					//   (including ranges)
					if ( wild_brackets( (TCHAR _far *)szSearchStr + i, (TCHAR)c, fIgnoreCase ) != 0 )
						goto wild_rewind;

					while (( szSearchStr[i]) && ( szSearchStr[i++] != _TEXT(']') ))
						;

				} else {

					if ( fWildStar ) {

						int nSaveI, nSaveOffset;

						// following a '*'; so we need to do a complex
						//   match since there could be any number of
						//   preceding characters
						for ( nSaveI = i, nSaveOffset = nOffset; ; ) {

							// don't search past the current line
							if ((( c == CR ) || ( c == LF )) && (( fFlags & FFIND_HEX_SEARCH ) == 0 ))
								goto RestartWildcardSearch;

							if ( szSearchStr[i] == _TEXT('[') ) {

								// get the first matching char
								if ( wild_brackets( (TCHAR _far *)szSearchStr+i, (TCHAR)c, fIgnoreCase ) == 0 ) {
									while (( szSearchStr[i] ) && ( szSearchStr[i++] != _TEXT(']') ))
										;
								}

							} else if ( c == (int)szSearchStr[i] )
								i++;

							else {
								// no match yet - keep scanning
								i = nSaveI;
								if ( --nOffset > nSaveOffset ) {

									if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 )
										LFile.lpCurrent -= ( nOffset - nSaveOffset);
									else
										LFile.lpCurrent += ( nOffset - nSaveOffset);
								}

								nOffset = nSaveOffset;

								if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 )
									nSaveOffset++;
								else if ( nSaveOffset > 0 )
									nSaveOffset--;
								goto NextWildChar;
							}

							if (( szSearchStr[i] == _TEXT('\0') ) || ( szSearchStr[i] == _TEXT('*') ) || ( szSearchStr[i] == _TEXT('?') ))
								break;
NextWildChar:
							if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 ) {

								if ((( c = GetNextChar()) == EOF ) || ( c == LFile.fEoL )) {
									i = 0;
									break;
								}

							} else if (( c = GetPrevChar()) == EOF ) {
								i = 0;
								break;
							}

							nOffset++;

							if ( fIgnoreCase )
								c = _ctoupper( c );
						}

						// if SearchStr is still an expression, we
						//   failed to find a match
						if ( szSearchStr[i] == _TEXT('[') ) {
							i = 0;
							nOffset = 0;
						}

						fWildStar = 0;

					} else if ( c == (int)szSearchStr[i] )
						i++;

					else {
wild_rewind:
						if ( --nOffset > 0 ) {

							if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 )
								LFile.lpCurrent -= nOffset;
							else
								LFile.lpCurrent += nOffset;
						}
						nOffset = 0;
						i = 0;
					}
				}
			}
		}

		// if at the end of the search string, we got a good match
		if (( szSearchStr[i] == _TEXT('\0') ) && ( nOffset > 0 ))
			i = LFile.nSearchLen = nOffset;
		else
			i = 0;
	}

	// reverse search meaning (for now, only works on a per-file basis, not per-line)
	if ( fFlags & FFIND_NOT ) {
		if ( i == 0 ) {
			i = LFile.nSearchLen;
			nOffset = 0;
		} else
			i = 0;
	}

	if ( i == LFile.nSearchLen ) {

		// get current position
		LFile.lViewPtr = ( (long)LFile.lFileOffset + (long)( LFile.lpCurrent - LFile.lpBufferStart ));
		if (( fFlags & FFIND_REVERSE_SEARCH ) == 0 ) {

			LFile.lViewPtr -= LFile.nSearchLen;
		}

		// hex search from FFIND?
		if (( fFlags & FFIND_NOERROR ) && ( fFlags & FFIND_HEX_DISPLAY ))
			return 1;

			// adjust line start & line count for hex display
		if ( lListFlags & LIST_HEX ) {

			LFile.lViewPtr -= ( LFile.lViewPtr % 16 );
			LFile.lCurrentLine = ( LFile.lViewPtr / 16 );

		} else {

			// get beginning of current line
			while ((( c = GetPrevChar()) != EOF ) && ( c != LFile.fEoL ))
				;
			if ( c != EOF )
				GetNextChar();

			// get line number (tricky because the screen may
			//   be wrapped)
			lTemp = (long)( LFile.lFileOffset + ( LFile.lpCurrent - LFile.lpBufferStart ));
			i = (int)ComputeLines( lTemp, LFile.lViewPtr );

			if ( fFlags & FFIND_TOPSEARCH )
				LFile.lCurrentLine = 0;
			LFile.lCurrentLine += ComputeLines( lLocalPtr, lTemp );

			// get beginning of line with search text
			for ( LFile.lViewPtr = lTemp; ( i > 0 ); i-- )
				ListMoveLine( 1 );
			ListSetCurrent( LFile.lViewPtr );
		}

		return 1;
	}

	// no matching string found
	if (( fFlags & FFIND_NOERROR ) == 0 ) {

		clear_header();
		WriteStrAtt( 0, 1, nInverse, LIST_NOT_FOUND );
		SetCurPos( 0, strlen( LIST_NOT_FOUND ) + 1 );
		honk();
		GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY );

	}

	return 0;
}




// get the previous buffer character
static int _near GetPrevChar( void )
{

	if ( LFile.lpCurrent <= LFile.lpBufferStart ) {		// at top of buffer?

		if ( LFile.lFileOffset == 0L )	// already at file beginning?

			return EOF;

		// move the 1st half of the buffer to the 2nd half
		_fmemmove( LFile.lpBufferStart + LFile.uBufferSize, LFile.lpBufferStart, LFile.uBufferSize );

		// read the previous block into the 1st half
		LFile.lFileOffset -= LFile.uBufferSize;

		ListFileRead( LFile.lpBufferStart, LFile.uBufferSize );
		LFile.lpCurrent += LFile.uBufferSize;
	}


	return *(--LFile.lpCurrent);
}


// get the next buffer character
static int _near GetNextChar( void )
{

	if ( LFile.lpCurrent == LFile.lpEOF )		// already at file end?
		return EOF;

	if ( LFile.lpCurrent >= LFile.lpBufferEnd ) {	// at end of buffer?

		// move the 2nd half of the buffer to the 1st half
		_fmemmove( LFile.lpBufferStart, LFile.lpBufferStart+LFile.uBufferSize, LFile.uBufferSize );

		// read a new block into the 2nd half & adjust all the
		//   pointers back to the 1st half
		LFile.lFileOffset += (unsigned int)LFile.uTotalSize;
		ListFileRead( LFile.lpBufferStart+LFile.uBufferSize, LFile.uBufferSize );

		LFile.lFileOffset -= LFile.uBufferSize;
		LFile.lpCurrent -= LFile.uBufferSize;
	}


	return *LFile.lpCurrent++;
}
