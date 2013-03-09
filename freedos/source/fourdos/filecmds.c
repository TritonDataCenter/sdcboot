

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


// FILECMDS.C - File handling commands (COPY, MOVE, DEL, etc.) for 4xxx / TCMD
//   Copyright (c) 1988 - 2003 Rex C. Conn   All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>

#include <msdos.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/locking.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"


extern void UpdateFuzzyTree( LPTSTR , LPTSTR , int );

static void wildname( LPTSTR , LPTSTR , LPTSTR );

static int FileCopy( LPTSTR , LPTSTR , unsigned long, long *, FILESEARCH * );


static int _near delete_fcb( unsigned int );
static int _near parse_fcb( unsigned int, unsigned int );


static int _fastcall FileOutput( LPTSTR , int );

static int fDiskFull;
RANGES aRanges;				// date/time/size ranges


// display the number of files copied/moved/renamed/deleted, with special
//   kludge for "1 file" vs. "2 files"
void _fastcall FilesProcessed( LPTSTR szFormat, long lFiles )
{
	printf( szFormat, lFiles, (( lFiles == 1L ) ? ONE_FILE : MANY_FILES ));
}


// COPY_OPTIONS
#define COPY_BY_ATTRIBUTES 1
#define COPY_ASCII 2
#define COPY_BINARY 4
#define COPY_CHANGED 8
#define COPY_NOERRORS 0x10
#define COPY_PERCENT 0x20
#define COPY_HIDDEN 0x40
#define COPY_FTP_ASCII 0x80
#define COPY_MODIFIED 0x100
#define COPY_NOTHING 0x200
#define COPY_KEEP 0x400
#define COPY_QUERY 0x800
#define COPY_QUIET 0x1000
#define COPY_REPLACE 0x2000
#define COPY_SUBDIRS 0x4000
#define COPY_TOTALS 0x8000
#define COPY_UPDATED 0x10000L
#define COPY_VERIFY 0x20000L
#define COPY_CLEAR_ARCHIVE 0x40000L
#define COPY_DOS62_Y 0x80000L
#define COPY_KEEP_ATTRIBUTES 0x100000L
#define COPY_RESTART 0x200000L
#define COPY_OVERWRITE_RDONLY 0x400000L
#define COPY_EXISTS 0x800000L


typedef struct 
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szSourceName[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	TCHAR szTargetName[MAXFILENAME];
	LPTSTR szFilename;
	LPTSTR pFirstCopyArg;		// pointer to first source arg in COPY
	unsigned long fOptions;
	unsigned long fFlags;
	long lFilesCopied;

	long lAppendOffset;

	FILESEARCH t_dir;
} COPY_STRUCT;

static int _copy( COPY_STRUCT *);
static int fClip;


// copy or append file(s)
int _near Copy_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	TCHAR szClip[MAXFILENAME], *pszTarget = NULL;
	int nArgc, nTlen, nReturn = 0, nLength, fOldVerify = 0;
	long lCopy;
	COPY_STRUCT Copy;

	Copy.lFilesCopied = 0L;
	Copy.lAppendOffset = 0;
	Copy.fFlags = COPY_CREATE;
	Copy.fOptions = 0;

	fDiskFull = 0;

	// remove the white space around appends (+)
	collapse_whitespace( pszCmdLine, _TEXT("+") );

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	gnExclusiveMode = gnInclusiveMode = gnOrMode = 0;

	// scan command line, parsing for & removing switches
	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); ) {

		// check for MS-DOS 6.2 /-Y option & convert it to a COPY /R
		if ( _stricmp( pszArg, _TEXT("/-Y") ) == 0 )
			lCopy = COPY_REPLACE;

		else if (( lCopy = switch_arg( pszArg, "*ABCEGH MNOPQRSTUVXYK Z" )) == -1 )

			return ( Usage( COPY_USAGE ));	// invalid switch

		Copy.fOptions |= lCopy;

		// leading ASCII (/A) or Binary (/B) switch?
		if ( nArgc == 0 ) {
			if ( lCopy & COPY_ASCII )
				Copy.fFlags |= ASCII_SOURCE;
			else if ( lCopy & COPY_BINARY )
				Copy.fFlags |= BINARY_SOURCE;
		}

		// remove leading /A & /B, & collapse spaces before
		//   trailing /A & /B
		if (( lCopy & ( COPY_ASCII | COPY_BINARY )) && ( nArgc > 0 )) {

			// strip leading whitespace
			while (( iswhite( *(--gpNthptr) )) && ( gpNthptr >= pszCmdLine ))
				strcpy( gpNthptr, gpNthptr+1 );

		} else if ( *pszArg == gpIniptr->SwChr ) {

			// remove the switch
			strcpy( gpNthptr, skipspace( gpNthptr + strlen( pszArg )));
			continue;

		} else if ( nArgc > 0 ) {
			// save pointer to potential target
			pszTarget = gpNthptr;
		}

		nArgc++;
	}

	// check for COMMAND.COM / CMD.EXE prompt if file exists & not in batch
	if (( gpIniptr->CopyPrompt ) && ( cv.bn < 0 )) 
		Copy.fOptions |= COPY_REPLACE;

	// check for flag to preserve all attributes (won't work w/Netware!)
	if ( Copy.fOptions & COPY_KEEP_ATTRIBUTES )
		Copy.fFlags |= MOVING_FILES;

	// if query, turn off quiet flag
	if ( Copy.fOptions & COPY_QUERY )
		Copy.fOptions &= ~( COPY_QUIET | COPY_TOTALS );

	// display percentage copied?
	if (( Copy.fOptions & COPY_PERCENT ) && (( Copy.fOptions & COPY_QUIET ) == 0 ))
		Copy.fFlags |= COPY_PROGRESS;

	// abort if no file arguments
	if ( first_arg( pszCmdLine ) == NULL )
		return ( Usage( COPY_USAGE ));

	// get the target name, or "*.*" for default name
	if ( pszTarget != NULL ) {

		copy_filename( Copy.szTarget, first_arg( pszTarget ));
 
		// check for ASCII (/A) or binary (/B) switches at end of line
		switch ( switch_arg( ntharg( pszTarget, 1 ), _TEXT("AB") )) {
		case 1:
			Copy.fFlags |= ASCII_TARGET;
			break;
		case 2:
			Copy.fFlags |= BINARY_TARGET;
		}

		*pszTarget = _TEXT('\0');		// remove target from command line

		// check target for device name
		if ( QueryIsDevice( Copy.szTarget )) {
			// set "target is device" flag
			Copy.fFlags |= DEVICE_TARGET;
		} else if ( QueryIsPipeName( Copy.szTarget )) {
			// set "target is device" flag
			Copy.fFlags |= PIPE_TARGET;
		}

		if ( Copy.fFlags & ( DEVICE_TARGET | PIPE_TARGET )) {
			// can't "replace" or "update" device or pipe!
			Copy.fOptions &= ~( COPY_REPLACE | COPY_UPDATED);
		}

	} else {
		if (( pszArg = (( Copy.fOptions & COPY_SUBDIRS ) ? gcdir( NULL, 0 ) : WILD_FILE )) == NULL )
			return ERROR_EXIT;
		strcpy( Copy.szTarget, pszArg );
	}

	if ( Copy.fOptions & COPY_VERIFY ) {

		// save original VERIFY status
		fOldVerify = QueryVerifyWrite();

		// set VERIFY ON
		SetVerifyWrite( 1 );

	}

	if ( setjmp( cv.env ) == -1 ) {
		nReturn = CTRLC;
		goto copy_abort;
	}

	// if target not a char device or pipe, build full filename
	if (( Copy.fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 ) {

		if ( mkfname( Copy.szTarget, 0 ) == NULL )
			goto copy_abort;

		// don't build target name yet if copying subdirs
		if (( Copy.fOptions & COPY_SUBDIRS ) == 0 ) {

			if (( AddWildcardIfDirectory( Copy.szTarget ) == 0 ) &&  ( Copy.szTarget[ strlen( Copy.szTarget ) - 1] == _TEXT('\\') )) {
				nReturn = error( ERROR_PATH_NOT_FOUND, Copy.szTarget );
				goto copy_abort;
			}
		}
	}

	// check for appending
	pszArg = pszCmdLine;
	do {
		pszArg = scan( pszArg, _TEXT("+"), QUOTES + 1 );

		if ( *pszArg == _TEXT('+') ) {

			// remove the '+' and set append flags
			*pszArg = _TEXT(' ');

			// default for appending files is ASCII
			if (( Copy.fFlags & BINARY_SOURCE ) == 0 )
				Copy.fFlags |= ASCII_SOURCE;

			// can't append to a device or pipe!
			if (( Copy.fFlags & DEVICE_TARGET ) == 0 )
				Copy.fFlags |= APPEND_FLAG;
		}

	} while ( *pszArg );

	Copy.pFirstCopyArg = pszCmdLine;

	nTlen = strlen( Copy.szTarget );

	for ( nReturn = nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); nArgc++ ) {

		// clear "source is device/pipe" flags
		Copy.fFlags &= ~( DEVICE_SOURCE | PIPE_SOURCE );

		if ( QueryIsDevice( pszArg ))
			Copy.fFlags |= DEVICE_SOURCE;
		else if ( QueryIsPipeName( pszArg ))
			Copy.fFlags |= PIPE_SOURCE;
		else {
			// if source is not a device, make a full filename
			if ( mkfname( pszArg, 0 ) == NULL )
				break;
			
			AddWildcardIfDirectory( pszArg );
		}

		copy_filename( Copy.szSource, pszArg );

		// check for trailing ASCII (/A) or Binary (/B) switches
		if (( lCopy = switch_arg( ntharg( pszCmdLine, nArgc+1 ), _TEXT("AB") )) > 0 ) {

			// remove the source ASCII or binary flags
			Copy.fFlags &= ~( ASCII_SOURCE | BINARY_SOURCE );
			Copy.fFlags |= (( lCopy == 1 ) ? ASCII_SOURCE : BINARY_SOURCE );
			nArgc++;
		}

		// check for dumdums doing an infinitely recursive COPY
		//   (i.e., "COPY /S . subdir" )
		nLength = ((( pszArg = path_part( Copy.szSource )) != NULL ) ? strlen( pszArg ) : 0 );
		if ( Copy.fOptions & COPY_SUBDIRS ) {
			if ( _strnicmp( pszArg, Copy.szTarget, nLength ) == 0 )
				return ( error( ERROR_4DOS_INFINITE_COPY, Copy.szSource ));
		}

		// save the source filename part ( for recursive calls and
		//   include lists)
		Copy.szFilename = Copy.szSource + nLength;
		pszArg = (LPTSTR)_alloca( ( strlen( Copy.szFilename ) + 1 ) * sizeof(TCHAR) );
		Copy.szFilename = strcpy( pszArg, Copy.szFilename );

		szClip[0] = _TEXT('\0');
		fClip = 0;
		if ( stricmp( Copy.szSource, CLIP ) == 0 ) {
			RedirToClip( szClip, 99 );
			if ( CopyFromClipboard( szClip ) != 0 )
				continue;
			strcpy( Copy.szSource, szClip );
			fClip = 1;
			Copy.fFlags &= ~DEVICE_SOURCE;
		} else if ( stricmp( Copy.szTarget, CLIP ) == 0 ) {
			RedirToClip( szClip, 99 );
			if ( Copy.fFlags & APPEND_FLAG ) {
				if ( CopyFromClipboard( szClip ) != 0 )
					continue;
			}
			strcpy( Copy.szTarget, szClip );
			fClip = 2;
			Copy.fFlags &= ~DEVICE_TARGET;
		}

		nReturn = _copy( &Copy );

		HoldSignals();
		if ( nReturn != CTRLC ) {

			if ( fClip & 1 )
				strcpy( Copy.szSource, CLIP );
			else if ( fClip & 2 ) {
				// copying to clipboard - get a handle to the temp
				//   file and use it to copy the file to the clip
				if (( fClip = _sopen( Copy.szTarget, _O_RDONLY | _O_BINARY, _SH_DENYNO )) >= 0 ) {
					CopyToClipboard( fClip );
					_close( fClip );
				}
				strcpy( Copy.szTarget, CLIP );
			}
		}

		if ( szClip[0] )
			remove( szClip );
		EnableSignals();

		// restore original "target"
		if (( Copy.fFlags & APPEND_FLAG ) == 0 )
			Copy.szTarget[ nTlen ] = _TEXT('\0');

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException ) || ( nReturn == CTRLC )) {
			nReturn = (( fDiskFull == 0 ) ? CTRLC : ERROR_EXIT );
			break;
		}


	}

copy_abort:

	// reset VERIFY flag to original state
	if ( Copy.fOptions & COPY_VERIFY )
		SetVerifyWrite( fOldVerify );

	if (( Copy.fOptions & COPY_QUIET ) == 0 ) {

		FilesProcessed( FILES_COPIED, Copy.lFilesCopied );
	}


	return (( Copy.lFilesCopied != 0L ) ? nReturn : ERROR_EXIT );
}



#pragma alloc_text( MISC_TEXT, _copy )


static int _copy( register COPY_STRUCT *Copy )
{
	register int nFind;
	int i, nReturn = 0;
	LPTSTR pszSourceArg, pszTargetArg;
	FILESEARCH dir;
	long lMode = 0;


	EnableSignals();

	// copy date/time/size range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	// get hidden & system files too
	if (( Copy->fOptions & COPY_BY_ATTRIBUTES ) || ( Copy->fOptions & COPY_HIDDEN))
		lMode |= 0x07;

	// if copying subdirectories, create the target (must be a directory!)
	if ( Copy->fOptions & COPY_SUBDIRS ) {

		// remove trailing '/' or '\'
		strip_trailing( Copy->szTarget+3, SLASHES );

		if (( Copy->fOptions & COPY_NOTHING ) == 0 ) {

			// if we created the directory, then copy any
			//    existing description
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 ) {

				i = is_dir( Copy->szTarget );

				MakeDirectory( Copy->szTarget, 1 );

				if ( is_dir( Copy->szTarget ) == 0 )
					return ( error( ERROR_PATH_NOT_FOUND, Copy->szTarget ));

				// copy description to newly-created target
				if ( i == 0 ) {
					copy_filename( Copy->szSourceName, path_part( Copy->szSource ));
					strip_trailing( Copy->szSourceName+3, SLASHES );
					process_descriptions( Copy->szSourceName, Copy->szTarget, DESCRIPTION_COPY );
				}
			}
		}

		if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
			mkdirname( Copy->szTarget, WILD_FILE );
	}

	// suppress error message from find_file
	if ( Copy->fOptions & COPY_NOERRORS ) {
		lMode |= FIND_NO_ERRORS;
		Copy->fFlags |= NO_ERRORS;
	}

	for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

		// if it's a device, only look for it once
		if ( Copy->fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) {

			if ( nFind == FIND_NEXT )
				break;

			copy_filename( Copy->szSourceName, Copy->szSource );

		} else {

			if ( find_file( nFind, Copy->szSource, ( lMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE ), &dir, Copy->szSourceName ) == NULL )
				break;

			if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
				FindClose( dir.hdir );
				return CTRLC;
			}

			CheckForBreak();

		}

		// clear overwrite query flag
		Copy->fOptions &= ~COPY_EXISTS;

		// don't copy unmodified files
		if (( Copy->fOptions & COPY_MODIFIED ) && (( dir.attrib & _A_ARCH ) == 0 ))
			continue;

		// build the target name
		wildname( Copy->szTargetName, Copy->szTarget, Copy->szSourceName );

		// if changed (/C), only copy source if target is older
		// if update (/U), only copy the source if the target is
		//   older or doesn't exist
		// if replace (/R) prompt before overwriting existing file

		if (( Copy->fOptions & ( COPY_KEEP | COPY_UPDATED | COPY_CHANGED | COPY_REPLACE )) && ( find_file( FIND_FIRST, Copy->szTargetName, 0x07L | FIND_CLOSE | FIND_EXACTLY | FIND_NO_ERRORS | FIND_NO_FILELISTS, &(Copy->t_dir), NULL ) != NULL )) {

			if ( Copy->fOptions & COPY_KEEP )
				continue;		// don't overwrite existing file (/O)

			if ( Copy->fOptions & ( COPY_UPDATED | COPY_CHANGED )) {

				if (( Copy->t_dir.fd.wr_date > dir.fd.wr_date ) || (( Copy->t_dir.fd.wr_date == dir.fd.wr_date ) && ( Copy->t_dir.ft.wr_time >= dir.ft.wr_time )))

					continue;
			}

			// if querying for replacement, don't ask if appending!
			if (( Copy->fOptions & COPY_REPLACE ) && ( Copy->fFlags & COPY_CREATE ))
				Copy->fOptions |= COPY_EXISTS;

		} else if ( Copy->fOptions & COPY_CHANGED )
			continue;	// target doesn't exist

		// check for special conditions w/append
		//   (the source is the same as the target)
		if ( _stricmp( Copy->szSourceName, Copy->szTargetName ) == 0 ) {

			// if not the first arg, check to make sure the arg
			//   wasn't the target & hasn't been overwritten
			if (( Copy->fFlags & COPY_CREATE ) == 0 ) {
				// check for "copy all.lst + *.lst"
				if ( _stricmp( first_arg( Copy->pFirstCopyArg ), Copy->szSourceName ) != 0 )
					nReturn = error( ERROR_4DOS_CONTENTS_LOST, Copy->szSourceName );
			}

			// if target file is first source file, ignore it
			if ( Copy->fFlags & APPEND_FLAG )
				goto next_copy;
		}

		if ((( Copy->fOptions & ( COPY_QUIET | COPY_TOTALS )) == 0 ) || (( Copy->fOptions & COPY_REPLACE ) && ( Copy->fOptions & COPY_EXISTS ))) {

			printf( "%s =>%s %s", (( fClip == 1 ) ? CLIP : Copy->szSourceName ), (( Copy->fFlags & COPY_CREATE ) ? NULLSTR : ">" ), (( fClip == 2 ) ? CLIP : Copy->szTargetName ));


			// query if /Q, or /R and target exists
			if ( Copy->fOptions & ( COPY_QUERY | COPY_EXISTS )) {

				if ((( i = QueryInputChar((( Copy->fOptions & COPY_EXISTS ) ? REPLACE : NULLSTR ), YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
					Copy->fOptions &= ~( COPY_QUERY | COPY_REPLACE );
				else if ( i == NO_CHAR )
					continue;
				else if ( i == ESCAPE ) {
					FindClose( dir.hdir );
					break;
				}
			} else
				crlf();
		}

		if (( Copy->fOptions & COPY_NOTHING ) == 0 ) {

			// if /Z, overwrite RDONLY files
			if ( Copy->fOptions & COPY_OVERWRITE_RDONLY )
				SetFileMode( Copy->szTargetName, 0 );

			if ((( nReturn = FileCopy( Copy->szTargetName, Copy->szSourceName, Copy->fFlags, &( Copy->lAppendOffset ), &dir )) == CTRLC ) || ( cv.fException ) || ( setjmp( cv.env ) == -1 )) {
				// reset the directory search handles
				FindClose( dir.hdir );
				return CTRLC;
			}

			if ( nReturn == 0 ) {

				Copy->lFilesCopied++;

				if ( Copy->fOptions & COPY_CLEAR_ARCHIVE ) {
					// clear the archive bit
					SetFileMode( Copy->szSourceName, dir.attrib & 0xDF );
				}

				// copy an existing description to the new name
				if (( Copy->fFlags & ( APPEND_FLAG | DEVICE_SOURCE | DEVICE_TARGET | PIPE_SOURCE | PIPE_TARGET)) == 0 )
					process_descriptions( Copy->szSourceName, Copy->szTargetName, DESCRIPTION_COPY );
			}
		}

next_copy:
		EnableSignals();

		// if target is unambiguous file, append next file(s)
		//   if appending to wildcard, use first target name
		if ( strpbrk( Copy->szTarget, WILD_CHARS ) == NULL ) {
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
				Copy->fFlags |= APPEND_FLAG;
		} else if ( Copy->fFlags & APPEND_FLAG )
			copy_filename( Copy->szTarget, Copy->szTargetName );

		// if appending, clear the CREATE flag
		if ( Copy->fFlags & APPEND_FLAG )
			Copy->fFlags &= ~COPY_CREATE;

	}

	// copy subdirectories too?
	if ( Copy->fOptions & COPY_SUBDIRS ) {

		CheckFreeStack( 1024 );

		sprintf( Copy->szSource, FMT_DOUBLE_STR, path_part( Copy->szSource), WILD_FILE );

		// save the current source filename start
		pszSourceArg = strchr( Copy->szSource, _TEXT('*') );

		// save the current target filename start
		if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
			pszTargetArg = Copy->szTarget + strlen( path_part( Copy->szTarget ));

		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, and to return
		//   only subdirectory names

		for ( nFind = FIND_FIRST; ( find_file( nFind, Copy->szSource, ( lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// make the new "source" and "target"

			sprintf( pszSourceArg, FMT_PATH_STR, dir.szFileName, Copy->szFilename );

			// strip the filename part from the target
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
				copy_filename( pszTargetArg, dir.szFileName );

			// process directory tree recursively
			nReturn = _copy( Copy );
			if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSourceArg, WILD_FILE );

		}
	}

	return nReturn;
}


#define MOVE_BY_ATTRIBUTES 1
#define MOVE_CHANGED 2
#define MOVE_TO_DIR 4
#define MOVE_NOERRORS 8
#define MOVE_FORCEDEL 0x10
#define MOVE_PERCENT 0x20
#define MOVE_HIDDEN 0x40
#define MOVE_FTP_ASCII 0x80
#define MOVE_MODIFIED 0x100
#define MOVE_NOTHING 0x200
#define MOVE_KEEP 0x400
#define MOVE_QUERY 0x800
#define MOVE_QUIET 0x1000
#define MOVE_REPLACE 0x2000
#define MOVE_SUBDIRS 0x4000
#define MOVE_TOTALS 0x8000
#define MOVE_UPDATED 0x10000L
#define MOVE_VERIFY 0x20000L
#define MOVE_WIPE 0x40000L
#define MOVE_OVERWRITE_RDONLY 0x80000L
#define MOVE_RESTART 0x100000L
#define MOVE_EXISTS 0x200000L
#define MOVE_SOURCEISDIR 0x400000L

typedef struct 
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szSourceName[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	TCHAR szTargetName[MAXFILENAME];
	LPTSTR szFilename;
	long fFlags;
	long lFilesMoved;
	FILESEARCH t_dir;
} MOVE_STRUCT;

static int _mv( MOVE_STRUCT *);


// move files by renaming across directories or copying/deleting
int _near Mv_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	unsigned int fTargetIsDir, fFreed;
	int nArgc, nTlen, nReturn = 0, nLength, fOldVerify = 0;
	QDISKINFO DiskInfo;
	MOVE_STRUCT Move;

	long lBytesFreed = 0L;


	Move.lFilesMoved = 0L;
	fDiskFull = 0;
	fClip = 0;

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove switches
	// abort if no filename arguments

	if (( GetSwitches( pszCmdLine, "*CDEFGH MNOPQRSTUV Z", &(Move.fFlags), 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))

		return ( Usage( MOVE_USAGE ));

	// if query before moving, disable MOVE_QUIET & MOVE_TOTALS
	if ( Move.fFlags & MOVE_QUERY )
		Move.fFlags &= ~( MOVE_QUIET | MOVE_TOTALS );

	// check for COMMAND.COM / CMD.EXE prompt if file exists & not in batch
	if (( gpIniptr->CopyPrompt ) && ( cv.bn < 0 )) 
		Move.fFlags |= MOVE_REPLACE;

	// get the target name, or "*.*" for default name
	if (( pszArg = last_arg( pszCmdLine, &nArgc )) != NULL ) {

		copy_filename( Move.szTarget, pszArg );
		*gpNthptr = _TEXT('\0');		// remove target from command line

	} else {
		// turn off "create directory" flag - if no target is specified,
		//   files are being moved to the current (existing!) directory
		Move.fFlags &= ~MOVE_TO_DIR;

		if (( pszArg = (( Move.fFlags & MOVE_SUBDIRS ) ? gcdir( NULL, 0 ) : WILD_FILE )) == NULL )
			return ERROR_EXIT;
		strcpy( Move.szTarget, pszArg );
	}

	// can't move a file to a device or pipe!
	if (( QueryIsDevice( Move.szTarget )) || ( QueryIsPipeName( Move.szTarget )))
		return ( error( ERROR_ACCESS_DENIED, Move.szTarget ));

	if ( mkfname( Move.szTarget, 0 ) == NULL )
		return ERROR_EXIT;

	// is the target a directory?
	if (( fTargetIsDir = is_dir( Move.szTarget )) == 0 ) {

		if ( Move.fFlags & MOVE_TO_DIR ) {

			// if /D, target MUST be directory, so prompt to create it

			printf( MOVE_CREATE_DIR, Move.szTarget );

			if ( QueryInputChar( NULLSTR, YES_NO ) != YES_CHAR )
				return ERROR_EXIT;

		// if > 1 source, target must be dir!
		} else if (( nArgc > 1 ) || ( Move.szTarget[ strlen( Move.szTarget)-1] == _TEXT('\\') ))
			return ( error( ERROR_PATH_NOT_FOUND, Move.szTarget ));
	}

	nTlen = strlen( Move.szTarget );

	if ( Move.fFlags & MOVE_VERIFY ) {

		// save original VERIFY status
		fOldVerify = QueryVerifyWrite();

		// set VERIFY ON
		SetVerifyWrite( 1 );
	}

	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); nArgc++ ) {

		if ( mkfname( pszArg, 0 ) == NULL ) {
			nReturn = ERROR_EXIT;
			break;
		}

		Move.fFlags &= ~MOVE_SOURCEISDIR;
		if ( is_dir( pszArg )) {

			// check to see if it's just a directory rename in the
			//   same path
			copy_filename( Move.szSource, path_part( pszArg ) );
			if (( _stricmp( Move.szSource, path_part( Move.szTarget) ) == 0 ) && (is_file( Move.szTarget) == 0 ) && ( fTargetIsDir == 0 )) {
				if ( rename( pszArg, Move.szTarget ) == 0 ) {
					process_descriptions( pszArg, Move.szTarget, ( DESCRIPTION_COPY | DESCRIPTION_REMOVE ) );
					Move.lFilesMoved++;
					continue;
				}
			}

			// get entire contents of subdirectory
			mkdirname( pszArg, WILD_FILE );
			Move.fFlags |= MOVE_SOURCEISDIR;
		}

		copy_filename( Move.szSource, pszArg );

		// check for dumdums doing an infinitely recursive MOVE
		//   (i.e., "MOVE /S . subdir" )
		pszArg = path_part( Move.szSource );
		nLength = strlen( pszArg );
		if ( Move.fFlags & MOVE_SUBDIRS ) {
			if ( _strnicmp( pszArg, Move.szTarget, nLength ) == 0 ) {
				nReturn = error( ERROR_4DOS_INFINITE_MOVE, Move.szSource );
				break;
			}
		}

		// save the source filename part ( for recursive calls and
		//   include lists)
		Move.szFilename = Move.szSource + nLength;
		pszArg = (LPTSTR)_alloca( ( strlen( Move.szFilename ) + 1 ) * sizeof(TCHAR) );
		Move.szFilename = strcpy( pszArg, Move.szFilename );

		fFreed = 0;
		if (( strnicmp( Move.szSource, Move.szTarget, 2 ) != 0 ) && ( is_net_drive( Move.szSource ) == 0 ) && ( QueryDriveRemote( gcdisk( Move.szSource )) == 0 )) {
			fFreed = 1;

			// get TRUENAME (to see through JOIN & SUBST)
			pszArg = true_name( Move.szSource, first_arg( Move.szSource ));
			QueryDiskInfo( pszArg, &DiskInfo, 1 );

			lBytesFreed -= DiskInfo.BytesFree;
		}

		nReturn = _mv( &Move );

		// restore original target name
		Move.szTarget[ nTlen ] = _TEXT('\0');

		if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
			nReturn = (( fDiskFull == 0 ) ? CTRLC : ERROR_EXIT );
			break;
		}

		if ( fFreed ) {

			pszArg = true_name( Move.szSource, first_arg( Move.szSource ));
			QueryDiskInfo( pszArg, &DiskInfo, 1 );

			lBytesFreed += DiskInfo.BytesFree;
		}
	}

	// reset VERIFY flag to original state
	if ( Move.fFlags & MOVE_VERIFY )
		SetVerifyWrite( fOldVerify );

	if (( Move.fFlags & MOVE_QUIET) == 0 ) {\

		FilesProcessed( FILES_MOVED, Move.lFilesMoved );
		if (( lBytesFreed > 0L ) && ( Move.lFilesMoved > 0L ))
			printf( FILES_BYTES_FREED, lBytesFreed );
		crlf();
	}



	return (( Move.lFilesMoved != 0L ) ? nReturn : ERROR_EXIT );
}



#pragma alloc_text( MISC_TEXT, _mv )


static int _mv(register MOVE_STRUCT *Move)
{
	register int nFind;
	int nReturn = 0, i;
	unsigned int uRDONLY;
	unsigned long fCopyFlags = COPY_CREATE | MOVING_FILES;
	TCHAR *pszArg, *pszSourceArg, *pszTargetArg;
	long lMode = 0;

	long lAppendOffset = 0L;

	FILESEARCH dir;


	dir.hdir = INVALID_HANDLE_VALUE;
	if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
		FindClose( dir.hdir );
		return CTRLC;
	}

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	// get hidden & system files too
	if (( Move->fFlags & MOVE_BY_ATTRIBUTES ) || ( Move->fFlags & MOVE_HIDDEN))
		lMode |= 0x07;


	// if the target is a directory, add a wildcard filename
	if (( AddWildcardIfDirectory( Move->szTarget ) == 0 ) &&  (( Move->fFlags & MOVE_SUBDIRS ) || ( Move->fFlags & MOVE_TO_DIR ))) {

		// if moving subdirectories, create the target

		// remove trailing '/' or '\' in target
		strip_trailing( Move->szTarget+3, SLASHES );

		MakeDirectory( Move->szTarget, 1 );
		if ( is_dir( Move->szTarget ) == 0 )
			return ( error( ERROR_PATH_NOT_FOUND, Move->szTarget ));

		// move description to newly-created target
		if ( Move->fFlags & MOVE_SOURCEISDIR ) {
			copy_filename( Move->szSourceName, path_part( Move->szSource) );
			strip_trailing( Move->szSourceName+3, SLASHES );
			process_descriptions( Move->szSourceName, Move->szTarget, DESCRIPTION_COPY );
		}

		mkdirname( Move->szTarget, WILD_FILE );
	}

	// suppress error message from find_file
	if ( Move->fFlags & MOVE_NOERRORS ) {
		lMode |= FIND_NO_ERRORS;
		fCopyFlags |= NO_ERRORS;
	}

	// expand the wildcards
	for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT) {

		if ( find_file( nFind, Move->szSource, ( lMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE ), &dir, Move->szSourceName ) == NULL )
			break;

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			FindClose( dir.hdir );
			return CTRLC;
		}


		CheckForBreak();

		// prevent destructive moves like "mv *.* test.fil"
		if (( strpbrk( Move->szTarget, WILD_CHARS ) == NULL ) && ( nFind == FIND_NEXT)) {
			FindClose( dir.hdir );
			return ( error( ERROR_4DOS_CANT_CREATE, Move->szTarget ));
		}

		// don't move unmodified files?
		if (( Move->fFlags & MOVE_MODIFIED ) && (( dir.attrib & _A_ARCH ) == 0 ))
			continue;

		// make the target name
		wildname( Move->szTargetName, Move->szTarget, Move->szSourceName );

		// clear the EXISTS flag
		Move->fFlags &= ~MOVE_EXISTS;

		// if changed (/C), only move source if target is older
		// if update (/U), only move the source if the target is
		//   older or doesn't exist
		// if replace (/R) prompt before overwriting existing file

		if (( Move->fFlags & ( MOVE_KEEP | MOVE_UPDATED | MOVE_CHANGED | MOVE_REPLACE )) && ( find_file( FIND_FIRST, Move->szTargetName, 0x07L | FIND_EXACTLY | FIND_CLOSE | FIND_NO_ERRORS | FIND_NO_FILELISTS, &( Move->t_dir), NULL ) != NULL )) {

			// if /O, don't overwrite an existing file
			if ( Move->fFlags & MOVE_KEEP )
				continue;

			if ( Move->fFlags & ( MOVE_UPDATED | MOVE_CHANGED )) {

				if (( Move->t_dir.fd.wr_date > dir.fd.wr_date) || (( Move->t_dir.fd.wr_date == dir.fd.wr_date ) && ( Move->t_dir.ft.wr_time >= dir.ft.wr_time )))

					continue;
			}

			if ( Move->fFlags & MOVE_REPLACE )
				Move->fFlags |= MOVE_EXISTS;

		} else if ( Move->fFlags & MOVE_CHANGED )
			continue;	// target doesn't exist

		// check for same name
		if ( _stricmp( Move->szSourceName, Move->szTargetName ) == 0 ) {
			FindClose( dir.hdir );
			return ( error( ERROR_4DOS_DUP_COPY, Move->szSourceName ));
		}

		if ((( Move->fFlags & ( MOVE_QUIET | MOVE_TOTALS )) == 0 ) || ( Move->fFlags & MOVE_EXISTS )) {

			printf( FMT_STR_TO_STR, Move->szSourceName, Move->szTargetName );

			if ( Move->fFlags & ( MOVE_QUERY | MOVE_EXISTS )) {
				if ((( i = QueryInputChar((( Move->fFlags & MOVE_EXISTS ) ? REPLACE : NULLSTR ), YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
					Move->fFlags &= ~( MOVE_QUERY | MOVE_REPLACE );
				else if ( i == NO_CHAR )
					continue;
				else if ( i == ESCAPE ) {
					FindClose( dir.hdir );
					break;
				}
			} else
				crlf();
		}

		if (( Move->fFlags & MOVE_NOTHING ) == 0 ) {

			// if source is RDONLY, remove the bit (Netware refuses
			//   to rename a RDONLY file)
			if ( dir.attrib & _A_RDONLY ) {
				SetFileMode( Move->szSourceName, (dir.attrib & ~_A_RDONLY) );
				uRDONLY = 1;
			} else
				uRDONLY = 0;

			// if /Z, overwrite RDONLY files
			if ( Move->fFlags & MOVE_OVERWRITE_RDONLY )
				SetFileMode( Move->szTargetName, 0 );

			// try a simple rename first (except if the MOVE is
			//  to a different drive)
			nReturn = -1;
			if ( _ctoupper( *Move->szSourceName ) == _ctoupper( *Move->szTargetName )) {

			    if (( nReturn = rename( Move->szSourceName, Move->szTargetName )) != 0 ) {
					// rename didn't work; try deleting target
					remove( Move->szTargetName );
					nReturn = rename( Move->szSourceName, Move->szTargetName );
			    }
			}

			if ( nReturn != 0 ) {

				if (( Move->fFlags & MOVE_PERCENT ) && (( Move->fFlags & MOVE_QUIET ) == 0 ))
					fCopyFlags |= COPY_PROGRESS;

				if ( Move->fFlags & MOVE_RESTART )
					fCopyFlags |= COPY_RESTARTABLE;

				// rename didn't work; try a destructive copy
				if ((( nReturn = FileCopy( Move->szTargetName, Move->szSourceName, fCopyFlags, &lAppendOffset, &dir )) == CTRLC ) || ( setjmp( cv.env ) == -1 )) {

					// reset the directory search handle
					FindClose( dir.hdir );
					SetFileMode( Move->szSourceName, dir.attrib );
					return CTRLC;
				}

				// FileCopy turns off ^C handling on return
				EnableSignals();

				if ( nReturn != 0 ) {
					// reset original attributes on error
					SetFileMode( Move->szSourceName, dir.attrib );
					continue;
				}

				// file copied OK, so remove the original

				// rub out read-only, hidden, and system files
				if ( dir.attrib & ( _A_RDONLY | _A_HIDDEN | _A_SYSTEM ))
					SetFileMode( Move->szSourceName, 0 );
			
				// overwrite a file with 0's before erasing it

				if ( Move->fFlags & MOVE_WIPE ) {

					char *lpZero;

					unsigned long lSize;

					if (( lSize = QueryFileSize( Move->szSourceName, 0 )) > 0L ) {

						if (( i = _sopen( Move->szSourceName, (_O_WRONLY | _O_BINARY), _SH_DENYRW )) < 0 )
							goto Move_Status;			

					    lpZero = (char *)malloc( 512 );					
						memset( lpZero, 0, 512 );

					    while ( lSize != 0L ) {
							_write( i, lpZero, (( lSize > 512L ) ? 512 : (int)lSize ));
							if ( lSize > 512 )
								lSize -= 512;
							else
								lSize = 0L;
					    }

					    free( lpZero );
					    _close( i );
					}
				}

				_doserrno = 0;

				if (( Move->fFlags & MOVE_FORCEDEL ) && ( gnOSFlags & DOS_IS_OS2 )) {

					pszSourceArg = Move->szSourceName;
_asm {
					push   di
					push   si
					mov    ax, 06400h
					mov    bx, 0CBh		; ordinal
					mov    cx, 0636Ch
					mov    dx, pszSourceArg
					int    21h
					mov    _doserrno, ax
					pop    si
					pop    di
}
				} else

				{

					remove( Move->szSourceName );
				}

Move_Status:
				if (( _doserrno != 0 ) && (( Move->fFlags & MOVE_NOERRORS ) == 0 )) {
					if ( _doserrno == ERROR_ACCESS_DENIED )
						_doserrno = ERROR_4DOS_CANT_DELETE;
					error( _doserrno, Move->szSourceName );
				}
			}

			// if source is RDONLY, restore the bit in the target
			if ( uRDONLY )
				SetFileMode( Move->szTargetName, dir.attrib );

			// increment # of files moved counter
			Move->lFilesMoved++;

			// move description to target
			process_descriptions( Move->szSourceName, Move->szTargetName, DESCRIPTION_COPY );
		}

	}

	// build wildcard source for directory search
	sprintf( Move->szSource, FMT_DOUBLE_STR, path_part( Move->szSource ), WILD_FILE );

	// save the current target filename start
	pszTargetArg = Move->szTarget + strlen( path_part( Move->szTarget ));

	// update the source descript.ion file by deleting descriptions
	//   for any moved or missing files
	if ((( Move->fFlags & MOVE_NOTHING ) == 0 ) && (( Move->lFilesMoved != 0L ) || ( Move->fFlags & MOVE_SUBDIRS )))
		process_descriptions( NULL, Move->szSource, DESCRIPTION_REMOVE );

	// move subdirectories too?
	if ( Move->fFlags & MOVE_SUBDIRS ) {

		CheckFreeStack( 1024 );

		// save the current source filename start
		pszSourceArg = strchr( Move->szSource, _TEXT('*') );

		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, and to return
		//   only subdirectory names


		for ( nFind = FIND_FIRST; ( find_file( nFind, Move->szSource, ( lMode | 0x10 | FIND_ONLY_DIRS | FIND_NO_ERRORS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// strip the filename part from the target
			//   and make the new "source" and "target"

			sprintf( pszSourceArg, FMT_PATH_STR, dir.szFileName, Move->szFilename );

			copy_filename( pszTargetArg, dir.szFileName );

			// process directory tree recursively
			Move->fFlags |= MOVE_SOURCEISDIR;
			nReturn = _mv( Move );
			if (( setjmp( cv.env ) == -1 ) || ( cv.fException ) || ( nReturn == CTRLC)) {
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSourceArg, WILD_FILE );

			if ( nReturn != 0 ) {
				FindClose( dir.hdir );
				break;
			}
		}

		// try to delete the subdirectory (unless it's the root or
		//   the current dir for that drive)
		if (( Move->fFlags & MOVE_NOTHING ) == 0 ) {

			// strip name and trailing '\'
			pszSourceArg[-1] = _TEXT('\0');

			if ((( pszArg = gcdir( Move->szSource, 1 )) != NULL ) && ( strlen( Move->szSource ) > 3 ) && ( _stricmp( Move->szSource, pszArg ) != 0 )) {
				// remove the directory & its description
				SetFileMode( Move->szSource, _A_SUBDIR );
				DestroyDirectory( Move->szSource );
				process_descriptions( NULL, Move->szSource, DESCRIPTION_REMOVE );
			} else {
				// can't use DestroyDirectory if move was on same drive!
				UpdateFuzzyTree( Move->szSource, NULL, 1 );
			}
		}

	}

	// set "target" back to original value
	*pszTargetArg = _TEXT('\0');

	return nReturn;
}


#pragma alloc_text( MISC_TEXT, FileCopy )



// copy from the source to the target files
//   szOutputName = target file name
//   szInputName = source file name
//   fFlags = create/append, device & ASCII/binary flags
//   lAppendOffset = current end offset in append file
static int FileCopy( LPTSTR szOutputName, LPTSTR szInputName, unsigned long fFlags, long *lAppendOffset, FILESEARCH *dir )

{

	register int nReturn = 0;
	unsigned int fEoF, uBufferSize;
	int nInputFile = -1, nOutputFile = -1;
	BYTE _far *lpbSegment, _far *lpbPtr;
	unsigned int uAttribute = 0, nBytesRead, nBytesWritten;

	unsigned long ulTransferred;
	struct {
		char _far *pgealist;
		char _far *pfealist;
		unsigned long oError;
	} eaop;


	// check for same name
	if ( _stricmp( szInputName, szOutputName ) == 0 ) {
		if ((( fFlags & NO_ERRORS ) == 0 ) || ( *lAppendOffset != 0L ))
			nReturn = error( ERROR_4DOS_DUP_COPY, szInputName );
		goto FileCopyExit;
	}


	
	// if source has type but target doesn't, set target to source type
	// if target has type but source doesn't, set source to target type
	if (( nReturn = (unsigned int)(fFlags & ( ASCII_SOURCE | BINARY_SOURCE | ASCII_TARGET | BINARY_TARGET))) == ASCII_SOURCE )
		fFlags |= ASCII_TARGET;
	else if ( nReturn == BINARY_SOURCE )
		fFlags |= BINARY_TARGET;
	else if ( nReturn == ASCII_TARGET )
		fFlags |= ASCII_SOURCE;
	else if ( nReturn == BINARY_TARGET )
		fFlags |= BINARY_SOURCE;

	// devices & pipes default to ASCII copy
	if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) && (( fFlags & BINARY_SOURCE ) == 0 ))
		fFlags |= ASCII_SOURCE;


	// request memory block
	// NT bombs on console reads with big buffer!
	uBufferSize = (( fFlags & DEVICE_SOURCE ) ? 0x400 : 0xFFF0 );

	if (( lpbSegment = (LPBYTE)AllocMem( &uBufferSize )) == 0L ) {
		nReturn = error( ERROR_NOT_ENOUGH_MEMORY, NULL );
		goto FileCopyExit;
	}

	if ( setjmp( cv.env ) == -1 ) {		// ^C trapping
		nReturn = CTRLC;
		goto filebye;
	}

	// open input file for read


	if (( nInputFile = _sopen( szInputName, (_O_RDONLY | _O_BINARY), _SH_DENYNO )) < 0 ) {
		nReturn = error( _doserrno, szInputName );
		goto filebye;
	}

	// lock the file (and check for somebody else already locking it!)
	if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 ) {
		if ( _locking( nInputFile, _LK_NBLCK, 0x7FFFFFFFL ) != 0 ) {
			if ( _doserrno != ERROR_INVALID_FUNCTION ) {
				nReturn = error( _doserrno, szInputName );
				goto filebye;
			}
		}
	}

	nReturn = 0;

	// if we are appending, open an existing file, otherwise create or
	//   truncate target file
	if (( fFlags & COPY_CREATE ) && (( fFlags & ( DEVICE_TARGET | PIPE_TARGET)) == 0 )) {

		// if source not a device, copy source attributes to target
		if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 ) {

			QueryFileMode( szInputName, &uAttribute );

			// strip wacky Netware bits & RDONLY bit
			//   (unless moving files)
			uAttribute &= (( fFlags & MOVING_FILES ) ? 0x27 : 0x26 );
		}

		eaop.pgealist = 0L;
		eaop.pfealist = lpbSegment;
		eaop.oError = 0L;

		if (( _osmajor < 10 ) || ( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) || ( gpIniptr->CopyEA == 0 )) {
no_ea_copy:
			nReturn = 0;

			// if not OS/2 or no EAs, use the old approach
			if (( nOutputFile = _sopen( szOutputName, (_O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC), _SH_DENYRW, ( _S_IREAD | _S_IWRITE ) )) < 0 ) {
				nReturn = error( _doserrno, szOutputName );
				goto filebye;
			}

		} else {

			// copy EAs if OS/2 compatibility box
			unsigned long _far *flptr;

			// kill target (& its EA's) before copy
			// if "access denied", don't try the create (there's
			//   a bug in OS/2 2.0 GA that will crash the DOS box)
			if (( remove( szOutputName ) == -1 ) && ( _doserrno == ERROR_ACCESS_DENIED )) {
				nReturn = error( _doserrno, szOutputName );
				goto filebye;
			}

			// set the buffer size
			flptr = (unsigned long _far *)lpbSegment;
			*flptr = (long)(uBufferSize - sizeof(unsigned long));

			if (( nReturn = _dos_createEA( szInputName, nInputFile, szOutputName, &nOutputFile, uAttribute, (unsigned int)&eaop )) != 0 )
				goto no_ea_copy;
		}

	// we're appending, or target is a device or pipe!
	} else if (( nOutputFile = _sopen( szOutputName, (_O_RDWR | _O_BINARY), _SH_DENYWR )) < 0 )
		nReturn = _doserrno;

	if ( nReturn ) {

		// DOS returns an odd error if it couldn't create the file!
		if (( fFlags & COPY_CREATE ) && ( nReturn == 2 ))
			nReturn = ERROR_4DOS_CANT_CREATE;

		nReturn = error( nReturn, szOutputName );
		goto filebye;
	}


	if ( fFlags & ( DEVICE_TARGET | PIPE_TARGET )) {

		// can't append to device or FTP file
		fFlags |= COPY_CREATE;

		// char devices & pipes default to ASCII copy
		if (( fFlags & ( BINARY_SOURCE | BINARY_TARGET )) == 0 )
			fFlags |= ( ASCII_TARGET | ASCII_SOURCE );
		else {
			// set raw mode for binary copy
			SetIOMode( nOutputFile, 0 );
		}

	}

	if (( fFlags & COPY_CREATE ) == 0 ) {

		// append at EOF of the target file (we have to kludge a bit
		//   here, because some files end in ^Z and some don't, & we
		//   may be in binary mode)

		// if continuing an append, move to saved EOF in target
		if ( *lAppendOffset != 0L )
			_lseek( nOutputFile, *lAppendOffset, SEEK_SET );
		else if ( fFlags & BINARY_SOURCE )
			*lAppendOffset = QuerySeekSize( nOutputFile );
		else {

		    // We should only get here if the source is ASCII, and 
		    //   the target is the first source.  Read the file until
		    //   EOF or we find a ^Z.
		    do {

			if (( nReturn = FileRead( nOutputFile, lpbSegment, uBufferSize, &nBytesRead )) != 0 ) {
				nReturn = error( nReturn, szOutputName );
				goto filebye;
			}

			// search for a ^Z
			if (( lpbPtr = (LPBYTE)_fmemchr( lpbSegment, 0x1A, nBytesRead )) != 0L ) {

				// backup to ^Z location
				*lAppendOffset += ((ULONG_PTR)lpbPtr - (ULONG_PTR)lpbSegment);
				_lseek( nOutputFile, *lAppendOffset, SEEK_SET );
				break;
			}

			*lAppendOffset += nBytesRead;

		    } while ( nBytesRead == uBufferSize );
		}
	}

	for ( ulTransferred = 0, fEoF = 0; (( fEoF == 0 ) && ( cv.fException == 0 )); ) {

		if (( fFlags & COPY_PROGRESS ) && ( QueryIsConsole( STDOUT )) && (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 )) {
			// display percent copied

			// have to use EVAL to do it in DOS
			// get string equivalents (for > 4Gb drives)
			char szBuffer[64];

			if ( dir->ulSize == 0L )
				strcpy( szBuffer, "100" );
			else {
				sprintf( szBuffer, "(%lu*100)/%lu=0.0", ulTransferred, dir->ulSize );
				evaluate( szBuffer );
			}
			printf( "%s%%\r", szBuffer );

		}

		if (( nReturn = FileRead( nInputFile, lpbSegment, uBufferSize, &nBytesRead )) != 0 ) {
			nReturn = error( nReturn, szInputName );
			goto filebye;
		}

		if ( fFlags & ASCII_SOURCE ) {

			// terminate the file copy at a ^Z
			if (( lpbPtr = (LPBYTE)_fmemchr( lpbSegment, 0x1A, nBytesRead )) != 0L ) {
				nBytesRead = (unsigned int)((ULONG_PTR)lpbPtr - (ULONG_PTR)lpbSegment);
				fEoF++;
			}
		}

		if ( nBytesRead == 0 )
			fEoF++;

		*lAppendOffset += nBytesRead;

		nBytesWritten = 0;


		if (( nReturn = FileWrite( nOutputFile, lpbSegment, nBytesRead, &nBytesWritten )) != 0 ) {
			nReturn = error( nReturn, szOutputName );
			goto filebye;
		}

		// if target is ASCII, add a ^Z at EOF and set "fEoF" to
		//   force a break after the write
		if (( fFlags & ASCII_TARGET ) && (( fEoF ) || (( nBytesRead < uBufferSize ) && (( fFlags & DEVICE_SOURCE ) == 0 )))) {
			
			if (( fFlags & DEVICE_TARGET ) == 0 ) {

				TCHAR chCtrlZ[2];

				chCtrlZ[0] = 26;
				wwrite( nOutputFile, chCtrlZ, 1 );
			}
			fEoF++;
		}

		if ( nBytesRead == 0 )
			break;

		if ( nBytesRead != nBytesWritten ) {	// error in writing

			// if writing to a device, don't expect full blocks
			if (( fFlags & DEVICE_TARGET ) == 0 ) {

				error( ERROR_4DOS_DISK_FULL, szOutputName );

				fDiskFull = 1;
				nReturn = CTRLC;
			}

			goto filebye;
		}

		ulTransferred += nBytesWritten;

		CheckForBreak();

	}

	// set the file date & time to the same as the source
	if (( fFlags & COPY_CREATE ) && (( fFlags & ( APPEND_FLAG | DEVICE_SOURCE | DEVICE_TARGET | PIPE_SOURCE | PIPE_TARGET )) == 0 )) {

		_dos_getftime( nInputFile, &nBytesRead, &nBytesWritten );
		_dos_setftime( nOutputFile, nBytesRead, nBytesWritten );

	}

filebye:
	FreeMem( lpbSegment );

	if ( nInputFile > 0 ) {

		if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 )
			_locking( nInputFile, _LK_UNLCK, 0x7FFFFFFFL );
		_close( nInputFile );
	}

	if ( nOutputFile > 0 ) {

		_close( nOutputFile );

		// if an error writing to the output file, delete it
		//   (unless we're appending
		if (( nReturn ) && ( fFlags & COPY_CREATE )) {
			SetFileMode( szOutputName, _A_NORMAL );
			remove( szOutputName );
		} else {
			// set the target file attributes
			SetFileMode( szOutputName, ( uAttribute | _A_ARCH ) );
		}
	}


FileCopyExit:
	// disable signal handling momentarily
	HoldSignals();
	
	return nReturn;
}



#pragma alloc_text( MISC_TEXT, wildname )


// expand a wildcard name
static void wildname( LPTSTR pszTarget, register LPTSTR pszSource, LPTSTR pszTemplate )
{
	register LPTSTR pszArg;
	TCHAR szString[MAXFILENAME+1];
	int nLength;

	char szShortName[MAXFILENAME+1];
	int nDestType = ifs_type( pszSource );

	
	// build a legal filename (right filename & extension size)
	insert_path( pszSource, fname_part( pszSource ), pszSource );

	// if source has path, copy it to target
	if (( pszArg = path_part( pszSource )) != NULL ) {
		strcpy( pszTarget, pszArg );
		pszSource += strlen( pszArg );
		pszTarget += strlen( pszArg );
	}

	// kludge for Win95 LFNs when copying to non-LFN-aware drive
	if ( nDestType == 0 ) {
		pszTemplate = strcpy( szShortName, pszTemplate );
		GetShortName( pszTemplate );
	}

	pszTemplate = fname_part( pszTemplate );

	while ( *pszSource ) {

		if ( *pszSource == _TEXT('*') ) {

			while (( *pszSource == _TEXT('*') ) || ( *pszSource == _TEXT('?') ))
				pszSource++;

			// we need to check for things like *abc
			sscanf( pszSource, _TEXT("%[^.*?]"), szString );
			nLength = strlen( szString );

			// insert substitute chars

			if ( fWin95LFN ) {
				// skip to extension
				while (( *pszTemplate ) && (( *pszTemplate != _TEXT('.') ) || ( *pszSource != _TEXT('.') )) && (( nLength == 0 ) || ( strnicmp( pszTemplate, szString, nLength ) != 0 )))
					*pszTarget++ = *pszTemplate++;
			} else {
				while (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
					*pszTarget++ = *pszTemplate++;
			}

		} else if ( *pszSource == _TEXT('?') ) {

			// single char substitution
			if (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
				*pszTarget++ = *pszTemplate++;
			pszSource++;

		} else if ( *pszSource == _TEXT('.') ) {

			while (( *pszTemplate ) && ( *pszTemplate++ != _TEXT('.') ))
				;
			*pszTarget++ = *pszSource++;		// copy '.'

		} else {

			*pszTarget++ = *pszSource++;
			if (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
				pszTemplate++;
		}
	}

	// strip trailing '.'
	if (( pszTarget[-1] == _TEXT('.') ) && ( pszTarget[-2] != _TEXT('.') ))
		pszTarget--;
	*pszTarget = _TEXT('\0');
}


#define REN_BY_ATTRIBUTES 1
#define REN_NOERRORS 2
#define REN_NOTHING 4
#define REN_QUERY 8
#define REN_QUIET 0x10
#define REN_SUBDIRS 0x20
#define REN_TOTALS 0x40

// rename files (including to different directories) or directories
int _near Ren_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	TCHAR szSource[MAXFILENAME], szTargetName[MAXFILENAME], szTarget[MAXFILENAME];
	int i, nArgc, nFind, nReturn = 0;
	unsigned long ulMode = 0;
	long fRen, lFilesRenamed = 0;
	FILESEARCH dir;

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove switches
	// get the target name, or abort if no target
	if (( GetSwitches( pszCmdLine, _TEXT("*ENPQST"), &fRen, 0 ) != 0 ) || (( pszArg = last_arg( pszCmdLine, &nArgc )) == NULL ))
		return ( Usage( RENAME_USAGE ));

	if ( fRen & REN_BY_ATTRIBUTES )
		ulMode |= 0x07;

	// if query, turn off quiet mode
	if ( fRen & REN_QUERY )
		fRen &= ~( REN_QUIET | REN_TOTALS );

	copy_filename( szTarget, pszArg );
	StripQuotes( szTarget );
	*gpNthptr = _TEXT('\0');		// remove target from command line

	// rename matching directories?
	if (( fRen & REN_SUBDIRS ) || ( strpbrk( szTarget, WILD_CHARS ) == NULL ))
		ulMode |= _A_SUBDIR;

	// suppress error message from find_file
	if ( fRen & REN_NOERRORS )
		ulMode |= FIND_NO_ERRORS;

	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc)) != NULL ); nArgc++ ) {

		if ( mkfname( pszArg, (( fRen & REN_NOERRORS ) ? MKFNAME_NOERROR : 0 )) == NULL )
			continue;

		// strip trailing backslashes
		if ( ulMode & _A_SUBDIR )
			strip_trailing( pszArg+3, SLASHES );

		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			if ( find_file( nFind, pszArg, ( ulMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szSource ) == NULL )
				break;

			if ( setjmp( cv.env ) == -1 ) {
				FindClose( dir.hdir );
				nReturn = CTRLC;
				goto rename_abort;
			}

			CheckForBreak();

			wildname( szTargetName, szTarget, szSource );

			if ( mkfname( szSource, 0 ) == NULL )
				return ERROR_EXIT;

			// nasty kludge to emulate COMMAND.COM & CMD.EXE stupid
			//   renaming convention (if source has path & target
			//   doesn't, rename into source directory)
			if ( path_part( szTargetName ) == NULL )
				strins( szTargetName, path_part( szSource ));

			if ( mkfname( szTargetName, 0 ) == NULL )
				return ERROR_EXIT;

			if ( is_dir( szTargetName )) {
				// destination is directory; create target name
				//   (unless we're renaming directories!)
				if ( !is_dir( szSource ))
					mkdirname( szTargetName, fname_part( szSource ));
			}

			if (( fRen & ( REN_QUIET | REN_TOTALS )) == 0 ) {

				printf( FMT_STR_TO_STR, szSource, szTargetName );

				if ( fRen & REN_QUERY ) {
					if ((( i = QueryInputChar( NULLSTR, YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
						fRen &= ~REN_QUERY;
					else if ( i == NO_CHAR )
						continue;
					else if ( i == ESCAPE ) {
						FindClose( dir.hdir );
						break;
					}
				} else
					crlf();
			}

			if (( fRen & REN_NOTHING ) == 0 ) {

				if ( rename( szSource, szTargetName ) == -1 ) {
					// check if error is with source or target
					nReturn = error( _doserrno, ((( _doserrno == ERROR_ACCESS_DENIED) || ( _doserrno == ERROR_NOT_SAME_DEVICE ) || ( _doserrno == ERROR_PATH_NOT_FOUND)) ? szTargetName : szSource ));

				} else {

					lFilesRenamed++;

					// rename description to target dir
					process_descriptions( szSource, szTargetName, DESCRIPTION_COPY );

					// update JPSTREE.IDX
					if ( is_dir( szTargetName )) {

						TCHAR szTemp[MAXFILENAME], szBuf[MAXLINESIZ];
						int n, fSave;
						FILESEARCH tdir;

						// remove source tree
						UpdateFuzzyTree( szSource, NULL, 1 );
						// add new target directory
						UpdateFuzzyTree( szTargetName, NULL, 0 );

						// now rename any subdirectories
						strcpy( szTemp, szTargetName );
						mkdirname( szTemp, WILD_FILE );

						if ( find_file( 0x4E, szTemp, 0x17 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_DOTNAMES | FIND_CLOSE | FIND_NO_FILELISTS, &tdir, NULL ) != NULL ) {
	
							// save command line (it'll be destroyed by the command() call)
							pszCmdLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );

							// build a temporary file
							szTemp[0] = _TEXT('\0');

							GetTempDirectory( szTemp );

							UniqueFileName( szTemp );

							sprintf( szBuf, _TEXT("*@tree /c \"%s\" >! \"%s\""), szTargetName, szTemp );

							// use TREE to build the directory list
							// make sure TREE isn't disabled!
							n = findcmd( _TEXT("TREE"), 1 );

							fSave = commands[ n ].fParse;
							commands[ n ].fParse &= ~CMD_DISABLED;
							i = command( szBuf, 0 );
							commands[ n ].fParse = fSave;

							// check for ^C or stack overflow
							if (( setjmp( cv.env ) == -1 ) || ( i == CTRLC ) || ( cv.fException )) {
								FindClose( dir.hdir );
								nReturn = CTRLC;
								goto rename_abort;
							}

							// insert temp file into JPSTREE.IDX
							UpdateFuzzyTree( szTargetName, szTemp, 0 );
							remove( szTemp );
						}
					}
				}
			}
		}

		// delete the descriptions for renamed or missing files
		if ((( fRen & REN_NOTHING ) == 0 ) && ( lFilesRenamed != 0L )) {
			strcat( pszArg, WILD_FILE );
			process_descriptions( NULL, pszArg, DESCRIPTION_REMOVE );
		}
	}

rename_abort:
	if (( fRen & REN_QUIET ) == 0 )
		FilesProcessed( FILES_RENAMED, lFilesRenamed );

	return (( lFilesRenamed != 0L ) ? nReturn : ERROR_EXIT );
}


#define DEL_BY_ATTRIBUTES 1
#define DEL_NOERRORS 2
#define DEL_FORCE 4
#define DEL_NOTHING 8
#define DEL_QUERY 0x10
#define DEL_QUIET 0x20
#define DEL_SUBDIRS 0x40
#define DEL_TOTALS 0x80
#define DEL_RMDIR 0x100
#define DEL_ALL 0x200
#define DEL_ZAP_ALL 0x400
#define DEL_DIRS_ONLY 0x800
#define DEL_WIPE 0x1000
#define DEL_KILL 0x2000
#define DEL_RECYCLE 0x4000


typedef struct
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	LPTSTR szFilename;
	long fFlags;
	long lFilesDeleted;
} DEL_STRUCT;

static int _del( DEL_STRUCT * );


// Delete file(s)
int _near Del_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	int nArgc, nReturn = 0, i, nLength;
	QDISKINFO DiskInfo;
	DEL_STRUCT Del;

	long lBytesFreed = 0L;


	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove switches
	if (( GetSwitches( pszCmdLine, _TEXT("*EFNPQSTXYZDWKR"), &(Del.fFlags), 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( DELETE_USAGE ));

	// if query, turn off quiet flag
	if ( Del.fFlags & DEL_QUERY )
		Del.fFlags &= ~( DEL_QUIET | DEL_TOTALS );

	Del.lFilesDeleted = 0L;
	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); nArgc++ ) {

		// build the source name
		if ( mkfname( pszArg, 0 ) == NULL )
			return ERROR_EXIT;

		// if it's a directory, append the "*.*" wildcard
		if (( AddWildcardIfDirectory( pszArg ) == 0 ) && ( Del.fFlags & DEL_DIRS_ONLY )) {
			if (( Del.fFlags & DEL_NOERRORS ) == 0 )
				nReturn = error( ERROR_4DOS_NOT_A_DIRECTORY, pszArg );
			continue;
		}

		copy_filename( Del.szSource, pszArg );

		nLength = strlen( path_part( Del.szSource ));

		// save the source filename part ( for recursive calls &
		//   include lists)
		Del.szFilename = Del.szSource + nLength;


		if ((( Del.fFlags & DEL_QUERY ) == 0 ) && (( Del.fFlags & DEL_ALL ) == 0 )) {

			// confirm before deleting an entire directory
			//   ( filename & extension == wildcards only)
			sscanf( Del.szFilename, _TEXT("%*[?*.;]%n"), &i );
			if (( i > 0 ) && ( Del.szFilename[i] == _TEXT('\0') )) {

				printf( "%s : ", Del.szSource );

				if ( QueryInputChar( ARE_YOU_SURE, YES_NO ) != YES_CHAR )
					continue;
			}
		}

		pszArg = (LPTSTR)_alloca( ( strlen( Del.szFilename ) + 1 ) * sizeof(TCHAR) );
		Del.szFilename = strcpy( pszArg, Del.szFilename );

		if ((( Del.fFlags & DEL_QUIET ) == 0 ) && ( is_net_drive( Del.szSource ) == 0 ) && ( QueryDriveRemote( gcdisk( Del.szSource )) == 0 )) {

			// get TRUENAME (to see through JOIN & SUBST)
			pszArg = true_name( Del.szSource, first_arg( Del.szSource ));
			QueryDiskInfo( pszArg, &DiskInfo, 1 );

			lBytesFreed -= DiskInfo.BytesFree;
		}

		i = _del( &Del );

		if (( setjmp( cv.env ) == -1 ) || ( i == CTRLC ))
			nReturn = CTRLC;


		if ((( Del.fFlags & DEL_QUIET ) == 0 ) && ( is_net_drive( Del.szSource ) == 0 ) && ( QueryDriveRemote( gcdisk( Del.szSource)) == 0 )) {

			pszArg = true_name( Del.szSource, first_arg( Del.szSource ));
			QueryDiskInfo( pszArg, &DiskInfo, 1 );

			lBytesFreed += DiskInfo.BytesFree;
		}

		if ( nReturn == CTRLC )
			break;

		if ( i != 0 )
			nReturn = ERROR_EXIT;
	}

	if (( Del.fFlags & DEL_QUIET ) == 0 ) {
		// display number of files deleted, & bytes freed
		FilesProcessed( FILES_DELETED, Del.lFilesDeleted );
		if ( lBytesFreed > 0L )
			printf( FILES_BYTES_FREED, lBytesFreed );
		crlf();
	}


	return (( Del.lFilesDeleted != 0L ) ? nReturn : ERROR_EXIT );
}



#pragma alloc_text( MISC_TEXT, _del )


// recursive file deletion routine
static int _del( register DEL_STRUCT *Del )
{
	register int i;
	unsigned long ulMode = 0;
	int nFind, nReturn = 0, rc = 0;
	LPTSTR pszArg, pszSource;
	FILESEARCH dir;

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	// delete hidden files too?
	if (( Del->fFlags & DEL_BY_ATTRIBUTES ) || ( Del->fFlags & DEL_ZAP_ALL ))
		ulMode |= 0x07;

	dir.hdir = INVALID_HANDLE_VALUE;

	// trap ^C in DEL loop, and close directory search handles
	if ( setjmp( cv.env ) == -1 ) {
_del_abort:
		FindClose( dir.hdir );
		return CTRLC;
	}

	// suppress error message from find_file
	if ( Del->fFlags & DEL_NOERRORS )
		ulMode |= FIND_NO_ERRORS;

	// do a new-style delete
	for ( nFind = FIND_FIRST; ( find_file( nFind, Del->szSource, ( ulMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE ), &dir, Del->szTarget) != NULL ); nFind = FIND_NEXT) {

		if ( Del->fFlags & DEL_QUERY ) {

			qputs( DELETE_QUERY );
			if ((( i = QueryInputChar( Del->szTarget, YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
				Del->fFlags &= ~DEL_QUERY;
			else if ( i == NO_CHAR )
				continue;
			else if ( i == ESCAPE ) {
				FindClose( dir.hdir );
				break;
			}

		} else if (( Del->fFlags & ( DEL_QUIET | DEL_TOTALS )) == 0 ) {

			printf( DELETING_FILE, Del->szTarget );

		}

		if (( Del->fFlags & DEL_NOTHING ) == 0 ) {

			// overwrite a file with 0's before erasing it
			if ( Del->fFlags & DEL_WIPE ) {

				char *lpZero;

				unsigned long lSize;

				if (( lSize = QueryFileSize( Del->szTarget, 0 )) > 0L ) {

					// if /Z(ap) flag, clear rdonly attributes before wiping
					if (( Del->fFlags & DEL_ZAP_ALL ) || ( gnInclusiveMode & _A_RDONLY ))
						SetFileMode( Del->szTarget, _A_NORMAL );

					if (( i = _sopen( Del->szTarget, (_O_WRONLY | _O_BINARY), _SH_DENYRW )) < 0) {
						rc = _doserrno;
						goto Del_Next;
					}

				    lpZero = (char *)malloc( 512 );					
				    memset( lpZero, 0, 512 );

				    while ( lSize != 0L ) {
						_write( i, lpZero, (( lSize > 512L ) ? 512 : (int)lSize ));
						if ( lSize > 512 )
							lSize -= 512;
						else
							lSize = 0L;
				    }

				    free( lpZero );
				    _close( i );
				}
			}

			// support for DEL /F(orce) in OS/2 2

			if (( Del->fFlags & ( DEL_FORCE | DEL_WIPE )) && ( gnOsVersion >= 2010 )) {
				rc = _doserrno = ForceDelete( Del->szTarget);
			} else
				rc = remove( Del->szTarget );


			CheckForBreak();

			if ( rc != 0 ) {

				// if /Z(ap) flag, clear hidden/rdonly
				//   attributes & try to delete again
				if ((( Del->fFlags & DEL_ZAP_ALL ) || ( gnInclusiveMode & _A_RDONLY )) && ( SetFileMode( Del->szTarget,_A_NORMAL ) == 0 )) {

					rc = remove( Del->szTarget );

				}
			}
Del_Next:
			if ( rc ) {
				if (( Del->fFlags & DEL_NOERRORS ) == 0 )
					nReturn = error( _doserrno, Del->szTarget );
				else
					nReturn = ERROR_EXIT;
			} else
				Del->lFilesDeleted++;
		}
	}

	// delete the description(s) for deleted or missing files
	sprintf( Del->szSource, FMT_DOUBLE_STR, path_part( Del->szSource ), WILD_FILE );

	// update description file
	if ((( Del->fFlags & DEL_NOTHING) == 0 ) && (( Del->lFilesDeleted != 0L ) || ( Del->fFlags & DEL_RMDIR )))
		process_descriptions( NULL, Del->szSource, DESCRIPTION_REMOVE );

	// delete matching subdirectory files too?
	if ( Del->fFlags & DEL_SUBDIRS ) {

		CheckFreeStack( 1024 );

		// save the current subdirectory start
		pszSource = strchr( Del->szSource, _TEXT('*') );
 
		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors and to only
		//   return subdir names

		for ( nFind = FIND_FIRST; ( find_file( nFind, Del->szSource, ( ulMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// make the new "source"

			sprintf( pszSource, FMT_PATH_STR, dir.szFileName, Del->szFilename );

			// process directory tree recursively
			nReturn = _del( Del );

			if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC))
				goto _del_abort;

			// restore the original name
			strcpy( pszSource, WILD_FILE );
		}
	}

	// delete the subdirectory?
	if (( Del->fFlags & DEL_RMDIR ) && (( Del->fFlags & DEL_NOTHING ) == 0 )) {

		// remove the last filename from the directory name
		pszSource = path_part( Del->szSource );
		strip_trailing( pszSource, SLASHES );

		// don't try to delete the root or current directories
		if ((( pszArg = gcdir( pszSource, 1 )) == NULL ) || (( strlen( pszSource) > 3 ) && ( _stricmp( pszSource, pszArg ) != 0 ))) {

			int nRC;

			if (( Del->fFlags & DEL_ZAP_ALL ) || ( gnInclusiveMode & _A_RDONLY ))
				SetFileMode( pszSource, _A_SUBDIR );
			if (( nRC = DestroyDirectory( pszSource )) != 0 ) {
				if (( Del->fFlags & DEL_NOERRORS ) == 0 )
					error( nRC, pszSource );
			} else	// remove the directory & its description
				process_descriptions( NULL, pszSource, DESCRIPTION_REMOVE );
		}
	}

	return nReturn;
}


#define ATTRIB_DIRS 2
#define ATTRIB_NOERRORS 4
#define ATTRIB_PAUSE 8
#define ATTRIB_QUIET 0x10
#define ATTRIB_SUBDIRS 0x20

typedef struct
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	LPTSTR szFilename;
	long fFlags;
	long lMode;

	char cAnd;
	char cOr;

} ATTRIB_STRUCT;

static int _attrib( ATTRIB_STRUCT * );


// change file attributes (HIDDEN, SYSTEM, READ-ONLY, and ARCHIVE)
int _near Attrib_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	LPTSTR pszTrailing;
	int nArgc, n, nReturn = 0;
	union {
		// file attribute structure

		unsigned char attribute;

		struct {
			unsigned rdonly : 1;
			unsigned hidden : 1;
			unsigned system : 1;
			unsigned reserved : 2;
			unsigned archive : 1;

		} bit;
	} and_mask, or_mask;
	ATTRIB_STRUCT Attrib;

	Attrib.lMode = 0;

	init_page_size();		// clear row & page length vars

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	// check for flags
	if ( GetSwitches( pszCmdLine, _TEXT("*DEPQS"), &(Attrib.fFlags), 0 ) != 0 )
		return (Usage( ATTRIB_USAGE ));

	if ( Attrib.fFlags & ATTRIB_PAUSE )
		gnPageLength = GetScrRows();

	Attrib.lMode |= (( Attrib.fFlags & ATTRIB_DIRS ) ? 0x17 : 0x07 );
	if ( gnInclusiveMode & _A_SUBDIR )
		Attrib.lMode |= _A_SUBDIR;

	// suppress error message from find_file
	if ( Attrib.fFlags & ATTRIB_NOERRORS )
		Attrib.lMode |= FIND_NO_ERRORS;

	and_mask.attribute = 0x37;

	or_mask.attribute = 0;

	nArgc = 0;
	strcpy( Attrib.szSource, WILD_FILE );	// default to *.*

	do {

	    if (( pszArg = ntharg( pszCmdLine, nArgc++ )) != NULL ) {

		if ( *pszArg == _TEXT('-') ) {

			// remove attribute (clear OR and AND bits)
			while ( *(++pszArg )) {

				switch ( _ctoupper( *pszArg )) {
				case 'A':
					or_mask.bit.archive = and_mask.bit.archive = 0;
					break;
				case 'H':
					or_mask.bit.hidden = and_mask.bit.hidden = 0;
					break;
				case 'R':
					or_mask.bit.rdonly = and_mask.bit.rdonly = 0;
					break;
				case 'S':
					or_mask.bit.system = and_mask.bit.system = 0;
					break;
				case '-':
					// kludge for RTPatch bug
					break;
				case '_':
					// ignore for ___A_ syntax
					break;
				case 'D':
					// ____D syntax
					Attrib.fFlags |= ATTRIB_DIRS;
					break;

				default:
					error( ERROR_INVALID_PARAMETER, pszArg );
					return (Usage( ATTRIB_USAGE ));
				}
			}

		} else if ( *pszArg == _TEXT('+') ) {

			// add attribute (set OR bits)
			while ( *(++pszArg )) {

				switch ( _ctoupper( *pszArg )) {
				case 'A':
					or_mask.bit.archive = 1;
					break;
				case 'H':
					or_mask.bit.hidden = 1;
					break;
				case 'R':
					or_mask.bit.rdonly = 1;
					break;
				case 'S':
					or_mask.bit.system = 1;
					break;
				case '+':
					// kludge for RTPatch bug
					break;
				case '_':
					// ignore for ___A_ syntax
					break;
				case 'D':
					// ____D syntax
					Attrib.fFlags |= ATTRIB_DIRS;
					break;

				default:
					error( ERROR_INVALID_PARAMETER, pszArg );
					return ( Usage( ATTRIB_USAGE ));
				}
			}

		} else {

		    // it must be a filename
		    copy_filename( Attrib.szSource, pszArg );

		    // check for trailing switches
		    // KLUDGE for COMMAND.COM compatibility (ATTRIB *.c +h)
		    for ( n = nArgc; (( pszTrailing = ntharg( pszCmdLine, n )) != NULL ); n++ ) {
			// check for another file spec
			if (( *pszTrailing != _TEXT('-') ) && ( *pszTrailing != _TEXT('+') ))
				goto change_atts;
		    }
		}

	    } else {

change_atts:
		if ( mkfname( Attrib.szSource, 0 ) == NULL )
			return ERROR_EXIT;

		// if source is a directory & no /D specified, add "\*.*"
		if ( Attrib.fFlags & ATTRIB_DIRS )
			strip_trailing( Attrib.szSource, _TEXT("\\/") );
		else
			AddWildcardIfDirectory( Attrib.szSource );

		// save the source filename part (for recursive calls and
		//   include lists)
		Attrib.szFilename = Attrib.szSource + strlen( path_part( Attrib.szSource));
		pszTrailing = (LPTSTR)_alloca( ( strlen( Attrib.szFilename ) + 1 ) * sizeof(TCHAR) );
		Attrib.szFilename = strcpy( pszTrailing, Attrib.szFilename );

		// change attributes
		Attrib.cAnd = and_mask.attribute;
		Attrib.cOr = or_mask.attribute;

		nReturn = _attrib( &Attrib );
		if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ))
			break;

	    }

	} while (( nReturn != CTRLC ) && ( pszArg != NULL ));

	return nReturn;
}



#pragma alloc_text( MISC_TEXT, _attrib, show_atts )


static int _attrib( register ATTRIB_STRUCT *Attrib )
{
	register unsigned int uOldAttribute;
	int nFind, nReturn = 0, rc, nAttribute;
	LPTSTR pszSource;
	FILESEARCH dir;

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	dir.hdir = INVALID_HANDLE_VALUE;

	// change attributes
	for ( nFind = FIND_FIRST; ( find_file( nFind, Attrib->szSource, ( Attrib->lMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, Attrib->szTarget ) != NULL ); nFind = FIND_NEXT ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			FindClose( dir.hdir );
			return CTRLC;
		}


		CheckForBreak();

		// kludge for Win32 GetFileAttributes API bug -- won't work with open files!!
		//  (If it isn't a filelist, we already have the attributes in "dir.attrib")

		// query file mode again because we might be using @filelists (lFileOffset >= 0)
		if ( dir.lFileOffset != -1L ) {
			if (( nReturn = QueryFileMode( Attrib->szTarget, (unsigned int *)&nAttribute )) == 0 )
				dir.attrib = nAttribute;
			else {
				if (( Attrib->fFlags & ATTRIB_NOERRORS ) == 0 )
					error( nReturn, Attrib->szTarget );
				nReturn = ERROR_EXIT;
				continue;
			}
		}

		// display old attributes, new attributes, and filename

		uOldAttribute = ( dir.attrib & 0x37 );

		dir.attrib = (( uOldAttribute & Attrib->cAnd ) | Attrib->cOr );

		// if attributes were changed, show new ones
		if (( Attrib->fFlags & ATTRIB_QUIET ) == 0 ) {

			qputs( show_atts( uOldAttribute ));

			if (( Attrib->cAnd != 0x37 ) || ( Attrib->cOr != 0 )) {

				printf( _TEXT(" -> %s"), show_atts( dir.attrib ));
			}

			printf( _TEXT("  %s"), Attrib->szTarget );
			crlf();
			_page_break();
		}

		// check for a change; ignore if same
		if ( uOldAttribute != dir.attrib ) {

			// can't change directory attribute
			dir.attrib &= ~_A_SUBDIR;
			if (( rc = SetFileMode( Attrib->szTarget, dir.attrib )) != 0 ) {
				if (( Attrib->fFlags & ATTRIB_NOERRORS ) == 0 )
					error( rc, Attrib->szTarget );
				nReturn = ERROR_EXIT;
			}
		}
	}

	if ( Attrib->fFlags & ATTRIB_SUBDIRS ) {
	// modify matching subdirectory files too?

		CheckFreeStack( 1024 );

		// strip the filename & add wildcards for directory search
		sprintf( Attrib->szSource, FMT_DOUBLE_STR, path_part( Attrib->szSource ), WILD_FILE );

		// save the current subdirectory start
		pszSource = strchr( Attrib->szSource, _TEXT('*') );
 
		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, & to only
		//   return subdir names
		for ( nFind = FIND_FIRST; ( find_file( nFind, Attrib->szSource, ( Attrib->lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT) {

			// make the new "source"
			sprintf( pszSource, FMT_PATH_STR, dir.szFileName, Attrib->szFilename );

			// process directory tree recursively
			nReturn = _attrib( Attrib );
			if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
				// reset the directory search handle
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSource, WILD_FILE );
		}
	}

	return nReturn;
}


LPTSTR _fastcall show_atts( register int nAttribute )
{

	static char szAttributes[6];
	strcpy( szAttributes, "_____" );

	if ( nAttribute & _A_RDONLY )
		szAttributes[0] = _TEXT('R');
	if ( nAttribute & _A_HIDDEN )
		szAttributes[1] = _TEXT('H');
	if ( nAttribute & _A_SYSTEM )
		szAttributes[2] = _TEXT('S');
	if ( nAttribute & _A_ARCH )
		szAttributes[3] = _TEXT('A');
	if ( nAttribute & _A_SUBDIR )
		szAttributes[4] = _TEXT('D');

	return szAttributes;
} 


typedef struct
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	LPTSTR pszFilename;
	long fFlags;
	long lMode;
	int nField;
} TOUCH_STRUCT;

#define TOUCH_CREATE 1
#define TOUCH_FORCE 2
#define TOUCH_QUIET 4
#define TOUCH_DUP 8
#define TOUCH_SUBDIRS 0x10
#define TOUCH_NOTHING 0x20
#define TOUCH_PRESERVE_DATE 0x40
#define TOUCH_PRESERVE_TIME 0x80

static int _touch( TOUCH_STRUCT *, DATETIME * );


// change a file's date & time
int _near Touch_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg, pszTemp;
	int nArgc, nReturn = 0, rc;
	unsigned int uHours, uMinutes, uSeconds;
	unsigned int uMonth, uDay, uYear;
	FILESEARCH dir;
	DATETIME sysDateTime;
	TOUCH_STRUCT Touch;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( TOUCH_USAGE ));

	Touch.nField = 0;
	Touch.fFlags = 0;
	Touch.lMode = 0x07;
	Touch.szSource[0] = _TEXT('\0');

	QueryDateTime( &sysDateTime );

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;
	
	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;

	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc | 0x800 )) != NULL ); nArgc++ ) {

		if ( *pszArg == gpIniptr->SwChr ) {

		  for ( ; ; ) {

			// kludge to allow (illegal) syntax like "touch /F/Q file"
			if ( *pszArg == gpIniptr->SwChr )
				pszArg++;
			if ( *pszArg == _TEXT('\0') )
				break;

			rc = _ctoupper( *pszArg++ );
			switch ( rc ) {
			case 'A':       // retrieve based on specified attribs
				pszArg = GetSearchAttributes( pszArg );
				Touch.lMode |= 0x17;
				break;

			case 'E':
				Touch.lMode |= 0x100;
				break;

			case 'C':
			    // create the file if it doesn't exist
			    Touch.fFlags |= TOUCH_CREATE;
			    break;

			case 'F':
			    // ride roughshod over read-only files
			    Touch.fFlags |= TOUCH_FORCE;
			    break;

			case 'N':
				Touch.fFlags |= TOUCH_NOTHING;
				break;

			case 'Q':
			    Touch.fFlags |= TOUCH_QUIET;
			    break;

			case 'R':
			    // duplicate this file's date & time
			    Touch.fFlags |= TOUCH_DUP;

			    // check for optional write/access/create field
			    if ( *pszArg == _TEXT(':') ) {
					pszArg++;
			        if (( *pszArg == _TEXT('W') ) || ( *pszArg == _TEXT('w') )) {
						Touch.nField = 0;
						pszArg++;
			        } else if (( *pszArg == _TEXT('A') ) || ( *pszArg == _TEXT('a') )) {
						Touch.nField = 1;
						pszArg++;
			        } else if (( *pszArg == _TEXT('C') ) || ( *pszArg == _TEXT('c') )) {
						Touch.nField = 2;
						pszArg++;
			        }
			    }

			    if (( *pszArg == _TEXT('\0') ) && (( pszArg = ntharg( pszCmdLine, ++nArgc | 0x800 )) == NULL ))
					return ( Usage( TOUCH_USAGE ));

			    if ( find_file( FIND_FIRST, pszArg, Touch.lMode | FIND_CLOSE | FIND_NO_ERRORS, &dir, NULL ) != NULL ) {

	                if ( Touch.nField == 1)
                       FileTimeToDOSTime( &(dir.ftLastAccessTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
               	    else if ( Touch.nField == 2)
                       	FileTimeToDOSTime( &(dir.ftCreationTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );

					sysDateTime.year = dir.fd.file_date.years;
					if ( sysDateTime.year < 120 )
						sysDateTime.year += 1980;
					sysDateTime.month = dir.fd.file_date.months;
					sysDateTime.day = dir.fd.file_date.days;
					sysDateTime.hours = dir.ft.file_time.hours;
					sysDateTime.minutes = dir.ft.file_time.minutes;
					sysDateTime.seconds = dir.ft.file_time.seconds;

				}

				pszArg = NULLSTR;
			    break;

			case 'D':
			case 'T':

			    if ( *pszArg == _TEXT(':') )
					pszArg++;
			    if (( *pszArg == _TEXT('W') ) || ( *pszArg == _TEXT('w') )) {
					Touch.nField = 0;
					pszArg++;
			    } else if (( *pszArg == _TEXT('A') ) || ( *pszArg == _TEXT('a') )) {
					Touch.nField = 1;
					pszArg++;
			    } else if (( *pszArg == _TEXT('C') ) || ( *pszArg == _TEXT('c') )) {
					Touch.nField = 2;
					pszArg++;
			    }

			    if ( rc == _TEXT('D') ) {

			        // valid date entry?
					if ( *pszArg == _TEXT('\0') ) {
						// keep the existing date
						Touch.fFlags |= TOUCH_PRESERVE_DATE;
					    break;
					}

					if ( GetStrDate( pszArg, &uMonth, &uDay, &uYear ) == 3 ) {

						if (( sysDateTime.year = uYear ) < 80 )
							sysDateTime.year += 2000;
					    else if ( sysDateTime.year < 100 )
						    sysDateTime.year += 1900;

						sysDateTime.month = (unsigned char)uMonth;
						sysDateTime.day = (unsigned char)uDay;
						pszArg = NULLSTR;
						break;
					}

					return ( error( ERROR_4DOS_INVALID_DATE, pszArg ));

			    } else {

			        uSeconds = 0;

					if ( *pszArg == _TEXT('\0') ) {
						// keep existing time
						Touch.fFlags |= TOUCH_PRESERVE_TIME;
						break;
					}

			        if (( sscanf( pszArg, DATE_FMT, &uHours, &uMinutes, &uSeconds ) >= 2 ) && ( uHours < 24 ) && ( uMinutes < 60 ) && ( uSeconds < 60 )) {

						// check for AM/PM syntax
						if (( pszArg = strpbrk( pszArg, _TEXT("AP") )) != NULL ) {
							if (( uHours == 12 ) && ( *pszArg == _TEXT('A') ))
								uHours -= 12;
							else if (( *pszArg == _TEXT('P') ) && ( uHours < 12 ))
							    uHours += 12;
						}

				        sysDateTime.hours = (unsigned char)uHours;
				        sysDateTime.minutes = (unsigned char)uMinutes;
				        sysDateTime.seconds = (unsigned char)uSeconds;
						sysDateTime.hundredths = 0;
					    pszArg = NULLSTR;
					    break;
			        }

			        return ( error( ERROR_4DOS_INVALID_TIME, pszArg ));
			    }

			case 'S':
			    Touch.fFlags |= TOUCH_SUBDIRS;
			    break;

			default:
			    return ( Usage( TOUCH_USAGE ));
			}
		  }

		  continue;
		}

		mkfname( pszArg, 0 );
		strcpy( Touch.szSource, pszArg );

		// save the source filename part (for recursive calls and
		//   include lists)
		Touch.pszFilename = Touch.szSource + strlen( path_part( Touch.szSource));
		pszTemp = (LPTSTR)_alloca( ( strlen( Touch.pszFilename ) + 1 ) * sizeof(TCHAR) );
		Touch.pszFilename = strcpy( pszTemp, Touch.pszFilename );

			nReturn = _touch( &Touch, &sysDateTime );

		if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ))
			break;

	}

	return (( Touch.szSource[0] == _TEXT('\0') ) ? Usage( TOUCH_USAGE ) : nReturn );
}


static int _touch( TOUCH_STRUCT *Touch, DATETIME *sysDateTime )
{
	int nReturn = 0, rc, nFind;
	unsigned int uAttribute = 0;
	FILESEARCH dir;
	LPTSTR pszSourceName;

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );
	
	dir.hdir = INVALID_HANDLE_VALUE;

	// touch specified files
	for ( nFind = FIND_FIRST; ( nFind != 0 ); nFind = FIND_NEXT ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			FindClose( dir.hdir );
			return CTRLC;
		}

		CheckForBreak();


		if ( find_file( nFind, Touch->szSource, ( Touch->lMode | FIND_NO_ERRORS | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, Touch->szTarget ) == NULL ) {

			if ( nFind == FIND_FIRST ) {

				// if empty file list, ignore it
				if ( dir.pszFileList != NULL )
					break;

			    // if file doesn't exist, create it
				if ( Touch->fFlags & TOUCH_NOTHING ) {
					strcpy( Touch->szTarget, Touch->szSource );
					nFind = 0;
				} else if (( Touch->fFlags & TOUCH_CREATE ) && (( nFind = _sopen( Touch->szSource, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _SH_DENYWR, _S_IWRITE | _S_IREAD )) >= 0 )) {
					_close( nFind );
					strcpy( Touch->szTarget, Touch->szSource );
					nFind = 0;
			    } else {
					if (( Touch->lMode & 0x100 ) == 0 )
						error( ERROR_FILE_NOT_FOUND, Touch->szSource );
					nReturn = ERROR_EXIT;
					break;
				}

			} else
			    break;
		}

		if ( Touch->fFlags & ( TOUCH_PRESERVE_DATE | TOUCH_PRESERVE_TIME )) {

			if ( Touch->nField == 1)
				FileTimeToDOSTime( &(dir.ftLastAccessTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
			else if ( Touch->nField == 2)
				FileTimeToDOSTime( &(dir.ftCreationTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );

			if ( Touch->fFlags & TOUCH_PRESERVE_DATE ) {
				sysDateTime->year = dir.fd.file_date.years;
				if ( sysDateTime->year < 120 )
					sysDateTime->year += 1980;
				sysDateTime->month = dir.fd.file_date.months;
				sysDateTime->day = dir.fd.file_date.days;
			}

			if ( Touch->fFlags & TOUCH_PRESERVE_TIME ) {
				sysDateTime->hours = dir.ft.file_time.hours;
				sysDateTime->minutes = dir.ft.file_time.minutes;
				sysDateTime->seconds = dir.ft.file_time.seconds;
			}

		}

		if (( Touch->fFlags & TOUCH_FORCE ) && (( Touch->fFlags & TOUCH_NOTHING ) == 0 )) {
			// set the target file attributes, less RDONLY
			QueryFileMode( Touch->szTarget, &uAttribute );
			SetFileMode( Touch->szTarget, uAttribute & 0x26 );
		}

		// display the time being set
		if (( Touch->fFlags & TOUCH_QUIET ) == 0 )
			printf( _TEXT("%s %02d:%02d:%02d  %s\r\n"), FormatDate( sysDateTime->month, sysDateTime->day, sysDateTime->year, 0x100 ), sysDateTime->hours, sysDateTime->minutes, sysDateTime->seconds, Touch->szTarget );

		if (( Touch->fFlags & TOUCH_NOTHING ) == 0 ) {
			if ((( rc =  SetFileDateTime( Touch->szTarget, 0, sysDateTime, Touch->nField )) != 0 ) && (( Touch->lMode & 0x100 ) == 0 ))
				nReturn = error( rc, Touch->szTarget );
		}

		// restore original attributes
		if (( Touch->fFlags & TOUCH_FORCE ) && (( Touch->fFlags & TOUCH_NOTHING ) == 0 ))
			SetFileMode( Touch->szTarget, uAttribute );
	}

	if ( Touch->fFlags & TOUCH_SUBDIRS ) {

		// modify matching subdirectory files

		CheckFreeStack( 1024 );

		// strip the filename & add wildcards for directory search
		sprintf( Touch->szSource, FMT_DOUBLE_STR, path_part( Touch->szSource ), WILD_FILE );

		// save the current subdirectory start
		pszSourceName = strchr( Touch->szSource, _TEXT('*') );
 
		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, & to only
		//   return subdir names
		for ( nFind = FIND_FIRST; ( find_file( nFind, Touch->szSource, ( Touch->lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// make the new "source"
			sprintf( pszSourceName, FMT_PATH_STR, dir.szFileName, Touch->pszFilename );

			// process directory tree recursively
			nReturn = _touch( Touch, sysDateTime );

			if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
				// reset the directory search handle
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSourceName, WILD_FILE );
		}
	}

	return nReturn;
}


#define FO_HEAD 1
#define FO_TAIL 2
#define FO_TYPE 4
#define FO_VERBOSE 8
#define FO_LINES 0x10
#define FO_NUMBERING 0x20
#define FO_PAUSE 0x40
#define FO_PIPE 0x80
#define FO_FOLLOW 0x100

// display file head to stdout
int _near Head_Cmd( LPTSTR pszCmdLine )
{
	int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_HEAD )) == USAGE_ERR )
		return ( Usage( HEAD_USAGE ));

	return nReturn;
}


// display file(s) to stdout
int _near Tail_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_TAIL )) == USAGE_ERR )
		return ( Usage( TAIL_USAGE ));

	return nReturn;
}


#define TYPE_NUMBER 2
#define TYPE_PAUSE 4

// display file(s) to stdout
int _near Type_Cmd( LPTSTR pszCmdLine )
{
	int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_TYPE )) == USAGE_ERR )
		return ( Usage( TYPE_USAGE ));

	return nReturn;
}


// display file(s) to stdout -- called by HEAD, TAIL, and TYPE
static int _fastcall FileOutput( LPTSTR pszCmdLine, int fOp )
{
	register TCHAR *pszArg, *pszLine;
	int nArgc, nFind, nReturn = 0, fEOF, nBytes = 0, nEditFlags = EDIT_DATA, fFlags = 0, nColumn = 0, nFH = -1;
	unsigned int uMode = _O_BINARY | _O_RDONLY;
	long lRow, lOptions, lDisplayLines = 10, lLines;
	unsigned long ulChars = (ULONG)-1L, ulAttributes = 0L;
	TCHAR chBKM;
	TCHAR szClip[MAXFILENAME], szSource[MAXFILENAME];
	FILESEARCH dir;

	LONG llEnd;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	init_page_size();		// clear row & page length vars

	// TYPE command?
	if ( fOp & FO_TYPE ) {

		// check for and remove switches
		if ( GetSwitches( pszCmdLine, _TEXT("*LP"), &lOptions, 0 ) != 0 )
			return USAGE_ERR;

		if ( lOptions & 1 )
			ulAttributes = 0x07;

		// print the line number?
		if ( lOptions & TYPE_NUMBER ) {
			fFlags |= FO_NUMBERING;
			nColumn = 7;
		}

		// pause after each page?
		if ( lOptions & TYPE_PAUSE ) {
			fFlags |= FO_PAUSE;
			gnPageLength = GetScrRows();
		}
	}

	// save arguments on stack so we can overwrite with input buffer
	pszLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );
	szSource[0] = _TEXT('\0');

	for ( nArgc = 0; ; nArgc++ ) {

		// check for input redirection - "head < file"
		if (( pszArg = ntharg( pszLine, nArgc )) == NULL ) {
			if ( szSource[0] )
				break;

			if ( QueryIsConsole( STDIN ) )
				return USAGE_ERR;

			pszArg = CONSOLE;
			nFH = STDIN;
		}

		fFlags &= ~FO_PIPE;

		if (( fOp & ( FO_HEAD | FO_TAIL ))  && ( *pszArg == gpIniptr->SwChr )) {

			// check for /A:xxx switch
			if (( _ctoupper( pszArg[1] ) == _TEXT('A') ) && ( pszArg[2] == _TEXT(':') )) {
				// set gnInclusiveMode and gnExclusiveMode
				GetSearchAttributes( pszArg + 2 );
				ulAttributes = 0x07;
				continue;
			}

			// number of characters to display
			if ( QueryIsSwitch( pszArg, _TEXT('C'), 0 )) {

				chBKM = _TEXT('\0');
				ulChars = (ULONG)-1L;
				pszArg += 2;
				if ( *pszArg == _TEXT(':'))
					pszArg++;
				sscanf( pszArg, _TEXT("%lu%c"), &ulChars, &chBKM );
				switch ( chBKM ) {
				case _TEXT('B'):
					ulChars *= 512;
					break;
				case _TEXT('k'):
					ulChars *= 1000;
					break;
				case _TEXT('K'):
					ulChars *= 1024;
					break;
				case _TEXT('m'):
					ulChars *= 1000000;
					break;
				case _TEXT('M'):
					ulChars *= 1048576;
				}
				continue;
			}

			// follow file update?
			if (( QueryIsSwitch( pszArg, _TEXT('F'), 0 )) && ( fOp & FO_TAIL )) {
				fFlags |= FO_FOLLOW;
				continue;
			}

			// number of lines to display
			if ( QueryIsSwitch( pszArg, _TEXT('N'), 0 )) {
				if ( isdigit( pszArg[2] ))
					pszArg += 2;
				else if (( pszArg[2] == _TEXT('\0')) && (( pszArg = ntharg( pszLine, nArgc+1 )) != NULL ) && ( isdigit ( *pszArg )))
					nArgc++;
				if ( pszArg )
					lDisplayLines = atoi( pszArg );
				continue;
			}

			// pause after each page?
			if ( QueryIsSwitch( pszArg, _TEXT('P' ), 0 )) {
				fFlags |= FO_PAUSE;
				gnPageLength = GetScrRows();
				continue;
			}

			// no header?
			if ( QueryIsSwitch( pszArg, _TEXT('Q' ), 0 )) {
				fFlags &= ~FO_VERBOSE;
				continue;
			}

			// header?
			if ( QueryIsSwitch( pszArg, _TEXT('V' ), 0 )) {
				fFlags |= FO_VERBOSE;
				continue;
			}

			return USAGE_ERR;
		}

		copy_filename( szSource, pszArg );
		mkfname( szSource, 0 );

		// display specified files (including hidden & read-only)
		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			lLines = lDisplayLines;

			szClip[0] = _TEXT('\0');
			if ( QueryIsDevice( szSource )) {

				if ( nFind == FIND_NEXT )
					break;
				nFind = FIND_NEXT;		// only allow this once!

				if ( stricmp( szSource, CLIP ) == 0 ) {

					RedirToClip( szClip, 99 );
					if ( CopyFromClipboard( szClip ) != 0 )
						break;
					strcpy( gszCmdline, szClip );

				} else {

					// kludge for knuckleheads who try to type character devices!
					LPTSTR pName;

					pName = fname_part( szSource );

					// nothing to do on a TYPE NUL
					if (( pName != NULL ) && ( stricmp( pName, _TEXT("NUL")) == 0 ))
						break;

					if (( pName == NULL ) || ( stricmp( pName, CONSOLE ) != 0 )) {
						nReturn = error( ERROR_ACCESS_DENIED, szSource );
						break;
					}
				}

			} else if ( QueryIsPipeName( szSource )) {

				// only look for named pipes once
				if ( nFind == FIND_NEXT )
					break;
				strcpy( gszCmdline, szSource );
				ulChars = (ULONG)-1L;
				fFlags |= FO_PIPE;

				// kludge for CMD.EXE compatibility -- if no wildcards, match hidden names; otherwise, require /A:xx
			} else if ( find_file( nFind, szSource, ( ulAttributes | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | (( strpbrk( szSource, WILD_CHARS ) == NULL ) ? 0x07 : 0 )), &(dir), gszCmdline ) == NULL ) {
				if ( nFind == FIND_FIRST )
					nReturn = ERROR_EXIT;
				break;
			}

			// print header for HEAD & TAIL
			if ( fFlags & FO_VERBOSE ) {
				qputs( _TEXT("---- ") );
				more_page( gszCmdline, 5 );
			}

#
				if ( nFH != STDIN ) {

					if (( nFH = _sopen( gszCmdline, uMode, _SH_DENYNO )) < 0 ) {
						nReturn = error( _doserrno, gszCmdline );
						if ( szClip[0] )
							break;
						continue;
					}
				}

				if ( setjmp( cv.env ) == -1 ) {

					if ( nFH != STDIN ) {
						_close( nFH );
						nFH = -1;
					}
					nReturn = CTRLC;
					break;
				}


				if (( fOp & FO_TAIL ) && ( ulChars != (ULONG)-1L )) {

					// first, get file size

						// back up "ulChars" (characters, not bytes!) from the end
						llEnd = _lseek( nFH, 0L, SEEK_END );

							_lseek( nFH, (( ulChars > llEnd ) ? -llEnd : -(LONG)ulChars ), SEEK_END );

					// now we'll fall through to the TYPE read routine & read the last "ulChars" of the file
				}

				// output the file to stdout, line-by-line
				if (( fOp & ( FO_HEAD | FO_TAIL )) && ( ulChars == (ULONG)-1L )) {

					if ( fOp & FO_TAIL ) {

						// TAIL - start at the end and count back "lLines"
						// seek to end, save size, then back up & start counting LF's

							llEnd = _lseek( nFH, 0L, SEEK_END );

						for ( lRow = lLines; ( lRow > 0 ); ) {

								if ( llEnd <= 0 ) {
									_lseek( nFH, 0L, SEEK_SET );
									break;
								}

								if (( llEnd -= CMDBUFSIZ ) < 0 )
									llEnd = 0;

								_lseek( nFH, llEnd, SEEK_SET );

								nBytes = _read( nFH, gszCmdline, CMDBUFSIZ );

								for ( --nBytes; ( --nBytes >= 0 ); ) {

									if (( gszCmdline[ nBytes ] == LF ) && ( --lRow <= 0 )) {
										nBytes++;

										_lseek( nFH, ( llEnd + nBytes ), SEEK_SET );
										break;
									}
								}

						}
					}


						for ( ; (( lLines > 0 ) && ( getline( nFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) > 0 )); lLines-- )
							more_page( gszCmdline, 0 );

					// close the file / pipe
					goto CloseHandles;

				} else if ( fFlags & ( FO_NUMBERING | FO_PAUSE )) {

					// TYPE /L or /P
					for ( lRow = 1; ( getline( nFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) > 0 ); lRow++ ) {
						if ( fFlags & FO_NUMBERING )
							printf( _TEXT("%4lu : "), lRow );
						more_page( gszCmdline, nColumn );
					}

					goto CloseHandles;
				}

			for ( fEOF = 0; ( fEOF == 0 ); ) {

				nBytes = 0;

					wwrite( STDOUT, (LPSTR)(glpPipe + llPipeOffset), uSize );

					break;

				} else {

					if (( nBytes = _read( nFH, gszCmdline, (( ulChars < CMDBUFSIZ ) ? ulChars : CMDBUFSIZ ))) <= 0 )
						break;

					// make sure we're not at the end of the file ("type file >> file")
					fEOF = _eof( nFH );
				}


				if (( nBytes = _read( nFH, gszCmdline, (unsigned int)(( ulChars < CMDBUFSIZ ) ? ulChars : CMDBUFSIZ ))) <= 0 )
					break;

				// make sure we're not at the end of the file ("type file >> file")
				fEOF = _eof( nFH );


				if ( wwrite( STDOUT, gszCmdline, nBytes ) == -1 ) {

						error( _doserrno, szSource );
					break;
				}

				// adjust the number of chars left to read
				if ( ulChars != (ULONG)-1L ) {
					// if max number of chars, stop
					if ( ulChars < (ULONG)CMDBUFSIZ )
						break;
					ulChars -= CMDBUFSIZ;
				}
			}

CloseHandles:

			{
				if ( nFH == STDIN )
					break;

				_close( nFH );
				nFH = -1;

			}

			// delete temporary CLIP: file
			if ( szClip[0] ) {
				remove( szSource );
				break;
			}
		}
	}

	return nReturn;
}


// y reads the standard input and writes to standard output, then invokes TYPE
//   to put one or more files to standard output
int _near Y_Cmd( LPTSTR pszCmdLine )
{

	extern char _osfile[];

	int nBytesRead, fEof;
	TCHAR *pszTemp, szYBuf[514];

	for ( fEof = 0; (( fEof == 0 ) && ( getline( STDIN, szYBuf, 512, EDIT_DATA ) > 0 )); ) {

		// look for ^Z
		if (( pszTemp = strchr( szYBuf, EoF )) != NULL ) {
			if ( pszTemp == szYBuf )
				break;
			*pszTemp = _TEXT('\0');
			fEof = 1;
		} else
			strcat( szYBuf, gszCRLF );

		nBytesRead = strlen( szYBuf );

		wwrite( STDOUT, szYBuf, nBytesRead );
	}

	// clear EOF flag
	_osfile[STDIN] &= ~( FEOFLAG | FCRLF );

	return ((( pszCmdLine ) && ( *pszCmdLine )) ? Type_Cmd( pszCmdLine ) : 0 );
}


// tee copies standard input to output & puts a copy in the specified file(s)
int _near Tee_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	register int i;
	int nBytesRead, fEof, nMode = 0, nReturn = 0, nFiles = 1, naFH[20], fTeeClip = -1;
	TCHAR *pszTemp, szClip[MAXFILENAME], szTeeBuffer[CMDBUFSIZ+2];
	long fTee;

	// check for "append" switch
	if (( GetSwitches( pszCmdLine, _TEXT("A"), &fTee, 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( TEE_USAGE ));

	nMode = (( fTee == 0 ) ? (_O_RDWR | _O_CREAT | _O_BINARY | _O_TRUNC) : (_O_RDWR | _O_CREAT | _O_BINARY | _O_APPEND));

	naFH[0] = STDOUT;
	szClip[0] = _TEXT('\0');

	if ( setjmp( cv.env ) == -1 )
		nReturn = CTRLC;

	else {

		for ( i = 0; (( pszArg = ntharg( pszCmdLine, i )) != NULL ) && ( nFiles < 20 ); i++, nFiles++ ) {

			mkfname( pszArg, 0 );

			// TEE'ing to CLIP: ?
			if ( stricmp( pszArg, CLIP ) == 0 ) {
				RedirToClip( szClip, 99 );
				pszArg = szClip;
			}

// FIXME - add FTP support!

			if (( naFH[nFiles] = _sopen( pszArg, nMode, _SH_DENYWR, ( _S_IREAD | _S_IWRITE ))) < 0 ) {
				nReturn = error( _doserrno, pszArg );
				goto tee_bye;
			}

			// save file handle for CLIP:
			if ( pszArg == szClip )
				fTeeClip = naFH[nFiles];
		}

		for ( fEof = 0; (( fEof == 0 ) && ( getline( STDIN, szTeeBuffer, CMDBUFSIZ, EDIT_DATA ) > 0 )); ) {

			// look for ^Z
			if (( pszTemp = strchr( szTeeBuffer, EoF )) != NULL ) {
				if ( pszTemp == szTeeBuffer )
					break;
				*pszTemp = _TEXT('\0');
				fEof = 1;
			} else
				strcat( szTeeBuffer, gszCRLF );
			nBytesRead = strlen( szTeeBuffer );

			for ( i = 0; ( i < nFiles ); i++ )
				wwrite( naFH[i], szTeeBuffer, nBytesRead );
		}
	}

tee_bye:
	// close the output files (this looks a bit peculiar because we have
	//   to allow for a ^C during the close(), & still close everything
	for ( ; ( nFiles > 1 ); nFiles-- ) {

		// is one of the TEE arguments CLIP: ?
		if ( fTeeClip == naFH[nFiles-1] )
			CopyToClipboard( fTeeClip );

		_close( naFH[nFiles-1] );
	}

	if ( szClip[0] )
		remove( szClip );


	return nReturn;
}
