

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


// DIRCMDS.C - Directory commands for 4dos
//	 Copyright (c) 1988 - 2003	Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"

#include "dblspace.h"
extern int _pascal CompDriveSetup(char, int, COMPDRIVEINFO *);
extern int _pascal CompDriveCleanup(COMPDRIVEINFO *);
extern int _pascal CompData(COMPDRIVEINFO *, FILESEARCH *, LPTSTR , ULONG *, ULONG *);

static void PrintDirHeader( LPTSTR , LPTSTR , TCHAR _far * );
static int GetDirectory(LPTSTR , LPTSTR );
static int DrawTree( LPTSTR , LPTSTR  );
static void _nxtrow(void);
static int _near files_cmp( DIR_ENTRY _far *, DIR_ENTRY _far *);
static void _near ssort(char _huge *, unsigned int);

DIR_ENTRY _huge * _near GetDescriptions( unsigned int, DIR_ENTRY _huge *, unsigned long, unsigned long, LPTSTR );


// disk total & free space
static QDISKINFO DiskInfo;

typedef struct {
	unsigned long ulLowPart;
	unsigned long ulHighPart;
} __int64;

// global compression ratio data
static COMPDRIVEINFO CDriveData;
static __int64 ulTreeUnCompSec;
static __int64 ulTreeCompSec;
static __int64 ulTotUnCompSec;
static __int64 ulTotCompSec;
static unsigned long ulLowMemFree;
static unsigned int uScreenRows;			// screen size in rows (base 0 )


// files and sizes within directory tree branch
static unsigned long ulTreeFiles, ulTreeDirs;
static __int64 ulTreeSize;		// size of all files in tree
static __int64 ulTreeAlloc; 	// allocated size of all files in tree
static unsigned int uDirRow;			// current row number
static unsigned long ulDirMode;			// search attribute
static unsigned int uDirColumns;		// number of columns (varies by screen size )
static unsigned int uScreenColumns; 	// screen size in columns (base 1)
static int nScreenColor;				// default screen color ( for colorization)
static int fConsole;					// if != 0, STDOUT is console
static __int64 ulTotalFileSize; 		// total of file sizes
static __int64 ulTotalAllocated;	// total amount of disk space actually used
extern RANGES aRanges;			// date/time/size ranges
static LPTSTR pszCPBuffer;		// pointer to temp buffer

static unsigned int nCPLength;
static unsigned int nMaxFilename;
static unsigned int uRecurse;		// current recursion level (for totals)

#define DIR_ENTRY_BLOCK 128
#define DIR_LFN_BYTES 16384


#pragma alloc_text( _TEXT, Tree_Cmd )
#pragma alloc_text( _TEXT, Dir_Cmd )

static void _near MoveFiles( unsigned int, DIR_ENTRY _huge *, unsigned long );


static unsigned long DirMemFree( void )
{
	return ((( (unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 )) -	((( (unsigned long)QuerySystemRAM() ) << 6 ) - (unsigned long)gpIniptr->HighSeg )) << 4 );
}


// Allocate space for directory entries and LFNs in 4DOS
// Allocates a single block which holds LFNs below files; adjusts addresses
// and pointers as required if either block moves
static void _near ReallocDir( DIR_ENTRY _huge **pFiles, unsigned long ulOldFileSize, unsigned long ulNewFileSize, unsigned int nEntries, char _huge **pLFNs, unsigned long ulOldLFNSize, unsigned long ulNewLFNSize, int fChangeLFNs )
{
	unsigned long ulOldSize, ulNewSize;
	unsigned int uLFNParagraphs, uNewFileSeg;
	register unsigned int i;
	char _huge *pBlock, _huge *pNewBlock, _huge *pNewLFN;
	DIR_ENTRY _huge *pNewFiles;
	
	uLFNParagraphs = (unsigned int)(( ulNewLFNSize + 0xF ) >> 4 );
	ulOldSize = ((( ulOldFileSize + 0xF ) >> 4 ) + (( ulOldLFNSize + 0xF ) >> 4 )) << 4;
	ulNewSize = ((( ulNewFileSize + 0xF ) >> 4 ) + (unsigned long)uLFNParagraphs ) << 4;
	
	if ( ulNewSize == ulOldSize )
		return;
	
	// Get the old block address, if any
	pBlock = ( *pLFNs == 0L ) ? (char _far *)*pFiles : (char _far *)*pLFNs;
	
	// If we are decreasing the LFN size, move the files array before the
	// block is reallocated (otherwise the MCB at the end of the smaller
	// block will clobber the data before we move it)
	if ( fChangeLFNs && ( ulNewLFNSize <= ulOldLFNSize ))
		MoveFiles( ( SELECTOROF( pBlock ) + uLFNParagraphs ), *pFiles, ulOldFileSize );
	
	if (( pNewBlock = ReallocMem( pBlock, ulNewSize )) == 0L) {
		if ( fChangeLFNs )
			*pLFNs = 0L;
		else
			*pFiles = 0L;
		return;
	}
	
	// Calculate new files segment (same as old, unless we expanded the
	// block and it moved)
	uNewFileSeg = SELECTOROF( pNewBlock ) + uLFNParagraphs;
	
	// If we are increasing the LFN size, move the files array after the
	// block is reallocated
	if ( fChangeLFNs && ( ulNewLFNSize > ulOldLFNSize ))
		MoveFiles( uNewFileSeg, *pFiles, ulOldFileSize );
	
	// Calculate new files array pointer so we are using the correct files
	// array if we have to move the LFNs
	pNewFiles = (DIR_ENTRY _huge *)MAKEP( uNewFileSeg, 0 );
	
	// If we have both LFNs and files, and the base moved, adjust the LFN
	// pointers in the files array
	if (( *pLFNs != 0L ) && ( *pFiles != 0L ) && ( pNewBlock != pBlock )) {
		for ( i = 0, pNewLFN = pNewBlock; ( i < nEntries ); i++ ) {
			pNewFiles[i].lpszLFN = pNewLFN;
			while ( *pNewLFN )
				pNewLFN++;
			pNewLFN++;
		}
	}
	
	// Save the new address of the modified block(s).  Files array can move
	// any time; LFNs move only if requested.
	*pFiles = pNewFiles;
	if (fChangeLFNs)
		*pLFNs = (char _huge *)pNewBlock;
	
	// Remmeber "low water mark" of free memory if requested
	if (( gpIniptr->INIDebug & INI_DB_DIRMEMTOT ) && (( ulOldSize = DirMemFree() ) < ulLowMemFree ))
		ulLowMemFree = ulOldSize;
}


static void _near MoveFiles( unsigned int uNewSeg, DIR_ENTRY _huge *Files, unsigned long ulOldFileSize )
{
	register unsigned int i;
	unsigned int uOldSeg, uFileBlocks, uBlockSegments, uRemaining;
	
	// Don't move if there's no files array
	if (Files != 0L) {
		
		uOldSeg = SELECTOROF( (char _huge *)Files );
		uFileBlocks = (unsigned int)( ulOldFileSize >> 15 );
		uRemaining = (unsigned int)( ulOldFileSize - (( (ULONG)uFileBlocks ) << 15 ));
		
		if ( uNewSeg > uOldSeg ) {
			
			// Adjust pointers to start after last 32K block
			uBlockSegments = ( uFileBlocks << 11 );
			uOldSeg += uBlockSegments;
			uNewSeg += uBlockSegments;
			
			// If moving up, move the last block first ...
			if ( uRemaining > 0 )
				_fmemmove( MAKEP( uNewSeg, 0 ), MAKEP( uOldSeg, 0 ), uRemaining );
			
			// ... then move full 32K blocks up, in reverse order
			for (i = 0; i < uFileBlocks; i++) {
				uOldSeg -= 0x800;
				uNewSeg -= 0x800;
				_fmemmove( MAKEP( uNewSeg, 0 ), MAKEP( uOldSeg, 0 ), 0x8000);
			}
			
		} else {
			
			// If moving down, move full 32K blocks down ...
			for (i = 0; i < uFileBlocks; i++) {
				_fmemmove( MAKEP( uNewSeg, 0 ), MAKEP( uOldSeg, 0 ), 0x8000);
				uOldSeg += 0x800;
				uNewSeg += 0x800;
			}
			
			// ... then move the last partial block down
			if ( uRemaining > 0 )
				_fmemmove( MAKEP( uNewSeg, 0 ), MAKEP( uOldSeg, 0 ), uRemaining );
		}
	}
}


static void _near FreeDir( DIR_ENTRY _huge *Files, char _huge *LFNs )
{
#ifdef __BETA
	if (gpIniptr->INIDebug & INI_DB_DIRMEM)
		printf("  FR1: free file space at %x, LFN space at %x, starting with %lub free\r\n", SELECTOROF(Files), SELECTOROF(LFNs), ((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
	FreeMem(( LFNs == 0L ) ? (char _far *)Files : (char _far *)LFNs );
#ifdef __BETA
	if (gpIniptr->INIDebug & INI_DB_DIRMEM)
		printf("  FR2: after space freed, %lub free\r\n", ((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
}


// add a DWORD to a QWORD
static void Add32To64( __int64 *ullVal, unsigned long ulVal )
{
	unsigned long ulTemp;
	
	ulTemp = ullVal->ulLowPart;
	ullVal->ulLowPart += ulVal;
	if ( ullVal->ulLowPart < ulTemp )
		ullVal->ulHighPart++;
}


// add a QWORD to a QWORD
static void Add64To64( __int64 *ullVal, __int64 *ullAdd )
{
	unsigned long ulTemp;
	
	ullVal->ulHighPart += ullAdd->ulHighPart;
	
	ulTemp = ullVal->ulLowPart;
	ullVal->ulLowPart += ullAdd->ulLowPart;
	if ( ullVal->ulLowPart < ulTemp )
		ullVal->ulHighPart++;
}


// subtract a QWORD from a QWORD; return result in second arg
static void Subtract64From64( __int64 *ullVal, __int64 *ullSubtract )
{
	unsigned long ulTemp;
	
	ullSubtract->ulHighPart = ullVal->ulHighPart - ullSubtract->ulHighPart;
	
	ulTemp = ullVal->ulLowPart;
	ullSubtract->ulLowPart = ullVal->ulLowPart - ullSubtract->ulLowPart;
	if ( ullSubtract->ulLowPart > ulTemp )
		ullSubtract->ulHighPart--;
}


// divide a QWORD by a QWORD, return a string
static LPTSTR  Divide64By64( __int64 *ullVal, __int64 *ullDivide, int nPrecision, int nPercent )
{
	static char szLongLong[80];
	
	sprintf( szLongLong, "(((4294967296*%lu)+%lu)/((4294967296*%lu)+%lu))*%d=0.%d", ullVal->ulHighPart, ullVal->ulLowPart, ullDivide->ulHighPart, ullDivide->ulLowPart, nPercent, nPrecision );
	
	evaluate( szLongLong );
	return szLongLong;
}


// Convert a 64-bit longlong to ASCII
static LPTSTR  Format64( __int64 *ullVal )
{
	static char szLongLong[32];
	
	sprintf( szLongLong, "(4294967296*%lu)+%lu", ullVal->ulHighPart, ullVal->ulLowPart );
	
	evaluate( szLongLong );
	AddCommas( szLongLong );
	
	return szLongLong;
}


// initialize directory variables
void init_dir( void )
{
	fConsole = QueryIsConsole( STDOUT );
	
	gszSortSequence[0] = _TEXT('\0');	   // default to sort by filename
	
	uRecurse = 0;
	gnDirTimeField = 0;
	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;
	glDirFlags = 0L;
	uDirColumns = 1;
	ulDirMode = 0x10;		// default to normal files + directories
	ulTreeFiles = ulTreeDirs = 0L;

	ulTreeSize.ulLowPart = ulTreeSize.ulHighPart = 0L;
	ulTreeAlloc.ulLowPart = ulTreeAlloc.ulHighPart = 0L;
	ulTreeCompSec.ulLowPart = ulTreeCompSec.ulHighPart = 0L;
	ulTreeUnCompSec.ulLowPart = ulTreeUnCompSec.ulHighPart = 0L;

	uScreenColumns = GetScrCols();
	nScreenColor = -1;
	init_page_size();

	uScreenRows = GetScrRows();
	CDriveData.cCDLetter = _TEXT('\0');
	CDriveData.nBufSeg = 0;
	ulLowMemFree = DirMemFree();
}


// initialize page size variables
void init_page_size( void )
{
	uDirRow = gnPageLength = 0;
}


// retrieve files based on specified attributes
LPTSTR _fastcall GetSearchAttributes( LPTSTR pszAttributes )
{
	register int nFileAttribute, fOr, fExclude;
	
	ulDirMode = FIND_BYATTS | 0x17;
	
	// kludge for DOS 5 format
	if ((pszAttributes != NULL ) && (*pszAttributes == _TEXT(':') ))
		pszAttributes++;
	
	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;
	
	// if no arg (/A), default to everything
	if (( pszAttributes == NULL ) || ( *pszAttributes == _TEXT('\0') ))
		return pszAttributes;
	
	// DOS 5-style; the '-' means exclusive search
	for ( ; ( *pszAttributes != _TEXT('\0') ); pszAttributes++ ) {
		
		fExclude = fOr = 0;
		
		if ( *pszAttributes == _TEXT('-') ) {
			pszAttributes++;
			fExclude = 1;
		} else if ( *pszAttributes == _TEXT('+') ) {
			// OR attribute
			pszAttributes++;
			fOr = 1;
		}
		
		nFileAttribute = _ctoupper( *pszAttributes );
		
		if ( nFileAttribute == _TEXT('R') )
			nFileAttribute = _A_RDONLY;
		else if ( nFileAttribute == _TEXT('H') )
			nFileAttribute = _A_HIDDEN;
		else if ( nFileAttribute == _TEXT('S') )
			nFileAttribute = _A_SYSTEM;
		else if ( nFileAttribute == _TEXT('D') )
			nFileAttribute = _A_SUBDIR;
		else if ( nFileAttribute == _TEXT('A') )
			nFileAttribute = _A_ARCH;
		else if ( nFileAttribute == _TEXT('N') )
			nFileAttribute = 0;
		else if ( nFileAttribute == _TEXT('_') )
			continue;
		else
			break;
		
		if ( fOr ) {
			
			if ( nFileAttribute == 0 )
				gnOrMode |= 0x1000;
			else
				gnOrMode |= nFileAttribute;
			
		} else if ( fExclude ) {
			// set the flags & turn off the opposite flag (for 
			//	 example, if set in an alias)
			gnExclusiveMode |= nFileAttribute;
			gnInclusiveMode &= ~nFileAttribute;
		} else {
			gnInclusiveMode |= nFileAttribute;
			gnExclusiveMode &= ~nFileAttribute;
		}
	}
	
	return pszAttributes;
}


// get the sort sequence
LPTSTR	_fastcall dir_sort_order( LPTSTR pszSort )
{
	// kludge for DOS 5 format
	if ( *pszSort == _TEXT(':') )
		pszSort++;
	
	sscanf( strlwr( pszSort ), _TEXT("%14[-acdeginsurtz]"), gszSortSequence );
	
	// if /O:C specified, make sure /C flag is set
	if ( strchr( gszSortSequence, _TEXT('c') ) != NULL )
		glDirFlags |= DIRFLAGS_COMPRESSION;
	
	return ( pszSort + strlen( gszSortSequence ));
}


static long fTree;

#define TREE_ASCII	0x01
#define TREE_BARE	0x02
#define TREE_CDD	0x04
#define TREE_FILES	0x08
#define TREE_HIDDEN 0x10
#define TREE_PAGE	0x20
#define TREE_SIZE	0x40
#define TREE_TIME	0x80


// display a directory list or tree
int _near Tree_Cmd( register LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	TCHAR szFilename[MAXFILENAME], szSequelBuffer[MAXPATH];
	int argc, nReturn = 0, fTime;
	
	szSequelBuffer[0] = _TEXT('\0');
	init_dir();
	
	fTime = GetMultiCharSwitch( pszCmdLine, _TEXT("T"), szFilename, 4 );
	
	// check for and remove switches
	if ( GetSwitches( pszCmdLine, _TEXT("ABCFHPS"), &fTree, 0 ) != 0 )
		return ( Usage( TREE_USAGE ));
	
	if ( fTime ) {
		
		fTree |= TREE_TIME;
		pszArg = szFilename;
		if ( *pszArg ) {
			if ( *pszArg == _TEXT(':') )
				pszArg++;
			if ( _ctoupper( *pszArg ) == _TEXT('A') )
				gnDirTimeField = 1;
			else if ( _ctoupper( *pszArg ) == _TEXT('C') )
				gnDirTimeField = 2;
		}
	}
	
	// if no filename arguments, use current directory
	if ( first_arg( pszCmdLine ) == NULL ) {
		if (( pszCmdLine = gcdir( NULL, 0 )) == NULL )
			return ERROR_EXIT;
		AddQuotes( pszCmdLine );
	}
	
	if ( fTree & TREE_PAGE )
		gnPageLength = GetScrRows();
	
	pszArg = (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) );
	pszCmdLine = strcpy( pszArg, pszCmdLine );
	
	for ( argc = 0; (( pszArg = ntharg( pszCmdLine, argc )) != NULL ); argc++ ) {
		
		if ( mkfname( pszArg, 0 ) == NULL ) {
			nReturn = ERROR_EXIT;
			break;
		}
		
		if ( is_dir( pszArg ) == 0 ) {
			nReturn = error( ERROR_PATH_NOT_FOUND, pszArg );
			continue;
		}
		
		// display "c:\wonky..."
		_nxtrow();
		if ( fTree & TREE_CDD ) {
			
			printf( FMT_STR, pszArg );
			
			// create an SFN if necessary
			strcpy( szFilename, pszArg );
			GetShortName( szFilename );
			
			// only write SFN if it's different from the LFN!
			if ( _stricmp( szFilename, pszArg ) != 0 )
				printf( _TEXT("\004\004%s"), szFilename );
			crlf();
			
		} else
			PrintDirHeader( FMT_STR, pszArg, 0L );
		
		copy_filename( szFilename, pszArg );

		mkdirname( szFilename, ((( ifs_type( szFilename ) != 0 ) && ( fWin95 )) ? _TEXT("*") : WILD_FILE ));

		if ((( nReturn = DrawTree( szFilename, szSequelBuffer )) != 0 ) || ( cv.fException ))
			break;
		
		if ( setjmp( cv.env ) == -1 ) {
			nReturn = CTRLC;
			break;
		}
	}

	return nReturn;
}


// display a directory tree
static int DrawTree( register LPTSTR pszCurrent, LPTSTR pszSequelBuffer )
{
	int nReturn = 0, fOK, nAttrib;
	unsigned long uMode = 0x10 | FIND_NO_ERRORS | FIND_NO_DOTNAMES | FIND_NO_FILELISTS;
	TCHAR *pszName, *pszSave, cAmPm = _TEXT(' ');
	FILESEARCH dir;
	LPTSTR pszAltName;
	
	// trap ^C and clean up
	if ( setjmp( cv.env ) == -1 )
		return CTRLC;
	
	if ( fTree & TREE_HIDDEN )
		uMode |= 0x07;
	
	if (( fTree & TREE_FILES ) == 0 )
		uMode |= FIND_ONLY_DIRS;
	
	fOK = ( find_file( FIND_FIRST, pszCurrent, uMode, &dir, NULL ) != NULL );
	
	while ( fOK ) {
		
		pszName = strdup( dir.szFileName );
		pszAltName = (( dir.szAlternateFileName[0] ) ? strdup( dir.szAlternateFileName ) : NULLSTR );
		nAttrib = dir.attrib;
		
		if ( fTree & TREE_SIZE ) {

			printf( (( nAttrib & _A_SUBDIR ) ? DIR_LABEL : "%9lu" ), dir.ulSize );
			qputs( _TEXT("  ") );
		}
		
		if ( fTree & TREE_TIME ) {
			
			int month, day, year, minutes, hours;

			if ( gnDirTimeField == 1)
				FileTimeToDOSTime( &(dir.ftLastAccessTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
			else if ( gnDirTimeField == 2)
				FileTimeToDOSTime( &(dir.ftCreationTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
			
			month = dir.fd.file_date.months;
			day = dir.fd.file_date.days;
			year = dir.fd.file_date.years;
			
			hours = dir.ft.file_time.hours;
			minutes = dir.ft.file_time.minutes;

			printf( _TEXT("%s "), FormatDate( month, day, year + 80, 0x100 ));
			
			if ( gaCountryInfo.fsTimeFmt == 0 ) {
				if ( hours >= 12 ) {
					hours -= 12;
					cAmPm = _TEXT('p');
				} else
					cAmPm = _TEXT('a');
			}
			
			printf( _TEXT("%2u%c%02u%c  "), hours,
				gaCountryInfo.szTimeSeparator[0], minutes, cAmPm );
		}
		
		// last directory on its level?
		fOK = ( find_file( FIND_NEXT, pszCurrent, uMode, &dir, NULL ) != NULL );
		
		// are we listing without graphics, or creating the CD / CDD database?	(JPSTREE.IDX)
		if (( fTree & TREE_BARE ) || ( fTree & TREE_CDD )) {
			
			TCHAR *pszArg, *pszPath;
			
			pszPath = path_part( pszCurrent );
			pszArg = strend( pszPath );
			strcpy( pszArg, pszName );
			
			// FIXME - for long directory names & /P
			qputs( pszPath );

			if (( fTree & TREE_CDD ) && ( fWin95LFN )) {

					LPTSTR lpszLFN;
					
					// create an SFN if necessary
					lpszLFN = strdup( pszPath );
					strcpy( pszArg, (( *pszAltName ) ? pszAltName : pszName ));
					GetShortName( pszPath );
					
					// only write SFN if it's different from the LFN!
					if ( _stricmp( lpszLFN, pszPath ) != 0 )
						printf( _TEXT("\004\004%s"), pszPath );
					free( lpszLFN );
				}
				
			} else if ( fTree & TREE_ASCII )
				printf( _TEXT("%s%c--%s"), pszSequelBuffer, (( fOK == 0 ) ? _TEXT('\\') : _TEXT('+') ), pszName );
			else {
				printf( _TEXT("%s%cÄÄ%s"), pszSequelBuffer, (( fOK == 0 ) ? _TEXT('À') : _TEXT('Ã') ), pszName );
			}

			_nxtrow();
			
			if ( *pszAltName )
				free( pszAltName );
			
			if ( nAttrib & _A_SUBDIR ) {

				CheckFreeStack( 1024 );

				strcat( pszSequelBuffer, (( fOK == 0 ) ? _TEXT("   ") : (( fTree & TREE_ASCII ) ? _TEXT("|  ") : _TEXT("³  ") ) ));
			
				pszSave = strdup( pszCurrent );
				insert_path( pszCurrent, pszName, pszCurrent );

				mkdirname( pszCurrent, ((( ifs_type( pszCurrent ) != 0 ) && ( fWin95 )) ? _TEXT("*") : WILD_FILE ));

				free( pszName );
				
				nReturn = DrawTree( pszCurrent, pszSequelBuffer );
				
				// check for ^C and reset ^C trapping
				if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ) || ( cv.fException ))
					return CTRLC;
				
				strcpy( pszCurrent, pszSave );
				free( pszSave );
				
				// Truncate sequel buffer to its previous value
				pszSequelBuffer[ strlen( pszSequelBuffer ) - 3 ] = _TEXT('\0');
			} else
				free( pszName );
		}
		
		return nReturn;
}


// display a directory list
int _near Dir_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg, pszTrailingSwitch;
	TCHAR szFileName[MAXFILENAME], szFName[MAXFILENAME];

	TCHAR szCPBuffer[592];

	int argc, n, nReturn, fNot;
	long lTemp;

	init_dir(); 					// initialize the DIR global variables
	pszCPBuffer = szCPBuffer;
	
	// default to *.* on FAT, * on LFN/NTFS

	strcpy( szFName, ((( ifs_type( NULL ) != 0 ) && ( fWin95 )) ? "*" : WILD_FILE ));

	argc = nReturn = 0;
	
	// reset the disk for nitwits like FASTOPEN & dumb caches
	if ( gpIniptr->DiskReset )
		reset_disks();
	
	// initialize date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	do {
		
		if (( pszArg = ntharg( pszCmdLine, argc++ )) != NULL ) {
			
			if ( *pszArg == gpIniptr->SwChr ) {
				
				// point past switch character
				for ( pszArg++; ( *pszArg != _TEXT('\0') ); ) {
					
					// disable option?
					if ( *pszArg == _TEXT('-') ) {
						fNot = 1;
						pszArg++;
					} else
						fNot = 0;
					lTemp = 0L;
					
					switch ( _ctoupper(*pszArg++)) {
					case '1':		// single column display
					case '2':		// 2 column display
					case '4':		// 4 column display
						uDirColumns = (( fNot ) ? 1 : ( pszArg[-1] - _TEXT('0') ));
						break;
						
					case 'A':		// retrieve based on specified attribs
						pszArg = GetSearchAttributes( pszArg );
						break;
						
					case 'B':		// no headers, details, or summaries
						lTemp |= ( DIRFLAGS_NO_HEADER | DIRFLAGS_NO_FOOTER | DIRFLAGS_NAMES_ONLY );
						break;
						
					case 'C':		// show compression ratio
						lTemp |= DIRFLAGS_COMPRESSION;

						if ( _ctoupper( *pszArg ) == 'H' ) {
							pszArg++;
							lTemp |= DIRFLAGS_HOST_COMPRESSION;
						}
						if ( _ctoupper( *pszArg ) == 'P' ) {
							pszArg++;
							lTemp |= DIRFLAGS_PERCENT_COMPRESSION;
						}

						break;
						
					case 'D':		// turn off directory colorization
						lTemp |= DIRFLAGS_NO_COLOR;
						break;
						
					case 'E':		// display in upper case
						lTemp |= DIRFLAGS_UPPER_CASE;
						break;
						
					case 'F':		// display fully expanded filename
						lTemp |= ( DIRFLAGS_FULLNAME | DIRFLAGS_NO_HEADER | DIRFLAGS_NO_FOOTER );
						break;
						
					case 'G':
						lTemp |= DIRFLAGS_ALLOCATED;
						break;
						
					case 'H':		// no "." or ".."
						lTemp |= DIRFLAGS_NO_DOTS;
						break;
						
					case 'J':		// DOS justify filenames
						lTemp |= DIRFLAGS_JUSTIFY;
						break;
						
					case 'K':		// no header
						lTemp |= DIRFLAGS_NO_HEADER;
						break;
						
					case 'L':		// display in lower case
						lTemp |= DIRFLAGS_LOWER_CASE;
						break;
						
					case 'M':		// no footer
						lTemp |= DIRFLAGS_NO_FOOTER;
						break;
						
					case 'N':
						// reset DIR defaults
						init_dir();

						break;
						
					case 'O':		// dir sort order
						if ( fNot )
							strcpy( gszSortSequence, _TEXT("u") );
						else
							pszArg = dir_sort_order( pszArg );
						break;
						
					case 'P':		// pause on pages
						if ( fNot )
							gnPageLength = 0;
						else {
							gnPageLength = GetScrRows();
						}
						break;

					case 'R':		// truncate descriptions
						lTemp |= DIRFLAGS_TRUNCATE_DESCRIPTION;
						break;
						
					case 'S':		// recursive dir scan
						lTemp |= DIRFLAGS_RECURSE;
						break;
						
					case 'T':		// display attributes or time field
						if ( *pszArg ) {
							
							if ( *pszArg == _TEXT(':') )
								pszArg++;
							
							switch ( _ctoupper( *pszArg )) {
							case 'A':		// last access
								gnDirTimeField = 1;
								break;
							case 'C':		// creation
								gnDirTimeField = 2;
								break;
							case 'W':		// last write (default)
								gnDirTimeField = 0;
								break;
							default:
								goto DirErrorExit;
							}
							pszArg = NULLSTR;
							
						} else {
							lTemp |= DIRFLAGS_SHOW_ATTS;
							if ( fNot == 0 )
								uDirColumns = 1;
						}
						break;
						
							case 'U':		// summaries only
								lTemp |= DIRFLAGS_SUMMARY_ONLY;
								if ( *pszArg == _TEXT('1') ) {
									lTemp |= DIRFLAGS_LOCAL_SUMMARY;
									pszArg++;
								} else if ( *pszArg == _TEXT('2') ) {
									lTemp |= DIRFLAGS_GLOBAL_SUMMARY;
									pszArg++;
								}
								break;
								
							case 'V':		// vertical sort
								lTemp |= DIRFLAGS_VSORT;
								break;
								
							case 'W':		// wide screen display
								uDirColumns = (( fNot ) ? 1 : ( uScreenColumns / 16 ));
								lTemp |= DIRFLAGS_WIDE;
								break;
								
							case 'X':		// display alternate name too

								if ( fWin95LFN == 0 )
									break;

								lTemp |= DIRFLAGS_NT_ALT_NAME;
								break;
								
							case 'Z':
								lTemp |= DIRFLAGS_LFN_TO_FAT;
								break;

							default:
DirErrorExit:
								error( ERROR_INVALID_PARAMETER, pszArg-1 );
								nReturn = Usage( DIR_USAGE );
								goto dir_bye;
						}
						
						// check for option being disabled vs. set
						if ( fNot )
							glDirFlags &= ~lTemp;
						else
							glDirFlags |= lTemp;
					}
					
				} else {
					
					// it must be a file or directory name
					copy_filename( szFName, pszArg );
					
					// check for trailing switches
					// KLUDGE for COMMAND.COM compatibility ( DIR *.c /w)
					for ( n = argc; (( pszTrailingSwitch = ntharg( pszCmdLine, n )) != NULL ); n++ ) {
						// check for another file spec
						if ( *pszTrailingSwitch != gpIniptr->SwChr )
							goto show_dir;
					}
				}
				
			} else {
				
show_dir:
			// put a path on the argument
			if ( mkfname( szFName, 0 ) != NULL ) {
				
				// kludge to convert /B/S to /F/S
				if (( glDirFlags & DIRFLAGS_RECURSE ) && ( glDirFlags & DIRFLAGS_NAMES_ONLY ))
					glDirFlags |= DIRFLAGS_FULLNAME;
				
				// disable /J if used with /F
				if ( glDirFlags & DIRFLAGS_FULLNAME )
					glDirFlags &= ~DIRFLAGS_JUSTIFY;
				
				if (( glDirFlags & DIRFLAGS_NO_HEADER ) == 0 )
					_nxtrow();
				
				// if not a server sharename, display volume label and
				//	 get usage stats for drive (we have to get the volume
				//	 label first to kludge around a bug in the the
				//	 Corel CD-ROM drivers)
				
				// DR-DOS returns an error on a JOINed drive!
				DiskInfo.ClusterSize = -1L;
				

				if ( is_net_drive( szFName ) == 0 ) {

						if (( glDirFlags & DIRFLAGS_NO_HEADER ) == 0 ) {
							if ( getlabel( szFName ) != 0 )
								continue;
							_nxtrow();
							
						}
						
						if ( QueryDiskInfo( szFName, &DiskInfo, 1 ) != 0 )
							continue;
					}

					// save filename part ( for recursive calls & include lists)
					copy_filename( szFileName, ( szFName + strlen( path_part( szFName ))) );
					
					// if not 1 column, turn off incompatible options
					if ( uDirColumns > 1 ) {
						glDirFlags &= ~DIRFLAGS_COMPRESSION;
						glDirFlags &= ~DIRFLAGS_PERCENT_COMPRESSION;

						glDirFlags &= ~DIRFLAGS_HOST_COMPRESSION;
	
					}
					
					// check to see if it's an LFN/NTFS partition
					//	 & the /N flag isn't set
					if (( ifs_type( szFName ) != 0 ) && (( glDirFlags & DIRFLAGS_LFN ) == 0 )) {
						glDirFlags |= DIRFLAGS_LFN;
						uRecurse = 0;
						nReturn = GetDirectory( szFName, szFileName );
						glDirFlags &= ~DIRFLAGS_LFN;
					} else
						nReturn = GetDirectory( szFName, szFileName );
					
					if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
						nReturn = CTRLC;
						goto dir_bye;
					}

					// display the free bytes
					if (( DiskInfo.ClusterSize > 0L ) && (( glDirFlags & ( DIRFLAGS_NO_FOOTER | DIRFLAGS_LOCAL_SUMMARY )) == 0 )) {

						printf( DIR_BYTES_FREE, DiskInfo.szBytesFree );

						_nxtrow();
					}
					
				} else
					nReturn = ERROR_EXIT;
			}
			
			} while (( nReturn != CTRLC ) && ( pszArg != NULL ));
			
			if (gpIniptr->INIDebug & INI_DB_DIRMEMTOT)
				printf("Maximum memory used %Lu bytes, lowest free %Lu bytes\r\n", DirMemFree() - ulLowMemFree, ulLowMemFree);
			
dir_bye:

			return nReturn;
}


static int GetDirectory( register LPTSTR pszCurrent, LPTSTR pszFileName )
{
	register unsigned int i;
	unsigned int j, k, n, uEntries = 0;
	unsigned int uFilesDisplayed = 0, uDirsDisplayed = 0;
	unsigned int uLastEntry, uVRows, uVOffset, uColumns;
	int nReturn = 0;
	unsigned long ulFiles, ulDirs;
	__int64 ulTSize, ulTAlloc;
	__int64 ulTempUnCompSec, ulTempCompSec;

	DIR_ENTRY _huge *files = 0L;	// file array in system memory
	LPTSTR pszPtr;

	// trap ^C and clean up
	if ( setjmp( cv.env ) == -1 ) {
dir_abort:
	dir_free( files );
	return CTRLC;
	}
	
	uColumns = uDirColumns;
	nMaxFilename = 0;
	uRecurse++;
	
	// force case change?
	if ( glDirFlags & DIRFLAGS_UPPER_CASE )
		strupr( pszCurrent );

	else if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LOWER_CASE ))
		strlwr( pszCurrent );
	
	// save the current tree size & entries for recursive display

	ulTSize.ulLowPart = ulTreeSize.ulLowPart;
	ulTSize.ulHighPart = ulTreeSize.ulHighPart;
	ulTAlloc.ulLowPart = ulTreeAlloc.ulLowPart;
	ulTAlloc.ulHighPart = ulTreeAlloc.ulHighPart;

	ulFiles = ulTreeFiles;
	ulDirs = ulTreeDirs;
	
	ulTempUnCompSec.ulLowPart = ulTreeUnCompSec.ulLowPart;
	ulTempUnCompSec.ulHighPart = ulTreeUnCompSec.ulHighPart;
	ulTempCompSec.ulLowPart = ulTreeCompSec.ulLowPart;
	ulTempCompSec.ulHighPart = ulTreeCompSec.ulHighPart;
	
	ulTotUnCompSec.ulLowPart = ulTotUnCompSec.ulHighPart = 0L;
	ulTotCompSec.ulLowPart = ulTotCompSec.ulHighPart = 0L;
	
	ulTotalFileSize.ulLowPart = ulTotalFileSize.ulHighPart = 0L;
	ulTotalAllocated.ulLowPart = ulTotalAllocated.ulHighPart = 0L;
	
	if ( is_dir( pszCurrent )) {

		pszPtr = (( glDirFlags & DIRFLAGS_LFN ) ? "*" : WILD_FILE );

		mkdirname( pszCurrent, pszPtr );
		pszFileName = pszPtr;

	} else {

		// if no include list & no extension specified, add default one
		//	 else if no filename, insert a wildcard filename
		if ( strchr( pszCurrent, _TEXT(';') ) == NULL ) {
			
			if ( ext_part( pszCurrent ) == NULL ) {

				// don't append "*" if name has wildcards
				//	 (for compatibility with COMMAND.COM)
				if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( strpbrk( pszCurrent, WILD_CHARS ) == NULL ))
					strcat( pszCurrent, WILD_EXT );

			} else {
				// point to the beginning of the filename
				pszPtr = pszCurrent + strlen( path_part( pszCurrent ));
				if ( *pszPtr == _TEXT('.') )
					strins( pszPtr, _TEXT("*") );
			}
		}
	}
	
	// set "get descriptions?" flag
	j = (( aRanges.pszDescription ) || ( strchr( gszSortSequence, _TEXT('i')) != NULL ) || ((( glDirFlags & DIRFLAGS_SUMMARY_ONLY ) == 0 ) && ( uColumns == 1 )));
	
	// check for file colorization (SET COLORDIR=...)
	if (( glDirFlags & DIRFLAGS_SUMMARY_ONLY ) == 0 )
		j |= 4;

	// look for matches
	if ( SearchDirectory( ulDirMode | FIND_RANGE | FIND_EXCLUDE, pszCurrent, ( DIR_ENTRY _huge **)&files, &uEntries, &aRanges, j ) != 0 ) {
		nReturn = ERROR_EXIT;
		goto do_dir_bye;
	}

	if ( uEntries > 0 ) {
		
		if (( glDirFlags & ( DIRFLAGS_NO_HEADER | DIRFLAGS_GLOBAL_SUMMARY )) == 0 ) {
			
			if ( glDirFlags & DIRFLAGS_RECURSING_NOW )
				_nxtrow();
			
			// display "Directory of c:\wonky\*.*"
			PrintDirHeader( DIRECTORY_OF, pszCurrent, ( TCHAR _far *)NULL );
			_nxtrow();
		}
		
		// mondo ugly kludge to emulate CMD.EXE behavior
		if (( uColumns > 1 ) && ( glDirFlags & DIRFLAGS_LFN ) && (( glDirFlags & DIRFLAGS_LFN_TO_FAT ) == 0 )) {
			
			// check if max LFN name is wider than screen!
			if (( nMaxFilename + 4 ) > ( uScreenColumns / uColumns )) {
				if (( uColumns = ( uScreenColumns / ( nMaxFilename + 4 ))) <= 1 ) {
					glDirFlags &= ~DIRFLAGS_VSORT;
					uColumns = 0;
				}
			}
		}
		
		// directory display loop
		for ( n = 0, uLastEntry = 0, uVRows = uVOffset = 0; ( n < uEntries ); n++ ) {
			
			if ( files[n].uAttribute & 0x10 )
				uDirsDisplayed++;
			else
				uFilesDisplayed++;
			
			// get file size & size rounded to cluster size

			Add32To64( &ulTotalFileSize, files[n].ulFileSize );

			if ( DiskInfo.ClusterSize != -1L ) {
				
				if ( files[n].ulFileSize > 0L ) {

					if ( files[n].lCompressedFileSize != 0xFFFFFFFFL ) {

						Add32To64( &ulTotalAllocated, files[n].lCompressedFileSize );

					} else {

						Add32To64( &ulTotalAllocated, (ULONG)(( files[n].ulFileSize + ( DiskInfo.ClusterSize - 1 )) / DiskInfo.ClusterSize ) * DiskInfo.ClusterSize );
					}
				}
			}
			
			// summaries only?
			if ( glDirFlags & DIRFLAGS_SUMMARY_ONLY )
				continue;
			
			// check for vertical (column) sort
			// NOTE: This is magic code!  If you change it, you
			//	 will probably break it!!
			if (( glDirFlags & DIRFLAGS_VSORT ) && ( uColumns > 0 )) {
				
				// check for new page
				if ( n >= uLastEntry ) {
					
					// get the last entry for previous page
					uVOffset = uLastEntry;
					
					// get last entry for this page
					if (( gnPageLength == 0 ) || (( uLastEntry += ( uColumns * ( gnPageLength - uDirRow))) > uEntries))
						uLastEntry = uEntries;
					
					uVRows = ((( uLastEntry - uVOffset ) + ( uColumns - 1 )) / uColumns );
				}
				
				i = n - uVOffset;
				k = ( i % uColumns );
				i = (( i / uColumns ) + ( k * uVRows )) + uVOffset;
				
				// we might have uneven columns on the last page!
				if ( uLastEntry % uColumns ) {
					j = ( uLastEntry % uColumns );
					if ( k > j )
						i -= ( k - j );
				}
				
			} else
				i = n;
			
			if ( glDirFlags & DIRFLAGS_ALLOCATED ) {
				if ( files[i].lCompressedFileSize != 0xFFFFFFFFL )
					files[i].ulFileSize = (( files[i].lCompressedFileSize + ( DiskInfo.ClusterSize - 1 )) / DiskInfo.ClusterSize ) * DiskInfo.ClusterSize;
				else
					files[i].ulFileSize = (( files[i].ulFileSize + ( DiskInfo.ClusterSize - 1 )) / DiskInfo.ClusterSize ) * DiskInfo.ClusterSize;
			}

			if ( glDirFlags & DIRFLAGS_FULLNAME ) {
				
				pszPtr = path_part( pszCurrent );
				
				// just print the fully expanded filename
				if ( glDirFlags & DIRFLAGS_NT_ALT_NAME ) {
					
					GetShortName( pszPtr );
					PrintDirHeader( _TEXT("%s%Fs"), pszPtr, files[i].szFATName );
					
				} else
					PrintDirHeader( _TEXT("%s%Fs"), pszPtr, ((( glDirFlags & DIRFLAGS_LFN ) && (( glDirFlags & DIRFLAGS_LFN_TO_FAT ) == 0 )) ? files[i].lpszLFN : files[i].szFATName ));
				
				uColumns = 1;
				
			} else if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LFN_TO_FAT ) || (( glDirFlags & DIRFLAGS_NT_ALT_NAME ) && (( uColumns > 1 ) || ( glDirFlags & DIRFLAGS_NAMES_ONLY )))) {
				
				if ((( glDirFlags & DIRFLAGS_NAMES_ONLY ) == 0 ) || ( uColumns > 1 )) {
					
					// if DIR /W, display directories with [ ]'s
					if (( glDirFlags & DIRFLAGS_WIDE ) && ( files[i].uAttribute & _A_SUBDIR ))
						
						nCPLength = sprintf( pszCPBuffer, _TEXT("[%Fs]%*s"), files[i].szFATName, 12 - _fstrlen( files[i].szFATName ), NULLSTR );
					else
						nCPLength = sprintf( pszCPBuffer, FMT_FAR_LEFT_STR, (( uColumns > 4 ) ? 14 : 12 ), files[i].szFATName );
					
					if ( uColumns <= 2 ) {

						nCPLength += sprintf( pszCPBuffer+nCPLength, ( files[i].uAttribute & _A_SUBDIR) ? DIR_LABEL : _TEXT("%9lu"), files[i].ulFileSize );

						nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT("  %s "),(( files[i].days == 0 ) ? _TEXT("        ") : FormatDate( files[i].months, files[i].days, files[i].years, 0 )));
						nCPLength += sprintf( pszCPBuffer+nCPLength, (( files[i].days == 0 ) ? _TEXT("      ") : (( gaCountryInfo.fsTimeFmt == 0 ) ? _TEXT("%2u%c%02u%c") : _TEXT(" %2u%c%02u") )),
							files[i].hours, gaCountryInfo.szTimeSeparator[0], files[i].minutes, files[i].szAmPm );
						
						if ( uColumns == 1 ) {
							if ( glDirFlags & DIRFLAGS_COMPRESSION ) {

								if ( files[i].uCompRatio != 0 ) {
									if ( glDirFlags & DIRFLAGS_PERCENT_COMPRESSION )
										sprintf( pszCPBuffer+nCPLength, DIR_PERCENT_RATIO, ( 1000 / files[i].uCompRatio ));
									else
										sprintf( pszCPBuffer+nCPLength, DIR_RATIO, ( files[i].uCompRatio / 10 ), ( files[i].uCompRatio % 10 ));
								}

							} else {
								if ( uScreenColumns > 40 ) {
									if ( glDirFlags & DIRFLAGS_SHOW_ATTS )
										nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT(" %s"), show_atts( files[i].uAttribute ));
									nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT(" %Fs"), files[i].lpszFileID );

									// wrap the description?
									if (( glDirFlags & DIRFLAGS_TRUNCATE_DESCRIPTION ) == 0 ) {
										
										while ((unsigned int)nCPLength >= uScreenColumns ) {
											
											for ( j = uScreenColumns-1; (( pszCPBuffer[j] != _TEXT(' ') ) && ( pszCPBuffer[j] != '\t' ) && ( j > 50 )); j-- )
												;
											
											// if no whitespace, make some!
											if ( j <= 50 ) {
												j = uScreenColumns - 1;
												strins( pszCPBuffer+j, _TEXT(" ") );
											}
											pszCPBuffer[j] = _TEXT('\0');
											
											// display the line
											color_printf( files[i].nColor, FMT_STR, pszCPBuffer );
											_nxtrow();
											
											// pad start of next line w/blanks
											wcmemset( pszCPBuffer, _TEXT(' '), 39 );
											strcpy( pszCPBuffer + 39, pszCPBuffer + j + 1 );
											nCPLength = strlen( pszCPBuffer );
										}
									}
								}
							}
						}
						
					} else if (( uColumns == 4 ) && (( glDirFlags & DIRFLAGS_WIDE ) == 0 )) {
						
						ULONG fsize;

						// display file size in Kb, Mb, or Gb
						if ( files[i].uAttribute & _A_SUBDIR )
							sprintf( pszCPBuffer+nCPLength, _TEXT("  <D>") );
						
						else {
							
							fsize = files[i].ulFileSize;
							
							// if between 1 & 10 Mb, display
							//	 decimal part too
							if (( files[i].ulFileSize >= 1048576L ) && ( files[i].ulFileSize < 10485760L )) {
								fsize /= 1048576L;
								sprintf( pszCPBuffer+nCPLength, _TEXT(" %lu.%luM"), (ULONG)fsize, (ULONG)(( (ULONG)files[i].ulFileSize % 1048576L ) / 104858L ));
							} else {
								for ( pszPtr = DIR_FILE_SIZE; (( fsize = (( fsize + 1023 ) / 1024L )) > 999 ); pszPtr++ )
									;
								sprintf( pszCPBuffer+nCPLength, _TEXT("%4lu%c"), (ULONG)fsize, *pszPtr );
							}
						}
					}
					
				} else
					_fstrcpy( pszCPBuffer, files[ i ].szFATName );
				
				// display the line
				color_printf( files[i].nColor, FMT_STR, pszCPBuffer );
				
			} else {
				
				// LFN format
				// clear color_printf string length
				nCPLength = 0;
				
				// format LFN long filename display
				if (( uColumns == 1 ) && ( glDirFlags & DIRFLAGS_WIDE ) == 0 ) {
					
					if (( glDirFlags & DIRFLAGS_NAMES_ONLY ) == 0 ) {
						
						// date
						nCPLength = sprintf( pszCPBuffer, (( files[i].days == 0 ) ? _TEXT("          ") : FormatDate( files[i].months,files[i].days,files[i].years, 0x100 )));
						
						// time
						nCPLength += sprintf( pszCPBuffer+nCPLength, (( files[i].days == 0 ) ? _TEXT("        ") : _TEXT("  %2u%c%02u%c") ),files[i].hours, gaCountryInfo.szTimeSeparator[0], files[i].minutes, files[i].szAmPm );
						
						// size or <DIR>

						nCPLength += sprintf( pszCPBuffer+nCPLength, ( files[i].uAttribute & _A_SUBDIR) ? LFN_DIR_LABEL : _TEXT("%15Lu"), files[i].ulFileSize );

						nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT("  ") );
					}
					
					// check if attribute display requested
					if ( glDirFlags & DIRFLAGS_SHOW_ATTS )
						nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT("%s  "), show_atts( files[i].uAttribute ));
					
					// alternate filename for NT & Win95
					if ( glDirFlags & DIRFLAGS_NT_ALT_NAME )
						nCPLength += sprintf( pszCPBuffer+nCPLength, FMT_FAR_LEFT_STR, 16, files[i].szFATName );
				}
				
				// set max LFN name width
				if ( uColumns > 1 ) {
					j = ( uScreenColumns / uColumns ) - 4;
				} else if ( uColumns < 1 )
					j = nMaxFilename;
				else
					j = 0;
				
				// filename
				if (( uColumns != 1 ) && ( files[i].uAttribute & _A_SUBDIR ))
					nCPLength += sprintf( pszCPBuffer+nCPLength, _TEXT("[%Fs]%*s"), files[i].lpszLFN, j - _fstrlen( files[i].lpszLFN ), NULLSTR );
				else
					nCPLength += sprintf( pszCPBuffer+nCPLength, FMT_FAR_LEFT_STR, (( glDirFlags & DIRFLAGS_NAMES_ONLY ) ? j : j+2 ), files[i].lpszLFN );

				if (( fConsole == 0 ) || ( gfCTTY ))
					qputs( pszCPBuffer );
					
				else {
						
					// wrap long LFN names
					for ( j = 0; ; ) {
							
						color_printf( files[i].nColor, FMT_PREC_STR, uScreenColumns, pszCPBuffer+j );
						if (( j += uScreenColumns ) >= nCPLength )
							break;
							
						if ( files[i].nColor != -1 )
							_nxtrow();
						else
							_page_break();
					}
				}
			}

			// if 2 or 4 column, or wide display, add spaces
			if (( uColumns <= 1 ) || ((( n + 1 ) % uColumns ) == 0 )) {
				if (( glDirFlags & DIRFLAGS_FULLNAME ) == 0 )
					_nxtrow();
			} else if (( glDirFlags & DIRFLAGS_LFN ) && (( glDirFlags & DIRFLAGS_LFN_TO_FAT ) == 0 ))
				qputs( _TEXT("  ") );
			else
				qputs(( glDirFlags & DIRFLAGS_WIDE ) ? _TEXT("  ") : _TEXT("   ") );


		}
			
		if (( uColumns > 1 ) && ( n % uColumns ))
			_nxtrow();
			
		ulTreeFiles += uFilesDisplayed;
		ulTreeDirs += uDirsDisplayed;

		Add64To64( &ulTreeAlloc, &ulTotalAllocated );
		Add64To64( &ulTreeSize, &ulTotalFileSize );
		Add64To64( &ulTreeCompSec, &ulTotCompSec );
		Add64To64( &ulTreeUnCompSec, &ulTotUnCompSec );
	}
		
	if (( glDirFlags & DIRFLAGS_GLOBAL_SUMMARY ) && (( glDirFlags & DIRFLAGS_RECURSE ) == 0 ))
		glDirFlags &= ~DIRFLAGS_GLOBAL_SUMMARY;
		
	// print the directory totals
	if ((( glDirFlags & ( DIRFLAGS_NO_FOOTER | DIRFLAGS_GLOBAL_SUMMARY )) == 0 ) && ((( uFilesDisplayed + uDirsDisplayed ) > 0 ) || (( glDirFlags & DIRFLAGS_RECURSE ) == 0 ))) {

		if (( glDirFlags & DIRFLAGS_COMPRESSION ) && (( ulTotCompSec.ulLowPart > 0 ) || ( ulTotCompSec.ulHighPart > 0 ))) {
			if ( glDirFlags & DIRFLAGS_PERCENT_COMPRESSION )
				printf( DIR_AVERAGE_PERCENT_RATIO, Divide64By64( &ulTotCompSec, &ulTotUnCompSec, 0, 100 ));
			else
				printf( DIR_AVERAGE_RATIO, Divide64By64( &ulTotUnCompSec, &ulTotCompSec, 1, 1 ));
			_nxtrow();
		}
		i = printf( DIR_BYTES_IN_FILES, Format64( &ulTotalFileSize ), (long)uFilesDisplayed, (( uFilesDisplayed == 1 ) ? ONE_FILE : MANY_FILES ), (LONG)uDirsDisplayed, (( uDirsDisplayed == 1 ) ? ONE_DIR : MANY_DIRS ));

		// if not a server sharename, display allocated & free space
		if (( pszCurrent[0] != '\\' ) && ( uFilesDisplayed > 0 ))
			printf( DIR_BYTES_ALLOCATED, Format64( &ulTotalAllocated ));
			
		_nxtrow();
	}
		
	// free memory used to store dir entries
	dir_free( files );
	files = 0L;
		
	// do a recursive directory search?
	if ( glDirFlags & DIRFLAGS_RECURSE ) {
			
		// remove filename
		CheckFreeStack( 1024 );
		insert_path( pszCurrent, (( glDirFlags & DIRFLAGS_LFN ) ? "*" : WILD_FILE ), path_part( pszCurrent ) );

		pszPtr = strchr( pszCurrent, _TEXT('*') );
			
		uEntries = 0;
			
		// search for subdirectories
		if ( SearchDirectory((( ulDirMode & 0xFF ) | 0x10 | FIND_ONLY_DIRS | FIND_EXCLUDE ), pszCurrent, (DIR_ENTRY _huge **)&files, &uEntries, &aRanges, 0 ) != 0 ) {
			nReturn = ERROR_EXIT;
			goto do_dir_bye;
		}

		for ( i = 0; ( i < uEntries ); i++ ) {
				
			glDirFlags |= DIRFLAGS_RECURSING_NOW;
				
			// make directory name
			sprintf( pszPtr, "%Fs\\%s", (( glDirFlags & DIRFLAGS_LFN ) ? files[i].lpszLFN : files[i].szFATName ), pszFileName );

			nReturn = GetDirectory( pszCurrent, pszFileName );
				
			// check for ^C and reset ^C trapping
			if (( setjmp( cv.env ) == -1) || ( nReturn == CTRLC ))
				goto dir_abort;

			// quit on error ( probably out of memory)
			if ( nReturn ) {
				dir_free( files );
				return nReturn;
			}
		}
			
		dir_free( files );
		files = 0L;
			
		// if we've got some subdirectories & some entries in the
		//	 current directory, print a subtotal
		if ((( glDirFlags & ( DIRFLAGS_NO_FOOTER | DIRFLAGS_LOCAL_SUMMARY )) == 0 ) && (( ulTreeFiles + ulTreeDirs ) > ( ulFiles + ulDirs ))) {
				
			// only display final figures?
			if ((( glDirFlags & DIRFLAGS_GLOBAL_SUMMARY ) == 0 ) || ( uRecurse <= 1 )) {

				// display figures for local tree branch
				strcpy( pszPtr, pszFileName );
				_nxtrow();
				PrintDirHeader( DIR_TOTAL, pszCurrent, (TCHAR _far *)NULL );

				if ( glDirFlags & DIRFLAGS_COMPRESSION ) {
						
					Subtract64From64( &ulTreeUnCompSec, &ulTempUnCompSec );
					Subtract64From64( &ulTreeCompSec, &ulTempCompSec );
						
					if (( ulTempCompSec.ulLowPart > 0 ) || ( ulTempCompSec.ulHighPart > 0 )) {
						if ( glDirFlags & DIRFLAGS_PERCENT_COMPRESSION )
							printf( DIR_AVERAGE_PERCENT_RATIO, Divide64By64( &ulTempCompSec, &ulTempUnCompSec, 0, 100 ));
						else
							printf( DIR_AVERAGE_RATIO, Divide64By64( &ulTempUnCompSec, &ulTempCompSec, 1, 1 ));
						_nxtrow();
					}
				}

				ulFiles = ( ulTreeFiles - ulFiles );
				ulDirs = ( ulTreeDirs - ulDirs );

				Subtract64From64( &ulTreeSize, &ulTSize );
					
				i = printf( DIR_BYTES_IN_FILES, Format64( &ulTSize ), ulFiles, (( ulFiles == 1 ) ? ONE_FILE : MANY_FILES ), ulDirs, (( ulDirs == 1 ) ? ONE_DIR : MANY_DIRS ));
					
				Subtract64From64( &ulTreeAlloc, &ulTAlloc );
					
				if ( DiskInfo.ClusterSize != -1L )
					printf( DIR_BYTES_ALLOCATED, Format64( &ulTAlloc ));
					
				_nxtrow();

			}
		}
	}
		
do_dir_bye:
		
	uRecurse--;
	return nReturn;
}


// free the array(s) used for directory storage
void _fastcall dir_free( DIR_ENTRY _huge *files )
{

	FreeDir( files, files[0].lpszLFNBase );
}


// print header or footer, with page wrapping optional
static void PrintDirHeader( LPTSTR pszHeader, LPTSTR pszName, TCHAR _far *pszOtherName )
{
	LPTSTR pszBuf;	// don't make register - will break _alloca!
	
	if ( pszOtherName == 0L )
		pszOtherName = NULLSTR;
	pszBuf = _alloca( ( strlen( pszHeader ) + strlen( pszName ) + _fstrlen( pszOtherName ) + 1 ) * sizeof(TCHAR) );
	sprintf( pszBuf, pszHeader, pszName, pszOtherName );
	more_page( pszBuf, 0 );
}


// go to next row & check for page full
static void _nxtrow( void )
{
	static unsigned int nRow, nColumn;
	int fCtrlC = 0;
	
	// check for colorized directories & scroll w/o ANSI
	if ( nScreenColor != -1 ) {
		
		GetCurPos( &nRow, &nColumn );
		if ( nRow == uScreenRows ) {
			
			// set our own scroll, because the BIOS will use the
			//	 color from the 1st char on the previous line
			Scroll( 0, 0, uScreenRows, uScreenColumns-1, 1, nScreenColor );
			SetCurPos( uScreenRows, 0 );
			
			// kludge for ^C / ^BREAK check
			_asm {
				mov 	ah, 01h
					int 	16h
					jz		NoCtrlC
					or		ax, ax
					jz		GotCtrlC
					cmp 	al, 3
					jne 	NoCtrlC
GotCtrlC:
				mov 	ax, 1
					jmp 	CCDone
NoCtrlC:
				xor 	ax, ax
CCDone:
				mov 	fCtrlC, ax
			}
			if ( fCtrlC )
				crlf();
		} else
			crlf();
	} else
		crlf();
	
	_page_break();
}


// pause if we're at the end of a page & /P was specified
void _page_break( void )
{
	if ( ++uDirRow == gnPageLength ) {
		
		uDirRow = 0;

		// make sure STDOUT hasn't been redirected
		if ( QueryIsConsole( STDOUT ) == 0 )
			return;
		
		qputs( PAUSE_PAGE_PROMPT );
		if ( GetKeystroke( EDIT_NO_ECHO | EDIT_SCROLL_CONSOLE | EDIT_ECHO_CRLF ) == ESC )
			BreakOut();
	}
}



#define TOTAL_ATTRIBUTES 6


// get filename colors
void ColorizeDirectory( DIR_ENTRY _huge *files, unsigned int uEntries, int fDefaultColor )
{
	register unsigned int i, n;
	unsigned int nColorEntries, uSize;
	int nRow, nColumn;
	int j, nLength, nFG, nBG;
	TCHAR _far *lpszExt, _far *pszColorDir, szBuffer[32];
	CDIR _far *cdir = 0L;
	
	// check for no colors requested or STDOUT redirected
	if (( fConsole == 0 ) || ( gfCTTY) || ( glDirFlags & DIRFLAGS_NO_COLOR))
		return;
		
		// if no COLORDIR variable, was ColorDir defined in 4DOS.INI?
		if ((( pszColorDir = get_variable( COLORDIR )) == 0L ) && ( gpIniptr->DirColor != INI_EMPTYSTR))
			pszColorDir = (TCHAR _far *)( gpIniptr->StrData + gpIniptr->DirColor );

		if ( pszColorDir == 0L )
			return;
		
		// if fDefaultColor = 1, then write a space & reread it to get the default
		//	 background color
		if ( fDefaultColor ) {
			// get current cursor position
			GetCurPos( &nRow, &nColumn );
			qputc( STDOUT, _TEXT(' ') );
			SetCurPos( nRow, nColumn );
		}

		
		GetAtt( (unsigned int *)&nScreenColor, &uSize );
		
		n = ( nScreenColor >> 4 );
		
		// create the structure with the extensions & colors
		for ( nColorEntries = 0, uSize = 0; ( *pszColorDir ); ) {
			
			if (( nColorEntries % ( 1024 / sizeof(CDIR))) == 0 ) {
				uSize += 1024;
				if ((cdir = (CDIR _far *)ReallocMem((char _far *)cdir,(ULONG)uSize )) == 0L )
					return;
			}
			
			// get the next extension in color list
			sscanf_far( pszColorDir, _TEXT(" %29F[^ \t:;] %n"), cdir[nColorEntries].szColorName, &nLength );
			
			// default to white foreground
			nFG = 7;
			nBG = n;
			
			// get the color for this extension
			if (( lpszExt = _fstrchr( pszColorDir, _TEXT(':') )) != 0L ) {
				sscanf_far( ++lpszExt, _TEXT("%30[^;]"), szBuffer );
				ParseColors( szBuffer, &nFG, &nBG );
			}
			
			// add it to the list
			cdir[nColorEntries].sColor = ( nFG + ( nBG << 4 ));
			nColorEntries++;
			
			pszColorDir += nLength;
			
			if (( *pszColorDir == _TEXT(':') ) || ( *pszColorDir == _TEXT(';') )) {
				// skip the color specs & get next extension
				while (( *pszColorDir ) && ( *pszColorDir++ != _TEXT(';') ))
					;
			}
		}
		
		// compare file extension against extensions saved in colordir struct
		for ( i = 0; ( i < uEntries ); i++ ) {
			
			// get filename extension
			// in LFN name, the *LAST* '.' is the extension!
			lpszExt = (( glDirFlags & DIRFLAGS_LFN ) ? files[i].lpszLFN : files[i].szFATName );
			if (( lpszExt = _fstrrchr( lpszExt, _TEXT('.') )) == 0L )
				lpszExt = NULLSTR;
			else
				lpszExt++;
			
			for ( n = 0; ( n < nColorEntries ); n++ ) {
				
				if ( wild_cmp( cdir[n].szColorName, lpszExt, TRUE, TRUE ) != 0 ) {
					
					// check for "colorize by attributes"
					if ( _fstrlen( cdir[n].szColorName ) > 3 ) {
						for ( j = 0; ( j < TOTAL_ATTRIBUTES ); j++ ) {
							if ( _fstricmp( cdir[n].szColorName, colorize_atts[j].pszType ) == 0 ) {
								if ( files[i].uAttribute & colorize_atts[j].uAttribute )
									goto got_dcolor;
								if (( j == 6 ) && ( files[i].uAttribute == 0 ))
									goto got_dcolor;
							}
						}
					}
					
					continue;
				}
got_dcolor:
				files[i].nColor = cdir[n].sColor;
				break;
			}
		}
		
		FreeMem( cdir );
}


#define NUMERIC_SORT 0x1000
#define NO_EXTENSION_SORT 0x10
#define ASCII_SORT 1

// FAR string compare
int PASCAL fstrcmp( TCHAR _far *lpszStr1, TCHAR _far *lpszStr2, int fASCII )
{
	static TCHAR szFmt[] = "%ld%n";
	long ln1, ln2;
	int i;

	while ( *lpszStr1 ) {
		
		// sort numeric strings (i.e., 3 before 13)
		if (( fASCII & NUMERIC_SORT ) && is_unsigned_digit( *lpszStr1 ) && is_unsigned_digit( *lpszStr2 )) {
			
			// kludge for morons who do "if %1# == -# ..."
			if ((( isdigit( *lpszStr1 ) == 0 ) && ( isdigit( lpszStr1[1] ) == 0 )) || (( isdigit( *lpszStr2 ) == 0 ) && (isdigit(lpszStr2[1]) == 0 ))) {
				fASCII &= ~NUMERIC_SORT;
				goto NotNumeric;
			}
			
			sscanf_far( lpszStr1, szFmt, &ln1, &i );
			lpszStr1 += i;
			sscanf_far( lpszStr2, szFmt, &ln2, &i );
			lpszStr2 += i;
			
			if (ln1 != ln2)
				return (( ln1 > ln2 ) ? 1 : -1 );
			
		} else {
NotNumeric:
		if (( i = ( _ctoupper( *lpszStr1 ) - _ctoupper( *lpszStr2 ))) != 0 )
			return i;
		
		if (( fASCII & NO_EXTENSION_SORT ) && ( *lpszStr1 == _TEXT('.') )) {
			if (( _fstrchr( lpszStr1+1, _TEXT('.') ) == NULL ) && ( _fstrchr( lpszStr2+1, _TEXT('.') ) == NULL ))
				return 0;
		}
		
		lpszStr1++;
		lpszStr2++;
		}
	}
	
	return ( _ctoupper( *lpszStr1 ) - _ctoupper( *lpszStr2 ));
}


// compare routine for filename sort
static int _near files_cmp( DIR_ENTRY _far *a, DIR_ENTRY _far *b )
{
	register LPTSTR pszSort;
	register int nError = 0;
	int nSortOrder, nReverse = 0, fASCII = NUMERIC_SORT;
	TCHAR _far *lpszA, _far *lpszB;

	// 'r' sort everything in reverse order
	for ( nSortOrder = 0, pszSort = gszSortSequence; ( *pszSort != _TEXT('\0') ); pszSort++ ) {
		if ( *pszSort == _TEXT('r') ) {
			nSortOrder = 1;
			break;
		}
	}

	for ( pszSort = gszSortSequence; (( *pszSort ) && ( nError == 0 )); pszSort++ ) {

		// check for backwards sort for this single field
		nReverse = 0;
		if ( *pszSort == _TEXT('-') ) {
			nReverse++;
			if ( pszSort[1] )
				pszSort++;
		}

		if ( *pszSort == _TEXT('u') )
			return nError;

		if ( *pszSort == _TEXT('a') ) {

			// don't do a numeric sort
			fASCII = ASCII_SORT;

		} else if ( *pszSort == _TEXT('c') ) {
			// sort in compression order
			nError = (( a->uCompRatio < b->uCompRatio ) ? -1 : ( a->uCompRatio > b->uCompRatio ));

		} else if (( *pszSort == _TEXT('d') ) || ( *pszSort == _TEXT('t') )) {

			// sort by date / time
			if (( a->years > b->years ) || (( a->years == b->years ) && (( a->months > b->months ) || (( a->months == b->months ) && ( a->days > b->days )))))
				nError = 1;
			else if (( a->years < b->years ) || (( a->years == b->years ) && (( a->months < b->months ) || (( a->months == b->months ) && ( a->days < b->days )))))
				nError = -1;
			else {

				unsigned int i, n;

				// kludge for AM/PM support
				i = a->hours;
				n = b->hours;
				if ( a->szAmPm == _TEXT('p') ) {
					if ( a->hours < 12 )
						a->hours += 12;
				} else if (( a->szAmPm == _TEXT('a') ) && ( a->hours == 12 ))
					a->hours = 0;

				if ( b->szAmPm == _TEXT('p') ) {
					if ( b->hours < 12 )
						b->hours += 12;
				} else if (( b->szAmPm == _TEXT('a') ) && ( b->hours == 12 ))
					b->hours = 0;

				if (( a->hours > b->hours ) || (( a->hours == b->hours ) && (( a->minutes > b->minutes ) || (( a->minutes == b->minutes ) && ( a->seconds > b->seconds )))))
					nError = 1;
				else if (( a->hours < b->hours ) || (( a->hours == b->hours ) && (( a->minutes < b->minutes ) || (( a->minutes == b->minutes ) && ( a->seconds < b->seconds )))))
					nError = -1;
				else
					nError = 0;

				a->hours = i;
				b->hours = n;
			}

		} else if ( *pszSort == _TEXT('e') ) {	   // extension sort

			if ( glDirFlags & DIRFLAGS_LFN ) {
				lpszA = a->lpszLFN;
				lpszB = b->lpszLFN;
			} else {
				lpszA = a->szFATName;
				lpszB = b->szFATName;
			}

			// sort current dir (.) & root dir (..) first
			for ( ; ( *lpszA == _TEXT('.') ); lpszA++ )
				;
			for ( ; ( *lpszB == _TEXT('.') ); lpszB++ )
				;

			// on LFN volumes, the extension is the LAST '.'
			if ( glDirFlags & DIRFLAGS_LFN ) {

				if (( lpszA = _fstrrchr( lpszA, _TEXT('.') )) == 0L )
					lpszA = NULLSTR;
				if (( lpszB = _fstrrchr( lpszB, _TEXT('.') )) == 0L )
					lpszB = NULLSTR;

			} else {

				// skip to extension
				for ( ; (( *lpszA != _TEXT('\0') ) && (*lpszA != _TEXT('.') )); lpszA++)
					;
				for ( ; (( *lpszB != _TEXT('\0') ) && (*lpszB != _TEXT('.') )); lpszB++)
					;
			}

			nError = fstrcmp( lpszA, lpszB, fASCII );

		} else if (( *pszSort == _TEXT('s') ) || ( *pszSort == _TEXT('z') )) {

			// sort by filesize
			nError = (( a->ulFileSize < b->ulFileSize ) ? -1 : ( a->ulFileSize > b->ulFileSize ));

		} else if ( *pszSort == _TEXT('i') )				// sort by description (ID)
			nError = fstrcmp( a->lpszFileID, b->lpszFileID, fASCII );

		else if ( *pszSort == _TEXT('n') ) {

			// sort by filename (not including extensions)
			if ( glDirFlags & DIRFLAGS_LFN )
				nError = fstrcmp( a->lpszLFN, b->lpszLFN, fASCII | NO_EXTENSION_SORT );
			else
				nError = fstrcmp( a->szFATName, b->szFATName, fASCII | NO_EXTENSION_SORT );

		} else if ( *pszSort == _TEXT('g') ) {
			// sort directories together
			nError = (( b->uAttribute & _A_SUBDIR ) - ( a->uAttribute & _A_SUBDIR ));
		}

		// reverse order for this one field?
		if ( nReverse )
			nError = -nError;
	}

	if ( nError == 0 ) {

		// sort directories first
		if (( nError = (( b->uAttribute & _A_SUBDIR) - ( a->uAttribute & _A_SUBDIR ))) == 0 ) {
			if ( glDirFlags & DIRFLAGS_LFN )
				nError = fstrcmp( a->lpszLFN, b->lpszLFN, fASCII );
			else
				nError = fstrcmp( a->szFATName, b->szFATName, fASCII );

			// reverse order?
			if ( nReverse )
				nError = -nError;
		}
	}

	return (( nSortOrder ) ? -nError : nError );
}


// do a Shell sort on the directory (much smaller & less stack than Quicksort)
static void _near ssort( char _huge *lpBase, unsigned int uEntries )
{
	register unsigned int i, uGap;
	char _huge *lpPtr1, _huge *lpPtr2;
	char szTmp[ sizeof(DIR_ENTRY) ];
	long j;
	
	for ( uGap = ( uEntries >> 1 ); ( uGap > 0 ); uGap >>= 1) {
		
		for ( i = uGap; ( i < uEntries ); i++ ) {
			
			for ( j = ( i - uGap ); ( j >= 0L ); j -= uGap ) {
				
				lpPtr1 = lpPtr2 = lpBase;
				lpPtr1 += ( j * sizeof( DIR_ENTRY));
				lpPtr2 += (( j + uGap ) * sizeof( DIR_ENTRY));
				
				if ( files_cmp(( DIR_ENTRY _far *)lpPtr1,( DIR_ENTRY _far *)lpPtr2 ) <= 0 )
					break;
				
				// swap the two records
				_fmemmove( szTmp, lpPtr1, sizeof( DIR_ENTRY) );
				_fmemmove( lpPtr1, lpPtr2, sizeof( DIR_ENTRY) );
				_fmemmove( lpPtr2, szTmp, sizeof( DIR_ENTRY) );
			}
		}
	}
}


// Read the directory and set the pointer to the saved directory array
//		attrib - search attribute
//		pszArg - filename/directory to search
//		hptr - pointer to address of FILES array
//		entries - pointer to number of entries in FILES
//		nFlags: 0 = no descriptions
//				1 = get descriptions
//				2 = don't save anything except name
int PASCAL SearchDirectory( long nAttribute, LPTSTR pszArg, DIR_ENTRY _huge **hptr, register unsigned int *puEntries, RANGES *paRanges, int nFlags )
{
	register unsigned int nFVal;
	unsigned long size = 0L;
	DIR_ENTRY _huge *files;
	LPTSTR pszSavedArg;
	FILESEARCH dir;
	int nReturn, fCompressed = -1;
	ULONG UnCompSec = 0L, CompSec = 0L;
	LPTSTR pszEndCompPath;
	char szCompPath[MAXFILENAME];
	jmp_buf saved_env;
	int iHSize = 0, cchLength;
	ULONG ulLFNSize = 0L;
	unsigned char _huge *hpLFNNames = 0L;	 // pointer inside LFN names segment
	unsigned char _huge *hpLFNStart = 0L;

	files = *hptr;

#if _DOS && defined(__BETA)
	if ( gpIniptr->INIDebug & INI_DB_DIRMEM )
		printf( "Searching %s:\r\n", pszArg );
#endif

	// create new var so we don't overwrite original argument
	pszSavedArg = pszArg;

	// if pszArg has a ';', it must be an include list
	if ( strchr( pszArg, _TEXT(';') ) != NULL ) {
		pszArg = (LPTSTR)_alloca( ( strlen( pszSavedArg ) + 1 ) * sizeof(TCHAR) );
		strcpy( pszArg, pszSavedArg );
	}

	// copy date/time range info
	memmove( &(dir.aRanges), paRanges, sizeof(RANGES ) );

	for ( nFVal = FIND_FIRST; ; nFVal = FIND_NEXT ) {

		// if error, no entries & no recursion, display error & bomb
		if ( find_file( nFVal, pszArg, ( nAttribute | FIND_NO_CASE | FIND_NO_ERRORS | FIND_NO_FILELISTS ), &dir, NULL ) == NULL ) {

			if (( *puEntries == 0 ) && (( glDirFlags & DIRFLAGS_RECURSE) == 0 ))
				error( ERROR_FILE_NOT_FOUND, pszSavedArg );

			// clean up Doublespace stuff
			if ( fCompressed > 0 ) {
				CompDriveCleanup( &CDriveData );
				// restore the old "env"
				memmove( cv.env, saved_env, sizeof(saved_env) );
			}
			break;
		}

		// set up for compression if requested
		if (( nFVal == FIND_FIRST ) && ( glDirFlags & ( DIRFLAGS_COMPRESSION | DIRFLAGS_ALLOCATED )) && (( nAttribute & FIND_ONLY_DIRS ) == 0 )) {

			if (( nReturn = CompDriveSetup( *pszArg, (int)(( glDirFlags & DIRFLAGS_HOST_COMPRESSION ) != 0 ), &CDriveData )) != 0 ) {

				if ( nReturn > 0 )
					return ( error( nReturn, pszArg ));

				// compression info requested, but not there
				fCompressed = 0;

			} else {

				// save path for Win95 compression
				strcpy(szCompPath, path_part( pszArg ));
				pszEndCompPath = strend( szCompPath );

				// save the old "env" (^C destination)
				memmove( saved_env, cv.env, sizeof(saved_env) );

				// drive is compressed
				fCompressed = 1;

				if ( setjmp( cv.env ) == -1 ) {
					CompDriveCleanup( &CDriveData );
					longjmp( saved_env, -1 );
				}
			}
		}

		// save the full name before it gets truncated (DIR /C /Z)
		if (( fCompressed > 0 ) && ( dir.ulSize != 0L ) && (( dir.attrib & _A_SUBDIR ) == 0 ))
			strcpy( pszEndCompPath, dir.szFileName );

		// if a /B or /H, ignore "." and ".."
		if ((( glDirFlags & DIRFLAGS_NAMES_ONLY ) || ( glDirFlags & DIRFLAGS_NO_DOTS )) && ( QueryIsDotName( dir.szFileName )))
			continue;

		// allocate the temporary memory blocks
		if (( *puEntries % DIR_ENTRY_BLOCK ) == 0 ) {
			size += (sizeof( DIR_ENTRY ) * DIR_ENTRY_BLOCK);
			ReallocDir( &files, size - (sizeof(DIR_ENTRY) * DIR_ENTRY_BLOCK), size, *puEntries, &hpLFNStart, ulLFNSize, ulLFNSize, 0 );
#ifdef __BETA
			if (gpIniptr->INIDebug & INI_DB_DIRMEM)
				printf("  DIR: %d files (%lub) at %x, %lub LFNs at %x, %lub free\r\n", (unsigned int)(size / sizeof(DIR_ENTRY)), size, SELECTOROF(files), ulLFNSize, SELECTOROF(hpLFNStart), ((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
		}

		// set original pointer to allow cleanup on ^C
		if (( *hptr = files ) == 0L ) {

			if ( fCompressed > 0 ) {
				CompDriveCleanup( &CDriveData );
				// restore the old "env"
				memmove( cv.env, saved_env, sizeof(saved_env) );
			}

			FreeDir( files, hpLFNStart );

			return ( error( ERROR_NOT_ENOUGH_MEMORY, NULL ));
		}

		// initialize LFN names
		files[ *puEntries ].lpszLFNBase = 0L;

		// include the trailing _TEXT('\0') in the count (for NT)
		cchLength = strlen( dir.szFileName ) + 1;

		// since LFN partitions can have long filenames, we need
		//	 to store them in a different segment for 4DOS
		if ( glDirFlags & DIRFLAGS_LFN ) {

			// Do we need to allocate more memory for filenames?
			if (( iHSize -= ( cchLength * sizeof(TCHAR)) ) <= 0 ) {

				char _huge *old_hptr = (char _huge *)files[0].lpszLFNBase;

				iHSize += (DIR_LFN_BYTES - 1);
				ulLFNSize += DIR_LFN_BYTES;

				ReallocDir( &files, size, size, *puEntries, &hpLFNStart, ulLFNSize - DIR_LFN_BYTES, ulLFNSize, 1 );
				*hptr = files;
#ifdef __BETA
				if ( gpIniptr->INIDebug & INI_DB_DIRMEM )
					printf("  LFN: %d files (%lub) at %x, %lub LFNs at %x, %lub free\r\n", (unsigned int)(size / sizeof(DIR_ENTRY)), size, SELECTOROF(files), ulLFNSize, SELECTOROF(hpLFNStart), ((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
				if (( files[0].lpszLFNBase = hpLFNStart) == 0L ) {
					FreeDir( files, files[0].lpszLFNBase );

					return ( error( ERROR_NOT_ENOUGH_MEMORY, NULL ));
				}

				if ( *puEntries == 0 )
					hpLFNStart = hpLFNNames = files[0].lpszLFNBase;

				// Adjust LFN pointer for any shift in LFN block
				else
					hpLFNNames += (hpLFNStart - old_hptr);
			}

			dir.szFileName[ cchLength ] = _TEXT('\0');

			// kludge for /E and /L support on LFN/NTFS drives
			if ( glDirFlags & DIRFLAGS_LOWER_CASE )
				strlwr( dir.szFileName );
			else if ( glDirFlags & DIRFLAGS_UPPER_CASE )
				strupr( dir.szFileName );

			files[ *puEntries ].lpszLFN = (TCHAR _far *)_fmemmove( hpLFNNames, (char _far *)dir.szFileName, cchLength * sizeof(TCHAR) );
			hpLFNNames += cchLength;

			files[ *puEntries ].szFATName[0] = _TEXT('\0');
			if ( glDirFlags & DIRFLAGS_LFN_TO_FAT ) {

				// if filename more than 12 chars (FAT maximum),
				//	 append a ^P
				if ( cchLength > 13 )
					dir.szFileName[11] = (TCHAR)gchRightArrow;

				cchLength = 13;
				dir.szFileName[ 12 ] = _TEXT('\0');
				_fstrcpy( files[*puEntries].szFATName, dir.szFileName );
			}

			// save the alternate filename into the FAT area
			if ( glDirFlags & DIRFLAGS_NT_ALT_NAME ) {

				// kludge for /E and /L support on LFN/NTFS drives
				if (( glDirFlags & DIRFLAGS_LOWER_CASE ) && ( dir.szAlternateFileName[0] ))
					strlwr( dir.szAlternateFileName );				
				if (( glDirFlags & DIRFLAGS_UPPER_CASE ) && ( dir.szAlternateFileName[0] ))
					strupr( dir.szAlternateFileName );

				dir.szAlternateFileName[12] = _TEXT('\0');
				if ( dir.szAlternateFileName[0] )
					_fstrcpy( files[*puEntries].szFATName, dir.szAlternateFileName );
				else {
					if (( uDirColumns > 1 ) || ( glDirFlags & ( DIRFLAGS_LFN_TO_FAT | DIRFLAGS_NAMES_ONLY | DIRFLAGS_FULLNAME ))) {
						// no alt name, and LFN too long -- disable the flag!
						if ( cchLength > 13 )
							glDirFlags &= ~DIRFLAGS_NT_ALT_NAME;
						else
							_fstrcpy( files[*puEntries].szFATName, dir.szFileName );
					}
				}

				if ( uDirColumns > 1 )
					cchLength = 13;
			}

			// get max column width
			if (( cchLength - 1 ) > (int)nMaxFilename )
				nMaxFilename = cchLength - 1;

		} else {

			// no default case conversion for NT!
			if (( glDirFlags & DIRFLAGS_LOWER_CASE ) || ((( glDirFlags & DIRFLAGS_UPPER_CASE) == 0 ) && (( dir.attrib & _A_SUBDIR ) == 0 ) && ( gpIniptr->Upper == 0 )))
				strlwr( dir.szFileName );

			// save the name into the FAT area
			_fmemmove( files[ *puEntries ].szFATName, dir.szFileName, cchLength * sizeof(TCHAR) );
		}

		// kludge around (another!) Netware bug with subdirectory sizes
		files[ *puEntries ].ulFileSize = (( dir.attrib & _A_SUBDIR ) ? 0L : dir.ulSize );
		files[ *puEntries ].uAttribute = dir.attrib;

		if ( nFlags & 2 )
			goto NextDirEntry;

		if ( dir.fLFN ) {
			if ( gnDirTimeField == 1 )
				FileTimeToDOSTime( &(dir.ftLastAccessTime ), &(dir.ft.wr_time ), &(dir.fd.wr_date ) );
			else if ( gnDirTimeField == 2 )
				FileTimeToDOSTime( &(dir.ftCreationTime ), &(dir.ft.wr_time ), &(dir.fd.wr_date ) );
		}

		// save the date & time
		files[*puEntries].years = dir.fd.file_date.years + 80;
		files[*puEntries].months = dir.fd.file_date.months;
		files[*puEntries].days = dir.fd.file_date.days;
		files[*puEntries].seconds = dir.ft.file_time.seconds;
		files[*puEntries].minutes = dir.ft.file_time.minutes;
		files[*puEntries].hours = dir.ft.file_time.hours;

		files[*puEntries].uMarked = 0;
		files[*puEntries].nColor = -1;
		files[ *puEntries ].lpszFileID = NULLSTR;
		files[ *puEntries ].lCompressedFileSize = (unsigned long)0xFFFFFFFFL;

		// get compression information if requested
		if ( fCompressed > 0 ) {

			// default ratio
			files[*puEntries].uCompRatio = 0;

			if (( dir.ulSize != 0L ) && (( dir.attrib & _A_SUBDIR ) == 0 )) {

				if ( CompData( &CDriveData, &dir, szCompPath, &UnCompSec, &CompSec ) == 0 ) {
					files[ *puEntries ].lCompressedFileSize = ( CompSec * 512L );
					files[*puEntries].uCompRatio = (unsigned int)(((( 100 * UnCompSec ) / CompSec ) + 5 ) / 10 );
					Add32To64( &ulTotUnCompSec, UnCompSec );
					Add32To64( &ulTotCompSec, CompSec );
				}
			}
		}

		// time - check for 12-hour format
		if ( gaCountryInfo.fsTimeFmt == 0 ) {

			if ( files[ *puEntries].hours >= 12 ) {
				files[ *puEntries ].hours -= 12;
				files[ *puEntries ].szAmPm = _TEXT('p');
			} else
				files[ *puEntries ].szAmPm = _TEXT('a');

			if ( files[ *puEntries ].hours == 0 )
				files[ *puEntries ].hours = 12;

		} else
			files[ *puEntries ].szAmPm = _TEXT(' ');

NextDirEntry:
		(*puEntries)++;
	}

	// shrink the LFN block
	if (( glDirFlags & DIRFLAGS_LFN ) && ( hpLFNNames != 0L )) {

		ReallocDir( &files, size, size, *puEntries, &hpLFNStart, ulLFNSize, (unsigned long)( hpLFNNames - hpLFNStart ) + 2, 1);
#ifdef __BETA
		if (gpIniptr->INIDebug & INI_DB_DIRMEM)
			printf("  LFS: %d files (%lub) at %x, %lub LFNs at %x, %lub free\r\n", 
			(unsigned int)(size / sizeof(DIR_ENTRY)), size, SELECTOROF(files), 
			(unsigned long)( hpLFNNames - hpLFNStart ) + 2, SELECTOROF(hpLFNStart), 
			((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
		*hptr = files;
		files[0].lpszLFNBase = hpLFNStart;
		ulLFNSize = (unsigned long)( hpLFNNames - hpLFNStart ) + 2;
	}

	if ( *puEntries > 0 ) {

		// get the file descriptions?
		if ( nFlags & 1 ) {

			// don't get descriptions on LFN partitions - they
			//	 already have long filenames
			if ((( glDirFlags & DIRFLAGS_COMPRESSION ) == 0 ) && ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LFN_TO_FAT ) || ( aRanges.pszDescription ) || ( strchr( gszSortSequence, _TEXT('i')) != NULL ))) {

				// did we run out of memory getting descriptions?
				if (( *hptr = GetDescriptions( *puEntries, files, size, ulLFNSize, pszArg )) == 0L )
					return ERROR_EXIT;

				files = *hptr;

				// only match specified descriptions?
				if ( paRanges->pszDescription ) {

					for ( nFVal = 0; ( nFVal < *puEntries ); ) {

						// remove non-matching entries
						if ( wild_cmp( paRanges->pszDescription, files[nFVal].lpszFileID, FALSE, TRUE ) != 0 ) {

							(*puEntries)--;
							if ( nFVal >= *puEntries )
								break;

							_fmemmove( &(files[nFVal].uAttribute), &(files[nFVal+1].uAttribute), sizeof(DIR_ENTRY) * ( *puEntries - nFVal) );

						} else
							nFVal++;
					}
				}
			}
		}

		// unless /Ou (unsorted) specified, sort the dir array
		if ( gszSortSequence[0] != _TEXT('u') )
			ssort( (char _huge *)*hptr, *puEntries );

		if ( nFlags & 4 ) {
			// check for file colorization (SET COLORDIR=...)
			ColorizeDirectory( files, *puEntries, 1 );
		}

		// restore base pointer to LFN names
		files[0].lpszLFNBase = hpLFNStart;

		// COMMAND.COM formatting requested?
		if (( glDirFlags & DIRFLAGS_JUSTIFY ) && (( nAttribute & FIND_ONLY_DIRS ) == 0 )) {

			register unsigned int i;
			register int k;

			for ( i = 0; ( i < *puEntries ); i++ ) {

				// patch filename for DOS-type justification
				k = files[i].szFATName[0];
				if (( k != _TEXT('\0') ) && ( k != _TEXT('.') )) {

					for ( k = _fstrlen( files[i].szFATName ) - 1; ( k > 0 ); k-- ) {

						if ( files[i].szFATName[k] == _TEXT('.') ) {

							// move extension & pad w/blanks
							_fmemmove( files[i].szFATName+8, files[i].szFATName+k, 4 * sizeof(TCHAR) );

							for ( ; ( k < 9 ); k++ )
								files[i].szFATName[k] = _TEXT(' ');
							break;
						}
					}
				}

				// add termination for LFN names
				files[i].szFATName[12] = _TEXT('\0');
			}
		}
	}

	return 0;
}


// read the description file (if any)
//	 entries - number of files to match with description
//	 files	 - huge pointer to directory array
//	 pszName - directory/file name
#if _DOS
DIR_ENTRY _huge * _near GetDescriptions( unsigned int uEntries, DIR_ENTRY _huge *files, unsigned long ulSize, unsigned long ulLFNSize, LPTSTR pszName )
#else
DIR_ENTRY * GetDescriptions( unsigned int uEntries, DIR_ENTRY *files, LPTSTR pszName )
#endif
{
	register unsigned int i, n;
	char _huge *hpLFNStart = files[0].lpszLFNBase;

	unsigned int uColumns, uLength;
	int hFH, nEditFlags = EDIT_NO_PIPES;
	long lFilesSize, lSize = 0L;
	TCHAR *pszArg, *pszDescription, *pszSaveFileName = NULL, *pszSaveDescription = NULL, szLine[1024];
	TCHAR _huge *hptr, _huge *fptr;

	// read the descriptions ( from path if necessary)
	insert_path( szLine, DESCRIPTION_FILE, pszName );
	
	if (( hFH = _sopen( szLine, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 )
		return files;
	HoldSignals();
	
	// get size before descriptions are added
	lSize = lFilesSize = (( (unsigned long)uEntries + 1L ) * sizeof(DIR_ENTRY) );
	
	hptr = (TCHAR _huge *)&( files[ uEntries ] );
	
	// get screen width, unless STDOUT has been redirected
	if ( fConsole == 0 )
		uColumns = 0x7FFF;
	
	else {
		uColumns = uScreenColumns - 40;
		
		// adjust for /T (display attributes)
		if ( glDirFlags & DIRFLAGS_SHOW_ATTS )
			uColumns -= 6;
	}

ReadAgain:

	while ( getline( hFH, szLine, 1023, nEditFlags ) > 0 ) {
		
		pszDescription = scan( szLine, _TEXT(" \t,"), _TEXT("\"") );
		if (( pszDescription != BADQUOTES) && ( *pszDescription )) {
			
			// strip trailing '.' (3rd party bug)
			if (( szLine[0] != _TEXT('.') ) && ( pszDescription[-1] == _TEXT('.') ))
				pszDescription[-1] = _TEXT('\0');
			
			*pszDescription++ = _TEXT('\0');
			if ( szLine[0] == _TEXT('"') )
				StripQuotes( szLine );
			
			for ( i = 0; ( i < uEntries ); i++ ) {
				
				// already have a description?
				if (( files[i].lpszFileID[0] ) && ( szLine[0] != _TEXT('.') ))
					continue;
				
				if ( pszSaveFileName ) {
					strcpy( szLine, pszSaveFileName );
					pszDescription = pszSaveDescription;
					pszSaveFileName = NULL;
				}
				
				pszSaveDescription = NULL;
				fptr = (( glDirFlags & DIRFLAGS_LFN) ? files[i].lpszLFN : files[i].szFATName );

				// get descriptions for "." and ".."
				if (( szLine[0] != _TEXT('.') ) && ( *fptr == _TEXT('.') ) && (( fptr[1] == _TEXT('\0') ) || ( fptr[1] == _TEXT('.') )) && ( files[i].uAttribute & 0x10 )) {
					
					if (( pszArg = path_part( pszName )) != NULL ) {
						
						strip_trailing( pszArg+3, _TEXT("\\") );
						if ( fptr[1] == _TEXT('.') ) {
							// get description for ".."
							if (( pszArg = path_part( pszArg )) != NULL )
								strip_trailing( pszArg+3, _TEXT("\\") );
						}

						// look in parent directory for a description
						if ( pszArg ) {
							pszSaveFileName = strcpy( (LPTSTR)_alloca( ( strlen( szLine ) + 1 ) * sizeof(TCHAR) ), szLine );
							pszSaveDescription = strcpy( (LPTSTR)_alloca( ( strlen( pszDescription ) + 1 ) * sizeof(TCHAR) ), pszDescription );
							strcpy( szLine, pszArg );
							process_descriptions( szLine, szLine, DESCRIPTION_READ );
							pszDescription = szLine;
							goto GotMatch;
						}
					}
				}

				// try for a match
				n = _fstricmp( fptr, (TCHAR _far *)szLine );
				if (( n != 0 ) && ( glDirFlags & DIRFLAGS_LFN )) {
					// failed; try for a match on the SFN
					n = _fstricmp( files[i].szFATName, (TCHAR _far *)szLine );
				}
				
				if ( n == 0 ) {
GotMatch:
				// if uLength > screen width, truncate w/right arrow
				uLength = strlen( pszDescription );
				if (( glDirFlags & DIRFLAGS_TRUNCATE_DESCRIPTION ) && ( uLength > uColumns )) {
					pszDescription[ uColumns-1 ] = gchRightArrow;
					pszDescription[ uColumns ] = _TEXT('\0');
					uLength = uColumns;
				}
				
				lSize += (unsigned long)(uLength + 1) * sizeof(TCHAR);
				if ( lSize >= lFilesSize ) {
					
					lFilesSize += 8192L;

					// save current pointer to files (so we can see if it moves)
					fptr = (char _huge *)files;
					
					ReallocDir( &files, ulSize, lFilesSize, uEntries, &hpLFNStart, ulLFNSize, ulLFNSize, 0 );
#ifdef __BETA
					if (gpIniptr->INIDebug & INI_DB_DIRMEM)
						printf("  DES: %d files (%lub) at %x, %lub LFNs at %x, %lub free\r\n", (unsigned int)(lFilesSize / sizeof(DIR_ENTRY)), lFilesSize, SELECTOROF(files), ulLFNSize, SELECTOROF(hpLFNStart), ((unsigned long)(unsigned int)ServCtrl( SERV_AVAIL, 0 ) << 4));
#endif
					files[0].lpszLFNBase = hpLFNStart;
					ulSize = lFilesSize;
					if ( files == 0L ) {
							error( ERROR_NOT_ENOUGH_MEMORY, NULL );
							break;
						}
						
						// adjust far pointers if realloc moved "files"
						if (( fptr != (char _huge *)files ) && ( i != 0 )) {
							// read the descriptions again!
							RewindFile( hFH );
							hptr = (char _huge *)&( files[ uEntries ] );
							lSize = (( (unsigned long)uEntries + 1L ) * sizeof(DIR_ENTRY) );
							for ( i = 0; ( i < uEntries ); i++ )
								files[i].lpszFileID = 0L;
							goto ReadAgain;
						}
					}
					
					// copy the description
					for ( files[ i ].lpszFileID = hptr; (( *pszDescription ) && ( *pszDescription != (TCHAR)0x04 ) && ( *pszDescription != (TCHAR)0x1A )); )
						*hptr++ = *pszDescription++;
					*hptr++ = _TEXT('\0');
					
					if ( pszSaveFileName == NULL )
						break;
				}
			}
		}
	}
	
	_close( hFH );
	EnableSignals();
	
	return files;
}
