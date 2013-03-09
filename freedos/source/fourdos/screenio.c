

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


// SCREENIO.C - Screen input routine for the 4xxx / TCMD family
//   (c) 1991 - 2002 Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dos.h>
#include <malloc.h>
#include <string.h>

#include "4all.h"


static void _fastcall CloseDirSearchHandles( int, FILESEARCH * );
static int _fastcall TabComplete( LPTSTR, LPTSTR, int );
static int _fastcall FilenameLength( LPTSTR );
static int _fastcall PositionEGets( LPCTSTR );
static void _fastcall clearline( LPTSTR );
static void _fastcall efputs( LPCTSTR );
static void _fastcall PadBlanks( int );

void CutEgets( void );

LPTSTR pszEgetsBase;					// start of line
static LPTSTR pszCurrentPos;			// current position

LPTSTR pszMarkStart = (LPTSTR)-1L;		// block marking
LPTSTR pszMarkEnd = (LPTSTR)-1L;

static int nScreenRows;			// current screen size
static int nScreenColumns;
static int nHomeRow;			// starting row & column
static int nHomeColumn;
static int nCursorRow;			// current row & column position
static int nCursorColumn;
static int nEditFlag;
static int fPassword;
static int nColor;
static int nInverse;

// invalid filename characters
static char szInvalidChars[] = "  , |<> \t;,`+=";


// get string from STDIN, with full line editing
//	pszStr       = input string
//	nMaxLength = max entry length
//	nEditFlag  = EDIT_COMMAND = command line
//		    EDIT_DATA = other input
//		    EDIT_ECHO = echo line & get DATA
//		    EDIT_DIALOG = edit line within current dialog window
int PASCAL egets( LPTSTR pszStr, int nMaxLength, int fFlags )
{
	register int c;
	register LPTSTR pszPtr;
	int i, n, fEcho = 1, fLFN = 1, fExecuteNow = 0, nMatches, fQuote;
	TCHAR *pszArg, szSource[MAXFILENAME+1];
	TCHAR _far *lpszPtr, _far *pszKeys;
	LPTSTR fpStartCP;

	pszKeys = (TCHAR _far *)NULLSTR;

	// get the insert mode, unless "init insert/overstrike" set
	if ( gpIniptr->EditMode <= 1 )
		gnEditMode = gpIniptr->EditMode;

	pszEgetsBase = pszCurrentPos = pszStr;

	nEditFlag = fFlags;
	fPassword = (( nEditFlag & EDIT_PASSWORD ) != 0 );

	// if not echoing existing input, clear the input line
	if (( nEditFlag & EDIT_ECHO ) == 0 )
		*pszCurrentPos = _TEXT('\0');

	if (( nEditFlag & EDIT_NO_INPUTCOLOR ) || (( nColor = (( nEditFlag & EDIT_DIALOG ) ? gpIniptr->ListColor : gpIniptr->InputColor )) == 0 )) {
		nColor = -1;
		nInverse = 0x70;
	} else {
		// remove blink & intensity bits, and check for black on black
		if (( nInverse = ( nColor & 0x77 )) == 0)
			nInverse = 0x70;
		else {
			// ROR 4 to get inverse
			nInverse = ( nInverse >> 4 ) + (( nInverse << 4 ) & 0xFF );
		}
	}

redisplay_egets:

	// set the default cursor size
	if ( gpIniptr->EditMode < 2 )
		SetCurSize( );

	// get the number of screen lines and columns
	nScreenRows = GetScrRows();
	nScreenColumns = GetScrCols();

	// get the cursor position
	GetCurPos( &nHomeRow, &nHomeColumn );

	nCursorRow = nHomeRow;
	nCursorColumn = nHomeColumn;

	efputs( pszEgetsBase );

	// check for line wrap & adjust home position accordingly
	PositionEGets( strend( pszEgetsBase ));

	// get & set starting cursor position
	if ( fEcho )
		PositionEGets( pszCurrentPos );

	for ( ; ; ) {

egets_key:
		// get keystroke from keyboard or alias keylist
		if ( *pszKeys == _TEXT('\0') ) {

			if ( fExecuteNow )
				c = CR;
			else {
				fEcho = 1;
				if (( c = GetKeystroke( EDIT_NO_ECHO | EDIT_SCROLL_CONSOLE | EDIT_SWAP_SCROLL | nEditFlag )) == EOF )
					goto egots_cr_key;

				if (( c != LEFT_MOUSE_BUTTON ) && ( c != RIGHT_MOUSE_BUTTON ) && ( c != MIDDLE_MOUSE_BUTTON ))
					c = cvtkey( c, MAP_GEN | MAP_EDIT );
			}

		} else {

			c = cvtkey( *pszKeys++, MAP_GEN | MAP_EDIT );

			// set escaped character
			if (( c == (int)gpIniptr->EscChr ) && (( gpIniptr->Expansion & EXPAND_NO_ESCAPES ) == 0 )) {
				c = escape_char( *pszKeys );
				if ( c )
					pszKeys++;
			}
		}

		// get & set cursor position
		if (( fEcho ) && ( c < FBIT ))
			PositionEGets( pszCurrentPos );

egots_key:

		if (( pszMarkStart != (LPTSTR)-1L ) && ( pszMarkStart >= pszEgetsBase )) {

			switch ( c ) {
			case F1:	// these keys don't disable marked block
			case INS:
			case CUR_UP:
			case CUR_DOWN:
			case PgUp:
			case PgDn:
			case BACKSPACE:
			case DEL:
			case 22:
			case 25:
				break;
			default:
				if (( c < 32 ) || ( c > FBIT )) {

					// if block already marked, turn it off

					pszMarkEnd = (LPTSTR)-1L;
					pszMarkStart = (LPTSTR)-1L;

					// redraw to normal atts
					clearline( pszEgetsBase );
					efputs( pszEgetsBase );
					// get & set cursor position
					PositionEGets( pszCurrentPos );

				}
			}
		}

		switch ( c ) {
		case CTRLC:
			// abort from BIOS_KEY()
			if (( nEditFlag & EDIT_DIALOG ) == 0 ) {
				BreakHandler();
			}

			return 0;

		case LF:

			glpHptr = 0L;

			//lint -fallthrough
		case CR:
egots_cr_key:
			// reset cursor shape on exit
			if ( gpIniptr->EditMode < 2 )
				SetCurSize( );

			// go to EOL before CR/LF (may be multiple lines)
			if ( fEcho )
				PositionEGets( strend( pszCurrentPos ));
			if (( nEditFlag & EDIT_NO_CRLF ) == 0 )
				crlf();

			if ( nEditFlag & EDIT_COMMAND ) {

				// adjust history pointer if command line entry
				//  (glpHptr reset to NULL for a new command,
				//  or if HistoryCopy=YES, or for an @@ alias)
				// otherwise, set it to just past the current
				if (( gpIniptr->HistoryCopy ) || (( fExecuteNow == 1 ) && ( fEcho == 0 )))
					glpHptr = 0L;

				else if ( glpHptr != 0L ) {

					if ( _fstrcmp( pszEgetsBase, glpHptr ) != 0 )
						glpHptr = 0L;

					else {

						// move history entry to end
						if ( gpIniptr->HistoryMove ) {
							// kill current entry & force it
							//   to be saved again at the end
							lpszPtr = next_env( glpHptr );
							pszKeys = end_of_env( glpHptr );
							_fmemmove( glpHptr, lpszPtr, (unsigned int)(( pszKeys - lpszPtr ) + 1 ) * sizeof(TCHAR) );
							glpHptr = 0L;
						} else
							glpHptr++;
					}
				}

			}

			return ( strlen( pszEgetsBase ));

		case F1:	// help me!
			if ( fRuntime == 0 ) {

				// save line because PROMPT destroys "cmdline"
				pszArg = _strdup( pszEgetsBase );

				if (( c = _help( first_arg( pszEgetsBase ), NULL )) != 0 ) {

					// HELP didn't restore the screen!
					// set the cursor position to end of screen
					SetCurPos( GetScrRows(), 0 );

					// redisplay the prompt
					ShowPrompt();
				}

				// restore and redisplay the command line
				strcpy( pszEgetsBase, pszArg );
				free( pszArg );

				if ( c != 0 )
					goto redisplay_egets;

			}
			break;

		case CTL_F1:	// help me!

			if ( fRuntime == 0 ) {

				LPTSTR pszSavedLine;

				szInvalidChars[2] = _TEXT('%');
				szInvalidChars[1] = _TEXT(' ');
				szInvalidChars[0] = (( gpIniptr->UnixPaths) ? _TEXT(' ') : gpIniptr->SwChr );

				// get beginning of arg under cursor
				for ( pszArg = pszCurrentPos; ( pszArg > pszEgetsBase ); pszArg-- ) {
					if (( pszArg[-1] < 33 ) || ( strchr( szInvalidChars, pszArg[-1] ) != NULL ))
						break;
				}

				if (( pszArg = first_arg( pszArg )) == NULL ) {
					honk();
					continue;
				}

				sscanf( pszArg, _TEXT("%[^[=,+]"), pszArg );

				// save line because PROMPT destroys "cmdline"
				pszSavedLine = _strdup( pszEgetsBase );

				if (( c = _help( pszArg, NULL )) != 0 ) {

					// HELP didn't restore the screen!
					// set the cursor position to end of screen
					SetCurPos( GetScrRows(), 0 );

					// redisplay the prompt
					ShowPrompt();
				}

				// restore and redisplay the command line
				strcpy( pszEgetsBase, pszSavedLine );
				free( pszSavedLine );

				if ( c != 0 )
					goto redisplay_egets;

			}
			break;

		case 6:		// ^F to expand aliases
			if (( nEditFlag & EDIT_COMMAND ) == 0 ) {
				honk();
				break;
			}

			// perform alias expansion
			clearline( pszEgetsBase );
			for ( pszArg = pszEgetsBase; ; ) {

				pszArg = skipspace( pszArg );
				if ( alias_expand( pszArg ) != 0 ) {
					honk();
					break;
				}

				pszArg = scan( pszArg, NULL, QUOTES );
				if ( *pszArg == _TEXT('\0'))
					break;

				// skip conditionals
				if ( *pszArg == _TEXT('|') ) {
					if ( *(++pszArg) == _TEXT('|') )
						pszArg++;
				} else if ( *pszArg == _TEXT('&')) {
					if ( *(++pszArg) == _TEXT('&'))
						pszArg++;
				} else
					pszArg++;
			}

			efputs( pszEgetsBase );
			pszCurrentPos = strend( pszEgetsBase );
			break;

		case 22:	// ^V get line from clipboard
			if ( GetClipboardLine( 0, szSource, MAXFILENAME ) == 0 ) {

				// first, delete any marked text

				if (( pszMarkStart != (LPTSTR)-1L ) && ( pszMarkStart >= pszEgetsBase ))
					CutEgets();

				if ( strlen( pszEgetsBase ) + strlen( szSource ) < nMaxLength ) {
					clearline( pszCurrentPos );
					strins( pszCurrentPos, szSource );
					efputs( pszCurrentPos );
					pszCurrentPos += strlen( szSource );
				} else
					honk();
			}
			break;

		case 25:	// ^Y copy marked block to clipboard
			if ( pszMarkStart != (LPTSTR)-1L )
				CopyTextToClipboard( pszMarkStart, ( pszMarkEnd - pszMarkStart ) );
			break;

		case 11:	// ^K save line to history & clear input
			if ( strlen( pszEgetsBase ) > 0 ) {
				glpHptr = 0L;
				addhist( pszEgetsBase );
			}
			//lint -fallthrough

		case ESC:	// cancel line
			if (( nEditFlag & EDIT_DIALOG ) && ( *pszEgetsBase == _TEXT('\0') ))
				goto egots_cr_key;

			pszCurrentPos = pszEgetsBase;
			clearline( pszCurrentPos );
			*pszCurrentPos = _TEXT('\0');
			break;

		case CUR_LEFT:
			if ( pszCurrentPos > pszEgetsBase )
				pszCurrentPos--;
			break;

		case CUR_RIGHT:
			if ( *pszCurrentPos )
				pszCurrentPos++;
			break;

		case 12:
		case CTL_LEFT:
			// delete word left (^L ) or move word left (^left)
			for ( pszPtr = pszCurrentPos; ( pszCurrentPos > pszEgetsBase ); ) {
				if (( !iswhite( *(--pszCurrentPos))) && (( iswhite( pszCurrentPos[-1] )) || ( !ispunct(*pszCurrentPos) && ispunct( pszCurrentPos[-1]))))
					break;
			}

			if ( c == 12) {		// delete word left (^L )
				c = PositionEGets( strend( pszCurrentPos ));
				strcpy( pszCurrentPos, pszPtr );
				c -= PositionEGets( strend( pszCurrentPos ));
				PositionEGets( pszCurrentPos );
				efputs( pszCurrentPos );
				PadBlanks( c );
			}

			break;

		case 18:
		case CTL_RIGHT:
			// delete word right (^R or ^BS) or move word right
			for (pszPtr = pszCurrentPos; (*pszCurrentPos != _TEXT('\0') ); ) {
				if ((!iswhite(*(++pszCurrentPos))) && ((iswhite( pszCurrentPos[-1])) || (!ispunct(*pszCurrentPos) && ispunct( pszCurrentPos[-1]))))
					break;
			}

			if ( c == 18 ) {	   // delete word right (^R)
				c = PositionEGets( strend( pszCurrentPos ));
				pszCurrentPos = strcpy( pszPtr, pszCurrentPos );
				c -= PositionEGets( strend( pszCurrentPos ));
				PositionEGets( pszCurrentPos );
				efputs( pszCurrentPos );
				PadBlanks( c );
			}

			break;

		case INS:
			gnEditMode ^= 1;
			SetCurSize( );
			break;

		case HOME:
			pszCurrentPos = pszEgetsBase;
			break;

		case END:
			// goto end of line
			pszCurrentPos += strlen( pszCurrentPos );
			break;

		case CTL_HOME:
			// delete from start of line to cursor
			clearline( pszEgetsBase );
			pszCurrentPos = strcpy( pszEgetsBase, pszCurrentPos );
			efputs( pszCurrentPos );
			break;

		case CTL_END:
			// erase to end of line
			clearline( pszCurrentPos );
			*pszCurrentPos = _TEXT('\0');
			break;

		case CTL_F5:
			// toggle batch debugging
			if (( fRuntime == 0 ) && ( cv.bn >= 0 ) && (( bframe[cv.bn].nFlags & BATCH_COMPRESSED ) == 0 ))
				gpIniptr->SingleStep ^= 1;
			continue;

			// keyboard block marking
		case SHIFT_LEFT:
		case SHIFT_RIGHT:
		case SHIFT_HOME:
		case SHIFT_END:
		case CTL_SHIFT_LEFT:
		case CTL_SHIFT_RIGHT:

			// no cut/paste marking allowed on passwords!
			if ( fEcho == 0 )
				break;

			// redraw to normal atts

			if ( pszMarkStart != (LPTSTR)-1L ) {
				pszMarkStart = pszMarkEnd = (LPTSTR)-1L;
				clearline( pszEgetsBase );
				efputs( pszEgetsBase );
				// get & set cursor position
				PositionEGets( pszCurrentPos );
			}

			for ( ; ; ) {

				if ( pszMarkStart == (LPTSTR)-1L ) {
					fpStartCP = pszCurrentPos;
					pszMarkStart = fpStartCP;
				}

				pszArg = pszCurrentPos;

				// move the cursor
				if ( c == SHIFT_LEFT ) {
					if ( pszCurrentPos > pszEgetsBase )
						pszCurrentPos--;
				} else if ( c == SHIFT_RIGHT ) {
					if ( *pszCurrentPos )
						pszCurrentPos++;
				} else if ( c == SHIFT_HOME ) {
					// mark from beginning of line to current
					pszCurrentPos = pszEgetsBase;
				} else if ( c == SHIFT_END ) {
					// mark to end of line
					while ( *pszCurrentPos != _TEXT('\0') )
						pszCurrentPos++;
				} else if ( c == CTL_SHIFT_RIGHT ) {
					// word right
					for ( pszPtr = pszCurrentPos; ( *pszCurrentPos != _TEXT('\0') ); ) {
						if (( !iswhite( *(++pszCurrentPos) )) && (( iswhite( pszCurrentPos[-1] )) || ( !ispunct( *pszCurrentPos ) && ispunct( pszCurrentPos[-1] ))))
							break;
					}
				} else if ( c == CTL_SHIFT_LEFT ) {
					// word left
					for ( pszPtr = pszCurrentPos; ( pszCurrentPos > pszEgetsBase ); ) {
						if (( !iswhite( *(--pszCurrentPos) )) && (( iswhite( pszCurrentPos[-1] )) || ( !ispunct( *pszCurrentPos ) && ispunct( pszCurrentPos[-1] ))))
							break;
					}
				}

				if ( pszArg > pszCurrentPos )
					pszArg = pszCurrentPos;

				PositionEGets( pszArg );
				clearline( pszArg );

				// redraw in inverse
				pszMarkEnd = pszCurrentPos;

				if ( pszMarkEnd < fpStartCP ) {
					pszMarkStart = pszMarkEnd;
					pszMarkEnd = fpStartCP;
				} else {
					pszMarkStart = fpStartCP;
				}

				// redraw to normal atts
				efputs( pszArg );

				// get & set cursor position
				PositionEGets( pszCurrentPos );

				// if next key is Shift-cursor, stay in loop
				c = cvtkey( GetKeystroke( EDIT_NO_ECHO | EDIT_SWAP_SCROLL ), MAP_GEN | MAP_EDIT);
				if (( c < SHIFT_LEFT ) || ( c > CTL_SHIFT_RIGHT ))
					goto egots_key;
			}

		case 4:		// delete history entry
		case 5:		// goto end of history
		case F3:	// recall last command (like COMMAND.COM)

		case CUR_UP:	// get previous command from history
		case CUR_DOWN:	// get next command from history

			// disable if no history or not getting command line
			if ((( nEditFlag & EDIT_COMMAND ) == 0 ) || ( *glpHistoryList == _TEXT('\0') )) {
				honk();
				break;
			}

			// F3 behaves like COMMAND.COM
			//   (get remainder of previous line)
			if ( c == F3) {

				lpszPtr = prev_hist( NULL );
				i = (int)( pszCurrentPos - pszEgetsBase );

				// abort if new line longer than old one
				if ((int)_fstrlen( lpszPtr ) <= i )
					break;
				lpszPtr += i;
				goto redraw;
			}

			// ^E go to history end
			if ( c == 5 ) {
				glpHptr = 0L;
				pszCurrentPos = pszEgetsBase;
				clearline( pszCurrentPos );
				*pszCurrentPos = _TEXT('\0');
			}

			// if there are any characters on the current line,
			//   try to match them (from the END of the history
			//   list; otherwise, just get the previous or next
			//   entry in the history list

			pszArg = skipspace( pszEgetsBase );
			fQuote = ( *pszArg == _TEXT('"') );
			n = 0;

			if (( pszPtr = strend( pszArg )) > pszArg ) {
				n = (int)( pszPtr - pszArg );
				glpHptr = 0L;
			} else if ( c == 4 ) {
				// don't delete unknown history entries!
				honk();
				break;
			}

			for ( nMatches = 0; ; ) {

				TCHAR _far *lpszLastHptr;
				TCHAR _far *lpszSaveHptr;
				TCHAR _far *lpszStart;

				// ^D delete line from history,
				//   then return previous entry
				if (( c == 4 ) && ( glpHptr != 0L )) {
					lpszPtr = next_env( glpHptr );
					_fmemmove( glpHptr, lpszPtr, ( gpIniptr->HistorySize - ( lpszPtr - glpHistoryList )) * sizeof(TCHAR) );
				}

				// did we just add a quote?  we need to ignore it for the next test
				if (( fQuote == 0 ) && ( *pszArg == _TEXT('"') ))
					pszArg++;

				if ( c == CUR_DOWN ) {

					lpszSaveHptr = lpszLastHptr = next_hist( glpHptr );

					for ( lpszPtr = lpszSaveHptr; ; lpszLastHptr = lpszPtr ) {
						i = 1;

						// ignore leading '"'
						if ((( lpszStart = lpszPtr ) != 0L ) && ( *lpszStart == _TEXT('"') ) && ( fQuote == 0 ))
							lpszStart++;

						if (( lpszStart == 0L ) || (( i = _fstrnicmp( lpszStart, pszArg, n )) == 0 ) || (( lpszPtr = next_hist( lpszPtr )) == lpszSaveHptr ) || ( lpszPtr == lpszLastHptr ))
							break;
					}

				} else {

					lpszSaveHptr = lpszLastHptr = prev_hist( glpHptr );

					for ( lpszPtr = lpszSaveHptr; ; lpszLastHptr = lpszPtr ) {
						i = 1;

						// ignore leading '"'
						if ((( lpszStart = lpszPtr ) != 0L ) && ( *lpszStart == _TEXT('"') ) && ( fQuote == 0 ))
							lpszStart++;

						if (( lpszStart == 0L ) || (( i = _fstrnicmp( lpszStart, pszArg, n )) == 0 ) || (( lpszPtr = prev_hist( lpszPtr )) == lpszSaveHptr ) || ( lpszPtr == lpszLastHptr ))
							break;
					}
				}

				// failed a match?
				if ( i != 0 ) {

					honk();

					// if no more matches for ^D, clear
					//   line & abort
					if ( c == 4) {
						pszCurrentPos = pszEgetsBase;
						clearline( pszCurrentPos );
						*pszCurrentPos = _TEXT('\0');
						break;
					}

					// kludge for HistWrap=No -- we don't want to bomb out of
					//   the routine if we had earlier matches - we might want
					//   to go back down the history
					if (( nMatches == 0 ) || ( gpIniptr->HistoryWrap ))
						break;

					goto StayInLoop;

				} else
					nMatches++;

				// redraw current line
				pszArg = pszCurrentPos = pszEgetsBase;
redraw:
				glpHptr = lpszPtr;

				clearline( pszCurrentPos );
				if ( glpHptr != 0L ) {
					_fstrcpy( pszCurrentPos, glpHptr );
					efputs( pszCurrentPos );
				}

				// put cursor at EOL
				pszCurrentPos += strlen( pszCurrentPos );

				if ( c == F3)
					break;

				// check for wrap at end of screen
				PositionEGets( pszCurrentPos );

				// if next key is cur up, cur down, or ^D, stay
				//   in the loop
StayInLoop:
				//				i = c;
				c = cvtkey( GetKeystroke( EDIT_NO_ECHO | EDIT_SCROLL_CONSOLE | EDIT_SWAP_SCROLL ), MAP_GEN | MAP_EDIT );

				if (( c != CUR_UP ) && ( c != CUR_DOWN ) && ( c != 4 ))
					goto egots_key;

			}

			break;

		case PgUp:	// display a popup history selection box
		case PgDn:
			c = PgUp;
			//lint -fallthrough

		case CTL_PgUp:	// display a popup directory selection box
		case CTL_PgDn:

			{
				extern int gnPopExitKey;
				TCHAR _far * _far *lppList = 0L;
				int saved_char, fFirst = 1;
				unsigned long size = 0L;

				lpszPtr = (( c == PgUp )  ? glpHistoryList : glpDirHistory );
				saved_char = c;
				i = c = 0;

				// disable if not getting command line buffer
				if ( nEditFlag & EDIT_COMMAND ) {

					if (( saved_char == CTL_PgUp ) || ( saved_char == CTL_PgDn ) || ( saved_char == F6 )) {

						// if we're doing directory selection, only attempt a match if we're inside the first word
						for ( pszPtr = pszEgetsBase; ( *pszPtr ); pszPtr++ ) {
							if ( isspace( *pszPtr )) {
								fFirst = 0;
								break;
							}
						}
					}

					// get the list into an array
					for ( ; ( *lpszPtr ); lpszPtr = next_env( lpszPtr )) {

						// check for partial match 
						//   (but only if we're inside the first word on the line if directory selection)
						if (( fFirst ) || ( saved_char == PgUp )) {
							if ( _fstrnicmp( pszEgetsBase, lpszPtr, strlen( pszEgetsBase )) != 0 )
								continue;
						}

						// allocate memory for 256 entries at a time
						if (( c % 256 ) == 0 ) {
							size += 1024;
							lppList = (TCHAR _far * _far *)ReallocMem( lppList, size );
						}

						lppList[ c++ ] = lpszPtr;
						if ( saved_char == PgUp ) {
							// check for current history pointer
							if ( lpszPtr == glpHptr - 1 )
								i = c + 1;
							else if ( lpszPtr == glpHptr )
								i = c;
						}
					}
				}

				// no entries or no matches?
				if ( c == 0 ) {
					honk();
					break;
				}

				if (( i == 0 ) || ( i > c ))
					i = c;

				// display the popup selection list
				if (( lpszPtr = wPopSelect( gpIniptr->PWTop, gpIniptr->PWLeft, gpIniptr->PWHeight, gpIniptr->PWWidth, lppList, c, i, (( saved_char == PgUp ) ? HISTORY_TITLE : DIRECTORY_TITLE ), NULL, NULL, 0 )) != 0L ) {

					if ( saved_char == PgUp ) {

						_fstrcpy( pszEgetsBase, lpszPtr );
						glpHptr = lpszPtr;

						// display line & set cursor to end
						PositionEGets( pszEgetsBase );
						efputs( pszEgetsBase );
						pszCurrentPos = strend( pszEgetsBase );

					} else {
						_fstrcpy( szSource, lpszPtr );
						if ( gnPopExitKey == LF ) {
							strins( pszCurrentPos, szSource );
							efputs( pszCurrentPos );
						}
					}
				}

				FreeMem( lppList );

				// reenable signal handling after cleanup
				EnableSignals();

				// if we had a ^C in wPopSelect, abort after cleanup
				if ( cv.fException ) {
					crlf();
					longjmp( cv.env, -1 );
				}

				if ( gnPopExitKey == CR ) {

					if ( saved_char != PgUp ) {
						__cd( szSource, CD_CHANGEDRIVE | CD_SAVELASTDIR );
					}
					
					goto egots_cr_key;
				}

				break;
			}

		case BACKSPACE:
		case DEL:
			// delete marked block?
			if (( pszMarkStart != (LPTSTR)-1L ) && ( pszMarkStart >= pszEgetsBase ))
				CutEgets();
			else {

				if ( c == BACKSPACE ) {

					if ( pszCurrentPos <= pszEgetsBase ) {
						pszCurrentPos = pszEgetsBase;
						break;
					}

					pszCurrentPos--;
				}

				if ( *pszCurrentPos ) {
					c = PositionEGets( strend( pszCurrentPos ));
					strcpy( pszCurrentPos, pszCurrentPos+1 );
					c -= PositionEGets( strend( pszCurrentPos ));
					PositionEGets( pszCurrentPos );
					efputs( pszCurrentPos );
					PadBlanks( c );
				}
			}

			break;

		case 1:
			// toggle LFN / SFN
			if ( fWin95LFN )
				fLFN ^= 1;
			break;

		case F8:
		case F10:
		case F12:
			// convert an initial F8 or F10 or F12 to an F9
			c = F9;
		case F7:
			// popup a listbox with matching filenames
		case F9:
			// F9 - substitute file name
			// F8 - substitute previous file name
			// F10 - append next matching filename
			// F12 - append same filename
			{

				TCHAR pszFileName[MAXFILENAME], szVarName[128], *pszCmdBase, *pszStart;
				TCHAR _far * _far *lppList = 0L, _far *fpListStart = 0L, _far *lpszEnvPtr;
				int nFind, nLength, nOffset, fIncludeList = 0, fEnvVar, fURL = 0;
				unsigned int uListSize = 0xFFF0;
				unsigned long ulSize = 0L;
				FILESEARCH dir;

				szInvalidChars[1] = _TEXT(' ');
				dir.hdir = INVALID_HANDLE_VALUE;

				// disable if not getting command line buffer
				if ( nEditFlag & EDIT_COMMAND ) {

					unsigned int uExpansion;

					// save the current expansion state (we have to turn it off to avoid problems with %+)
					uExpansion = gpIniptr->Expansion;

					// make sure we're not to the right of a pipe or redir or command separator
					pszCmdBase = pszEgetsBase;

					HoldSignals();
					gpIniptr->Expansion |= EXPAND_NO_VARIABLES | EXPAND_NO_ESCAPES;
					while ((( pszArg = scan( pszCmdBase, NULLSTR, QUOTES )) != BADQUOTES ) && ( pszCurrentPos > pszArg )) {
						while (( *pszArg == gpIniptr->CmdSep ) || ( *pszArg == _TEXT('|')) || ( *pszArg == _TEXT('&') ) )
							pszArg++;
						if ( pszArg <= pszCurrentPos )
							pszCmdBase = pszArg;
					}
					gpIniptr->Expansion = uExpansion;
					EnableSignals();

					szInvalidChars[0] = gpIniptr->SwChr;
					if (( gpIniptr->UnixPaths ) && ( szInvalidChars[0] == _TEXT('/') ))
						szInvalidChars[0] = _TEXT(' ');

					// Win95 LFNs use '"', but everything else doesn't
					if ( fWin95LFN == 0 ) {
						szInvalidChars[2] = '"';
						szInvalidChars[3] = '=';
					}

RedoFileName:
					// find start of filename
					for ( pszArg = NULL, pszStart = pszCurrentPos; ( pszStart > pszCmdBase ); pszStart--) {

						// NTFS / LFN quoted name?

						if (( szInvalidChars[2] != '"' ) && ( pszStart[-1] == '"' )) {

							if (( --pszStart > pszCmdBase ) && ( strchr( _TEXT(" \t"), pszStart[-1] ) == NULL )) {
								while ((--pszStart > pszCmdBase ) && (*pszStart != _TEXT('"') ))
									;
							}
							break;
						}

						// check for an include list entry
						if ( pszStart[-1] == _TEXT(';') ) {

							// check for DR-DOS password
							if ( pszStart[-2] == _TEXT(';') )
								pszStart--;
							else if ( pszArg == NULL ) {
								fIncludeList = 1;
								pszArg = pszStart;
							}

						} else if (( pszStart[-1] < 33 ) || ( strchr( szInvalidChars, pszStart[-1] ) != NULL ))
							break;
					}

					// determine the filename length
					nLength = FilenameLength( pszStart );

					// save original source template
					sprintf( szSource, FMT_PREC_STR, (( nLength > MAXFILENAME ) ? MAXFILENAME : nLength ), pszStart );

					// skip leading '@' in @c:\... syntax (it *can't* be a valid filename!)
					if (( szSource[0] == _TEXT('@') ) && ( strpbrk( szSource, _TEXT("\\/:") ) != NULL )) {
						pszStart++;
						nLength--;
						strcpy( szSource, szSource + 1 );
					}

					// is this an include list entry?
					if ( pszArg != NULL ) {

						nLength = FilenameLength( pszArg );
						sprintf( pszFileName, FMT_PREC_STR, (( nLength > MAXFILENAME) ? MAXFILENAME : nLength ), pszArg );

						if ( path_part( pszFileName ) == NULL )
							insert_path( szSource, pszFileName, szSource );
						else
							strcpy( szSource, pszFileName );
						pszStart = pszArg;
					}

					// is it a variable or filename?
					fEnvVar = ( szSource[0] == _TEXT('%') );

					if ( fEnvVar ) {
						strcat( szSource, _TEXT("*") );

					} else {

						// replace "." and ".." w/expanded name
						if ((( pszArg = fname_part( szSource )) != NULL ) && (( stricmp( pszArg, _TEXT(".") ) == 0 ) || ( stricmp( pszArg, _TEXT("..") ) == 0 ))) {
							mkfname( szSource, 0 );

							mkdirname( szSource, ((( ifs_type( szSource ) != 0 ) && ( fWin95 )) ? "*" : WILD_FILE ));

						} else {
							// append wildcard(s)
							pszArg = ext_part( szSource );

							// kludge for (what else??) another Netware bug!
							//   (Netware fails to return anything with a
							//   construct like "file.abc*")
							if ( pszArg == NULL )
								strcat( szSource, ((( ifs_type( szSource ) != 0 ) && ( fWin95 )) ? "*" : WILD_FILE ));
							else if (( fWin95LFN ) || ( strlen( pszArg ) < 4 ))
								strcat( szSource, "*" );

						}

						// if we have a "...", we need to expand it
						if ( strstr( szSource, _TEXT("...") ) != NULL )
							mkfname( szSource, 0 );
						else
							StripQuotes( szSource );
					}

					// determine the filename length
					nLength = FilenameLength( pszStart );

					if ( fEnvVar )
						lpszEnvPtr = glpEnvironment;

					for ( nOffset = -1, nFind = FIND_FIRST; ; ) {

						if ( c == F8 ) {

							// get previous entry
							// already at start of dir?
							if ( nOffset <= 0 )
								honk();
							else
								nOffset--;

							if ( fEnvVar )
								lpszEnvPtr = glpEnvironment;

							// each F8 causes a rescan from the beginning, so close the dir handle
							CloseDirSearchHandles( fURL, &dir );
							dir.hdir = INVALID_HANDLE_VALUE;
							nFind = FIND_FIRST;
						}

						for ( n = 0; ; ) {

							pszPtr = skipspace( pszCmdBase );

							if ( fEnvVar ) {

								// look for a matching environment variable
								pszArg = NULL;
								for ( ; ( *lpszEnvPtr ); lpszEnvPtr = next_env( lpszEnvPtr )) {

									sscanf_far( lpszEnvPtr, _TEXT("%127[^=]"), szVarName );

									if (( szVarName[0] ) && ( wild_cmp( szSource+1, szVarName, FALSE, TRUE ) == 0 )) {
										strins( szVarName, _TEXT("%") );
										pszArg = szVarName;
										lpszEnvPtr = next_env( lpszEnvPtr );
										break;
									}
								}

								if ( pszArg == NULL )
									break;

							} else {

								// kludge for embedded ' in firstarg
								pszPtr = ntharg( pszCmdBase, (( *pszPtr == _TEXT('\'') ) ? 0 : 0x1000 ));

								i = (( gpIniptr->CompleteHidden ) ? 0x07 : 0 );

								// check for matching file or dir

								if (( pszArg = find_file( nFind, szSource, (( fLFN ) ? 0x10 | FIND_NO_ERRORS | FIND_NO_DOTNAMES | FIND_NO_FILELISTS : 0x10 | FIND_SFN | FIND_NO_ERRORS | FIND_NO_DOTNAMES | FIND_NO_FILELISTS ) | i, &dir, pszFileName )) == NULL ) {

									if (( nFind == FIND_FIRST ) && ( *pszStart == _TEXT('@') )) {
										// no match with leading '@', try it without!
										szInvalidChars[1] = _TEXT('@');
										goto RedoFileName;
									}

									break;
								}

								nFind = FIND_NEXT;

								// if not first arg, check FileCompletion
								if (( pszPtr != NULL ) && ( pszStart > pszCmdBase + strlen( pszPtr ))) {

									// check for desired file types
									if ( TabComplete( pszPtr, pszArg, dir.attrib ) == 0 )
										continue;
								}

								// adjust for a LFN with embedded whitespace
								AddQuotes( pszArg );

								// if first arg of the command, only
								//   display dirs and executable exts

								if (( c != F10 ) && ( pszStart == skipspace( pszCmdBase ) ) && ( szSource[ strlen( szSource ) - 1 ] == '*' )) {

									if ( dir.attrib & _A_SUBDIR ) {

										strcat( pszArg, "\\" );

									} else {

										TCHAR _far *lpPathExt;

										if (( pszPtr = ext_part( pszArg )) == NULL )
											continue;

										// check for PATHEXT & use that if set
										if (( gpIniptr->PathExt ) && (( lpPathExt = get_variable( PATHEXT )) != NULL )) {

											TCHAR _far *lpExt;
											TCHAR szExtension[32];

											for ( lpExt = lpPathExt; *lpExt; lpExt += i ) {

												// get next PATHEXT argument
												sscanf_far( lpExt, _TEXT("%*[;,=]%31[^;,=]%n"), szExtension, &i );
												if ( _stricmp( pszPtr, szExtension ) == 0 )
													break;
											}

											if (( *lpExt == _TEXT('\0') ) && ( *(executable_ext( pszPtr )) == _TEXT('\0') ))
												continue;

										} else {

											for ( i = 0; ( executables[i] != NULL ); i++ ) {
												if ( _stricmp( pszPtr, executables[i] ) == 0 )
													break;
											}

											if (( executables[i] == NULL ) && ( *(executable_ext( pszPtr )) == _TEXT('\0') ))
												continue;
										}
									}


								} else if (( gpIniptr->AppendDir ) && ( dir.attrib & _A_SUBDIR ) && ( fIncludeList == 0 )) {

									strcat( pszArg, (( gpIniptr->UnixPaths ) && ( strchr( pszArg, '/' ) != NULL )) ? _TEXT("/") : _TEXT("\\")  );

								}
							}

							i = strlen( pszArg ) + strlen( pszEgetsBase );
							if ( c == F10 )
								i += 2;
							else
								i -= nLength;

							if ( i >= nMaxLength ) {
								pszArg = NULL;
								break;
							}

							// remove a path added for include lists
							if ( fIncludeList ) {
								strcpy( pszArg, fname_part( pszFileName ) );
								AddQuotes( pszArg );
							}

							if ( c == F7 ) {

								// allocate memory for 64 entries at a time
								if (( n % 64 ) == 0 ) {

									ulSize += 256;

									if (( lppList = (TCHAR _far * _far *)ReallocMem( lppList, ulSize )) == 0L ) {
										n = 0;
										break;
									}

									if ( n == 0 ) {
										HoldSignals();
										lpszPtr = (TCHAR _far *)AllocMem( &uListSize );
										uListSize -= 0xFF;
										lppList[0] = fpListStart = lpszPtr;
									}
								}

								// add filename to list
								i = strlen( pszArg ) + 1;

								if (( lppList == 0L ) || ( fpListStart == 0L ) || ((((unsigned int)( lpszPtr - fpListStart ) + i ) * sizeof(TCHAR)) >= uListSize )) {

									FreeMem( fpListStart );
									FreeMem( lppList );

									// free directory and FTP search handles!
									CloseDirSearchHandles( fURL, &dir );
									goto egets_key;
								}

								StripQuotes( pszArg );
								lppList[ n ] = _fstrcpy( lpszPtr, pszArg );
								lpszPtr += i;
							}

							// if this isn't a "get list" or "get previous" then break now
							if (( c != F7 ) && ( c != F8 )) {
								nOffset++;
								break;
							}

							if (( n++ >= nOffset ) && ( c == F8 ))
								break;
						}

						// display the popup selection list
						if (( c == F7 ) && ( n > 0 )) {

							if (( lpszPtr = wPopSelect( gpIniptr->PWTop, gpIniptr->PWLeft, gpIniptr->PWHeight, gpIniptr->PWWidth, lppList, n, 1, (( fEnvVar ) ? VARIABLES_TITLE : FILENAMES_TITLE ), NULL, NULL, 1 )) != 0L ) {

								_fstrcpy( pszFileName, lpszPtr );
								if ( fEnvVar == 0 ) {
									pszArg = strlast( pszFileName );
									if ( *pszArg == _TEXT('\\') ) {
										*pszArg = _TEXT('\0');
										AddQuotes( pszFileName );
										strcat( pszFileName, _TEXT("\\") );
									} else 
										AddQuotes( pszFileName );
								}
								pszArg = pszFileName;
							}

							FreeMem( fpListStart );
							FreeMem( lppList );

							// reenable signal handling after cleanup
							EnableSignals();

							if ( lpszPtr == 0L ) {
								// free directory and FTP search handles!
								CloseDirSearchHandles( fURL, &dir );
								goto egets_key;
							}

							// if we had a ^C in wPopSelect, abort after cleanup
							if ( cv.fException ) {
								crlf();
								// free directory and FTP search handles!
								CloseDirSearchHandles( fURL, &dir );
								longjmp( cv.env, -1 );
							}
						}

						if ( pszArg != NULL ) {

							// collapse the command line
							//   and insert new filename & path
							if ( c == F10 ) {
RepeatMatch:
								// insert space for next match
								if ( *pszArg != _TEXT(' ') )
									strins( pszArg, _TEXT(" ") );

								// skip past previous arg
								pszStart += nLength;

							} else {
								// clear to EOL
								clearline( pszStart );
								strcpy( pszStart, pszStart + nLength );
							}

							pszCurrentPos = pszStart;
							PositionEGets( pszCurrentPos );

							strins( pszCurrentPos, pszArg );
							efputs( pszCurrentPos );

							// put cursor at end of new argument
							nLength = strlen( pszArg );
							pszCurrentPos += nLength;

							// check for wrap at end of screen
							PositionEGets( strend( pszCurrentPos ));
							PositionEGets( pszCurrentPos );

							// adjust for inserted space in F10/F12
							if (( c == F10 ) || ( c == F12 )) {
								pszStart++;
								nLength--;
							}

						} else
							honk();

						if ( c == F7 ) {
							// free directory and FTP search handles!
							CloseDirSearchHandles( fURL, &dir );
							goto egets_key;
						}
TryAgain:
						n = c;
						c = cvtkey( GetKeystroke( EDIT_NO_ECHO | EDIT_SCROLL_CONSOLE | EDIT_SWAP_SCROLL ), MAP_GEN | MAP_EDIT );

						// check for ^A (toggle LFN / SFN)

						if (( fEnvVar == 0 ) && ( c == 1 ) && ( fWin95LFN )) {

							fLFN ^= 1;
							nOffset++;
							c = F8;

							// check for next character F8, F9, F10, or F12
						} else if ( c == F12 ) {

							if ( pszArg == NULL ) {
								honk();
								goto TryAgain;
							}

							goto RepeatMatch;

						} else if (( c != F8 ) && ( c != F9 ) && ( c != F10 )) {
							// free directory and FTP search handles!
							CloseDirSearchHandles( fURL, &dir );
							goto egots_key;
						}
					}
				}
			}

		default:

			// allow users to enter characters normally trapped by
			//  4xxx by preceding them with an ASCII 255
			if ( c == 0xFF ) {

				if ( *pszKeys == _TEXT('\0') )
					c = GetKeystroke( EDIT_NO_ECHO | nEditFlag );
				else
					c = *pszKeys++;

			} else {

				// clear high bit which may have been set by cvtkey
				c &= ~NORMAL_KEY;

				// check for a redefined key (Fn key, Alt key, etc.)
				if (( *pszKeys == _TEXT('\0') ) && (( c < 32 ) || ( c > FBIT ))) {

					// check for user defined key aliases
					for ( pszKeys = glpAliasList; ( *pszKeys != _TEXT('\0') ); pszKeys = next_env( pszKeys )) {

						if ( *pszKeys == _TEXT('@') ) {

							// @@n means "execute immediately, with
							//   no echo"
							if ( pszKeys[1] == _TEXT('@') ) {
								fExecuteNow = 1;
								fEcho = 0;
								pszKeys++;
							}

							// accept either "@59" or "@F1" syntax
							if ( isdigit( pszKeys[1] ) == 0 )
								pszKeys++;

							sscanf_far( pszKeys, _TEXT("%*[^=]=%n"), &n );

							// matched the current key?
							if (( n > 0 ) && (( i = keyparse( pszKeys, n-1 )) == c )) {
								// we found a key alias
								pszKeys += n;
								goto egets_key;
							}
						}

						fEcho = 1;
						fExecuteNow = 0;
					}
				}
			}

			// replace marked block?

			if (( pszMarkStart != (LPTSTR)-1L ) && ( pszMarkStart >= pszEgetsBase )) {

				CutEgets();
				if (( gnEditMode == 0 ) && ( *pszCurrentPos != _TEXT('\0') ))
					strcpy( pszCurrentPos, pszCurrentPos + 1 );
			}

			// check for invalid or overlength entry
			if (( c > FBIT ) || (( strlen( pszEgetsBase ) >= nMaxLength ) && (( *pszCurrentPos == _TEXT('\0') ) || ( gnEditMode ))))
				honk();

			else if (( nEditFlag & EDIT_DIGITS ) && ( isdigit( c ) == 0 ))
				honk();

			else {

				// open a hole for insertions
				if ( gnEditMode )
					memmove( pszCurrentPos + 1, pszCurrentPos, ( strlen( pszCurrentPos ) + 1 ) * sizeof(TCHAR) );
				else if ( *pszCurrentPos == TAB )	// collapse tab
					clearline( pszCurrentPos );

				// add new terminator if at EOL
				if ( *pszCurrentPos == _TEXT('\0') )
					pszCurrentPos[1] = _TEXT('\0');

				*pszCurrentPos = (TCHAR)c;
				if ( fEcho )
					efputs( pszCurrentPos );
				pszCurrentPos++;

				// adjust home row if necessary
				if ( fEcho )
					PositionEGets( strend( pszCurrentPos ));
			}
		}

		// get & set cursor position
		if ( fEcho )
			PositionEGets( pszCurrentPos );
	}
}


static void _fastcall CloseDirSearchHandles( int fURL, FILESEARCH *dir )
{
	DosFindClose( dir->hdir );

	dir->hdir = INVALID_HANDLE_VALUE;
}


// check the FileComplete .INI directive for a matching extension / type
static int _fastcall TabComplete( LPTSTR pszCommand, LPTSTR pszFilename, int nAttribute )
{
	register int i, j;
	int nLength, fNot;
	TCHAR *pszArg, *pszExt, szName[128], szExtensions[128];
	TCHAR _far *fpszComplete;
	TCHAR _far *lpszPtr;

	pszArg = (LPTSTR)_alloca( ( strlen( pszCommand ) + 1 ) * sizeof(TCHAR) );
	pszCommand = strcpy( pszArg, pszCommand );

	// if no FileCompletion variable, was FileCompletion defined in *.INI?

	if ((( fpszComplete = get_variable( _TEXT("FileCompletion") )) == 0L ) && ( gpIniptr->FC != INI_EMPTYSTR ))
		fpszComplete = (TCHAR _far *)( gpIniptr->StrData + gpIniptr->FC );

	if ( fpszComplete == 0L ) {
		if ( gpIniptr->CompleteHidden )
			return 1;
		return (( nAttribute & ( _A_HIDDEN | _A_SYSTEM )) ? 0 : 1 );
	}

	if ((( pszExt = ext_part( pszFilename )) != NULL ) && ( *pszExt == _TEXT('.') ))
		pszExt++;
	else
		pszExt = NULLSTR;

	while ( *fpszComplete ) {

		// get the next filename in completion list
		szName[0] = _TEXT('\0');
		nLength = 0;
		sscanf_far( fpszComplete, _TEXT(" %127[^ \t:;] %n"), szName, &nLength );

		// does it match the first command?
		pszArg = fname_part( pszCommand );
		if (( pszArg != NULL ) && ( stricmp( pszArg, szName ) == 0 ) && (( lpszPtr = _fstrchr( fpszComplete, _TEXT(':') )) != 0L )) {

			// get the matching extensions for this filename
			sscanf_far( ++lpszPtr, _TEXT("%127[^;]"), szExtensions );

			for ( i = 0; (( pszArg = ntharg( szExtensions, i )) != NULL ); i++ ) {

				if ( *pszArg == _TEXT('!') ) {
					pszArg++;
					fNot = 1;
				} else
					fNot = 0;

				if (( wild_cmp( pszArg, pszExt, TRUE, TRUE ) == 0 ) && (( nAttribute & ( _A_HIDDEN | _A_SYSTEM )) == 0 ))
					return ( fNot == 0 );

				// check for "complete by attributes"
				if ( strlen( pszArg ) > 3 ) {

					// check for "no dirs"
					if ( stricmp( pszArg, MANY_FILES ) == 0 )
						return (( nAttribute & _A_SUBDIR ) == 0 );

					for ( j = 0; ( j < 6 ); j++ ) {
						if ( stricmp( pszArg, colorize_atts[j].pszType ) == 0 ) {
							if ( nAttribute & colorize_atts[j].uAttribute )
								return 1;
							// special case for "normal"
							if (( j == 6 ) && ( nAttribute == 0 ))
								return 1;
						}
					}
				}
			}

			return 0;
		}

		fpszComplete += nLength;

		if (( *fpszComplete == _TEXT(':') ) || ( *fpszComplete == _TEXT(';') )) {
			// skip the extensions & get next filename
			while (( *fpszComplete ) && ( *fpszComplete++ != _TEXT(';') ))
				;
		}
	}

	if ( gpIniptr->CompleteHidden )
		return 1;

	return (( nAttribute & ( _A_HIDDEN | _A_SYSTEM )) ? 0 : 1 );
}


// return the length of the filename (bounded by szInvalidChars)
static int _fastcall FilenameLength( LPTSTR pszFileName )
{
	LPTSTR pszEnd;

	// determine the filename length
	// allow quoted filenames
	pszEnd = scan( pszFileName, szInvalidChars, QUOTES + 1 );

	return ((int)( pszEnd - pszFileName ));
}


// get & set column position in the current line
static int _fastcall PositionEGets( LPCTSTR pCur )
{
	LPTSTR pszPtr;

	nCursorColumn = nHomeColumn;

	// increment column position
	for ( pszPtr = pszEgetsBase; ( pszPtr != pCur ); pszPtr++ )
		incr_column( *pszPtr, &nCursorColumn );

	// check for line wrap
	nCursorRow = nHomeRow + ( nCursorColumn / nScreenColumns );
	nCursorColumn = ( nCursorColumn % nScreenColumns );

	// adjust cursor row for wrap at end of screen
	if ( nCursorRow > nScreenRows ) {
		nHomeRow -= ( nCursorRow - nScreenRows );
		nCursorRow = nScreenRows;
	}

	// set cursor to "pszCurrentPos"
	SetCurPos( nCursorRow, nCursorColumn );

	// return length in columns from pszEgetsBase to pszCurrentPos
	return ((( nCursorRow - nHomeRow ) * nScreenColumns ) + ( nCursorColumn - nHomeColumn ));
}


// scrub the line from "pszLine" onwards
static void _fastcall clearline( LPTSTR pszLine )
{
	register int i;

	i = PositionEGets( strend( pszLine ));

	// position cursor at "ptr" and fill line with blanks
	i -= PositionEGets( pszLine );
	PadBlanks( i );
	PositionEGets( pszLine );
}



// print the string to STDOUT
static void _fastcall efputs( LPCTSTR lpcszStr )
{
	register int nRow = 0;

	LPTSTR szPadStr;
	int nColumn, nMColor;

	// if password field, display *'s instead of the characters
	szPadStr = (( fPassword ) ? _TEXT("********") : NULLSTR );

	for ( ; *lpcszStr; lpcszStr++ ) {

		nColumn = nCursorColumn;
		incr_column( *lpcszStr, &nCursorColumn );

		if (( pszMarkStart != (LPTSTR)-1 ) && ( lpcszStr >= pszMarkStart ) && ( lpcszStr < pszMarkEnd )) {
			// switch to inverse color
			nMColor = nInverse;
		} else
			nMColor = nColor;

		if ( *lpcszStr == TAB ) {
			// do tab expansion
			color_printf( nMColor, FMT_LEFT_STR, ( nCursorColumn - nColumn ), szPadStr );
			nColumn = nCursorColumn;
		} else
			color_printf( nMColor, FMT_CHAR, (( fPassword ) ? _TEXT('*') : *lpcszStr ));

		if (( nMColor != -1 ) && ( nRow < ( nCursorColumn / nScreenColumns ))) {
			crlf();
			nRow++;
		}
	}
}


// print the specified number of spaces
static void _fastcall PadBlanks( int nWidth )
{
	color_printf( -1, FMT_LEFT_STR, nWidth, NULLSTR );
}


void CutEgets( void )
{
	register unsigned int i, n;
	LPTSTR pszCut;

	// we may be starting on a wrapped line (with embedded '\0's)

	for ( n = 0, pszCut = pszEgetsBase; ( pszCut < pszMarkStart ); pszCut++ ) {
		if ( *pszCut )
			n++;
	}

	for ( i = 0; ( pszCut < pszMarkEnd ); pszCut++ ) {
		if ( *pszCut )
			i++;
	}

	pszMarkStart = pszMarkEnd = (LPTSTR)-1L;

	// ignore marking past the end of the current line
	if ( n >= (unsigned int)strlen( pszEgetsBase ))
		return;

	pszCurrentPos = pszEgetsBase + n;

	if (( n = strlen( pszCurrentPos )) < i )
		i = n;

	// remove marked text
	clearline( pszCurrentPos );
	strcpy( pszCurrentPos, pszCurrentPos + i );
	efputs( pszCurrentPos );
	PositionEGets( pszCurrentPos );
}
