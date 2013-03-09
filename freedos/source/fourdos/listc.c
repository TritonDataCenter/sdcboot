

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


// LISTC.C - file listing routines for 4xxx family
//	 Copyright (c) 1993 - 2002	Rex C. Conn  All rights reserved

int ListEntry( LPTSTR );
static int _fastcall _list( LPTSTR );
static void _near ListUpdateScreen( void );
static void _near _fastcall list_wait( LPTSTR );
static void _near clear_header( void );
static void _near wWriteListStr( int, int, POPWINDOWPTR, LPTSTR );

static int  fDirtyHeader;		// if != 0, redisplay header
static long fSearchFlags = 0;


#pragma alloc_text( _TEXT, ListEntry, List_Cmd )

typedef struct {
	union {
		unsigned short wr_time;
		struct {
			unsigned seconds : 5;
			unsigned minutes : 6;
			unsigned hours : 5;
		} file_time;
	} ft;
	union {
		unsigned short wr_date;
		struct {
			unsigned days : 5;
			unsigned months : 4;
			unsigned years : 7;
		} file_date;
	} fd;
} DOSFILEDATE;



// dummy routine for far entry to list_cmd
int ListEntry( LPTSTR szFilename )
{
	return ( List_Cmd( szFilename ));
}


// edit or display a file with vertical & horizontal scrolling & text searching
int _near List_Cmd( LPTSTR pszCmdLine )
{
	int nFVal, nReturn = 0, argc;
	TCHAR szSource[MAXFILENAME+1], szFileName[MAXFILENAME+1], *pszArg;
	FILESEARCH dir;

	memset( &dir, '\0', sizeof(FILESEARCH) );
	
	// check for and remove switches
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;
	
	// check for /T"search string"
	GetMultiCharSwitch( pszCmdLine, _TEXT("T"), szSource, 255 );
	if ( szSource[0] == _TEXT('"') )
		sscanf( szSource+1, _TEXT("%79[^\"]"), szListFindWhat );
	else if ( szSource[0] )
		sprintf( szListFindWhat, FMT_PREC_STR, 79, szSource );
	
	if ( GetSwitches( pszCmdLine, _TEXT("*HIRSWX"), &lListFlags, 0 ) != 0 )
		return ( Usage( LIST_USAGE ));
	
	if ( szSource[0] )
		lListFlags |= LIST_SEARCH;
	
	// check for pipe to LIST w/o explicit /S switch
	if ( first_arg( pszCmdLine ) == NULL ) {

		if ( _isatty( STDIN ) == 0 )

				lListFlags |= LIST_STDIN;
			else if (( lListFlags & LIST_STDIN ) == 0 )
				return ( Usage( LIST_USAGE ));
	}
	
	// initialize buffers & globals
	
	if ( ListInit() )
		return ERROR_EXIT;
	nCurrent = nStart = 0;
	
	// ^C handling
	if ( setjmp( cv.env ) == -1 ) {
list_abort:
		FindClose( dir.hdir );
		Cls_Cmd( NULL );
		nReturn = CTRLC;
		goto list_bye;
	}
	
RestartFileSearch:
	for ( argc = 0; ; argc++ ) {
		
		// break if at end of arg list, & not listing STDIN
		if (( pszArg = ntharg( pszCmdLine, argc )) == NULL ) {
			if (( lListFlags & LIST_STDIN ) == 0 )
				break;
		} else
			strcpy( szSource, pszArg );
		
		for ( nFVal = FIND_FIRST; ; ) {
			
			szClip[0] = _TEXT('\0');
			
			// if not reading from STDIN, get the next matching file
			if (( lListFlags & LIST_STDIN ) == 0 ) {
				
				// qualify filename
				if ( nFVal == FIND_FIRST ) {
					mkfname( szSource, 0 );
					if ( is_dir( szSource ))
						mkdirname( szSource, WILD_FILE );
				}

				if ( stricmp( szSource, CLIP ) == 0 ) {
					
					RedirToClip( szClip, 99 );
					if ( CopyFromClipboard( szClip ) != 0 )
						break;
					strcpy( szFileName, szClip );
					
				} else if ( QueryIsPipeName( szSource )) {
					
					// only look for pipe once
					if ( nFVal == FIND_NEXT )
						break;
					copy_filename( szFileName, szSource );
					
				} else if ( find_file( nFVal, szSource, ( FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | 0x07), &dir, szFileName ) == NULL ) {
					nReturn = (( nFVal == FIND_FIRST ) ? ERROR_EXIT : 0 );
					break;
					
				} else if ( nStart < nCurrent ) {
					nStart++;
					nFVal = FIND_NEXT;
					continue;
					
				} else if ( dir.ulSize > 0L )
					LFile.lSize = dir.ulSize;
			}
			
			// clear the screen (scrolling the buffer first to save the current screen)

			Cls_Cmd( NULL );
			
			if (( nReturn = _list( szFileName )) == CTRLC )
				goto list_abort;
			
			if ( szClip[0] )
				remove( szClip );
			
			if ( nReturn != 0 )
				break;
			
			SetCurPos( nScreenRows, 0 );
			
			if (( szClip[0] ) || ( lListFlags & LIST_STDIN ))
				break;

			if ( LFile.hHandle > 0 )
				_close( LFile.hHandle );
			LFile.hHandle = -1;
			
			// increment index to current file
			if ( nCurrent < nStart ) {
				FindClose( dir.hdir );
				nStart = 0;
				goto RestartFileSearch;
			} else {
				nFVal = FIND_NEXT;
				nCurrent++;
				nStart++;
			}
		}
		
		// we can only read STDIN once!
		lListFlags &= ~LIST_STDIN;
	}
	
	crlf();
list_bye:
	FreeMem( LFile.lpBufferStart );
	if ( LFile.hHandle > 0 )
		_close( LFile.hHandle );
	LFile.hHandle = -1;
	
	return nReturn;
}


static int _fastcall _list( LPTSTR pszFileName )
{
	register int c, i;
	TCHAR szDescription[512], szHeader[132], szLine[32];
	long lTemp, lRow;
	POPWINDOWPTR wn = NULL;
	FILESEARCH dir;
	
	// get default normal and inverse attributes
	if ( gpIniptr->ListColor != 0 ) {
		SetScrColor( nScreenRows, nScreenColumns, gpIniptr->ListColor );

	}
	
	// set colors
	GetAtt( (unsigned int *)&nNormal, (unsigned int *)&nInverse );
	if ( gpIniptr->ListStatusColor != 0 )
		nInverse = gpIniptr->ListStatusColor;
	
	// flip the first line to inverse video
	clear_header();
	
	// open the file & initialize buffers
	if ( ListOpenFile( pszFileName ))
		return ERROR_EXIT;
	
	// kludge for empty files or pipes
	if ( LFile.lSize == 0L )
		LFile.lSize = 1L;
	
	for ( ; ; ) {
		
		// display header
		if ( fDirtyHeader ) {
			clear_header();
			sprintf( szHeader, LIST_HEADER, fname_part(LFile.szName), gchVerticalBar, gchVerticalBar, gchVerticalBar );
			WriteStrAtt( 0, 0, nInverse, szHeader );
			fDirtyHeader = 0;
		}
		
		// display location within file
		//	(don't use color_printf() so we won't have
		//	problems with windowed sessions)

		i = sprintf( szHeader, LIST_LINE, LFile.nListHorizOffset, LFile.lCurrentLine + gpIniptr->ListRowStart, (int)((( LFile.lViewPtr + 1 ) * 100 ) / LFile.lSize ));

		WriteStrAtt( 0, ( nScreenColumns - i ), nInverse, szHeader );
		SetCurPos( 0, 0 );
		
		if ( lListFlags & LIST_SEARCH ) {
			
			lListFlags &= ~LIST_SEARCH;
			fSearchFlags = 0;
			if ( lListFlags & LIST_REVERSE ) {
				
				c = LIST_FIND_CHAR_REVERSE;
				fSearchFlags |= FFIND_REVERSE_SEARCH;
				
				// goto the last row
				while ( ListMoveLine( 1 ) != 0 )
					;
			} else
				c = LIST_FIND_CHAR;
			
			if ( lListFlags & LIST_NOWILDCARDS )
				fSearchFlags |= FFIND_NOWILDCARDS;
			bListSkipLine = 0;
			goto FindNext;
		}
		
		// get the key from the BIOS, because
		//	 STDIN might be redirected
		
		if ((( c = cvtkey( GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT), MAP_GEN | MAP_LIST)) == (TCHAR)ESC ))
			break;
		
		switch ( c ) {
		case CTRLC:
			return CTRLC;
			
		case CUR_LEFT:
		case CTL_LEFT:
			
			if (( lListFlags & LIST_WRAP ) || ( LFile.nListHorizOffset == 0 ))
				goto bad_key;
			
			if (( LFile.nListHorizOffset -= (( c == CUR_LEFT ) ? 8 : 40 )) < 0 )
				LFile.nListHorizOffset = 0;
			break;
			
		case CUR_RIGHT:
		case CTL_RIGHT:
			
			if (( lListFlags & LIST_WRAP ) || ( LFile.nListHorizOffset >= MAXLISTLINE + 1 ))
				goto bad_key;
			
			if ((LFile.nListHorizOffset += ((c == CUR_RIGHT) ? 8 : 40 )) > MAXLISTLINE + 1 )
				LFile.nListHorizOffset = MAXLISTLINE+1;
			break;
			
		case CUR_UP:
			
			if ( ListMoveLine( -1 ) == 0 )
				goto bad_key;
			
			Scroll(1, 0, nScreenRows, nScreenColumns, -1, nNormal);
			DisplayLine( 1, LFile.lViewPtr );
			continue;
			
		case CUR_DOWN:
			
			if ( ListMoveLine( 1 ) == 0 )
				goto bad_key;
			
			Scroll( 1, 0, nScreenRows, nScreenColumns, 1, nNormal );
			
			// display last line
			lTemp = LFile.lViewPtr;
			lRow = (nScreenRows - 1);
			lTemp += MoveViewPtr( lTemp, &lRow );
			if ( lRow == ( nScreenRows - 1 ))
				DisplayLine( nScreenRows, lTemp );
			continue;
			
		case HOME:
			
			ListHome();
			break;
			
		case END:
			
			// goto the last row
			list_wait( LIST_WAIT );
			while ( ListMoveLine( 1 ) != 0 )
				;

		case PgUp:
			
			// already at TOF?
			if ( LFile.lViewPtr == 0L )
				goto bad_key;
			
			for ( i = 1; (( i < nScreenRows ) && ( ListMoveLine( -1 ) != 0 )); i++ )
				;
			
			break;
			
		case PgDn:
		case SPACE:
			
			if ( ListMoveLine( nScreenRows - 1 ) == 0 )
				goto bad_key;
			
			break;
			
		case F1:	// help

			// don't allow a ^X exit from HELP
			_help( gpInternalName, HELP_NX );

			continue;
			
		case TAB:
			// change tab size
			
			// disable ^C / ^BREAK handling
			HoldSignals();
			
			wn = wOpen( 2, 5, 4, strlen( LIST_TABSIZE ) + 10, nInverse, LIST_TABSIZE_TITLE, NULL );
			wn->nAttrib = nNormal;
			wClear();
			
			wWriteListStr( 0, 1, wn, LIST_TABSIZE );
			egets( szLine, 2, (EDIT_DIALOG | EDIT_BIOS_KEY | EDIT_NO_CRLF | EDIT_DIGITS));
			wRemove( wn );
			if (( i = atoi( szLine )) > 0 )
				TABSIZE = i;
			
			// enable ^C / ^BREAK handling
			EnableSignals();
			break;
			
		case DEL:
			// delete this file
			if (( lListFlags & LIST_STDIN ) == 0 ) {
				
				// disable ^C / ^BREAK handling
				HoldSignals();
				
				wn = wOpen( 2, 5, 4, strlen( LIST_DELETE ) + 10, nInverse, LIST_DELETE_TITLE, NULL );
				wn->nAttrib = nNormal;
				wClear();
				
				wWriteListStr( 0, 1, wn, LIST_DELETE );
				i = GetKeystroke( EDIT_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT );
				wRemove( wn );
				
				// enable ^C / ^BREAK handling
				EnableSignals();
				
				if ( i == (TCHAR)YES_CHAR ) {
					if ( LFile.hHandle > 0 )
						_close( LFile.hHandle );
					LFile.hHandle = -1;
					
					remove( pszFileName );
					return 0;
				}
			}
			break;
			
		case INS:
			// save to a file
			ListSaveFile();
			break;
			
		case LIST_INFO_CHAR:
			{
				unsigned int uSize = 1024;
				TCHAR _far *lpszText, _far *lpszArg;
				int fFSType, fSFN = 1;
				TCHAR szBuf[16];

				DOSFILEDATE laDir, crDir;

				// disable ^C / ^BREAK handling
				HoldSignals();
				
				memset( &dir, '\0', sizeof(FILESEARCH) );
				if (( lListFlags & LIST_STDIN ) == 0 ) {
					if ( find_file( FIND_FIRST, LFile.szName, 0x07 | FIND_CLOSE | FIND_EXACTLY, &dir, NULL ) == NULL ) {
						honk();
						continue;
					}
				}
				
				// display info on the current file
				i = (( lListFlags & LIST_STDIN ) ? 5 : 10 );
				if (( fFSType = ifs_type( LFile.szName )) != FAT )
					i = 11;
				wn = wOpen( 2, 1, i, 77, nInverse, gpInternalName, NULL );
				
				wn->nAttrib = nNormal;
				wClear();
				i = 0;
				
				if ( lListFlags & LIST_STDIN )
					wWriteStrAtt( 0, 1, nNormal, LIST_INFO_PIPE );
				
				else {
					
					szDescription[0] = _TEXT('\0');
					process_descriptions( LFile.szName, szDescription, DESCRIPTION_READ );
					
					lpszText = lpszArg = (TCHAR _far *)AllocMem( &uSize );

					strcpy(szBuf, FormatDate( dir.fd.file_date.months, dir.fd.file_date.days, dir.fd.file_date.years + 80, 0 ));
					
					if (fFSType != FAT) {
						FileTimeToDOSTime( &(dir.ftLastAccessTime), &( laDir.ft.wr_time), &( laDir.fd.wr_date) );
						FileTimeToDOSTime( &(dir.ftCreationTime), &(crDir.ft.wr_time), &(crDir.fd.wr_date) );
						strcpy( szLine, FormatDate( laDir.fd.file_date.months, laDir.fd.file_date.days, laDir.fd.file_date.years + 80, 0 ));
						sprintf_far( lpszText, LIST_INFO_LFN,
							LFile.szName, szDescription, dir.ulSize,
							szBuf,
							dir.ft.file_time.hours, gaCountryInfo.szTimeSeparator[0],dir.ft.file_time.minutes,
							szLine,
							laDir.ft.file_time.hours, gaCountryInfo.szTimeSeparator[0], laDir.ft.file_time.minutes,
							FormatDate( crDir.fd.file_date.months, crDir.fd.file_date.days, crDir.fd.file_date.years + 80, 0 ),
							crDir.ft.file_time.hours, gaCountryInfo.szTimeSeparator[0], crDir.ft.file_time.minutes);
					} else {
						
						sprintf_far( lpszText, LIST_INFO_FAT,
							LFile.szName, szDescription, dir.ulSize,
							szBuf, dir.ft.file_time.hours, 
							gaCountryInfo.szTimeSeparator[0],dir.ft.file_time.minutes );
					}

					// print the text
					for ( ; ( *lpszArg != _TEXT('\0') ); i++ ) {
						sscanf_far( lpszArg, _TEXT("%[^\n]%*c%n"), szHeader, &uSize );

						// allow for long filenames ...
						if (( i == 0 ) && ( strlen( szHeader ) > 73 )) {
							c = szHeader[73];
							szHeader[73] = _TEXT('\0');
							wWriteStrAtt( i++, 1, nNormal, szHeader );
							szHeader[73] = (TCHAR)c;
							wWriteStrAtt( i, 15, nNormal, szHeader+73 );
							fSFN = 0;
						} else

							wWriteStrAtt( i, 1, nNormal, szHeader );
						lpszArg += uSize;
					}
					
					FreeMem( lpszText );
				}
				
				wWriteListStr( i+fSFN, 1, wn, PAUSE_PROMPT );
				GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY );
				
				wRemove( wn );
				
				// enable ^C / ^BREAK handling
				EnableSignals();
				
				continue;
			}
			
		case LIST_GOTO_CHAR:
			
			// goto the specified line / hex offset
			
			// disable ^C / ^BREAK handling
			HoldSignals();
			
			wn = wOpen( 2, 5, 4, strlen( LIST_GOTO_OFFSET ) + 20, nInverse, LIST_GOTO_TITLE, NULL );
			wn->nAttrib = nNormal;
			wClear();
			
			wWriteListStr( 0, 1, wn, (( lListFlags & LIST_HEX) ? LIST_GOTO_OFFSET : LIST_GOTO));
			i = egets( szLine, 10, (EDIT_DIALOG | EDIT_BIOS_KEY | EDIT_NO_CRLF));
			wRemove( wn );
			
			// enable ^C / ^BREAK handling
			EnableSignals();
			
			if ( i == 0 )
				break;
			list_wait( LIST_WAIT );
			
			// if in hex mode, jump to offset 
			if ( lListFlags & LIST_HEX ) {
				strupr( szLine );
				sscanf( szLine, _TEXT("%lx"), &lRow );
				lRow = lRow / 0x10;
			} else if ( sscanf( szLine, FMT_LONG, &lRow ) == 0 )
				continue;
			
			lRow -= gpIniptr->ListRowStart;
			if ( lRow >= 0 ) {
				LFile.lViewPtr = MoveViewPtr( 0L, &lRow );
				LFile.lCurrentLine = lRow;
			} else {
				LFile.lViewPtr += MoveViewPtr( LFile.lViewPtr, &lRow );
				LFile.lCurrentLine += lRow;
			}
			break;
			
		case LIST_HIBIT_CHAR:
			
			// toggle high bit filter
			lListFlags ^= LIST_HIBIT;
			break;
			
		case LIST_WRAP_CHAR:
			
			// toggle line wrap
			lListFlags ^= LIST_WRAP;
			nRightMargin = (( lListFlags & LIST_WRAP ) ? GetScrCols() : MAXLISTLINE );
			
			// recalculate current line
			list_wait( LIST_WAIT );
			
			// get start of line
			LFile.nListHorizOffset = 0;
			
			// line number probably changed, so recompute everything
			LFile.lCurrentLine = ComputeLines( 0L, LFile.lViewPtr );
			LFile.lViewPtr = MoveViewPtr( 0L, &(LFile.lCurrentLine ));
			break;
			
		case LIST_HEX_CHAR:
			
			// toggle hex display
			lListFlags ^= LIST_HEX;
			
			// if hex, reset to previous 16-byte
			//	 boundary
			if ( lListFlags & LIST_HEX ) {

				LFile.lViewPtr -= ( LFile.lViewPtr % 16 );
			} else {
				// if not hex, reset to start of line
				ListMoveLine( 1 );
				ListMoveLine( -1 );
			}
			
			// recalculate current line
			list_wait( LIST_WAIT );
			LFile.lCurrentLine = ComputeLines( 0L, LFile.lViewPtr );
			break;
			
		case LIST_FIND_CHAR:
		case LIST_FIND_CHAR_REVERSE:
			// find first matching string
			bListSkipLine = 0;
			//lint -fallthrough
			
		case LIST_FIND_NEXT_CHAR:
		case LIST_FIND_NEXT_CHAR_REVERSE:
			// find next matching string
			
			if (( c == LIST_FIND_CHAR ) || ( c == LIST_FIND_CHAR_REVERSE )) {
				
				// disable ^C / ^BREAK handling
				HoldSignals();
				
				fSearchFlags = 0;
				wn = wOpen( 2, 1, 4, 75, nInverse, (( c == LIST_FIND_NEXT_CHAR_REVERSE ) ? LIST_FIND_TITLE_REVERSE : LIST_FIND_TITLE ), NULL );
				wn->nAttrib = nNormal;
				wClear();
				
				if ( lListFlags & LIST_HEX ) {
					wWriteListStr( 0, 1, wn, LIST_FIND_HEX );
					if ( GetKeystroke( EDIT_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT ) == YES_CHAR )
						fSearchFlags |= FFIND_HEX_SEARCH;
					wClear();
				}
				
				wWriteListStr( 0, 1, wn, LIST_FIND );
				egets( szListFindWhat, 64, (EDIT_DIALOG | EDIT_BIOS_KEY | EDIT_NO_CRLF));
				wRemove( wn );
				
				// enable ^C / ^BREAK handling
				EnableSignals();
				
			} else {
FindNext:
			// a "Next" has to be from current position
			fSearchFlags &= ~FFIND_TOPSEARCH;
			}
			
			if ( szListFindWhat[0] == _TEXT('\0') )
				continue;
			
			sprintf( szDescription, LIST_FIND_WAIT, szListFindWhat );
			list_wait( szDescription );
			
			// save start position
			lTemp = LFile.lViewPtr;
			lRow = LFile.lCurrentLine;
			
			if (( c == LIST_FIND_CHAR_REVERSE ) || ( c == LIST_FIND_NEXT_CHAR_REVERSE )) {
				// start on the previous line
				fSearchFlags |= FFIND_REVERSE_SEARCH;
			} else {
				fSearchFlags &= ~FFIND_REVERSE_SEARCH;
				// skip the first line (except on /T"xxx")
				if ( bListSkipLine )
					ListMoveLine( 1 );
			}
			
			bListSkipLine = 1;
			if ( SearchFile( fSearchFlags ) != 1 ) {
				ListSetCurrent( lTemp );
				LFile.lViewPtr = lTemp;
				LFile.lCurrentLine = lRow;
			} else
				LFile.fDisplaySearch = (( fSearchFlags & FFIND_CHECK_CASE ) ? 2 : 1 );
			
			break;
			
		case LIST_PRINT_CHAR:
			
			// print the file
			ListPrintFile();
			continue;
			
		case LIST_CONTINUE_CHAR:
		case CTL_PgDn:
			return 0;
			
		case LIST_PREVIOUS_CHAR:
		case CTL_PgUp:
			// previous file
			if ( nCurrent > 0 ) {
				nCurrent--;
				return 0;
			}
			//lint -fallthrough
			
		default:
bad_key:
			honk();
			continue;
		}
		
		// rewrite the display
		ListUpdateScreen();
	}
	
	return 0;
}


// display a "WAIT" message
static void _near _fastcall list_wait( LPTSTR pszPrompt )
{
	// clear the header line
	clear_header();
	
	// display "WAIT" in blinking chars
	WriteStrAtt( 0, (( nScreenColumns / 2 ) - ( strlen( pszPrompt ) / 2 )), ( nInverse | 0x80 ), pszPrompt );
}


// clear the LIST header line to inverse blanks
static void _near clear_header( void )
{
	fDirtyHeader = 1;
	Scroll( 0, 0, 0, nScreenColumns, 0, nInverse );
}


// redraw the screen (except for the status line)
static void _near ListUpdateScreen( void )
{
	register int nRow;
	long lTemp = LFile.lViewPtr;
	
	Scroll( 1, 0, nScreenRows, nScreenColumns, 0, nNormal );
	
	// if no name yet, we're still initializing!
	if ( LFile.szName[0] == _TEXT('\0') )
		return;
	
	for ( nRow = 1; ( nRow <= nScreenRows ); nRow++ )
		lTemp += DisplayLine( nRow, lTemp );

	LFile.fDisplaySearch = 0;
}


// print the file on the default printer
void ListPrintFile( void )
{
	register int i, n;
	int c, nRows, nBytesPrinted, nFH;
	long lTemp;
	POPWINDOWPTR wn;
	TCHAR szBuffer[MAXLISTLINE+1];
	int fKBHit;
	
	// disable ^C / ^BREAK handling
	HoldSignals();
	
	wn = wOpen( 2, 1, 4, strlen( LIST_PRINTING ) + 8, nInverse, LIST_PRINT_TITLE, NULL );
	wn->nAttrib = nNormal;
	wClear();
	sprintf( szBuffer, LIST_QUERY_PRINT, LIST_PRINT_FILE_CHAR, LIST_PRINT_PAGE_CHAR );
	wWriteListStr( 0, 1, wn, szBuffer );
	
	if ((( c = GetKeystroke( EDIT_ECHO | EDIT_UC_SHIFT | EDIT_BIOS_KEY )) == LIST_PRINT_FILE_CHAR ) || ( c == LIST_PRINT_PAGE_CHAR )) {
		
		// save start position
		lTemp = LFile.lViewPtr;
		
		// display "Printing ..."
		wWriteListStr( 0, 1, wn, LIST_PRINTING );
	
		if (( nFH = _sopen((( gpIniptr->Printer != INI_EMPTYSTR ) ? gpIniptr->StrData + gpIniptr->Printer : _TEXT("LPT1") ), (_O_BINARY | _O_WRONLY | _O_CREAT), _SH_DENYNO, _S_IWRITE | _S_IREAD )) >= 0 ) {

			if ( setjmp( cv.env ) == -1 ) {
				_close( nFH );
				return;
			}

			// reset to beginning of file
			if ( c == LIST_PRINT_FILE_CHAR )
				ListSetCurrent( 0L );
			else {
				nRows = GetScrRows();
				ListSetCurrent( LFile.lViewPtr );
			}
			
			// print the header (filename, date & time)
			qprintf( nFH, _TEXT("%s   %s  %s\r\n\r\n"), LFile.szName, gdate( 0 ), gtime( gaCountryInfo.fsTimeFmt ) );

			do {
				// abort printing if a key is hit

				// kbhit() in DOS tries to read from STDIN, which screws
				//	 up a LIST /S pipe
				_asm {
					mov 	ah, 1
					int 	16h
					mov 	fKBHit, 1
					jnz 	KBHDone
					mov 	fKBHit, 0
KBHDone:
				}
				if ( fKBHit && ( GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY ) == ESC ))
					break;

				i = FormatLine( szBuffer, MAXLISTLINE, LFile.lViewPtr, &nBytesPrinted, TRUE );
				LFile.lViewPtr += 16;
				
				// replace 0-31 with "."
				if ( lListFlags & LIST_HEX ) {
					for ( n = 0; ( n < i ); n++ ) {
						if ( szBuffer[n] < 32 )
							szBuffer[n] = _TEXT('.');
					}
				}
				
				if (( c == LIST_PRINT_PAGE_CHAR ) && ( nRows-- <= 0 ))
					break;
				
			} while (( nBytesPrinted > 0 ) && ( qprintf( nFH, _TEXT("%.*s\r\n"), i, szBuffer ) > 0 ));
			
			// print a formfeed
			qputc( nFH, _TEXT('\014') );
			_close( nFH );
			
			// restore start position
			LFile.lViewPtr = lTemp;
			ListSetCurrent( LFile.lViewPtr );
			
		} else
			honk();
	}
	
	wRemove( wn );
	
	// enable ^C / ^BREAK handling
	EnableSignals();
}



// write the file (or pipe) to another file
static void _near ListSaveFile( void )
{
	int i, nFH, nMode;
	long lTemp;
	POPWINDOWPTR wn;
	TCHAR szBuffer[ MAXLISTLINE+1 ];
	
	// disable ^C / ^BREAK handling
	HoldSignals();
	
	wn = wOpen( 2, 1, 4, GetScrCols() - 2, nInverse, LIST_SAVE_TITLE, NULL );
	wn->nAttrib = nNormal;
	wClear();
	wWriteListStr( 0, 1, wn, LIST_QUERY_SAVE );
	egets( szBuffer, MAXFILENAME, EDIT_DATA | EDIT_BIOS_KEY );
	
	if ( szBuffer[0] ) {
		
		// save start position
		lTemp = LFile.lViewPtr;
		
		nMode = _O_BINARY;
		
		if (( nFH = _sopen( szBuffer, ( nMode | _O_WRONLY | _O_CREAT | _O_TRUNC ), _SH_DENYNO, _S_IWRITE | _S_IREAD )) >= 0 ) {
			
			// reset to beginning of file

				ListSetCurrent( 0L );
			
			do {
				
				for ( i = 0; ( i < MAXLISTLINE ); i++ ) {
					
					// don't call GetNextChar unless absolutely necessary
					if ( LFile.lpCurrent == LFile.lpEOF )
						break;

					szBuffer[i] = (TCHAR)GetNextChar();
				}

				szBuffer[i] = _TEXT('\0');
				
			} while (( i > 0 ) && ( wwrite( nFH, szBuffer, i ) > 0 ));
			
			_close( nFH );
			
			// restore start position
			LFile.lViewPtr = lTemp;
			ListSetCurrent( LFile.lViewPtr );
			
		} else
			honk();
	}
	
	wRemove( wn );
	
	// enable ^C / ^BREAK handling
	EnableSignals();
}


// write a string to a popup window & position cursor at end
static void _near wWriteListStr( int nRow, int nColumn, POPWINDOWPTR wn, LPTSTR pszString )
{
	wWriteStrAtt( nRow, nColumn, wn->nAttrib, pszString );
	wSetCurPos( nRow, nColumn + strlen( pszString ));
}
