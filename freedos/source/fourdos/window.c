

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


// WINDOW.C - popup windows for 4xxx family
//   Copyright (c) 1991 - 2002 Rex C. Conn

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "4all.h"


#define SORT_LIST 1
#define NO_CHANGE 2
#define CDD_COLORS 4
#define BATCH_DEBUG 0x10

static void _fastcall ScrollBar( int, int );
static TCHAR _far * _near wSelect( TCHAR _far * _far *, int, int, LPTSTR );

static POPWINDOWPTR aWin[4];		// array of active windows
static POPWINDOWPTR pwWin = NULL;		// pointer to current window
static int nCurrentWindow = -1;
int gnPopExitKey;
int gnPopupSelection;
static unsigned int uDefault, uNormal, uInverse;
static TCHAR szSearch[32];
TCHAR *pszBatchDebugLine;


// Open a window with the specified coordinates.
POPWINDOWPTR wOpen( register int nTop, int nLeft, int nBottom, int nRight, int nAttribute, TCHAR *pszTitle, TCHAR *pszBottomTitle )
{
	register POPWINDOWPTR pwWindow;
	unsigned int uWindowSize;
	int nWidth;
	char _far * lpScreen;

	pwWindow = (POPWINDOWPTR)malloc( sizeof(POPWINDOW) );

	pwWindow->fShadow = ( nRight < (int)GetScrCols() - 1 );

	// get window size, including drop shadow (1 row & 2 columns)
	nWidth = (( nRight + (( pwWindow->fShadow ) ? 3 : 1 )) - nLeft );
	uWindowSize = ((( nBottom + pwWindow->fShadow + 1 ) - nTop ) * nWidth ) + 2;

	uWindowSize *= 2;	// char + attribute
	if (( pwWindow->lpScreenSave = AllocMem( &uWindowSize )) == 0L ) {
		free( pwWindow );
		return NULL;
	}

	// store parameters in window control block
	pwWindow->nTop = nTop;
	pwWindow->nLeft = nLeft;
	pwWindow->nBottom = nBottom;
	pwWindow->nRight = nRight;

	pwWindow->nAttrib = nAttribute;
	pwWindow->nHOffset = 0;
	pwWindow->nCursorRow = pwWindow->nCursorColumn = 0;

	// save old cursor position
	GetCurPos( &(pwWindow->nOldRow), &(pwWindow->nOldColumn) );

	// Copy existing text where window will be placed to the save buffer.
	for ( lpScreen = pwWindow->lpScreenSave; ( nTop <= ( nBottom + pwWindow->fShadow )); nTop++ ) {

		ReadCellStr( lpScreen, nWidth, nTop, nLeft );

		lpScreen += nWidth * 2;		// char + attribute
	}

	// draw the window border (with a shadow) and clear the text area
	_box( pwWindow->nTop, nLeft, nBottom, nRight, 1, pwWindow->nAttrib, pwWindow->nAttrib, pwWindow->fShadow, 0 );

	if ( strlen( pszTitle ) >= ( nRight - ( nLeft + 1 )))
		pszTitle[ nRight - ( nLeft + 1 ) ] = _TEXT('\0');

	WriteStrAtt( pwWindow->nTop, nLeft + (( ++nRight - ( nLeft + strlen( pszTitle ))) / 2 ), pwWindow->nAttrib, pszTitle );

	if ( pszBottomTitle != NULL ) {
		if ( strlen( pszBottomTitle ) >= ( nRight - ( nLeft + 1 )))
			pszBottomTitle[ nRight - ( nLeft + 1 ) ] = _TEXT('\0');
		WriteStrAtt( pwWindow->nBottom, nLeft + (( ++nRight - ( nLeft + strlen( pszBottomTitle ))) / 2 ), pwWindow->nAttrib, pszBottomTitle );
	}

	// set active window
	pwWin = aWin[ ++nCurrentWindow ] = pwWindow;

	// set the window cursor
	wSetCurPos( 0, 0 );

	// remove blink & intensity, and ROR 4 to get inverse
	nAttribute &= 0x77;

	if (( pwWindow->nInverse = gpIniptr->LBBar ) == 0 )
		pwWindow->nInverse = ( nAttribute >> 4 ) + (( nAttribute << 4 ) & 0xFF );

	return pwWindow;
}


// Remove the specified window.  Must be the "top" window if overlapping
//   windows are used; tiled windows can be removed randomly.
void _fastcall wRemove( register POPWINDOWPTR pwWindow )
{
	register unsigned int nWidth;
	char _far *lpScreen;

	if ( pwWindow == NULL )
		return;

	nWidth = (( pwWindow->nRight + (( pwWindow->fShadow ) ? 3 : 1 )) - pwWindow->nLeft );

	// repaint the saved screen image
	for ( lpScreen = pwWindow->lpScreenSave; ( pwWindow->nTop <= ( pwWindow->nBottom + pwWindow->fShadow )); pwWindow->nTop++ ) {

		WriteCellStr( lpScreen, nWidth, pwWindow->nTop, pwWindow->nLeft );

		lpScreen += nWidth * 2;
	}

	// reset cursor to original location
	SetCurPos( pwWindow->nOldRow, pwWindow->nOldColumn );

	// free dynamic storage
	FreeMem( pwWindow->lpScreenSave );
	free( pwWindow );
	if ( --nCurrentWindow >= 0 )
		pwWin = aWin[ nCurrentWindow ];
	else
		pwWin = NULL;
}


// clear the "active" part of a window and home internal text cursor
void wClear( void )
{
	Scroll( pwWin->nTop + 1, pwWin->nLeft + 1, pwWin->nBottom - 1, pwWin->nRight - 1, 0, pwWin->nAttrib );
	wSetCurPos( 0, 0 );
}


// set the active window cursor location (0,0 based)
void _fastcall wSetCurPos( int nRow, int nColumn )
{
	pwWin->nCursorRow = nRow;
	pwWin->nCursorColumn = nColumn;
	SetCurPos( pwWin->nTop + nRow + 1, pwWin->nLeft + nColumn + 1 );
}


// write a string to a window, truncating at right margin
void wWriteStrAtt( int nRow, int nColumn, int nAttribute, TCHAR _far *lpszLine )
{
	register int nLength;
	int nCurrentColumn;
	TCHAR _far *lpszStartLine = lpszLine;

	for ( nCurrentColumn = pwWin->nLeft + 1, nLength = 0; ( *lpszLine != _TEXT('\0') ); lpszLine++ ) {

		incr_column( *lpszLine, &nCurrentColumn );

		// check right & left margins
		if ( nCurrentColumn >= ( pwWin->nRight + pwWin->nHOffset ))
			break;

		if ( nCurrentColumn > ( pwWin->nLeft + pwWin->nHOffset + 1 ))
			nLength++;
		else
			lpszStartLine++;
	}

	SetCurPos( pwWin->nTop + nRow + 1, pwWin->nLeft + nColumn + 1 );
	color_printf( nAttribute, FMT_FAR_PREC_STR, nLength, lpszStartLine );
}


// Display a scroll bar along the right border of the active window.
static void _fastcall ScrollBar( int nPosition, int nOutOf )
{
	register int i, nHigh;

	nHigh = ( pwWin->nBottom - pwWin->nTop ) - 3;

	WriteChrAtt( pwWin->nTop+1, pwWin->nRight, pwWin->nInverse, gchUpArrow );
	WriteChrAtt( pwWin->nBottom-1, pwWin->nRight, pwWin->nInverse, gchDownArrow );

	for ( i = 0; ( i < nHigh ) ; i++ )
		WriteChrAtt( pwWin->nTop + 2 + i, pwWin->nRight, pwWin->nAttrib, gchDimBlock);

	// position the "thumbwheel"
	if ( nPosition > 1 ) {
		if (( i = ((( nHigh * nPosition ) / nOutOf ) + ((( nHigh * nPosition) % nOutOf ) != 0 ))) == 1 )
			i++;
		if (( i == nHigh ) && ( nPosition < nOutOf ))
			i--;
	} else
		i = 1;
	pwWin->nThumb = pwWin->nTop + i + 1;

	WriteChrAtt( pwWin->nThumb, pwWin->nRight, pwWin->nAttrib, gchBlock );
}


static void _near ScrollBack( TCHAR _far * _far *lppList, int nFirst )
{
	Scroll( pwWin->nTop+1, pwWin->nLeft+1, pwWin->nBottom-1, pwWin->nRight-1, -1, pwWin->nAttrib );
	wWriteStrAtt( 0, 0, pwWin->nAttrib, lppList[nFirst-1] );
}


static void _near ScrollForward( TCHAR _far * _far *lppList, int nFirst, int nLast )
{
	Scroll( pwWin->nTop+1, pwWin->nLeft+1, pwWin->nBottom-1, pwWin->nRight-1, 1, pwWin->nAttrib);
	wWriteStrAtt(( nLast - nFirst ), 0, pwWin->nAttrib, lppList[nLast-1] );
}


// display a selection list
static TCHAR _far * _near wSelect( TCHAR _far * _far *lppList, int nCount, register int nCurrent, LPTSTR pszChars )
{
	extern int PASCAL _line(int, int, int, int, int, int, int);
	register int i;
	int nHeight, j, nIndent, nSearch;
	int nTurnOff, nWidth, nFirst, nLast, nRows, nBar = 0, nLoop = 0;
	unsigned int uKey;
	TCHAR _far *pszListEntry;
	TCHAR szSearchDisp[32];

	gnPopExitKey = 0;
	nIndent = pwWin->nLeft + 1;

	nWidth = ( pwWin->nRight - pwWin->nLeft ) - 1;

	nRows = ( pwWin->nBottom - pwWin->nTop ) - 1;

redraw:
	// turn on scrollbar if more than 1 page
	if (( nHeight = nCount ) > nRows ) {
		if ( nRows > 2 )
			nBar = 1;
		nHeight = nRows;
	}

	// set page start
	if (( nCurrent > nRows ) && ((( pwWin->fPopupFlags & NO_CHANGE ) == 0 ) || ( nLoop == 0 ))) {
		if (( nCount - nCurrent ) < nRows / 2 )
			nFirst = ( nCount - nRows ) + 1;
		else 
			nFirst = ( nCurrent - ( nRows / 2 )) + 1;
		if ( nFirst < 1 )
			nFirst = 1;
	} else
		nFirst = 1;

	nLoop = 1;

	// clear window & write a page's worth
Redraw2:
	wClear();
	for ( i = 1, j = nFirst - 1; (( j < nCount ) && ( i <= nHeight )); i++, j++ )
		wWriteStrAtt( i - 1, 0, pwWin->nAttrib, lppList[j] );

	szSearch[0] = _TEXT('\0');
	nSearch = 0;

	SetMousePosition( pwWin->nTop, pwWin->nLeft );
	MouseCursorOn();
	for ( ; ; ) {

		if (( pwWin->fPopupFlags & NO_CHANGE ) == 0 ) {

			while ( nCurrent < nFirst ) {
				nFirst--;
				// only scroll within page
				if (( nCurrent + nRows ) >= nFirst )
					ScrollBack( lppList, nFirst );
			}
		}

		nLast = nFirst + nHeight - 1;

		if (( pwWin->fPopupFlags & NO_CHANGE ) == 0 ) {

			while ( nCurrent > nLast ) {

				nFirst++;
				nLast++;

				// only scroll within page
				if (( nCurrent - nRows ) <= nLast )
					ScrollForward( lppList, nFirst, nLast );
			}
		}

		nTurnOff = nCurrent;

		if ( nBar )
			ScrollBar( nCurrent, nCount );

		// flip the current line to inverse video
		i = pwWin->nTop + 1 + ( nCurrent - nFirst );
		if (( nCurrent >= nFirst ) && ( nCurrent <= nLast )) {
			SetLineColor( i, nIndent, nWidth, pwWin->nInverse );
			SetCurPos( i, nIndent );
		} else
			SetCurPos( pwWin->nTop, pwWin->nLeft );

		// with DOS, get the key from the BIOS, because
		//   STDIN might be redirected (%@SELECT[con,...])
		uKey = cvtkey( GetKeystroke( EDIT_NO_ECHO | EDIT_BIOS_KEY | EDIT_UC_SHIFT | EDIT_MOUSE_BUTTON ), MAP_GEN | MAP_HWIN );

		switch ( uKey ) {

		case LF:
		case CR:
ExitKey:
			gnPopExitKey = uKey;
			gnPopupSelection = nCurrent - 1;

			if ( pwWin->fPopupFlags & CDD_COLORS ) {

				pszListEntry = lppList[ gnPopupSelection ] + _fstrlen( lppList[ gnPopupSelection ] ) + 1;
				if ( *pszListEntry == 4 ) {
					pszListEntry++;

					if ( fWin95 == 0 )
						return pszListEntry;
				}
			}

			MouseCursorOff();
			return ( lppList[ gnPopupSelection ] );

		case ESC:
			MouseCursorOff();
			return 0L;

		case F1:

			// popup some help

			// disallow ^X exit
			_help( (( pwWin->fPopupFlags & BATCH_DEBUG ) ? "Debugging" : NULL ), HELP_NX );

			continue;

		case LEFT_MOUSE_BUTTON:
		case RIGHT_MOUSE_BUTTON:

			// left / right mouse button!
			uKey = (( uKey == LEFT_MOUSE_BUTTON ) ? CR : LF );

			while (1) {

				int nRow, nColumn, nButton;

				_asm {
					mov       ax,1680h
					int       2Fh		; give up Win timeslice
				}

				// loop until button is not pressed
				GetMousePosition( &nRow, &nColumn, &nButton );

				// button released yet?
				if ( nButton == 0 ) {

					if (( nColumn == pwWin->nRight ) && ( nRow > pwWin->nTop ) && ( nRow < pwWin->nBottom )) {

						// CurUp / CurDn
						if ( nRow == pwWin->nTop + 1 )
							goto CursorUp;
						if ( nRow == pwWin->nBottom - 1 )
							goto CursorDown;

						// PgUp / PgDn
						if ( nRow < pwWin->nThumb )
							goto PageUp;
						if ( nRow > pwWin->nThumb )
							goto PageDown;
						break;
					}

					if (( nRow <= pwWin->nTop ) || ( nRow >= pwWin->nBottom ))
						break;
					if (( nColumn <= pwWin->nLeft ) || ( nColumn >= pwWin->nRight ))
						break;

					nCurrent = ( nRow - ( pwWin->nTop + 1 )) + nFirst;
					goto ExitKey;
				}
			}

			break;

		case 4:
			// delete the current line (history & CD lists only!)
			if ((( lppList[0] < glpHistoryList ) || ( lppList[0] >= ( glpHistoryList + gpIniptr->HistorySize ))) && (( lppList[0] < glpDirHistory ) || ( lppList[0] >= ( glpDirHistory + gpIniptr->DirHistorySize )))) {
				honk();
				break;
			}

			i = nCurrent - 1;

			// collapse the current entry from the list block
			//   and History / Directory list
			j = _fstrlen( lppList[i] ) + 1;
			nCount--;

			_fmemmove( lppList[i], lppList[i] + j, (unsigned int)((( end_of_env(lppList[nCount]) - lppList[i]) + 1 ) - j ) * sizeof(TCHAR) );

			// adjust the list array pointers
			for ( ; ( i < nCount ); i++ )
				lppList[i] = ( lppList[i+1] - j );

			if (( nCurrent > nCount ) && ( --nCurrent <= 0 ))
				return 0L;

			goto redraw;

		case CUR_LEFT:
			if ( pwWin->nHOffset > 0 ) {
				pwWin->nHOffset -= 4;
				goto Redraw2;
			}
			honk();
			break;

		case CUR_RIGHT:
			pwWin->nHOffset += 4;
			goto Redraw2;

		case PgUp:
PageUp:
			if ( pwWin->fPopupFlags & NO_CHANGE ) {

				if (( nFirst -= nHeight - 1 ) < 1 )
					nFirst = 1;
				goto Redraw2;

			} else if (( nCurrent -= nHeight - 1 ) < 1 )
				nCurrent = 1;
			break;

		case PgDn:
PageDown:
			if ( pwWin->fPopupFlags & NO_CHANGE ) {

				if (( nFirst + ( nHeight - 1 )) <= nCount )
					nFirst += nHeight - 1;
				goto Redraw2;

			} else if (( nCurrent += nHeight - 1 ) > nCount )
				nCurrent = nCount;
			break;

		case HOME:
		case CTL_PgUp:

			nFirst = 1;
			pwWin->nHOffset = 0;
			if ( pwWin->fPopupFlags & NO_CHANGE )
				goto Redraw2;
			nCurrent = 1;
			goto redraw;

		case END:
		case CTL_PgDn:

			if ( pwWin->fPopupFlags & NO_CHANGE ) {
				if (( nFirst = nCount - ( nHeight - 1 )) < 1 )
					nFirst = 1;
				goto Redraw2;
			}
			nCurrent = nCount;
			goto redraw;

		case CUR_UP:
CursorUp:
			if ( pwWin->fPopupFlags & NO_CHANGE ) {

				if ( nFirst > 1 ) {
					nFirst--;
					ScrollBack( lppList, nFirst );
					continue;
				}

			} else if ( nCurrent > 1 ) {
				nCurrent--;
				break;
			}

			honk();
			break;

		case CUR_DOWN:
CursorDown:
			if ( pwWin->fPopupFlags & NO_CHANGE ) {

				if ( nLast < nCount ) {
					nFirst++;
					nLast++;
					ScrollForward( lppList, nFirst, nLast );
					continue;
				}

			} else if ( nCurrent < nCount ) {
				nCurrent++;
				break;
			}

			honk();
			break;

		case BS:
			nSearch = 0;
			break;

		default:

			if (( pwWin->fPopupFlags & BATCH_DEBUG ) && ( pszChars != NULL )) {

				if ( uKey == F8 )
					uKey = _TEXT('T');
				else if ( uKey == F10 )
					uKey = _TEXT('S');

				if ( strchr( pszChars, (TCHAR)uKey ) != NULL ) {

					extern void _fastcall PopupEnvironment( int );
					extern int ListEntry( LPTSTR );

					// check for popup alias or environment list
					if ( uKey == _TEXT('A') )
						PopupEnvironment( 1 );

					else if ( uKey == _TEXT('V') )
						PopupEnvironment( 0 );

					else if ( uKey == _TEXT('L') ) {

						// list file
						TCHAR szFileName[MAXFILENAME];
						POPWINDOWPTR pwWindow;
						jmp_buf saved_env;

						pwWindow = wOpen( 5, 2, 7, 77, uInverse, _TEXT("LIST"), NULL );
						pwWindow->nAttrib = uNormal;
						wClear();

						wWriteStrAtt( 0, 1, uNormal, _TEXT("File: ") );
						egets( szFileName, 72, ( EDIT_DIALOG | EDIT_BIOS_KEY | EDIT_NO_CRLF ));
						wRemove( pwWindow );

						if ( szFileName[0] ) {

							// save the old "env" (^C destination)
							memmove( saved_env, cv.env, sizeof(saved_env) );

							// kludge to save/restore screen
							pwWindow = wOpen( 0, 0, GetScrRows(), GetScrCols()-1, uDefault, NULLSTR, NULL );
							ListEntry( szFileName );
							wRemove( pwWindow );

							// restore the old "env"
							memmove( cv.env, saved_env, sizeof(saved_env) );
						}

					} else if ( uKey == _TEXT('X') ) {

						TCHAR _far * xlist[2];

						// display the expanded line
						xlist[0] = (TCHAR _far *)pszBatchDebugLine;
						wPopSelect( 5, 2, 1, 75, xlist, 1, 1, _TEXT("Expanded Line"), NULL, NULL, 0 );

					} else
						goto ExitKey;
					continue;
				}

			} else if ( uKey <= 0xFF ) {

				// search for entry beginning with szSearch
ResetSearch:
				if ( nSearch >= 32 )
					nSearch = 0;
				szSearch[ nSearch++ ] = (TCHAR)uKey;
				szSearch[ nSearch ] = _TEXT('\0');

				for ( j = 0, i = nCurrent - 1; ( i < nCount ); i++ ) {
StartLoop:
					pszListEntry = lppList[ i ];
					while (( *pszListEntry == _TEXT(' ') ) || ( *pszListEntry == _TEXT('\t')))
						pszListEntry++;

					if ( _fstrnicmp( szSearch, pszListEntry, strlen( szSearch )) == 0 ) {
						nCurrent = i + 1;
						goto NoHonk;
					}

					if ( i == ( nCount - 1 )) {
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
#ifdef VER2
							// alternate incremental search code replaces everything
							// below "} else if ( j == 1 ) {" line above
							// discard non-matching character and beep
							nSearch--;
							szSearch[ nSearch ] = _TEXT('\0');
							break;
#endif
						}
					}
				}
			}

			honk();
		}
NoHonk:
		if (( pwWin->fPopupFlags & BATCH_DEBUG ) == 0 ) {

			// display the search string in the bottom right border
			_line( pwWin->nBottom, (pwWin->nLeft + 1), (pwWin->nRight - pwWin->nLeft - 1), 1, 0, pwWin->nAttrib, 0 );

			if ( nSearch > 0 ) {
				sprintf( szSearchDisp, FMT_PREC_STR, (pwWin->nRight - pwWin->nLeft - 1), szSearch);
				wWriteStrAtt( (pwWin->nBottom - pwWin->nTop - 1), (pwWin->nRight - pwWin->nLeft - strlen(szSearchDisp) - 1), pwWin->nAttrib, szSearchDisp );
			}
		}

		// flip the old line back to normal video
		if (( pwWin->fPopupFlags & NO_CHANGE ) == 0 )
			SetLineColor( pwWin->nTop + 1 + (nTurnOff - nFirst), nIndent, nWidth, pwWin->nAttrib );
	}
}


// do a Shell sort on the list (much smaller & less stack than Quicksort)
static void _near ssort( TCHAR _far * _far * lppList, unsigned int uEntries )
{
	register unsigned int i, uGap;
	int j;
	TCHAR _far *pTmp;

	for ( uGap = ( uEntries >> 1 ); ( uGap > 0 ); uGap >>= 1 ) {

		for ( i = uGap; ( i < uEntries ); i++ ) {

			for ( j = ( i - uGap ); ( j >= 0L ); j -= uGap ) {

				if ( _fstricmp( lppList[ j ], lppList[ j + uGap ] ) <= 0 )
					break;

				// swap the two records
				pTmp = lppList[ j ];
				lppList[ j ] = lppList[ j + uGap ];
				lppList[ j + uGap ] = pTmp;
			}
		}
	}
}


// popup a selection list
TCHAR _far * wPopSelect( register int nTop, int nLeft, int nHeight, int nWidth, TCHAR _far * _far *lppList, int nEntries, int nCurrent, LPTSTR pszTitle, LPTSTR pszBottomTitle, LPTSTR pszKeys, int fOptions )
{
	int i, nBottom, nRight;
	TCHAR _far *pszListEntry = 0L;
	jmp_buf saved_env;

	// reset mouse & ensure we have "mouse present" flag set
	MouseReset();

	if ( fOptions & SORT_LIST )
		ssort( lppList, nEntries );

	// check for valid Top and Left parameters
	if (( i = (( GetScrCols() - 1 ) - nWidth )) < 0 )
		i = 0;

	if ( nLeft > i )
		nLeft = (( nLeft == 999 ) ? i / 2 : i );

	if (( i = (( GetScrRows() - 1 ) - nHeight )) < 0 )
		i = 0;

	if ( nTop > i )
		nTop = (( nTop == 999 ) ? i / 2 : i );

	nBottom = ( nTop + nHeight ) + 1;
	nRight = ( nLeft + nWidth ) + 1;

	// check for valid sizes for current screen
	i = GetScrRows();
	if ( nBottom > i )
		nBottom = i;
	i--;

	if (( nBottom > i ) && ( nTop > 0 )) {
		if (( nTop -= ( nBottom - i )) < 0 )
			nTop = 0;
		nBottom = i;
	}

	i = GetScrCols() - 1;
	if ( nRight > i )
		nRight = i;
	i -= 2;

	if (( nRight > i ) && ( nLeft > 0 )) {
		if (( nLeft -= ( nRight - i )) < 0 )
			nLeft = 0;
		nRight = i;
	}

	// if less than 1 page of entries, size window downwards
	if ( nEntries < (( nBottom - nTop ) - 1 ))
		nBottom = nTop + nEntries + 1;

	// get the popup window color, or default to inverse of current
	if ( pwWin == NULL ) {

		// save original color
		GetAtt( &uDefault, &uInverse );

		if (( uInverse = (( fOptions & CDD_COLORS ) ? gpIniptr->CDDColor : gpIniptr->PWColor )) != 0 ) {
			// ROR 4 to get inverse
			uNormal = ( uInverse >> 4 ) + (( uInverse << 4 ) & 0xFF );
		} else
			GetAtt( &uNormal, &uInverse );
	}

	// save the old "env" (^C destination)
	memmove( saved_env, cv.env, sizeof(saved_env) );

	if ( setjmp( cv.env ) != -1 ) {

		// open the popup window
		if ( wOpen( nTop, nLeft, nBottom, nRight, uInverse, pszTitle, pszBottomTitle ) == NULL )
			return 0L;

		pwWin->fPopupFlags = fOptions;

		// call the selection window
		pszListEntry = wSelect( lppList, nEntries, nCurrent, pszKeys );
	}

	// remove the window, restore the screen & free memory
	wRemove( pwWin );

	// restore the old "env"
	memmove( cv.env, saved_env, sizeof(saved_env) );

	// disable signal handling momentarily
	HoldSignals();

	return pszListEntry;
}
