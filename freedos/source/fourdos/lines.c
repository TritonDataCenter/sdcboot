

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


//  LINES.C - line drawing for 4dos
//
// Draws a horizontal or vertical line of the specified width (single or 
//   double) using line drawing characters, starting at the specified row
//   and column of the current window and continuing for "len" characters.
//   The line can overlay what is already on the screen, so that the
//   appropriate T, cross or corner character is generated if it runs into
//   another line on the screen at right angles to itself.
//
// The algorithm involves looking at adjacent character cells to determine
// if the character generated should connect in one direction or another.

#include "product.h"

#include <stdio.h>
#include <string.h>

#include "4all.h"


static int _fastcall __drawline( LPTSTR, int );
int PASCAL _line( int, int, int, int, int, int, int );
static int _fastcall GetLineChar( int, int );


#define N   1
#define S   2
#define E   4
#define W   8
#define H2  16
#define V2  32

static TCHAR szBreakdown[] = {
	N|S,		// '³'
	N|S|W,		// '´'
	N|S|W|H2,	// 'µ'
	N|S|W|V2,	// '¶'
	S|W|V2,		// '·'
	S|W|H2,		// '¸'
	N|S|W|H2|V2,	// '¹'
	N|S|V2,		// 'º'
	W|S|H2|V2,	// '»'
	N|W|H2|V2,	// '¼'
	N|W|V2,		// '½'
	N|W|H2,		// '¾'
	W|S,		// '¿'
	N|E,		// 'À'
	N|W|E,		// 'Á'
	W|E|S,		// 'Â'
	N|S|E,		// 'Ã'
	E|W,		// 'Ä'
	N|S|E|W,	// 'Å'
	N|S|E|H2,	// 'Æ'
	N|S|E|V2,	// 'Ç'
	N|E|H2|V2,	// 'È'
	E|S|H2|V2,	// 'É'
	N|E|W|H2|V2,	// 'Ê'
	E|W|S|H2|V2,	// 'Ë'
	N|S|E|H2|V2,	// 'Ì'
	E|W|H2,		// 'Í'
	N|S|E|W|H2|V2,  // 'Î'
	N|E|W|H2,	// 'Ï'
	N|E|W|V2,	// 'Ð'
	E|W|S|H2,	// 'Ñ'
	E|W|S|V2,	// 'Ò'
	N|E|V2,		// 'Ó'
	N|E|H2,		// 'Ô'
	S|E|H2,		// 'Õ'
	S|E|V2,		// 'Ö'
	N|S|E|W|V2,	// '×'
	N|S|E|W|H2,	// 'Ø'
	N|W,		// 'Ù'
	E|S,		// 'Ú'
};


static TCHAR szLineChars[] = {
	' ',		// empty
	'³',		// N
	'³',		// S
	'³',		// S|N
	'Ä',		// E
	'À',		// E|N
	'Ú',		// E|S
	'Ã',		// E|S|N
	'Ä',		// W
	'Ù',		// W|N
	'¿',		// W|S
	'´',		// W|S|N
	'Ä',		// W|E
	'Á',		// W|E|N
	'Â',		// W|E|S
	'Å',		// W|E|S|N
	' ',		// H2
	'³',		// H2|N
	'³',		// H2|S
	'³',		// H2|S|N
	'Í',		// H2|E
	'Ô',		// H2|E|N
	'Õ',		// H2|E|S
	'Æ',		// H2|E|S|N
	'Í',		// H2|W
	'¾',		// H2|W|N
	'¸',		// H2|W|S
	'µ',		// H2|W|S|N
	'Í',		// H2|W|E
	'Ï',		// H2|W|E|N
	'Ñ',		// H2|W|E|S
	'Ø',		// H2|W|E|S|N
	' ',		// V2
	'º',		// V2|N
	'º',		// V2|S
	'º',		// V2|S|N
	'Ä',		// V2|E
	'Ó',		// V2|E|N
	'Ö',		// V2|E|S
	'Ç',		// V2|E|S|N
	'Ä',		// V2|W
	'½',		// V2|W|N
	'·',		// V2|W|S
	'¶',		// V2|W|S|N
	'Ä',		// V2|W|E
	'Ð',		// V2|W|E|N
	'Ò',		// V2|W|E|S
	'×',		// V2|W|E|S|N
	' ',		// V2|H2
	'º',		// V2|H2|N
	'º',		// V2|H2|S
	'º',		// V2|H2|S|N
	'Í',		// V2|H2|E
	'È',		// V2|H2|E|N
	'É',		// V2|H2|E|S
	'Ì',		// V2|H2|E|S|N
	'Í',		// V2|H2|W
	'¼',		// V2|H2|W|N
	'»',		// V2|H2|W|S
	'¹',		// V2|H2|W|S|N
	'Í',		// V2|H2|W|E
	'Ê',		// V2|H2|W|E|N
	'Ë',		// V2|H2|W|E|S
	'Î' 		// V2|H2|W|E|S|N
};


#pragma alloc_text( _TEXT, DrawHline_Cmd )

// draw a horizontal or vertical line directly to the display
int _near DrawHline_Cmd( LPTSTR pszCmdLine )
{
	return ( __drawline( pszCmdLine, 0 ));
}


#pragma alloc_text( _TEXT, DrawVline_Cmd )

// draw a horizontal or vertical line directly to the display
int _near DrawVline_Cmd( LPTSTR pszCmdLine )
{
	return ( __drawline( pszCmdLine, 1 ));
}


// draw a horizontal or vertical line directly to the display
static int _fastcall __drawline( LPTSTR pszCmdLine, int fVertical )
{
	register int nAttribute = -1;
	int nRow, nColumn, nLength, nStyle, n;

	// get the arguments & colors
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( (( fVertical ) ? DRAWVLINE_USAGE : DRAWHLINE_USAGE )));

	if ( sscanf( pszCmdLine, _TEXT("%d%d%d%d%n"), &nRow, &nColumn, &nLength, &nStyle, &n ) == 5 )
		nAttribute = GetColors( pszCmdLine+n, 0 );

	// if row or column == 999, center the line
	if ( nColumn == 999 ) {
		nColumn = ( GetScrCols() - (( fVertical ) ? 0 : nLength )) / 2;
		if ( nColumn < 0 )
			nColumn = 0;
	}

	if ( nRow == 999 ) {
		nRow = ( GetScrRows() - (( fVertical ) ? nLength : 0 )) / 2;
		if ( nRow < 0 )
			nRow = 0;
	}

	return ((( nAttribute == -1 ) || ( verify_row_col( nRow, nColumn )) || ( _line( nRow, nColumn, nLength, nStyle, fVertical, nAttribute, 1 ) != 0 )) ? Usage( (( fVertical ) ? DRAWVLINE_USAGE : DRAWHLINE_USAGE ) ) : 0 );
}


#define BOX_SHADOWED 1
#define BOX_ZOOMED 2

#pragma alloc_text( _TEXT, Drawbox_Cmd )

// draw a box directly to display memory
int _near Drawbox_Cmd( LPTSTR pszCmdLine )
{
	register TCHAR *pszArg, *pszLine;
	int nTop, nLeft, nBottom, nRight, nStyle, nAttribute = -1, nFill = -1, n, nFlags = 0, nShade;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( DRAWBOX_USAGE ));

	// get the arguments & colors
	if ( sscanf( pszCmdLine, _TEXT("%d%d%d%d%d%n"), &nTop, &nLeft, &nBottom, &nRight, &nStyle, &n ) == 6 ) {

		pszLine = pszCmdLine + n;
		nAttribute = GetColors( pszLine, 0 );

		// check for a FILL color
		if (( *pszLine ) && ( _strnicmp( first_arg( pszLine ), BOX_FILL, 3 ) == 0 ) && (( pszArg = first_arg( next_arg( pszLine, 1 ))) != NULL )) {

			if ( _strnicmp( pszArg, BRIGHT, 3 ) == 0 ) {
				// set intensity bit
				nFill = 0x80;
				if (( pszArg = first_arg( next_arg( pszLine, 1 ))) == NULL )
					return ( Usage( DRAWBOX_USAGE ));
			} else
				nFill = 0;

			if (( nShade = color_shade( pszArg )) <= 15 ) {
				nFill |= ( nShade << 4 );
				next_arg( pszLine, 1 );
			}
		}

		// check for a SHADOW or ZOOM
		while ( *pszLine ) {
			if ( _strnicmp( pszLine, BOX_SHADOW, 3 ) == 0 )
				nFlags |= BOX_SHADOWED;
			else if ( _strnicmp( pszLine, BOX_ZOOM, 3 ) == 0 )
				nFlags |= BOX_ZOOMED;
			next_arg( pszLine, 1 );
		}
	}

	if (( nAttribute == -1 ) || ( verify_row_col( nTop, nLeft )) || ( verify_row_col( nBottom, nRight )))
		return ( Usage( DRAWBOX_USAGE ));

	if ( nLeft == 999 ) {
		if (( nLeft = (( GetScrCols() - nRight ) / 2 )) < 0 )
			nLeft = 0;
		nRight += nLeft;
	}

	if ( nTop == 999 ) {
		if (( nTop = (( GetScrRows() - nBottom ) / 2 )) < 0 )
			nTop = 0;
		nBottom += nTop;
	}

	_box( nTop, nLeft, nBottom, nRight, nStyle, nAttribute, nFill, nFlags, 1 );

	return 0;
}


// draw a box, with an optional shadow & connectors
void PASCAL _box( register int nTop, int nLeft, int nBottom, int nRight, int nStyle, int nAttribute, int nFill, int nFlags, int nConnector )
{
	register int nWidth;
	int nZoomTop, nZoomBottom, nZoomLeft, nZoomRight;
	int nRowDiff, nColumnDiff, nRowInc = 0, nColumnInc = 0;

	// zoom the window?
	if ( nFlags & BOX_ZOOMED ) {

		// zooming requires a fill color; use default if none specified
		if ( nFill == -1 )
			GetAtt( (unsigned int *)&nFill, (unsigned int *)&nZoomTop );

		nZoomTop = nZoomBottom = ( nTop + nBottom ) / 2;
		nZoomLeft = nZoomRight = ( nLeft + nRight ) / 2;

		// get the increment for each zoom
		// (This makes the zoom smooth in all dimensions)
		if (( nRowDiff = nZoomTop - nTop ) <= 0 )
			nRowDiff = 1;
		if (( nColumnDiff = nZoomLeft - nLeft ) <= 0 )
			nColumnDiff = 1;

		if ( nRowDiff > nColumnDiff ) {
			// tall skinny box
			nRowInc = ( nRowDiff / nColumnDiff );
			nColumnInc = 1;
		} else {
			// short wide box
			nColumnInc = ( nColumnDiff / nRowDiff );
			nRowInc = 1;
		}

	} else {

		nZoomTop = nTop;
		nZoomBottom = nBottom;
		nZoomLeft = nLeft;
		nZoomRight = nRight;
	}

	do {

		if ( nFlags & BOX_ZOOMED ) {

			// if zooming, increment the box size
			nZoomTop -= nRowInc;
			if ( nZoomTop < nTop )
				nZoomTop = nTop;

			nZoomBottom += nRowInc;
			if ( nZoomBottom > nBottom )
				nZoomBottom = nBottom;

			nZoomLeft -= nColumnInc;
			if ( nZoomLeft < nLeft )
				nZoomLeft = nLeft;

			nZoomRight += nColumnInc;
			if ( nZoomRight > nRight )
				nZoomRight = nRight;
		}

		// clear the box to the specified attribute
		if ( nFill != -1 )
			Scroll( nZoomTop, nZoomLeft, nZoomBottom, nZoomRight, 0, nFill );

		if ( nStyle == 0 )
			nWidth = 0;
		else if (( nStyle == 2 ) || ( nStyle == 4 ))
			nWidth = 2;
		else
			nWidth = 1;

		// draw the two horizontals & the two verticals
		_line( nZoomTop, nZoomLeft, (nZoomRight-nZoomLeft)+1, nWidth, 0, nAttribute, nConnector );
		_line( nZoomBottom, nZoomLeft, (nZoomRight-nZoomLeft)+1, nWidth, 0, nAttribute, nConnector );

		if ( nStyle == 3 )
			nWidth = 2;
		else if ( nStyle == 4 )
			nWidth = 1;

		_line( nZoomTop, nZoomLeft, (nZoomBottom-nZoomTop)+1, nWidth, 1, nAttribute, nConnector );
		_line( nZoomTop, nZoomRight, (nZoomBottom-nZoomTop)+1, nWidth, 1, nAttribute, nConnector );

		// slow things down a bit
		SysBeep( 0, 1 );

	} while (( nFlags & BOX_ZOOMED ) && (( nZoomTop > nTop ) || ( nZoomBottom < nBottom ) || ( nZoomLeft > nLeft ) || ( nZoomRight < nRight )));

	// check for a shadow
	if ( nFlags & BOX_SHADOWED ) {

		nLeft += 2;
		nRight++;
		if ( nLeft >= nRight )
			nLeft = nRight - 1;

		// read the character and attribute, and change
		//   the attribute to black background, low intensity
		//   foreground
		SetLineColor( ++nBottom, nLeft, (nRight-nLeft), 7 );

		// shadow the right side of the window
		for ( nTop++; ( nTop <= nBottom ); nTop++ )
			SetLineColor( nTop, nRight, 2, 7 );
	}
}


// draw a line, making proper connectors along the way
int PASCAL _line( int nRow, int nColumn, int nLength, int nWidth, int nDirection, int nAttribute, int nConnector )
{
	register int ch, i;
	int nSavedRow, nSavedColumn, nBits, nBottom, nRight;
	TCHAR szBuffer[256];

	// truncate overly long lines
	if ( nLength > 255 )
		nLength = 255;

	// save starting row & column
	nSavedRow = nRow;
	nSavedColumn = nColumn;

	nBottom = GetScrRows();
	nRight = GetScrCols() - 1;

	// check for non-ASCII character sets
	if ( QueryCodePage() == 932 )
		nWidth = 0;

	for ( i = 0; ( i < nLength ); ) {

		// Read original character - if not a line drawing char,
		//   just write over it.  Otherwise, try to make a connector
		//   if "connector" != 0
		if ( nWidth == 0 )
			szBuffer[i] = gchBlock;

		else if ((( nConnector == 0 ) && ( i != 0 ) && ( i != nLength - 1 )) || (( ch = GetLineChar( nRow, nColumn )) == -1 )) {

			if ( nDirection == 0 )
				szBuffer[i] = (( nWidth == 1 ) ? 'Ä' : 'Í' );
			else
				szBuffer[i] = (( nWidth == 1 ) ? '³' : 'º' );

		} else {

			nBits = (( nDirection == 0 ) ? ( szBreakdown[ch] & ~H2 ) | W | E | (( nWidth == 1 ) ? 0 : H2 ) : ( szBreakdown[ch] & ~V2 ) | N | S | (( nWidth == 1 ) ? 0 : V2 ));

			if (( i == 0 ) || ( nDirection )) {

				// at start look & see if connect needed
				nBits &= ~W;

				if (( nColumn > 0 ) && (( ch = GetLineChar( nRow, nColumn-1 )) >= 0 )) {
					if ( szBreakdown[ ch ] & E )
						nBits |= W;
				}
			}

			if (( i == nLength - 1 ) || ( nDirection )) {

				// at end look & see if connect needed
				nBits &= ~E;

				if ((nColumn < nRight) && ((ch = GetLineChar(nRow, nColumn+1)) >= 0)) {
					if (szBreakdown[ch] & W)
						nBits |= E;
				}
			}

			if (( nDirection == 0 ) || ( i == 0 )) {

				// at start look & see if connect needed
				nBits &= ~N;

				if (( nRow > 0 ) && (( ch = GetLineChar(nRow-1, nColumn)) >= 0)) {
					if ( szBreakdown[ ch ] & S )
						nBits |= N;
				}
			}

			if (( nDirection == 0 ) || ( i == nLength - 1 )) {

				// at end look & see if connect needed
				nBits &= ~S;

				if (( nRow < nBottom ) && (( ch = GetLineChar(nRow+1, nColumn)) >= 0)) {
					if ( szBreakdown[ ch ] & N )
						nBits |= S;
				}
			}

			szBuffer[i] = szLineChars[ nBits ];
		}

		i++;

		if ( nDirection == 0 ) {
			if ( ++nColumn > nRight )
				break;
		} else {
			if ( ++nRow > nBottom )
				break;
		}
	}

	szBuffer[i] = '\0';

	// write the line directly to the display
	if ( nDirection == 0 )
		WriteStrAtt( nSavedRow, nSavedColumn, nAttribute, szBuffer );
	else
		WriteVStrAtt( nSavedRow, nSavedColumn, nAttribute, szBuffer );

	return 0;
}


// Make sure the specified row & column are on the screen!
int _fastcall verify_row_col( unsigned int nRow, unsigned int nColumn )
{
	return ((( nRow > (unsigned int)GetScrRows() ) && ( nRow != 999 )) || (( nColumn > (unsigned int)( GetScrCols() - 1 )) && ( nColumn != 999 )));
}


// Read the character at the specified cursor location.
//   Return -1 if not a line drawing char, or the offset into the "szBreakdown"
//   table if it is.
static int _fastcall GetLineChar( int nRow, int nColumn )
{
	char caCell[4];

	ReadCellStr( caCell, 1, nRow, nColumn );

	return ((( caCell[0] < 179 ) || ( caCell[0] > 218 )) ? -1 : caCell[0] - 179 );
}
