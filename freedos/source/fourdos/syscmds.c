

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


// SYSCMDS.C - System commands for 4DOS
//   Copyright (c) 1988 - 2002 Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"


static void _fastcall GetFuzzyDir( LPTSTR );
void UpdateFuzzyTree( LPTSTR, LPTSTR, int );
static int _global( unsigned long, long, LPTSTR, LPTSTR );
static void ReadAndWriteFiles( int, int, TCHAR *, int, int );

#pragma alloc_text( MISC_TEXT, GetFuzzyDir, UpdateFuzzyTree, ReadAndWriteFiles )
#pragma alloc_text( MISC_TEXT, MakeDirectory, DestroyDirectory, SaveDirectory )
#pragma alloc_text( MISC_TEXT, gtime, gdate, _timer, _log_entry, getlabel )
#pragma alloc_text( MISC_TEXT, GetLogName )

TCHAR gaPushdQueue[ DIRSTACKSIZE ];	// PUSHD/POPD storage area
static TCHAR szLastDirectory[ MAXPATH ];		// last dir for "CD -"


// change the current directory
int _near Cd_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	int fFlags = CD_SAVELASTDIR;
	long fCD = 0;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		while (( pszArg = first_arg( pszCmdLine )) != NULL ) {
			if ( stricmp( pszArg, _TEXT("/N") ) == 0 ) {
				fCD |= 2;
				pszCmdLine = skipspace( pszCmdLine + 2 );
			} else
				break;
		}
	}

	// disable fuzzy searching
	if ( fCD & 2 )
		fFlags |= CD_NOFUZZY;

	// if no args or arg == disk spec, print current working directory
	if ( pszCmdLine )
		StripQuotes( pszCmdLine );

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ) || (( _stricmp( pszCmdLine + 1, _TEXT(":") ) == 0 ) && (( fCD & 1 ) == 0 ))) {

		if (( pszArg = gcdir( pszCmdLine, 0 )) == NULL )
			return ERROR_EXIT;	// invalid drive

		printf( FMT_STR_CRLF, pszArg );
		return 0;
	}


	return ( __cd( pszCmdLine, fFlags ));
}


// change the current disk and directory
int _near Cdd_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszPtr, pszArg;
	TCHAR szDrives[128], szTreePath[MAXFILENAME], szTemp[MAXFILENAME];
	int i, n, nReturn = 0, fFlags = CD_CHANGEDRIVE | CD_SAVELASTDIR, fSave, fUpdate;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if ( _strnicmp( pszCmdLine, _TEXT("/U"), 2 ) == 0 ) {
			// update tree(s)
			fUpdate = 1;
			pszCmdLine[1] = _TEXT('S');
		} else if ( _strnicmp( pszCmdLine, _TEXT("/D"), 2 ) == 0 ) {
			// delete tree(s)
			fUpdate = 2;
			pszCmdLine[1] = _TEXT('S');
		} else
			fUpdate = 0;

		// disable fuzzy searching
		if ( _strnicmp( pszCmdLine, _TEXT("/N "), 3 ) == 0 ) {
			fFlags |= CD_NOFUZZY;
			strcpy( pszCmdLine, skipspace( pszCmdLine + 3 ));

			// if /A, display current directory on all drives C: - Z:
		} else if ( _stricmp( pszCmdLine, _TEXT("/A") ) == 0 ) {

			for ( n = 3; ( n <= 26 ); n++ ) {

				if ( QueryDriveReady( n )) {
					sprintf( szDrives, _TEXT("%c:"), n + 64 );
					if (( pszPtr = gcdir( szDrives, 1 )) != NULL )
						printf( FMT_STR_CRLF, pszPtr );
				}
			}

			return 0;

		} else if ( _strnicmp( pszCmdLine, _TEXT("/S"), 2 ) == 0 ) {

			LPTSTR pszSave;

			// get tree index location
			if ( gpIniptr->TreePath != INI_EMPTYSTR )
				pszPtr = (LPTSTR)( gpIniptr->StrData + gpIniptr->TreePath );

			else
				pszPtr = _TEXT("C:\\");
			strcpy( szTreePath, pszPtr );
			mkdirname( szTreePath, _TEXT("jpstree.idx") );
			AddQuotes( szTreePath );

			// scan drive for wildcard searches
			pszSave = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );

			pszPtr = skipspace( pszSave + 2 );
			if ( *pszPtr == _TEXT('\0') ) {
				// default to all hard disks
				SetDriveString( szDrives );
FormatDriveNames:
				for ( pszPtr = szDrives; ( *pszPtr != _TEXT('\0') ); pszPtr += 4 )
					strins( pszPtr+1, _TEXT(":\\ ") );
				pszPtr = szDrives;

			} else {
				pszPtr = skipspace( pszPtr );
				if ( strpbrk( pszPtr, _TEXT(":\\. \t") ) == NULL ) {
					strcpy( szDrives, pszPtr );
					goto FormatDriveNames;
				}
				pszPtr = strcpy( (LPTSTR)_alloca( ( strlen( pszPtr ) + 1 ) * sizeof(TCHAR) ), pszPtr );
			}

			for ( i = 0; (( pszArg = ntharg( pszPtr, i )) != NULL ); i++ ) {

				if ( fUpdate ) {
					// remove existing tree from JPSTREE.IDX
					strcpy( szTemp, pszArg );
					UpdateFuzzyTree( szTemp, NULL, 1 );
					if ( fUpdate == 2 )
						continue;
				}

				// use TREE to build the directory list
				printf( BUILDING_INDEX, pszArg );
				if ( fUpdate ) {
					// build a temporary file
					pszArg = strcpy( (LPTSTR)_alloca( ( strlen( pszArg ) + 1 ) * sizeof(TCHAR) ), pszArg );
					szTemp[0] = _TEXT('\0');

					GetTempDirectory( szTemp );

					UniqueFileName( szTemp );
					AddQuotes( szTemp );
				}

				sprintf( pszCmdLine, _TEXT("*@tree /c%s %s %s %s"), (( gpIniptr->CompleteHidden ) ? _TEXT("/h") : NULLSTR ), pszArg, (( i == 0 ) ? _TEXT(">!") : _TEXT(">>!") ), (( fUpdate ) ? szTemp : szTreePath ));

				// make sure TREE isn't disabled!
				n = findcmd( _TEXT("TREE"), 1 );
				fSave = commands[ n ].fParse;
				commands[ n ].fParse &= ~CMD_DISABLED;

				nReturn = command( pszCmdLine, 0 );

				commands[ n ].fParse = fSave;

				// check for ^C or stack overflow
				if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ) || ( cv.fException ))
					return CTRLC;

				if ( fUpdate ) {
					// insert temp file into JPSTREE.IDX
					UpdateFuzzyTree( pszArg, szTemp, 0 );
					remove( szTemp );
				}
			}

			return 0;
		}
	}

	return ((( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) ? Usage( CDD_USAGE ) : __cd( pszCmdLine, fFlags ));
}


// get the location of JPSTREE.IDX
static void _fastcall GetFuzzyDir( LPTSTR pszDir )
{
	register LPTSTR pszTreePath;

	if ( gpIniptr->TreePath != INI_EMPTYSTR )
		pszTreePath = (LPTSTR)( gpIniptr->StrData + gpIniptr->TreePath );

	else
		pszTreePath = _TEXT("C:\\");

	copy_filename( pszDir, pszTreePath );
	StripQuotes( pszDir );
	mkdirname( pszDir, _TEXT("jpstree.idx") );
}


static void ReadAndWriteFiles( int nInput, int nOutput, TCHAR *pszLine, int nLength, int nEditFlags )
{
	while ( getline( nInput, pszLine, nLength, nEditFlags ) > 0 ) {

		// skip blank lines
		if ( *pszLine == _TEXT('\0') )
			continue;

		qprintf( nOutput, FMT_STR_CRLF, pszLine );
	}
}


// update JPSTREE.IDX (on _mkdir() & _rmdir() )
//   fDelete = 0 to insert, 1 to delete
void UpdateFuzzyTree( LPTSTR pszDirectory, LPTSTR pszInsertFile, int fDelete )
{
	register int i;
	int nFH, nFH2 = -1, fTruncate = 0, nEditFlags = EDIT_NO_PIPES;
	unsigned int uBlockSize, uBytesRead = 0, uMode;
	TCHAR szDir[ MAXFILENAME ], szPath[ MAXFILENAME ], szLine[ MAXFILENAME + 128 ];
	long lReadOffset, lWriteOffset = 0L, lBlockSize;

	char szTempFile[MAXFILENAME];

	szTempFile[0] = '\0';

	// get tree index location
	GetFuzzyDir( szDir );
	uMode = (( fDelete ) ? _O_RDWR | _O_BINARY : _O_CREAT | _O_RDWR | _O_BINARY );

	// don't create JPSTREE if fuzzy is disabled (but still update it!)
	if ( gpIniptr->FuzzyCD == 0 )
		uMode &= ~_O_CREAT;

	if (( nFH = _sopen( szDir, uMode, _SH_DENYWR, _S_IREAD | _S_IWRITE )) < 0 )
		return;

	// disable ^C / ^BREAK handling
	HoldSignals();

	copy_filename( szPath, pszDirectory );
	StripQuotes( szPath );

	// remove trailing _TEXT('/') or '\'
	strip_trailing( szPath+3, SLASHES );

	// if we're adding an entry, look for its parent directory
	if (( fDelete == 0 ) && ( pszInsertFile == NULL )) {
		copy_filename( szPath, path_part( pszDirectory ));
		strip_trailing( szPath+3, SLASHES );
	}

	for ( ; ; ) {

		lReadOffset = (LONG)_lseek( nFH, 0L, SEEK_CUR );
		if ( getline( nFH, szLine, sizeof(szLine) / sizeof(TCHAR), nEditFlags ) <= 0 )
			break;

		if ( szLine[0] == _TEXT('\0') )
			continue;

		// look for argument match
		sscanf( szLine, _TEXT("%255[^\004\r\n]"), szDir );

		// find where the insertion goes
		i = strlen( szPath );
		if ( fDelete ) {

			// check for a partial match (i.e., "W:\" means delete EVERYTHING on W:!)
			if ( _strnicmp( szPath, szDir, i ) != 0 )
				continue;

			// got a partial match -- does it match a full pathname?
			if (( stricmp( szPath+1, _TEXT(":\\")) != 0 ) && ( stricmp( szPath+1, _TEXT(":/")) != 0 )) {
				if (( szDir[i] != _TEXT('\0')) && ( szDir[i] != _TEXT('\\')) && ( szDir[i] != _TEXT('/') ))
					continue;
			}

		} else if ( _stricmp( szPath, szDir ) != 0 )
			continue;

		if (( fDelete ) || ( pszInsertFile )) {

			// delete all of this directory's subdirectories
			mkdirname( szPath, NULLSTR );

			do {
				lWriteOffset = (LONG)_lseek( nFH, 0L, SEEK_CUR );
				if ( getline( nFH, szLine, sizeof(szLine) / sizeof(TCHAR), nEditFlags ) <= 0 )
					break;
			} while ( _strnicmp( szPath, szLine, i ) == 0 );

			fTruncate = 1;

		} else {
			lReadOffset = (LONG)_lseek( nFH, 0L, SEEK_CUR );
			lWriteOffset = lReadOffset;
		}

		// get remaining size
		lBlockSize = ( (long)QuerySeekSize( nFH ) - lWriteOffset );

		if ( lBlockSize > 0L ) {

			uBlockSize = (unsigned int)lBlockSize;
			_lseek( nFH, lWriteOffset, SEEK_SET );

			// write remainder of file to temp file
			//   (can't read/write more than 64K in DOS!)
			GetTempDirectory( szTempFile );
			mkdirname( szTempFile, "JPSIDX.TMP" );

			if (( nFH2 = _sopen( szTempFile, _O_WRONLY | _O_BINARY | _O_CREAT, _SH_DENYWR, _S_IREAD | _S_IWRITE )) >= 0 ) {
				ReadAndWriteFiles( nFH, nFH2, szLine, sizeof(szLine) / sizeof(TCHAR), nEditFlags );
				_close( nFH2 );
			}
		}

		_lseek( nFH, lReadOffset, SEEK_SET );

		if ( fDelete == 0 ) {

			if ( pszInsertFile != NULL ) {

				// insert temp file here
				if (( nFH2 = _sopen( pszInsertFile, _O_RDONLY | _O_BINARY, _SH_DENYWR )) >= 0 ) {
					ReadAndWriteFiles( nFH2, nFH, szLine, sizeof(szLine) / sizeof(TCHAR), nEditFlags );
					_close( nFH2 );
				}

				pszInsertFile = NULL;

			} else {

				// insert new entry here!
				strcpy( szPath, pszDirectory );
//				mkfname( szPath, MKFNAME_NOERROR );

				// remove trailing '/' or '\'
				strip_trailing( szPath+3, SLASHES );

				qprintf( nFH, FMT_STR, szPath );

				// if the SFN is different, insert it here too!
				strcpy( szDir, szPath );
				GetShortName( szDir );

				szLine[0] = _TEXT('\0');
				if ( _stricmp( szDir, szPath ) != 0 )
					sprintf( szLine, _TEXT("\004\004%.2048s"), szDir );
				strcat( szLine, gszCRLF );

				qprintf( nFH, FMT_STR, szLine );
			}

			lWriteOffset = (LONG)_lseek( nFH, 0L, SEEK_CUR );
		}

		// write remainder of file
		if ( lWriteOffset > 0L ) {

			// append temp file here
			if (( szTempFile[0] ) && (( nFH2 = _sopen( szTempFile, _O_RDONLY | _O_BINARY, _SH_DENYWR )) >= 0 )) {
				ReadAndWriteFiles( nFH2, nFH, szLine, sizeof(szLine), nEditFlags );
				_close( nFH2 );
				remove( szTempFile );
			}

		}

		// truncate the file?
		if ( fTruncate )
			_chsize( nFH, (LONG)_lseek( nFH, 0L, SEEK_CUR ));
		break;
	}

	// end of JPSTREE and no insert point found?
	if ( fDelete == 0 ) {

		if ( pszInsertFile ) {

			if (( nFH2 = _sopen( pszInsertFile, _O_RDONLY | _O_BINARY, _SH_DENYWR )) >= 0 ) {
				// append temp file to end of JPSTREE.IDX
				ReadAndWriteFiles( nFH2, nFH, szLine, sizeof(szLine) / sizeof(TCHAR), nEditFlags );
				_close( nFH2 );
			}

		} else if ( lWriteOffset == 0L ) {

			// append new entry
			strcpy( szPath, pszDirectory );
			mkfname( szPath, MKFNAME_NOERROR );

			// remove trailing '/' or '\'
			strip_trailing( szPath+3, SLASHES );

			qprintf( nFH, FMT_STR, szPath );

			// if the SFN is different, insert it here too!
			strcpy( szDir, szPath );
			GetShortName( szDir );

			szLine[0] = _TEXT('\0');
			if ( _stricmp( szDir, szPath ) != 0 )
				sprintf( szLine, _TEXT("\004\004%.2048s"), szDir );
			strcat( szLine, gszCRLF );

			qprintf( nFH, FMT_STR, szLine );
		}
	}

	_close( nFH );

	// enable ^C / ^BREAK handling
	EnableSignals();
}


int _fastcall __cd( LPTSTR dir, int fFlags )
{
	register LPTSTR pszPtr = NULL;
	register int i;
	int n, nFH, nLength, fWildAll = 0, nReturn = 0, nEditFlags = EDIT_DATA | EDIT_NO_PIPES;
	unsigned int uPass = 0, uMatches;
	unsigned long ulListSize, ulSize = 0L;
	TCHAR dirname[ MAXPATH*2 ], szDirectory[ MAXPATH*2 ], szDrive[3];
	TCHAR *pszPath, *pszSavedPath;
	TCHAR _far * _far *lppList = 0L;
	TCHAR _far *pszCDPATH, _far *lpszListEntry, _far *lpszStartList;

	// save original "dir" (if it was passed as "path_part" it will be
	//   overwritten)
	pszPath = (LPTSTR)_alloca( ( strlen( dir ) + 1 ) * sizeof(TCHAR) );
	dir = strcpy( pszPath, dir );

	pszSavedPath = NULLSTR;

	i = gcdisk( NULL );
	if ( i == 0 )
		pszPtr = gcdir( NULL, 1 );

	if ((( i != 0 ) && ( QueryDriveReady( i ) == 1 )) || ( pszPtr != NULL )) {

		// save current directory for "CD -" or "CDD -"
		if (( pszPtr != NULL ) || (( pszPtr = gcdir( NULL, 0 )) != NULL )) {
			pszSavedPath = (LPTSTR)_alloca( ( strlen( pszPtr ) + 1 ) * sizeof(TCHAR) );
			strcpy( pszSavedPath, pszPtr );
		}
	}

	// save to cd / cdd popup window stack?
	if ( fFlags & CD_SAVELASTDIR ) {

		if ( _stricmp( dir, _TEXT("-") ) == 0 ) {
			dir = szLastDirectory;
			if ( *dir == _TEXT('\0') )
				return 0;
		}

		if ( pszPtr != NULL )
			SaveDirectory( glpDirHistory, pszPtr );
	}

	// "is_dir" is kludge for semi-invalid filenames like "User [W2KRC1]"
	if (( strpbrk( dir, WILD_CHARS ) == NULL ) || ( is_dir( dir ))) {

		copy_filename( dirname, dir );

		// look for _CDPATH / CDPATH environment variable
		if ((( pszCDPATH = get_variable( CDPATH )) == 0L ) && (( pszCDPATH = get_variable( CDPATH + 1 )) == 0L ))
			pszCDPATH = NULLSTR;

		for ( ; ; ) {

			// expand the directory name to support things like "cd ..."
			if ( mkfname( dirname, (( fFlags & CD_NOERROR ) ? MKFNAME_NOERROR : 0 )) == NULL )
				return ERROR_EXIT;

			// remove trailing \ or / in syntax like "cd c:\123\wks\"
			if (( pszPtr = strchr( dirname, _TEXT(':') )) != NULL )
				pszPtr += 2;
			else
				pszPtr = dirname + 3;
			strip_trailing( pszPtr, SLASHES );

ChangeDirectory:
			// try to change to that directory

			if ( _chdir( dirname ) == 0 ) {

				if ( fFlags & CD_SAVELASTDIR )
					strcpy( szLastDirectory, pszSavedPath );

				if (( fFlags & CD_CHANGEDRIVE ) == 0 )
					return 0;

				// if "change drive" flag set, change to it

				if ( _chdrive( gcdisk( dirname )) != 0 ) 
					return (( fFlags & CD_NOERROR ) ? ERROR_EXIT : error( ERROR_INVALID_DRIVE, dirname ));

				// set the current disk (for Netware compatibility)
				gnCurrentDisk = gcdisk( dirname );

				return 0;

			} else if ( uPass != 0 ) 
				return (( fFlags & CD_NOERROR ) ? ERROR_EXIT : error( _doserrno, dirname ));

			// get next CDPATH arg, skipping '=', ';', ','
			sscanf_far( pszCDPATH, _TEXT(" %*[,;=]%n"), &nLength );
			pszCDPATH += nLength;

			// if the CD fails, check to see if we can try (more) CDPATH
			if (( dir[1] == _TEXT(':') ) || ( *dir == _TEXT('\\') ) || ( *dir == _TEXT('/') ) || ( *pszCDPATH == _TEXT('\0') )) {
				if ( gpIniptr->FuzzyCD != 0 )
					break;
				return (( fFlags & CD_NOERROR ) ? ERROR_EXIT : error( _doserrno, dir ));
			}

			// get new directory to try
			sscanf_far( pszCDPATH, _TEXT(" %255[^;=,]%n"), dirname, &nLength );
			pszCDPATH += nLength;

			// append the original argument to the new pathname
			mkdirname( dirname, dir );
		}
	}

	// try wildcard matching
	// if FuzzyCD == 0, don't build or search JPSTREE.IDX
	if (( gpIniptr->FuzzyCD == 0 ) || ( fFlags & CD_NOFUZZY ))
		return (( fFlags & CD_NOERROR ) ? ERROR_EXIT : error( ERROR_PATH_NOT_FOUND, dir ));

	szDrive[0] = _TEXT('\0');

	StripQuotes( dir );

	// remove trailing \ or /
	strip_trailing( dir + 1, SLASHES );

	// if CD * , display current drive only
	if (( stricmp( dir, _TEXT("*") ) == 0 ) && (( fFlags & CD_CHANGEDRIVE ) == 0 )) {

		if ((( pszPtr = gcdir( NULL, 1 )) != NULL ) && ( pszPtr[1] == _TEXT(':') ))
			sprintf( szDrive, _TEXT("%.2s"), pszPtr );

		// if "d:*", only display dirs for that drive
	} else if (( pszPtr = path_part( dir )) != NULL ) {

		// save & remove drive spec
		if ( pszPtr[1] == _TEXT(':') ) {
			sprintf( szDrive, _TEXT("%.2s"), pszPtr );
			strcpy( pszPtr, pszPtr + 2 );
			strcpy( dir, dir + 2 );
		}
	}

	copy_filename( dirname, dir );
	pszPtr = (LPTSTR)_alloca( ( strlen( dirname ) + 3 ) * sizeof(TCHAR) );
	dir = strcpy( pszPtr, dirname );

	// get tree index location
	GetFuzzyDir( dirname );

	// build JPSTREE.IDX if it doesn't exist
	if ( is_file( dirname ) == 0 ) {
		if ( command( _TEXT("cdd /s"), 0 ) == CTRLC )
			return CTRLC;
	}

	if (( nFH = _sopen( dirname, _O_RDONLY | _O_BINARY, _SH_DENYWR )) < 0 )
		return 0;

	uMatches = 0xFFF0L;
	lpszListEntry = lpszStartList = (TCHAR _far *)AllocMem( &uMatches );
	ulListSize = uMatches - 0xFF;

	if ( setjmp( cv.env ) == -1 ) {
		nReturn = CTRLC;
		_close( nFH );
		goto ExitCD;
	}

	//  Read list array looking for match.  If we have more than
	//   one match, call wPopSelect

	fWildAll = ( stricmp( dir, _TEXT("*") ) == 0 );

	pszPath = path_part( dir );

	for ( i = uMatches = 0; ; uPass++ ) {

		// kludge to allow wildcard search even if FuzzyCD == 0
		if ( uPass >= gpIniptr->FuzzyCD ) {
			if ( uPass == 0 ) {
				if ( strpbrk( dir, WILD_CHARS ) == NULL )
					break;
			} else
				break;
		}

		// on first pass, only look for exact matches
		// on second pass, append a "*"; on third pass prefix a "*"
		if ( uPass == 1 ) {

			// if we already got an exact match, quit now
			if (( i > 0 ) && ( strpbrk( dir, WILD_CHARS ) == NULL ))
				break;
			strcat( dir, _TEXT("*") );

			// if "*dir*" match requested, skip the second pass and go
			//   straight to the third pass
			if ( gpIniptr->FuzzyCD == 3 )
				continue;

		} else if ( uPass == 2 ) {
			if ( *dir == _TEXT('*') )
				break;
			strins( dir, _TEXT("*") );
		}

		// rewind the file
		RewindFile( nFH );

		while ( getline( nFH, szDirectory, MAXPATH, nEditFlags ) > 0 ) {

			if ( szDirectory[0] == _TEXT('\0') )
				continue;

			// check for "d:*" syntax
			if (( szDrive[0] ) && ( _ctoupper( szDrive[0] ) != _ctoupper( szDirectory[0] ) ))
				continue;

			sscanf( szDirectory, _TEXT("%[^\004]"), dirname );
			if ( fWildAll == 0 ) {

				if ( pszPath != NULL ) {
					if ( wild_cmp( dir, ((( *pszPath == _TEXT('\\') ) || ( *pszPath == _TEXT('/') )) ? dirname + 2 : dirname + 3 ), 2, TRUE ) != 0 )
						continue;
				} else if ( wild_cmp( dir, fname_part( dirname ), 2, TRUE ) != 0 )
					continue;
			}

			// allocate memory for 64 entries at a time
			if (( uMatches % 64 ) == 0 ) {

				ulSize += 256;

				lppList = (TCHAR _far * _far *)ReallocMem( lppList, ulSize );
			}

			uMatches++;

			nLength = strlen( szDirectory ) + 1;

			if (( lppList == 0L ) || ((((unsigned int)( lpszListEntry - lpszStartList ) + nLength ) * sizeof(TCHAR) ) >= ulListSize )) {

				TCHAR _far *lpszTemp;

				ulListSize += 0x8000;
				if (( lpszTemp = (TCHAR _far *)ReallocMem( lpszStartList, ulListSize )) == 0L ) {
					_close( nFH );
					nReturn = error ( ERROR_NOT_ENOUGH_MEMORY, NULL );
					goto ExitCD;
				}

				if ( lpszTemp != lpszStartList ) {

					// adjust array if list was moved
					lpszListEntry = lpszStartList = lpszTemp;

					for ( n = 0; ( n < i ); n++ ) {
						lppList[ n ] = lpszListEntry;
						lpszListEntry += _fstrlen( lpszListEntry ) + 1;
						if ( *lpszListEntry == 4 )
							lpszListEntry += _fstrlen( lpszListEntry ) + 1;
					}
				}
			}

			lppList[ i ] = _fstrcpy( lpszListEntry, szDirectory );

			// check for SFN's appended following a ^D
			if (( pszCDPATH = _fstrchr( lpszListEntry, 4 )) != 0L )
				*pszCDPATH = _TEXT('\0');
			lpszListEntry += nLength;
			i++;
		}

		if ( uMatches != 0 )
			break;
	}

	_close( nFH );

	if (( i == 0 ) || ( uMatches == 0 )) {
		nReturn = (( fFlags & CD_NOERROR ) ? ERROR_EXIT : error( ERROR_PATH_NOT_FOUND, dir ));
		goto ExitCD;
	}

	dirname[0] = _TEXT('\0');
	*(++lpszListEntry) = _TEXT('\0');

	pszCDPATH = NULLSTR;
	if ( uMatches == 1 )
		_fstrcpy( dirname, lppList[0] );

	else {
		// call the popup window
		if (( lpszListEntry = wPopSelect( gpIniptr->CDDTop, gpIniptr->CDDLeft, gpIniptr->CDDHeight, gpIniptr->CDDWidth, lppList, i, 1, _TEXT(" Change Directory "), NULL, NULL, (( fWildAll ) ? 4 : 5 ) )) != 0L )
			_fstrcpy( dirname, lpszListEntry );
	}

	// free the pointer array & list memory
ExitCD:
	FreeMem( lpszStartList );
	FreeMem( lppList );

	// if we aborted wPopSelect with ^C, bomb after cleanup
	if ( cv.fException )
		return CTRLC;

	if (( nReturn == 0 ) && ( dirname[0] )) {
		uPass = 3;
		goto ChangeDirectory;
	}

	return nReturn;
}


// save the directory in the specified dir stack
void _fastcall SaveDirectory( TCHAR _far *lpszDirectoryStack, LPTSTR pszDirectory )
{
	register int nLength;
	int uSize;
	TCHAR _far *pszNewEntry, _far *pszDup;

	if ( lpszDirectoryStack == glpDirHistory )
		uSize = gpIniptr->DirHistorySize;
	else
		uSize = DIRSTACKSIZE;

	nLength = ( strlen( pszDirectory ) + 1 );

	// if CD / CDD history, remove duplicates
	if (( pszDup = lpszDirectoryStack ) == glpDirHistory ) {

		TCHAR _far *lpNext;

		while ( *pszDup != _TEXT('\0') ) {

			lpNext = next_env( pszDup );
			if ( _fstricmp( pszDup, pszDirectory ) == 0 ) {
				// remove duplicate
				_fmemmove( pszDup, lpNext, (int)(( end_of_env( pszDup ) - lpNext ) + 1 ) * sizeof(TCHAR) );
			} else
				pszDup = lpNext;
		}
	}

	// if directory queue overflow, remove the oldest entry
	while (((( pszNewEntry = end_of_env( lpszDirectoryStack )) - lpszDirectoryStack ) + nLength ) >= ( uSize - 2 ))
		_fmemmove( lpszDirectoryStack, next_env( lpszDirectoryStack ), (int)(( pszNewEntry - next_env( lpszDirectoryStack )) + 1 ) * sizeof(TCHAR) );

	_fstrcpy( pszNewEntry, pszDirectory );
	pszNewEntry[ nLength ] = _TEXT('\0');
}


int _fastcall MakeDirectory( LPTSTR pszDirectory, int fUpdateFuzzy )
{
	if ( _mkdir( pszDirectory ) == -1 )
		return -1;

	if ( fUpdateFuzzy ) {
		// delete the subdirectory from JPSTREE.IDX (if it exists), then add it again
		UpdateFuzzyTree( pszDirectory, NULL, 1 );
		UpdateFuzzyTree( pszDirectory, NULL, 0 );
	}

	return 0;
}


int _fastcall DestroyDirectory( LPTSTR pszDirectory )
{
	if ( _rmdir( pszDirectory ) == -1 )
		return -1;

	UpdateFuzzyTree( pszDirectory, NULL, 1 );

	return 0;
}


// create directory
int _near Md_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0;
	unsigned int nArg, fUpdateFuzzy;
	long fMD;
	LPTSTR pszDirectory;
	LPTSTR pszPath;

	// check for and remove switches; abort if no directory name arguments
	if (( GetSwitches( pszCmdLine, _TEXT("SN"), &fMD, 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( MD_USAGE ));
	fUpdateFuzzy = (( fMD & 2 ) == 0 );


	for ( nArg = 0; (( pszDirectory = ntharg( pszCmdLine, nArg | 0x800 )) != NULL ); nArg++) {

		mkfname( pszDirectory, 0 );

		// remove trailing '/' or '\'
		strip_trailing( pszDirectory+3, SLASHES );

		// create directory tree if /S specified
		if ( fMD & 1 ) {

			// skip UNC name
			if ( pszDirectory[0] == _TEXT('\\') ) {
				for ( pszPath = pszDirectory + 2; (( *pszPath != _TEXT('\0') ) && ( *pszPath++ != _TEXT('\\') )); )
					;
			} else
				pszPath = pszDirectory + 3;

			for ( _doserrno = 0; ( *pszPath != _TEXT('\0') ); pszPath++ ) {

				if (( *pszPath == _TEXT('\\') ) || ( *pszPath == _TEXT('/') )) {
					*pszPath = _TEXT('\0');

					if (( MakeDirectory( pszDirectory, fUpdateFuzzy ) == -1 ) && ( _doserrno != ERROR_ACCESS_DENIED ))

						break;
					*pszPath = _TEXT('\\');
				}
			}
		}

		if ( MakeDirectory( pszDirectory, fUpdateFuzzy ) == -1 )
			nReturn = error( _doserrno, pszDirectory );
	}

	return nReturn;
}


// save the current directory & change to new one
int _near Pushd_Cmd( LPTSTR pszCmdLine )
{
	LPTSTR pszCurrentDirectory;
	TCHAR szDirectory[MAXPATH];

	if (( pszCurrentDirectory = gcdir( NULL, 0 )) == NULL )	// get current directory
		return ERROR_EXIT;

	strcpy( szDirectory, pszCurrentDirectory );

	// change disk and/or directory
	//   (if no arguments, just save the current directory)
	if (( pszCmdLine ) && ( *pszCmdLine ) && ( __cd( pszCmdLine, CD_CHANGEDRIVE | CD_SAVELASTDIR ) != 0 ))
		return ERROR_EXIT;

	// save the previous disk & directory
	SaveDirectory( gaPushdQueue, szDirectory );

	return 0;
}


// POP a PUSHD directory
int _near Popd_Cmd( LPTSTR pszCmdLine )
{
	LPTSTR pszDirectory;
	register int nRet;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if ( *pszCmdLine == _TEXT('*') ) {
			// clear directory stack
			gaPushdQueue[0] = _TEXT('\0');
			gaPushdQueue[1] = _TEXT('\0');
			return 0;
		}

		return ( Usage( POPD_USAGE ));
	}

	if ( gaPushdQueue[0] == _TEXT('\0') )
		return ( error( ERROR_4DOS_DIR_STACK_EMPTY, NULL ));

	// get last (newest) entry in queue
	for ( pszDirectory = (char *)( end_of_env( gaPushdQueue ) - 1 ); (( pszDirectory > gaPushdQueue ) && ( pszDirectory[-1] != _TEXT('\0') )); pszDirectory-- )
		;

	// change drive and directory
	nRet = __cd( pszDirectory, CD_CHANGEDRIVE | CD_SAVELASTDIR );

	// remove most recent entry from DIRSTACK
	pszDirectory[0] = _TEXT('\0');
	pszDirectory[1] = _TEXT('\0');

	return nRet;
}


// Display the directory stack created by PUSHD
int _near Dirs_Cmd( LPTSTR pszCmdLine )
{
	TCHAR _far *pszDirectory;

	if ( gaPushdQueue[0] == _TEXT('\0') )
		return (error( ERROR_4DOS_DIR_STACK_EMPTY, NULL ));

	for ( pszDirectory = gaPushdQueue; ( *pszDirectory != _TEXT('\0') ); pszDirectory = next_env( pszDirectory ))
		printf( FMT_FAR_STR_CRLF, pszDirectory );

	return 0;
}


// remove directory
int _near Rd_Cmd( LPTSTR pszCmdLine )
{
	register int nFind;
	unsigned int nArg, nReturn;
	TCHAR szSource[MAXFILENAME+1], *pszDirectory;
	FILESEARCH dir;

	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	if ( first_arg( pszCmdLine ) == NULL )
		return ( Usage( RD_USAGE ));

	dir.hdir = INVALID_HANDLE_VALUE;
	if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
		FindClose( dir.hdir );
		return CTRLC;
	}

	for ( nReturn = nArg = 0; (( pszDirectory = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		// expand filename & remove any trailing backslash
		mkfname( pszDirectory, 0 );
		strip_trailing( pszDirectory+3, SLASHES );

		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			if ( find_file( nFind, pszDirectory, ( 0x10L | FIND_ONLY_DIRS | FIND_RANGE | FIND_EXCLUDE ), &dir, szSource ) == NULL )
				break;

			if ( DestroyDirectory( szSource ) != 0 )
				nReturn = error( _doserrno, szSource );

		}

		// check for directory not found!
		if ( nFind == FIND_FIRST )
			nReturn = ERROR_EXIT;

		// remove descriptions from parent directory
		process_descriptions( NULL, pszDirectory, DESCRIPTION_REMOVE );
	}

	return nReturn;
}


// attach descriptive labels to filenames
int _near Describe_Cmd( LPTSTR pszCmdLine )
{
	register int nFind;
	register LPTSTR pszArg;
	int fHaveDescription, rc, nReturn = 0, nArg;
	unsigned int uMaxLength;
	TCHAR szSource[MAXFILENAME], szFileName[MAXFILENAME], szNewDescription[514];
	long fDescribe;
	FILESEARCH dir;

	// get file date/time ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove /A:-rhsda switch
	if (( GetSwitches( pszCmdLine, _TEXT("*"), &fDescribe, 1 ) != 0 ) || ( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( DESCRIBE_USAGE ));

	dir.hdir = INVALID_HANDLE_VALUE;
	if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
		FindClose( dir.hdir );
		return CTRLC;
	}

	for ( nArg = 0; (( pszArg = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		copy_filename( szSource, pszArg );

		// get file description
		szNewDescription[0] = _TEXT('\0');
		fHaveDescription = 0;
		for ( rc = nArg; (( pszArg = ntharg( pszCmdLine, rc )) != NULL ); rc++ ) {

			if ( rc > 0 ) {

				// is this argument predefined as a description?
				if (( strnicmp( pszArg, _TEXT("/D"), 2 ) == 0 ) && ( pszArg[2] == DOUBLE_QUOTE )) {
					pszArg += 2;
					goto HaveDescription;
				}

				// make sure it's not an NTFS/LFN filename
				if (( *pszArg == DOUBLE_QUOTE ) && (( ifs_type( pszArg ) == 0 ) || (( is_file( pszArg ) == 0 ) && ( is_dir( pszArg ) == 0 )))) {
HaveDescription:
					fHaveDescription = sscanf( pszArg + 1, _TEXT("%511[^\"]"), szNewDescription );
					break;
				}
			}
		}

		// if the current argument is a description, skip it
		if ( rc == nArg )
			continue;

		for ( nFind = FIND_FIRST; ( find_file( nFind, szSource, (0x17L | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szFileName ) != NULL ); nFind = FIND_NEXT ) {

			mkfname( szFileName, 0 );

			// display current description & get new one
			if ( fHaveDescription == 0 ) {

				szNewDescription[0] = _TEXT('\0');
				process_descriptions( szFileName, szNewDescription, DESCRIPTION_READ | DESCRIPTION_PROCESS );
				if ( _isatty(STDIN))
					printf( DESCRIBE_PROMPT, szFileName );

				// if input is redirected, exit if EoF
				if (( uMaxLength = strlen( szNewDescription )) < gpIniptr->DescriptMax )
					uMaxLength = gpIniptr->DescriptMax;
				else if ( uMaxLength > 511 )
					uMaxLength = 511;

				if ( getline( STDIN, szNewDescription, uMaxLength, EDIT_ECHO | EDIT_SCROLL_CONSOLE ) == 0 ) {
					if ( _isatty( STDIN ) == 0 )
						return nReturn;
				}
			}

			// write the new description
			if (( rc = process_descriptions( szFileName, szNewDescription, ( DESCRIPTION_WRITE | DESCRIPTION_CREATE | DESCRIPTION_REMOVE | DESCRIPTION_PROCESS ))) != 0 )
				nReturn = error( rc, DESCRIPTION_FILE );
		}
	}

	return nReturn;
}


// return the current volume name(s)
int _near Volume_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0, nArg;
	register LPTSTR pszDrive;

	// if no arguments, return the label for the default disk
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {
		pszCmdLine = gcdir( NULL, 0 );
		AddQuotes( pszCmdLine );
	}

	for ( nArg = 0; (( pszDrive = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		StripQuotes( pszDrive );
		if (( pszDrive[0] ) && ( pszDrive[1] == _TEXT('\0') ))
			strcat( pszDrive, _TEXT(":") );
		if ( getlabel( pszDrive ) != 0 )
			nReturn = ERROR_EXIT;
		else
			crlf();
	}

	return nReturn;
}


// read a disk label
int _fastcall getlabel( register LPTSTR lpszVolume )
{
	TCHAR szVolumeName[MAXFILENAME];
	unsigned long ulSerialNumber = 0L;

	// first check for a valid drive name

	if ( lpszVolume[1] != _TEXT(':') )
		return ( error( ERROR_INVALID_DRIVE, lpszVolume ));

	// read the label
	szVolumeName[0] = _TEXT('\0');
	if ( QueryVolumeInfo( lpszVolume, szVolumeName, &ulSerialNumber ) == NULL )
		return ERROR_EXIT;

	printf( VOLUME_LABEL, _ctoupper( *lpszVolume ), szVolumeName );

	// display the disk serial number
	if ( ulSerialNumber )
		printf( VOLUME_SERIAL, ( ulSerialNumber >> 16 ), ( ulSerialNumber & 0xFFFF ));

	return 0;
}


// return or set the path in the environment
int _near Path_Cmd( LPTSTR pszCmdLine )
{
	TCHAR _far *lpszEnvVar;

	// if no args, display the current PATH
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {
		printf( FMT_FAR_STR_CRLF, (( lpszEnvVar = get_variable( PATH_VAR )) == 0L ) ? (TCHAR _far *)NO_PATH : lpszEnvVar - 5 );
		return 0;
	}

	strip_leading( pszCmdLine, _TEXT(" \t=") );
	strins( pszCmdLine, _TEXT("=") );
	strins( pszCmdLine, PATH_VAR );

	// add argument to environment (in upper case for Netware bug)
	return ( add_variable( strupr( pszCmdLine )));
}


// set the command-line prompt
int _near Prompt_Cmd( LPTSTR pszCmdLine )
{
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		pszCmdLine = PROMPT_NAME;
	else {
		strip_leading( pszCmdLine, _TEXT(" \t=") );
		strins( pszCmdLine, _TEXT("=") );
		strins( pszCmdLine, PROMPT_NAME );
	}

	return ( add_variable( pszCmdLine ));
}


// Set the screen colors (using ANSI or the BIOS)
int _near Color_Cmd( LPTSTR pszCmdLine )
{
	int nColors;

	// scan the input line for the colors requested
	if (( nColors = GetColors( pszCmdLine, 1 )) < 0 )
		return ( Usage( COLOR_USAGE ));

	set_colors( nColors );

	return 0;
}


// display the disk status (total size, total used, & total free)
int _near Free_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0, nArg;
	LPTSTR pszDrive;
	QDISKINFO DiskInfo;
	TCHAR szBuf[128];

	// if no argument, return the default disk
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {
		if (( pszCmdLine = gcdir( NULL, 0 )) == NULL )
			return ERROR_EXIT;
		AddQuotes( pszCmdLine );
		pszCmdLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );
	}

	for ( nArg = 0; (( pszDrive = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		StripQuotes( pszDrive );

		if ( strnicmp( pszDrive, _TEXT("\\\\"), 2 ) != 0 ) {

			crlf();
			if ( getlabel( pszDrive )) {
				nReturn = ERROR_EXIT;
				continue;
			}
		}
		crlf();

		// get the total & free space
		if ( QueryDiskInfo( pszDrive, &DiskInfo, 0 ) == 0 ) {

			// kludge for > 4Gb drives
			sprintf( szBuf, _TEXT("%s-%s"), DiskInfo.szBytesTotal, DiskInfo.szBytesFree );
			evaluate( szBuf );
			AddCommas( szBuf );
			printf( TOTAL_DISK_USED, DiskInfo.szBytesTotal, szBuf );
			printf( BYTES_FREE, DiskInfo.szBytesFree );
			sprintf( szBuf, "((%s-%s) / %s)*100=.1n", DiskInfo.szBytesTotal, DiskInfo.szBytesFree, DiskInfo.szBytesTotal );

			// some CD's return 0 for total bytes!
			if ( DiskInfo.BytesTotal != 0L ) {
				evaluate( szBuf );
				printf( DISK_PERCENT, szBuf );
			}

		} else
			nReturn = ERROR_EXIT;
	}

	return nReturn;
}


// Display/Change the 4xxx / TCMD variables
int _near Setdos_Cmd( LPTSTR pszCmdLine )
{
	extern int fNoComma;

	register LPTSTR pszArg;
	register int fDisable = 0, i, nArg;

	// display current default parameters
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {

		TCHAR szBuf[10];

		// convert /X into readable form
		if ( gpIniptr->Expansion == 0 )
			strcpy( szBuf, _TEXT("0") );
		else {

			szBuf[0] = _TEXT('\0');
			for ( nArg = 0; ( nArg < 8 ); nArg++ ) {
				if ( gpIniptr->Expansion & (char)( 1 << nArg ))
					IntToAscii( nArg + 1, strend( szBuf ));
			}
		}

		printf( SETDOS_IS,

			gpIniptr->ANSI,
			gpIniptr->BrightBG,
			gpIniptr->CmdSep, gpIniptr->Descriptions, DESCRIPTION_FILE,
			gpIniptr->EscChr, gpIniptr->EvalMin,
			gaCountryInfo.szDecimal[0], gpIniptr->EvalMax,
			szBuf, gpIniptr->LineIn,
			gpIniptr->EditMode, gpIniptr->NoClobber,
			gpIniptr->ParamChr, (( gpIniptr->Rows == 0 ) ? GetScrRows() + 1 : gpIniptr->Rows),
			gpIniptr->CursO, gpIniptr->CursI, gpIniptr->Upper,
			gpIniptr->BatEcho, gpIniptr->SwChr, gpIniptr->SingleStep);


	} else {

		fNoComma = 1;
		for ( nArg = 0; (( pszArg = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

			if (( *pszArg != gpIniptr->SwChr ) || ( pszArg[1] == _TEXT('\0') ) || ( pszArg[2] == _TEXT('\0') )) {
setdos_error:
				fNoComma = 0;
				return ( error( ERROR_INVALID_PARAMETER, pszArg ));
			}

			// point past switch character
			pszArg++;

			switch ( _ctoupper( *pszArg++ )) {

			case 'A':	// A - ANSI state recognition
				gpIniptr->ANSI = (char)atoi( pszArg );
				break;

			case 'B':	// B - Bright/Blink background
				gpIniptr->BrightBG = (char)atoi( pszArg );
				SetBrightBG();
				break;

			case 'C':	// C - compound command separator
				if (( isalnum( *pszArg )) || ( *pszArg > 255 ))
					goto setdos_error;
				gpIniptr->CmdSep = *pszArg;
				break;

			case 'D':	// D - description enable/disable
				if (( *pszArg == _TEXT('0') ) || ( *pszArg == _TEXT('1') ))
					gpIniptr->Descriptions = (char)atoi( pszArg );

				else if (*pszArg == _TEXT('"') ) {
					// set the description file name
					sscanf( pszArg + 1, _TEXT("%63[^\"]"), DESCRIPTION_FILE );
				}
				break;

			case 'E':	// E - escape character
				if (( isalnum( *pszArg )) || ( *pszArg > 255 ))
					goto setdos_error;
				gpIniptr->EscChr = *pszArg;
				break;

			case 'F':	// set EVAL precision
				SetEvalPrecision( pszArg, &(gpIniptr->EvalMin), &(gpIniptr->EvalMax) );
				break;

			case 'G':	// set decimal & thousands characters
				if ( isalnum( *pszArg ))
					gpIniptr->DecimalChar = gpIniptr->ThousandsChar = 0;
				else {
					TCHAR cNewDecimal, cNewThousands;
					sscanf( pszArg, _TEXT("%c%c"), &cNewDecimal, &cNewThousands);
					gpIniptr->DecimalChar = ((cNewDecimal == _TEXT('.')) ? 1 : 2);
					gpIniptr->ThousandsChar = ((cNewThousands == _TEXT('.')) ? 1 : 2);
				}
				QueryCountryInfo();
				break;

			case 'I':	// I - disable or enable internal cmds
				if ( *pszArg == _TEXT('*') ) {

					// enable all commands
					nArg = QueryNumCmds();

					for ( i = 0; ( i < nArg ); i++ )
						commands[i].fParse &= ~CMD_DISABLED;
					break;
				}

				if (( nArg = findcmd( pszArg + 1, 1 )) < 0 )
					return ( error( ERROR_4DOS_UNKNOWN_COMMAND, pszArg + 1 ));

				if ( *pszArg == _TEXT('-') ) {
					// don't allow SETDOS itself to be disabled in runtime
					// version (protects against certain hacks)
					if (( fRuntime == 0 ) || ( _stricmp( pszArg+1, _TEXT("SETDOS") ) != 0 ))
						commands[nArg].fParse |= CMD_DISABLED;
				} else
					commands[nArg].fParse &= ~CMD_DISABLED;
				break;

			case 'L':
				// L - if 1, use INT 21 0Ah for input
				gpIniptr->LineIn = (char)atoi( pszArg );
				break;

			case 'M':
				// M - edit mode (overstrike or insert)
				gpIniptr->EditMode = (char)atoi( pszArg );
				break;

			case 'N':	// N - noclobber (output redirection)
				gpIniptr->NoClobber = (char)atoi( pszArg );
				break;

			case 'P':	// P - parameter character
				if ( isalnum( *pszArg ))
					goto setdos_error;
				gpIniptr->ParamChr = *pszArg;
				break;

			case 'R':	// R - number of screen rows
				gpIniptr->Rows = atoi( pszArg );
				break;

			case 'S':	// S - cursor shape
				// if CursO or CursI == -1, don't attempt
				//   to modify cursor at all
				sscanf( pszArg, _TEXT("%d%*1s%d"), &(gpIniptr->CursO), &(gpIniptr->CursI) );

				// force cursor shape change (for BAT files)
				SetCurSize( );
				break;

			case 'U':	// U - upper case filename display
				gpIniptr->Upper = (char)atoi( pszArg );
				break;

			case 'V':	// V - batch command echoing
				// don't allow SETDOS /V2 in runtime version (protects
				// against certain hacks)
				i = atoi( pszArg );

				if (( fRuntime == 0 ) || ( i < 2 ))
					gpIniptr->BatEcho = (char)i;
				break;

			case 'W':	// W - change switch char
				if ( isalnum( *pszArg ))
					goto setdos_error;

				// set the switch character
				_asm {
					mov	ax, 3701h
						xor	dh, dh
						mov	dl, byte ptr [pszArg]
						int	21h
				}
				gpIniptr->SwChr = (TCHAR)QuerySwitchChar();
				break;

			case 'X':	// X - enable/disable expansion
				//  bit on = disable feature
				//  bit 0=aliases, 1=nested aliases
				//    2=vars, 3=nested vars
				//    4=compounds, 5=redirection
				//    6=quotes, 7=escapes, 8=user functions

				for ( ; ( *pszArg != _TEXT('\0') ); pszArg++ ) {

					if ( is_signed_digit( *pszArg ) == 0 )
						goto setdos_error;

					if ( *pszArg == _TEXT('-') )
						fDisable++;
					else if ( *pszArg == _TEXT('+') )
						fDisable = 0;
					else {

						if (( i = ( *pszArg - _TEXT('0') )) == 0 )
							gpIniptr->Expansion = 0;
						else if ( fDisable )
							gpIniptr->Expansion |= (char)(1 << (--i));
						else
							gpIniptr->Expansion &= ~((char)(1 << (--i)));
					}
				}
				break;

			case 'Y':	// batch single-stepping
				if (( cv.bn < 0 ) || (( bframe[cv.bn].nFlags & BATCH_COMPRESSED ) == 0 ))
					gpIniptr->SingleStep = (char)atoi( pszArg );
				break;

			default:
				goto setdos_error;
			}
		}
		fNoComma = 0;
	}

	return 0;
}


// enable/disable disk verify
int _near Verify_Cmd( LPTSTR pszCmdLine )
{
	register int nVerify;

	// inquiring about verify status
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		printf( VERIFY_IS, QueryVerifyWrite() ? ON : OFF );

	else {		// setting new verify status

		if (( nVerify = OffOn( pszCmdLine )) == -1 )
			return ( Usage( VERIFY_USAGE ));

		SetVerifyWrite( nVerify );
	}

	return 0;
}


LPTSTR _fastcall GetLogName( int fHistFlag )
{
	return (char *)( gpIniptr->StrData + (( fHistFlag ) ? gpIniptr->HistLogName : gpIniptr->LogName ));
}


#define LOG_ERRORS 1
#define LOG_HISTORY 2
#define LOG_NAME 4

// enable/disable command logging, or rename the log file
int _near Log_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszFileName;
	TCHAR *pszArg, szLogName[128];
	int nArg;
	long fLogFlags;

	// check for LOG switches /E(rrors) /H(istory), /W(rite file name)
	if ( GetSwitches( pszCmdLine, _TEXT("EHW"), &fLogFlags, 1 ) != 0 )
		return ( Usage( LOG_USAGE ));

	pszFileName = GetLogName( (int)( fLogFlags & LOG_HISTORY ));

	if ( fLogFlags & LOG_ERRORS )
		gpIniptr->LogErrors = 1;

	if (( pszArg = first_arg( pszCmdLine )) == NULL ) {

		// inquiring about log status
		nArg = (( fLogFlags & LOG_HISTORY ) ? gpIniptr->HistLogOn : gpIniptr->LogOn);
		printf( LOG_IS, pszFileName, ( nArg ? ON : OFF ));

	} else if ( fLogFlags & LOG_NAME ) {		// change log file name?

		strcpy( szLogName, pszArg );

		if (( QueryIsDevice( szLogName ) == 0 ) && ( mkfname( szLogName, 0 ) == NULL ))
			return ERROR_EXIT;

		if ( fLogFlags & LOG_HISTORY ) {
			gpIniptr->HistLogOn = 1;

			strcpy((char *)( gpIniptr->StrData + gpIniptr->HistLogName ), szLogName );

		} else {
			gpIniptr->LogOn = 1;

			strcpy( (char *)( gpIniptr->StrData + gpIniptr->LogName ), szLogName );
		}

	} else if (( nArg = OffOn( pszCmdLine )) == -1 ) {

		// must be entering a LOG header
		return ( _log_entry( pszCmdLine, (int)( fLogFlags & LOG_HISTORY ) ));

	} else {
		// set (Hist)LogOn = 1 if "ON", 0 if "OFF"
		if ( fLogFlags & LOG_HISTORY )
			gpIniptr->HistLogOn = (char)nArg;
		else {
			gpIniptr->LogOn = (char)nArg;
			if ( nArg == 0 )
				gpIniptr->LogErrors = 0;
		}
	}

	return 0;
}


// append the command to the log file
int _fastcall _log_entry( LPTSTR pszCmd, int fHistoryFlag )
{
	register LPTSTR pszFileName;
	int nFH;

	// get log filename
	pszFileName = GetLogName( fHistoryFlag );

	// We can't open it in DENYRW under DOS, because SHARE has bugs!
	if (( nFH = _sopen( pszFileName, (_O_WRONLY | _O_BINARY | _O_CREAT | _O_APPEND), _SH_DENYWR, (_S_IREAD | _S_IWRITE))) < 0 ) {

		if ( fHistoryFlag )
			gpIniptr->HistLogOn = 0;
		else {
			gpIniptr->LogOn = 0;
			gpIniptr->LogErrors = 0;
		}

		return ( error( _doserrno, pszFileName ));
	}

	HoldSignals();

	if ( fHistoryFlag == 0 ) {

		qprintf( nFH, "[%s %s][%d] ", gdate( 1 ), gtime( gaCountryInfo.fsTimeFmt ), gpIniptr->ShellNum );

	}

	qprintf( nFH, FMT_STR_CRLF, skipspace( pszCmd ));

	_close( nFH );
	EnableSignals();

	return 0;
}


// flags for global execution of a command
#define GLOBAL_HIDDEN 1
#define GLOBAL_IGNORE 2
#define GLOBAL_PROMPT 4
#define GLOBAL_QUIET 8


// perform a command on the current directory & its subdirectories
int _near Global_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszTail;
	TCHAR szSavedDirectory[MAXPATH], szCurrentDirectory[MAXPATH];
	long fGlobal;
	int nReturn;
	long lOffset = 0L;

	// save the original directory
	if (( pszTail = gcdir( NULL, 0 )) == NULL )
		return ERROR_EXIT;

	strcpy( szSavedDirectory, pszTail );
	strcpy( szCurrentDirectory, pszTail );

	// check for GLOBAL switches /H(idden), /I( gnore), /P(rompt) & /Q(uiet)
	if (( GetSwitches( pszCmdLine, _TEXT("HIPQ"), &fGlobal, 1 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( GLOBAL_USAGE ));

	// save command line & call global recursive routine
	pszTail = _strdup( pszCmdLine );

	// test for GOTO / CANCEL / QUIT
	if ( cv.bn >= 0 )
		lOffset = bframe[cv.bn].lOffset;

	nReturn = _global( fGlobal, (LONG)lOffset, szCurrentDirectory, pszTail );

	if ( setjmp( cv.env ) == -1 )
		nReturn = CTRLC;

	free( pszTail );

	// restore original directory
	_chdir( szSavedDirectory );

	return nReturn;
}


// execute the command in the current directory & all its subdirectories
static int _global( unsigned long lFlags, long lOffset, register LPTSTR pszCurrentDirectory, LPTSTR pszLine )
{
	register LPTSTR pszDirectoryPath;
	int i, nFind, nReturn;
	FILESEARCH dir;
	unsigned long ulMode = ( 0x10 | FIND_ONLY_DIRS | FIND_NO_ERRORS );

	dir.hdir = INVALID_HANDLE_VALUE;

	// retrieve hidden directories if /H
	if ( lFlags & GLOBAL_HIDDEN )
		ulMode |= 0x07;

	// change to the specified directory
	if ( _chdir( pszCurrentDirectory ) == -1 )
		return (( lFlags & GLOBAL_IGNORE ) ? 0 : error( _doserrno, pszCurrentDirectory ));

	// display subdir name unless quiet switch set
	if ((( lFlags & GLOBAL_QUIET ) == 0 ) || ( lFlags & GLOBAL_PROMPT)) {

		printf( GLOBAL_DIR, pszCurrentDirectory );

		// prompt whether to process this directory
		if ( lFlags & GLOBAL_PROMPT) {

			if ((( i = QueryInputChar( NULLSTR, YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
				lFlags &= ~GLOBAL_PROMPT;
			else if ( i == NO_CHAR )
				goto next_global_dir;
			else if ( i == ESCAPE )
				return ERROR_EXIT;
		} else
			crlf();
	}

	// execute the command; if a non-zero result, test for /I( gnore) option
	nReturn = command( pszLine, 0 );

	if (( cv.fException ) || (( nReturn != 0 ) && (( lFlags & GLOBAL_IGNORE ) == 0 )))
		return nReturn;

next_global_dir:

	CheckFreeStack( 768 );

	// check for a CANCEL/QUIT during the GLOBAL
	if (( cv.bn >= 0 ) && ( bframe[cv.bn].lOffset != lOffset ))
		return 0;

	// make new directory search name
	mkdirname( pszCurrentDirectory, WILD_FILE );

	// save the current subdirectory start
	pszDirectoryPath = strchr( pszCurrentDirectory, _TEXT('*') );

	// search for all subdirectories in this (sub)directory
	for ( nReturn = 0, nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			if ( dir.hdir != INVALID_HANDLE_VALUE )
				FindClose( dir.hdir );
			nReturn = CTRLC;
			break;
		}

		if ( find_file( nFind, pszCurrentDirectory, ulMode, &dir, NULL ) == NULL )
			break;

		// make the new "pszCurrentDirectory"
		strcpy( pszDirectoryPath, dir.szFileName );

		// process directory tree recursively
		if (( nReturn = _global( lFlags, lOffset, pszCurrentDirectory, pszLine )) != 0 )
			break;
	}

	return nReturn;
}


// "except" the specified files from a command by changing their attributes
//    to hidden, running the command, and then unhiding them
int _near Except_Cmd( LPTSTR pszCmdLine )
{
	register TCHAR *pszArg, *pszExceptList;
	TCHAR szFileName[MAXFILENAME], *pszCommandList;
	int nReturn = 0, nFind, nArg;
	unsigned int uAttribute;
	FILESEARCH dir;

	// initialize date/time/size ranges
	GetRange( pszCmdLine, &(dir.aRanges), 1 );

	// preserve file exception list
	if ( dir.aRanges.pszExclude != NULL ) {
		pszArg = (LPTSTR)_alloca( ( strlen( dir.aRanges.pszExclude ) + 1 ) * sizeof(TCHAR) );
		dir.aRanges.pszExclude = strcpy( pszArg, dir.aRanges.pszExclude );
	}

	if (( pszCmdLine == NULL ) || ( *pszCmdLine != _TEXT('(')) || (( pszArg = strchr( pszCmdLine, _TEXT(')') )) == NULL ) || ( *(pszCommandList = skipspace( pszArg + 1 )) == _TEXT('\0') ))
		return ( Usage( EXCEPT_USAGE ));

	// terminate at the close paren ' )'
	*pszArg = _TEXT('\0');

	// save the command onto the stack
	pszCommandList = strcpy( (LPTSTR)_alloca( ( strlen( pszCommandList ) + 1 ) * sizeof(TCHAR) ), pszCommandList );

	// do variable expansion on the exception list
	if ( var_expand( strcpy( gszCmdline, pszCmdLine + 1 ), 1 ) != 0 )
		return ERROR_EXIT;

	// save the exception list onto the stack
	pszArg = (LPTSTR)_alloca( ( strlen( gszCmdline ) + 1 ) * sizeof(TCHAR) );
	pszExceptList = strcpy( pszArg, gszCmdline );

	dir.hdir = INVALID_HANDLE_VALUE;
	if ( setjmp( cv.env ) == -1 ) {
		nReturn = CTRLC;
		FindClose( dir.hdir );

	} else {

		// hide away the "excepted" files
		for ( nArg = 0; (( nReturn == 0 ) && (( pszArg = ntharg( pszExceptList, nArg )) != NULL )); nArg++) {

			for ( nFind = FIND_FIRST; ( find_file( nFind, pszArg, ( 0x10L | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szFileName ) != NULL ); nFind = FIND_NEXT ) {

				if (( nReturn = QueryFileMode( szFileName, &uAttribute )) == 0 ) {
					// can't set directory attribute!
					uAttribute &= ( _A_SUBDIR ^ 0xFFFF);
					nReturn = SetFileMode( szFileName, ( uAttribute | _A_HIDDEN) );
				}

				if ( nReturn != 0 ) {
					error( nReturn, szFileName );
					break;
				}
			}
		}

		if ( nReturn == 0 )
			nReturn = command( pszCommandList, 0 );
		if ( setjmp( cv.env ) == -1 )
			nReturn = CTRLC;
	}

	for ( nArg = 0; (( pszArg = ntharg( pszExceptList, nArg )) != NULL ); nArg++ ) {

		// unhide all of the "excepted" files
		for ( nFind = FIND_FIRST; ( find_file( nFind, pszArg, ( 0x17L | FIND_NO_ERRORS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szFileName ) != NULL ); nFind = FIND_NEXT ) {

			if ( QueryFileMode( szFileName, &uAttribute ) == 0 ) {
				// can't set directory attribute!
				uAttribute &= ( _A_SUBDIR ^ 0xFFFF );
				SetFileMode( szFileName, ( uAttribute & ( _A_HIDDEN ^ 0xFF)));
			}
		}
	}

	return nReturn;
}


#define TIMER_1 1
#define TIMER_2 2
#define TIMER_3 4
#define TIMER_SPLIT_TIME  8

// Stopwatch - display # of seconds between calls
int _near Timer_Cmd( LPTSTR pszCmdLine )
{
	register int nTimerNumber = 0;		// timer # (1, 2, or 3 - base 0 )
	long fTimer;
	TCHAR szBuffer[16];

	// get the timer number; default to /1 if none specified
	if ( GetSwitches( pszCmdLine, _TEXT("123S"), &fTimer, 0 ) != 0 )
		return ( Usage( TIMER_USAGE ));

	if ( fTimer & TIMER_2 )
		nTimerNumber = 1;
	else if ( fTimer & TIMER_3 )
		nTimerNumber = 2;

	printf( TIMER_NUMBER, nTimerNumber + 1 );

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;
	else
		pszCmdLine = skipspace( pszCmdLine );

	strip_trailing( pszCmdLine, WHITESPACE );

	if ((( gaTimers[nTimerNumber].fTimer == 0 ) || ( _stricmp( pszCmdLine, ON ) == 0 )) && ( _stricmp( pszCmdLine, OFF ) != 0 )) {

		DATETIME sysDateTime;

		QueryDateTime( &sysDateTime );

		printf( TIMER_ON, gtime( gaCountryInfo.fsTimeFmt ));

		// start timer - save current time
		gaTimers[nTimerNumber].fTimer = 1;
		gaTimers[nTimerNumber].uTHours = sysDateTime.hours;
		gaTimers[nTimerNumber].uTMinutes = sysDateTime.minutes;
		gaTimers[nTimerNumber].uTSeconds = sysDateTime.seconds;
		gaTimers[nTimerNumber].uTHundreds = sysDateTime.hundredths;

		MakeDaysFromDate( &(gaTimers[nTimerNumber].lDay), NULLSTR );

	} else {		// timer toggled off

		// check for split time; turn off timer if not
		if (( fTimer & TIMER_SPLIT_TIME ) == 0 )
			printf( TIMER_OFF, gtime( gaCountryInfo.fsTimeFmt ));

		_timer( nTimerNumber, szBuffer );
		printf( TIMER_ELAPSED, szBuffer );

		if (( fTimer & TIMER_SPLIT_TIME ) == 0 )
			gaTimers[nTimerNumber].fTimer = 0;
	}

	return 0;
}


void _fastcall _timer( int nTimerNumber, LPTSTR pszBuffer )
{
	int nHours = 0, nMinutes = 0, nSeconds = 0, nHundreds = 0;
	DATETIME sysDateTime;
	LONG lSplitDay;

	// save the split time
	if ( gaTimers[nTimerNumber].fTimer != 0 ) {

		QueryDateTime( &sysDateTime );

		nHours = sysDateTime.hours - gaTimers[nTimerNumber].uTHours;
		nMinutes = sysDateTime.minutes - gaTimers[nTimerNumber].uTMinutes;
		nSeconds = sysDateTime.seconds - gaTimers[nTimerNumber].uTSeconds;
		nHundreds = sysDateTime.hundredths - gaTimers[nTimerNumber].uTHundreds;

		// adjust negative fractional times
		if ( nHundreds < 0 ) {
			nHundreds += 100;
			nSeconds--;
		}

		if ( nSeconds < 0 ) {
			nSeconds += 60;
			nMinutes--;
		}

		if ( nMinutes < 0 ) {
			nMinutes += 60;
			nHours--;
		}

		// check for > 24 hours elapsed
		MakeDaysFromDate( &lSplitDay, NULLSTR );
		if (( lSplitDay - gaTimers[nTimerNumber].lDay ) > 0 )
			nHours += 24;
	}

	sprintf( pszBuffer, TIMER_SPLIT, nHours, gaCountryInfo.szTimeSeparator[0], nMinutes, gaCountryInfo.szTimeSeparator[0], nSeconds, gaCountryInfo.szDecimal[0], nHundreds);
}


// return the current date in the formats:
//	format_type == 0 : "Mon  Jan 1, 1999"
//	format_type == 1 : " 1/01/99"
//      format_type == 2 : "Mon 1/01/1999"
LPTSTR _fastcall gdate( int nDateFormat )
{
	static TCHAR szDate[20];
	DATETIME sysDateTime;

	QueryDateTime( &sysDateTime );

	if ( nDateFormat == 2 )
		sprintf( szDate, _TEXT("%.3s %s"), daytbl[(int)sysDateTime.weekday], FormatDate( sysDateTime.month, sysDateTime.day, sysDateTime.year, 0x100 ));

	else if ( nDateFormat == 1 )
		return ( FormatDate( sysDateTime.month, sysDateTime.day, sysDateTime.year, 0 ));

	else if ( gaCountryInfo.fsDateFmt != 1 ) {
		// USA or Japan
		sprintf( szDate, _TEXT("%.3s  %s %u, %4u"), daytbl[(int)sysDateTime.weekday], montbl[(int)sysDateTime.month-1], (int)sysDateTime.day, sysDateTime.year );
	} else {
		// Europe
		sprintf( szDate, _TEXT("%.3s  %u %s %4u"), daytbl[(int)sysDateTime.weekday],(int)sysDateTime.day, montbl[(int)sysDateTime.month-1], sysDateTime.year );
	}

	return szDate;
}


// return the current time as a string in the format "12:45:07"
LPTSTR _fastcall gtime( register int nTimeFormat )
{
	static TCHAR szTime[10];
	DATETIME sysDateTime;

	QueryDateTime( &sysDateTime );

	// check for 12 hour format
	if ( nTimeFormat == 0 ) {
		nTimeFormat = _TEXT('p');
		if ( sysDateTime.hours < 12 ) {
			if ( sysDateTime.hours == 0 )
				sysDateTime.hours = 12;
			nTimeFormat = _TEXT('a');
		} else if ( sysDateTime.hours > 12 )
			sysDateTime.hours -= 12;
	} else
		nTimeFormat = 0;

	// get the time format for the default country
	sprintf( szTime, TIME_FMT, sysDateTime.hours, gaCountryInfo.szTimeSeparator[0],sysDateTime.minutes, gaCountryInfo.szTimeSeparator[0], sysDateTime.seconds, nTimeFormat );

	return szTime;
}


// set new system date
int _near Date_Cmd( LPTSTR pszCmdLine )
{
	unsigned int uDay, uMonth, uYear;
	TCHAR szBuf[12];
	DATETIME sysDateTime;
	long fDate;

	if ( GetSwitches( pszCmdLine, _TEXT("T"), &fDate, 1 ) != 0 )
		return ( Usage( DATE_USAGE ));

	if ( fDate ) {
		printf( FMT_STR_CRLF, gdate( 2 ));
		return 0;
	}

	if (( pszCmdLine ) && ( *pszCmdLine )) {
		// date already in command line, so don't ask for it
		sprintf( szBuf, FMT_PREC_STR, 10, pszCmdLine );
		goto got_date;
	}

	// display current date & time
	printf( _TEXT("%s  %s"), gdate( 0 ), gtime( gaCountryInfo.fsTimeFmt ) );

	for ( ; ; ) {

		printf( NEW_DATE, dateformat[ gaCountryInfo.fsDateFmt ] );
		if ( egets( szBuf, 10, EDIT_DATA | EDIT_SCROLL_CONSOLE ) == 0 )
			break;
got_date:
		// valid date entry?
		if ( GetStrDate( szBuf, &uMonth, &uDay, &uYear ) == 3 ) {

			// SetDateTime() requires date & time together
			QueryDateTime( &sysDateTime );

			if (( sysDateTime.year = uYear ) < 80 )
				sysDateTime.year += 2000;
			else if ( sysDateTime.year < 127 )
				sysDateTime.year += 1900;

			sysDateTime.month = (unsigned char)uMonth;
			sysDateTime.day = (unsigned char)uDay;

			if ( SetDateTime( (DATETIME *)&sysDateTime ) == 0 )
				break;
		}

		error( ERROR_4DOS_INVALID_DATE, szBuf );
	}

	return 0;
}


// set new system time
int _near Time_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszAmPm;
	unsigned int uHours, uMinutes, uSeconds;
	TCHAR szBuf[12];
	DATETIME sysDateTime;
	long fTime;

	if ( GetSwitches( pszCmdLine, _TEXT("NTS"), &fTime, 1 ) != 0 )
		return ( Usage( TIME_USAGE ));

	// either /N (for DOS in OS/2) or /T just display the time
	if ( fTime & 3 ) {
		printf( FMT_STR_CRLF, gtime( gaCountryInfo.fsTimeFmt ) );
		return 0;
	}


	if (( pszCmdLine ) && ( *pszCmdLine )) {
		// time already in command line, so don't ask again
		sprintf( szBuf, FMT_PREC_STR, 10, pszCmdLine );
		goto got_time;
	}

	// display current date & time
	printf( _TEXT("%s  %s"), gdate( 0 ), gtime( gaCountryInfo.fsTimeFmt ) );

	for ( ; ; ) {

		qputs( NEW_TIME );
		if ( egets( szBuf, 10, EDIT_DATA | EDIT_SCROLL_CONSOLE ) == 0 )
			return 0;	// quit or no change
got_time:
		uSeconds = 0;
		if ( sscanf( szBuf, DATE_FMT, &uHours, &uMinutes, &uSeconds ) >= 2) {

			// check for AM/PM syntax
			if (( pszAmPm = strpbrk( strupr( szBuf ), _TEXT("AP") )) != NULL ) {
				if (( uHours == 12 ) && ( *pszAmPm == _TEXT('A') ))
					uHours -= 12;
				else if (( *pszAmPm == _TEXT('P') ) && ( uHours < 12 ))
					uHours += 12;
			}

			// SetDateTime requires date & time together
			QueryDateTime( &sysDateTime );

			sysDateTime.hours = (unsigned char)uHours;
			sysDateTime.minutes = (unsigned char)uMinutes;
			sysDateTime.seconds = (unsigned char)uSeconds;
			sysDateTime.hundredths = 0;

			if ( SetDateTime( (DATETIME *)&sysDateTime ) == 0 )
				break;
		}

		error( ERROR_4DOS_INVALID_TIME, szBuf );
	}

	return 0;
}


// display or set the code page (only for DOS 3.3+ & NT)
int _near Chcp_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0;

	// display current code page
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) {
		if (( nReturn = QueryCodePage()) == 0 )
			return ( error( ERROR_4DOS_INVALID_DOS_VER, NULL ));
		printf( CODE_PAGE, nReturn );
	}


	if (( pszCmdLine != NULL ) && ( isdigit( *pszCmdLine )) && (( nReturn = SetCodePage( atoi( pszCmdLine ))) != 0 ))
		nReturn = error( nReturn, pszCmdLine );

	return nReturn;
}



// call the external Help (4HELP.EXE)
int _near Help_Cmd( LPTSTR pszCmdLine )
{
	return ( _help( pszCmdLine, NULL ));
}


// display command to be executed
int _near Which_Cmd( LPTSTR pszCmdLine )
{
	int i, nArg, fEExt = 0, nResult = 0;
	TCHAR *pszToken, *pszCmd, *pszExt, szFile[MAXFILENAME];
	TCHAR _far *lpszAlias, _far *lpszExecutableExt, _far *lpszTemp;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( WHICH_USAGE ));

	for ( nArg = 0; (( pszToken = ntharg( pszCmdLine, nArg | 0x800 )) != NULL ); nArg++ ) {

		pszToken = strcpy( (LPTSTR)_alloca( ( strlen(pszToken) + 1 ) * sizeof(TCHAR) ), pszToken );

CheckForAlias:
		// check for aliases
		if (( lpszAlias = get_alias( pszToken )) != NULL ) {

			// go to alias name to determine if it's a shortcut option ("foo*bar=...")
			for ( lpszTemp = lpszAlias; (( lpszTemp > glpAliasList ) && ( lpszTemp[-1] != _TEXT('\0') )); lpszTemp-- )
				;
				
			printf( _TEXT("%.*Fs is an alias : %Fs\r\n"), (unsigned int)( lpszAlias - lpszTemp) - 1, lpszTemp, lpszAlias );
			continue;
		}

		if ( *pszToken == _TEXT('@') ) {
			pszToken++;
			goto CheckForAlias;
		}

		// skip leading @ and *
		while (( *pszToken == _TEXT('@') ) || (( *pszToken == _TEXT('*') ) && ( pszToken[1] != _TEXT('.')) ))
			pszToken++;

		// check for internal commands
		if (( i = findcmd( pszToken, 0 )) >= 0 ) {
			printf( WHICH_INTERNAL, commands[i].szCommand );
			continue;
		}

		StripQuotes( pszToken );
		strcpy( szFile, pszToken );

		// skip leading @ and *
		while (( *pszToken == _TEXT('@') ) || (( *pszToken == _TEXT('*') ) && ( pszToken[1] != _TEXT('.')) ))
			pszToken++;

		if (( pszCmd = searchpaths( pszToken, NULL, TRUE, &fEExt )) != NULL ) {

			if (( pszExt = ext_part( pszCmd )) != NULL ) {

				if ( fEExt )
					goto ExecutableExtension;

				if ( mkfname( pszCmd, 0 ) != NULL ) {

					if (( _stricmp( pszExt, COM ) == 0 ) || ( _stricmp( pszExt, EXE ) == 0 )) {

						printf( WHICH_EXTERNAL, pszToken, pszCmd );
						continue;

					} else if (( _stricmp( pszExt, BTM ) == 0 ) || ( _stricmp( pszExt, BAT ) == 0 )) {
						printf( WHICH_BATCH, pszToken, pszCmd );
						continue;
					}

ExecutableExtension:
					lpszExecutableExt = executable_ext( pszExt );
					if (( lpszExecutableExt ) && ( *lpszExecutableExt )) {
						printf( WHICH_EXECUTABLE_EXT, pszToken, lpszExecutableExt, pszCmd );
						continue;
					}

					else {
						printf( WHICH_EXTERNAL, pszToken, pszCmd );
						continue;
					}
				}
			}

		}

		printf( WHICH_UNKNOWN_CMD, pszToken );
		nResult = ERROR_FILE_NOT_FOUND;
	}

	return nResult;
}
