

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


// SELECTC.C - character-mode SELECT (for 4DOS & 4NT)
//   Copyright (c) 1993 - 2002 Rex C. Conn

static void _fastcall ShowSelectHeader( register SELECT_ARGS * );
static int _fastcall getmark( SELECT_ARGS *sel );
static void _near dirprint( SELECT_ARGS *, int, unsigned int, unsigned int );

#pragma alloc_text( _TEXT, Select_Cmd )

// select the files for a command
int _near Select_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	int nArg, nReturn = 0;
	long lMode = 0x10;
	SELECT_ARGS sel;
	TCHAR szBuffer[MAXLINESIZ], szScroll[4];

	strcpy( szScroll, _TEXT("/S") );

	// check for SELECT options
	if (( pszArg = pszCmdLine ) == NULL )
		return ( Usage( SELECT_USAGE ));

	if (( nReturn = InitSelect( &sel, pszArg, &lMode )) != 0 )
		return nReturn;

	if (( *pszArg == _TEXT('\0') ) || (( sel.pszFileVar = strpbrk( pszArg, _TEXT("([") )) == NULL ) ||
	    (( sel.pszCmdTail = scan( sel.pszFileVar, (( *sel.pszFileVar == _TEXT('(') ) ? _TEXT(")") : _TEXT("]")), _TEXT("`[") )) == BADQUOTES ) ||
	   ( sel.pszCmdTail[0] == _TEXT('\0') ))
		return ( Usage( SELECT_USAGE ));

	sel.cOpenParen = *sel.pszFileVar;
	sel.cCloseParen = sel.pszCmdTail[0];

	// rub out the '(' & ')' (or [])
	*sel.pszFileVar++ = _TEXT('\0');
	*sel.pszCmdTail++ = _TEXT('\0');

	// save the command on the stack
	sel.pszCmdStart = (LPTSTR)_alloca( ( strlen( pszArg ) + 1 ) * sizeof(TCHAR) );
	strcpy( sel.pszCmdStart, pszArg );
	pszArg = (LPTSTR)_alloca( ( strlen( sel.pszCmdTail ) + 1 ) * sizeof(TCHAR) );
	sel.pszCmdTail = strcpy( pszArg, sel.pszCmdTail );

	// do any variable expansion on the file spec
	if ( var_expand( strcpy( szBuffer, sel.pszFileVar ), 1 ) != 0 )
		return ERROR_EXIT;

	sel.pszFileVar = (LPTSTR)_alloca( ( strlen( szBuffer ) + 1 ) * sizeof(TCHAR) );
	strcpy( sel.pszFileVar, szBuffer );

	for ( nArg = 0; (( sel.pszArg = ntharg( sel.pszFileVar, nArg )) != NULL) && ( nReturn != CTRLC ); nArg++ ) {

		sel.sfiles = 0L;		// initialize pointer to file array
		sel.uEntries = 0;

		if ( setjmp( cv.env ) == -1 ) {
			dir_free( sel.sfiles );
			Cls_Cmd( NULL );
			nReturn = CTRLC;
			goto select_bye;
		}

		copy_filename( szBuffer, sel.pszArg );

		// force \*.* onto a directory name
		AddWildcardIfDirectory( szBuffer );

		// save path part of name
		if (( pszArg = path_part( szBuffer )) == NULL)
			pszArg = NULLSTR;
		strcpy( sel.szPath, pszArg );

		if ( mkfname( szBuffer, 0 ) == NULL) {
			nReturn = ERROR_EXIT;
			goto select_bye;
		}

		// check to see if it's an LFN partition
		if ( ifs_type( szBuffer ) != 0 )
			glDirFlags |= DIRFLAGS_LFN;

		// search the directory for the specified file(s)
		if ( SearchDirectory( (lMode | FIND_RANGE | FIND_EXCLUDE), szBuffer, (DIR_ENTRY _huge **)&( sel.sfiles), &(sel.uEntries), &(sel.aSelRanges), 5 ) != 0 ) {
			nReturn = ERROR_EXIT;
			goto select_bye;
		}

		if ( sel.uEntries > 0 ) {

			// clear the screen

			Cls_Cmd( NULL );

			// get default normal and inverse attributes
			if ( gpIniptr->SelectColor != 0 )
				SetScrColor( GetScrRows(), GetScrCols(), gpIniptr->SelectColor );

			// get default screen colors
			GetAtt( &(sel.uNormal), &(sel.uInverse) );
			if ( gpIniptr->SelectStatusColor != 0 )
				sel.uInverse = gpIniptr->SelectStatusColor;

			// display SELECT header
			ShowSelectHeader( &sel );

			// get filename colors
			ColorizeDirectory( sel.sfiles, sel.uEntries, 0 );

			sel.nDirLine = sel.nDirFirst = 0;		// beginning of page

			// get the selection list
			if ( getmark( &sel ) == -1 )
				sel.uMarked = 0;
			Cls_Cmd( NULL );

			if ( sel.uLength >= MAXLINESIZ ) {
				nReturn = error( ERROR_4DOS_COMMAND_TOO_LONG, NULL );
				sel.uMarked = 0;
			}

			// process the marked files in order
			nReturn = ProcessSelect( &sel, szBuffer );
		}

		dir_free( sel.sfiles);
	}

select_bye:

	return nReturn;
}


static void _fastcall ShowSelectHeader( register SELECT_ARGS *sel )
{
	// display SELECT header
	Scroll( 0, 0, 0, GetScrCols()-1, 0, sel->uInverse );
	SetCurPos( 0, 0 );
	color_printf( sel->uInverse, SELECT_HEADER, gchVerticalBar, gchVerticalBar, gchVerticalBar, gchVerticalBar );

	// display the command & the file spec(s)
	Scroll( 1, 0, 1, GetScrCols()-1, 0, sel->uNormal );
	SetCurPos( 1, 0 );
	color_printf( sel->uNormal, _TEXT("%s%c%s%c%s"), sel->pszCmdStart, sel->cOpenParen, sel->pszFileVar, sel->cCloseParen, sel->pszCmdTail );
}


// SELECT the files to mark
static int _fastcall getmark( register SELECT_ARGS *sel )
{
	extern int ListEntry( LPTSTR );
	register unsigned int c, i, j, n;
	int nSearch = 0;
	int nRow, nColumn, nButton;
	TCHAR szFilename[MAXFILENAME+1];
	TCHAR szSearch[32];
	jmp_buf saved_env;

	// display first page
	dirprint( sel, 1, sel->nDirLine, sel->uInverse );

	szSearch[0] = '\0';

	MouseReset();

	SetMousePosition( 2, 0 );
	MouseCursorOn();

	for ( sel->uMarked = 0; ; ) {

		c = GetKeystroke( EDIT_NO_ECHO | EDIT_UC_SHIFT | EDIT_MOUSE_BUTTON );
		switch ( c ) {
		case LEFT_MOUSE_BUTTON:

			// left mouse button
			while (1) {

				// loop until button is not pressed
				GetMousePosition( &nRow, &nColumn, &nButton );
				if (( nButton & 1 ) == 0 ) {

					if ( nRow < 2 )
						break;

					// set current line to normal
					dirprint( sel, -1, sel->nDirLine, sel->uNormal );
					sel->nDirLine = sel->nDirFirst + ( nRow - 2 );

					// move scroll bar down, beep if at end
					if ( sel->nDirLine >= ((int)sel->uEntries - 1 ))
						sel->nDirLine = (int)sel->uEntries - 1;

					goto MarkFile;
				}

				_asm {
					mov       ax,1680h
					int       2Fh		; give up Win timeslice
				}

			}
			break;

		case ESC:
			MouseCursorOff();
			return -1;

		case CR:
		case LF:
			// allow a single selection using the RETURN key
			if ( sel->uMarked == 0 )
				sel->sfiles[sel->nDirLine].uMarked = ++(sel->uMarked);
			MouseCursorOff();
			return sel->uMarked;

		case SELECT_LIST:
			// if LFN names with whitespace, quote them
			if ( glDirFlags & DIRFLAGS_LFN ) {
				sprintf( szFilename, _TEXT("%s%Fs"), sel->szPath, sel->sfiles[sel->nDirLine].lpszLFN );
				AddQuotes( szFilename );
			} else
				sprintf( szFilename, _TEXT("%s%Fs"), sel->szPath, sel->sfiles[ sel->nDirLine ].szFATName );

			// save the old "env" (^C destination)
			memmove( saved_env, cv.env, sizeof(saved_env) );
			ListEntry( szFilename );

			// restore the old "env"
			memmove( cv.env, saved_env, sizeof(saved_env) );

			// redraw SELECT screen
			ShowSelectHeader( sel );
			goto redraw_page;

		case '+':		// mark a file
		case SPACE:
MarkFile:
			if ( sel->sfiles[ sel->nDirLine ].uMarked == 0 ) {
				if (( sel->fFlags & SELECT_ONLY_ONE ) && ( sel->uMarkedFiles > 0 )) {
					honk();
					break;
				}
				sel->sfiles[ sel->nDirLine ].uMarked = ++(sel->uMarked);
			} else

		case '-':		// unmark a file
			sel->sfiles[sel->nDirLine].uMarked = 0;
			// lint -fallthrough

		case CUR_UP:
		case CUR_DOWN:

			if ( c != LEFT_MOUSE_BUTTON ) {

				// switch current line to normal
				dirprint( sel, -1, sel->nDirLine, sel->uNormal );
				if ( c != CUR_UP ) {

					// move scroll bar down, beep if at end
					if ( sel->nDirLine >= ((int)sel->uEntries - 1 )) {
						if (c == CUR_DOWN)
							honk();
					} else if (++(sel->nDirLine) > sel->nDirLast) {
						(sel->nDirFirst)++;
						Scroll( 2, 0, GetScrRows(), GetScrCols()-1, 1, sel->uNormal );
					}

				} else {

					// move scroll bar up
					if ( sel->nDirLine == 0 )
						honk();		// already at top
					else if (--(sel->nDirLine) < sel->nDirFirst) {
						(sel->nDirFirst)--;
						Scroll( 2, 0, GetScrRows(), GetScrCols()-1, -1, sel->uNormal );
					}
				}
			}

			// new line in inverse video
			dirprint( sel, -1, sel->nDirLine, sel->uInverse );
			break;

		case HOME:
			sel->nLeftMargin = sel->nDirFirst = sel->nDirLine = 0;
			// lint -fallthrough

		case END:
		case PgUp:
		case PgDn:

			i = GetScrRows() - 1;

			// change the directory page
			if ( c == END ) {

				sel->nDirLine = sel->uEntries - 1;
				if (( sel->nDirFirst = (( sel->nDirLine + 1 ) - i)) < 0 )
					sel->nDirFirst = 0;


			} else if ( c == PgDn ) {

				if ((int)( sel->nDirFirst + i) < (int)(sel->uEntries))
					sel->nDirFirst += i;
				if ( sel->nDirLine < sel->nDirFirst )
					sel->nDirLine = sel->nDirFirst;
				else if ((int)( sel->nDirLine + i ) >= (int)(sel->uEntries))
					sel->nDirLine = sel->uEntries - 1;

			} else if ( c == PgUp ) {

				if (( sel->nDirFirst -= i) < 0 )
					sel->nDirFirst = 0;
				if ( sel->nDirLine > sel->nDirFirst )
					sel->nDirLine = sel->nDirFirst;
				else if ((int)( sel->nDirLine - i ) < 0 )
					sel->nDirLine = 0;
			}
			goto redraw_page;

		case '*':		// reverse all current marks (no dirs)
			if ( sel->fFlags & SELECT_ONLY_ONE )
				break;
		case '/':		// unmark everything

			for ( i = 0; (( c < 0x80 ) && ( i < sel->uEntries )); i++ ) {

				if ( c == _TEXT('*') ) {
					if ( sel->sfiles[i].uMarked == 0 ) {
						if (( sel->sfiles[i].uAttribute & _A_SUBDIR) == 0 )
							sel->sfiles[i].uMarked = ++sel->uMarked;
					} else
						sel->sfiles[i].uMarked = 0;
				} else if (c == _TEXT('/'))
					sel->sfiles[i].uMarked = sel->uMarked = 0;
			}
			// lint -fallthrough

		case CUR_LEFT:
		case CUR_RIGHT:

			if ( c == CUR_LEFT ) {
				if (( sel->nLeftMargin -= 8 ) < 0 )
					sel->nLeftMargin = 0;
			} else if ( c == CUR_RIGHT ) {
				if ( sel->nLeftMargin < 512 )
					sel->nLeftMargin += 8;
			}

			// draw a new page
redraw_page:
			dirprint( sel, 1, sel->nDirLine, sel->uNormal );
			dirprint( sel, -1, sel->nDirLine, sel->uInverse );
			break;

		case F1:	// help

			_help( gpInternalName, HELP_NX );

			break;

		default:
			// try to match a string

			if ( c < FBIT ) {

				// search for entry beginning with szSearch
ResetSearch:
				if ( nSearch >= 32 )
					nSearch = 0;
				szSearch[ nSearch++ ] = (TCHAR)c;
				szSearch[ nSearch ] = _TEXT('\0');
				for ( j = 0, i = sel->nDirLine; ( i < sel->uEntries ); i++ ) {
StartLoop:

					if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LFN_TO_FAT ))
						n = _fstrnicmp( szSearch, sel->sfiles[i].szFATName, strlen( szSearch ));
					else
						n = _fstrnicmp( szSearch, sel->sfiles[i].lpszLFN, strlen( szSearch ));

					if ( n == 0 ) {
						// switch current line to uNormal
						dirprint( sel, -1, sel->nDirLine, sel->uNormal );
						sel->nDirLine = i;

						// draw page
						if (( sel->nDirLine < sel->nDirFirst ) || ( sel->nDirLine > sel->nDirLast )) {
							sel->nDirFirst = sel->nDirLine;
							dirprint( sel, 1, sel->nDirLine, sel->uNormal );
							dirprint( sel, -1, sel->nDirLine, sel->uInverse );
						} else
							dirprint( sel, -1, sel->nDirLine, sel->uInverse );
						goto NoHonk;
					}

					if ( i == ( sel->uEntries - 1 )) {
						if ( j == 0 ) {
							// try again from the beginning
							j = 1;
							i = 0;
							goto StartLoop;
						} else if ( j == 1 ) {
							if ( nSearch == 1 ) {
								nSearch = 0;
								break;
							}
							nSearch = 0;
							goto ResetSearch;
						}
					}
				}
			}

			// invalid input
			honk();
NoHonk:
			break;
		}
	}
}


// print directory for SELECT
//	page = directory page
//	line = line to print
//	attr = attribute to use for printing
static void _near dirprint( register SELECT_ARGS *sel, int nPage, register unsigned int uLine, unsigned int uAttribute )
{
	register int i;
	int n, nColumns, nRows;
	unsigned int uCurrentRow, uSavedRow, uLast, uColor, nOffset;
	TCHAR szBuffer[256];
	PCH lpszName;

	sel->uMarkedFiles = 0;

	uCurrentRow = uSavedRow = 2;		// set start row
	nRows = GetScrRows();
	uLast = nRows - 1;

	nColumns = GetScrCols();

	if ( nPage > 0 ) {			// print entire page
		// clear screen
		Scroll( 2, 0, nRows, nColumns-1, 0, sel->uNormal );
		uLine = sel->nDirFirst;
	} else				// just print 1 line
		uCurrentRow += ( uLine - sel->nDirFirst );

	if (( sel->nDirLast = ( sel->nDirFirst + ( uLast - 1 ))) >= (int)sel->uEntries )
		sel->nDirLast = sel->uEntries - 1;

	// print page numbre
	i = (( sel->nDirLine + 1 ) / uLast );
	if (( sel->nDirLine + 1 ) % uLast )
		i++;

	n = ( sel->uEntries / uLast );
	if ( sel->uEntries % uLast )
		n++;

	sprintf( szBuffer, SELECT_PAGE_COUNT, i, n );
	WriteStrAtt( 0, nColumns - ( strlen(SELECT_PAGE_COUNT) - 1 ), sel->uInverse, szBuffer );

	// initialize the SELECT line length
	sel->uLength = strlen( sel->pszCmdStart ) + strlen( sel->pszCmdTail );

	if ( sel->cOpenParen == _TEXT('(') )
		sel->uLength += strlen( sel->szPath ) + 13;

	for ( uLast = 0, sel->lTotalSize = 0L; ( uLast < sel->uEntries ); uLast++ ) {

		if ( sel->sfiles[uLast].uMarked > 0 ) {

			sel->uMarkedFiles++;
			if ( sel->cOpenParen == _TEXT('[') ) {

				// LFN partitions have filename in "pszLFN"
				lpszName = (( glDirFlags & DIRFLAGS_LFN ) ? sel->sfiles[uLast].lpszLFN : sel->sfiles[uLast].szFATName);
				i = _fstrlen( lpszName );
				sel->uLength += strlen( sel->szPath ) + i + 1;
			}

			sel->lTotalSize += (LONG)(( sel->sfiles[uLast].ulFileSize + 1023L ) / 1024L );
		}
	}

	// display the line length
	sprintf( szBuffer, _TEXT("%4u"), sel->uLength );
	WriteStrAtt( 0, 0, sel->uInverse, szBuffer );

	// display the number of files marked & the size (in Kb)
	sprintf( szBuffer, MARKED_FILES, sel->uMarkedFiles, sel->lTotalSize );
	WriteStrAtt( 1, nColumns - strlen( szBuffer ), sel->uNormal, szBuffer );

	// last row to print
	uLast = (( nPage > 0 ) ? sel->nDirLast : uLine );

	for ( nOffset = 0; ( uLine <= uLast ); uLine++, uCurrentRow++ ) {

		// check for colorization
		// get background color if none specified
		if (( sel->sfiles[uLine].nColor & 0x70 ) == 0 )
			sel->sfiles[uLine].nColor |= ( sel->uNormal & 0x70 );

		uColor = ((( uAttribute == sel->uNormal ) && ( sel->sfiles[ uLine ].nColor != -1 )) ? sel->sfiles[ uLine ].nColor : uAttribute );

		if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LFN_TO_FAT )) {

			// display the FAT filename
			nOffset = sprintf( szBuffer, _TEXT("%c%-12Fs"), (( sel->sfiles[uLine].uMarked > 0 ) ? gchRightArrow : _TEXT(' ')), sel->sfiles[uLine].szFATName );

		} else {
			// display the LFN filename (max of first 45 chars
			//   displayable on an 80-columns display)
			nOffset = sprintf( szBuffer, _TEXT("%c%-*.*Fs"), (( sel->sfiles[uLine].uMarked > 0 ) ? gchRightArrow : _TEXT(' ')), (nColumns-35), (nColumns-35), sel->sfiles[uLine].lpszLFN );
		}

		if ((( glDirFlags & DIRFLAGS_LFN ) == 0 ) || ( glDirFlags & DIRFLAGS_LFN_TO_FAT )) {

			nOffset += sprintf( szBuffer+nOffset, ( sel->sfiles[uLine].uAttribute & _A_SUBDIR) ? DIR_LABEL : "%9lu",sel->sfiles[uLine].ulFileSize );

			nOffset += sprintf( szBuffer+nOffset, (( gaCountryInfo.fsTimeFmt == 0 ) ? _TEXT("  %s %2u%c%02u%c") : _TEXT("  %s  %2u%c%02u")),
				FormatDate( sel->sfiles[ uLine ].months, sel->sfiles[uLine].days,sel->sfiles[uLine].years, 0 ),
				sel->sfiles[uLine].hours, gaCountryInfo.szTimeSeparator[0], sel->sfiles[uLine].minutes, sel->sfiles[uLine].szAmPm );

			if ( glDirFlags & DIRFLAGS_COMPRESSION ) {

				if ( sel->sfiles[ uLine ].uCompRatio != 0 ) {
					if (glDirFlags & DIRFLAGS_PERCENT_COMPRESSION)
						sprintf( szBuffer+nOffset, DIR_PERCENT_RATIO,(1000 / sel->sfiles[ uLine ].uCompRatio ));
					else
						sprintf( szBuffer+nOffset, DIR_RATIO,( sel->sfiles[ uLine ].uCompRatio / 10 ), ( sel->sfiles[uLine].uCompRatio % 10 ));
				}

			} else {

				n = _fstrlen( sel->sfiles[ uLine ].lpszFileID );
				i = (( n > sel->nLeftMargin ) ? sel->nLeftMargin : n );
				if (( n - i ) > ( nColumns - 40 ))
					sprintf( szBuffer+nOffset, _TEXT(" %.*Fs%c"),( nColumns - 41 ), sel->sfiles[uLine].lpszFileID+i, gchRightArrow );
				else
					sprintf( szBuffer+nOffset, _TEXT(" %Fs"), sel->sfiles[ uLine ].lpszFileID+i );
			}

			WriteStrAtt( uCurrentRow, 0, uColor, szBuffer );
			SetLineColor( uCurrentRow, 39, (nColumns - 39), uColor );

		} else {

			nOffset += sprintf( szBuffer+nOffset, ( sel->sfiles[ uLine ].uAttribute & _A_SUBDIR ) ? LFN_DIR_LABEL : _TEXT("%15Lu"), sel->sfiles[ uLine ].ulFileSize );

			sprintf( szBuffer+nOffset, _TEXT("  %s  %2u%c%02u%c"), FormatDate( sel->sfiles[ uLine ].months, sel->sfiles[ uLine ].days, sel->sfiles[ uLine ].years, 0 ),
				sel->sfiles[uLine].hours, gaCountryInfo.szTimeSeparator[0], sel->sfiles[ uLine ].minutes, sel->sfiles[ uLine ].szAmPm );
			WriteStrAtt( uCurrentRow, 0, uColor, szBuffer );
		}

		if ( uAttribute == sel->uInverse )
			uSavedRow = uCurrentRow;

		uAttribute = sel->uNormal;
	}

	// move the cursor to the currently selected row (this is
	//   for blind users who need the cursor to move)
	SetCurPos( uSavedRow, 0 );
}
