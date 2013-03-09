

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


// DOSTTY.C - 4DOS-specific tty i/o support routines (see also 4DOSUTIL.ASM)
//   Copyright 1998-2002 Rex C. Conn

#include "product.h"

#include <stdio.h>
#include <conio.h>
#include <io.h>

#include "4all.h"

static int fMousePresent = 0;


#pragma alloc_text( _TEXT, GetKeystroke )

// get a keystroke from STDIN, with optional echoing & CR/LF
unsigned int PASCAL GetKeystroke( register unsigned int eflag )
{
	int c, fClose = 0;
	int nRow, nColumn, nButton;

GetAnotherKeystroke:
	if ( eflag & ( EDIT_COMMAND | EDIT_MOUSE_BUTTON )) {

		// test for "Close" selected in Win95
		fClose = ( Win95EnableClose() == 0 );

		nButton = 0;
		while ( _kbhit() == 0 ) {

			// check for mouse button pressed
			if ( eflag & EDIT_MOUSE_BUTTON ) {

				GetMousePosition( &nRow, &nColumn, &nButton );
				if ( nButton & 1 )
					return LEFT_MOUSE_BUTTON; 		// left button
				if ( nButton & 2 )
					return RIGHT_MOUSE_BUTTON;		// right button
				if ( nButton & 4 )
					return MIDDLE_MOUSE_BUTTON;		// middle button
			}

			if (( fClose ) && ( Win95QueryClose() == 0 )) {
				Win95AckClose();
				Exit_Cmd( NULLSTR );
			}
_asm {
			mov       ax,1680h
			int       2Fh		; give up Win timeslice
}
		}
	}

	if ( eflag & EDIT_BIOS_KEY )
		c = bios_key();
	else {
		if ( QueryIsConsole( STDIN ) == 0 ) {
			c = 0;
			if ( _read( STDIN, (void *)&c, 1 ) <= 0 )
				c = EoF;
		} else
			c = _getch();
	}

	// The new keyboards return peculiar codes for separate cursor pad
	if (( c == 0 ) || (( c == 0xE0 ) && ( _kbhit() ))) {

		c = _getch() + FBIT;	// get extended scan code

		// get line marking keys
_asm {
		mov	ah, 02h
		mov	al, 0
		int	16h
		mov	nButton, ax
}
		// left or right shift keys down?
		if ( nButton & 3 ) {

			// left or right Ctrl keys down?
			if ( nButton & 4 ) {
				if ( c == CTL_RIGHT )
					c = CTL_SHIFT_RIGHT;
				else if ( c == CTL_LEFT )
					c = CTL_SHIFT_LEFT;
			} else if ( c == CUR_LEFT )
				c = SHIFT_LEFT;
			else if ( c == CUR_RIGHT )
				c = SHIFT_RIGHT;
			else if ( c == HOME )
				c = SHIFT_HOME;
			else if ( c == END )
				c = SHIFT_END;
		}
	}

	if ( c < FBIT ) {

		// check for upper case requested
		c = (( eflag & EDIT_UC_SHIFT ) ? _ctoupper( c ) : c );

		if (( eflag & EDIT_NO_ECHO ) == 0 )
			qputc( STDOUT, (char)(( eflag & EDIT_PASSWORD ) ? '*' : c ));
#if _RT == 0
	} else if (( c == CTL_F5 )&& ( cv.bn >= 0 ) && (( bframe[cv.bn].nFlags & BATCH_COMPRESSED ) == 0 )) {
		gpIniptr->SingleStep ^= 1;
		goto GetAnotherKeystroke;
#endif
	}

	// check for CRLF requested
	if ( eflag & EDIT_ECHO_CRLF )
		crlf();

	if ( fClose )
		Win95DisableClose();

	return (unsigned int)c;
}


// return the current number of screen rows (make it 0 based)
unsigned int GetScrRows( void )
{
	if ( gpIniptr->Rows != 0 )
		return ( gpIniptr->Rows - 1 );

_asm {
        push    bx
        mov     ax,01A00h		; function 1A returns active adapter
        int     10h			; al will return as 1a if supported
	mov	cx,24			; default to 24 (based 0)
        cmp     al,01ah
        jnz     not_vga

	cmp	bl,4			; is it an EGA or VGA?
	jb	not_vga			;   nope
	xor	ax,ax
	mov	es,ax
	xor	ch,ch
	mov	cl,es:[0484h]		; yes, so get rows from BIOS data area
not_vga:
	mov	ax,cx
        pop     bx
}
}


// return the current number of screen columns
unsigned int GetScrCols( void )
{
	// get the number of columns from the BIOS data area
	//   (TI Professionals return 0!)
	if ( gpIniptr->Columns != 0 )
		return gpIniptr->Columns;
_asm {
	xor	ax,ax
	mov	es,ax
	mov	ax,es:[044Ah]
	or	ax,ax
	jnz	not_ti_pro
	mov	ax,80
not_ti_pro:
}
}


// set the cursor shape; the defaults are in gpIniptr (CursO & CursI)
// "c_type" defaults are:
//	0 - overstrike
//	1 - insert
void PASCAL SetCurSize( void )
{
	// hi byte=start, low byte=end
	register unsigned int uShape, uType;

	// if CursO or CursI == -1, don't attempt to modify cursor at all
	if (( gpIniptr->CursO != -1 ) && ( gpIniptr->CursI != -1 )) {

	    // set the start lines for the cursor cell (as percentages)
	    // if 0%, make cursor invisible by setting start & end 0xFF
	    if (( uType = (( gnEditMode ) ? gpIniptr->CursI : gpIniptr->CursO )) == 0 )
		uShape = 0xFFFF;
	    else {
		// get the start & end lines for the cursor cell
		uShape = GetCellSize( uType );
	    }
_asm {
	    mov    ax, 0100h
	    mov    cx, uShape
	    push   bp		; for PC1 bug
	    int    10h		; set the cursor shape
	    pop    bp
}
	}

	// check for no background/blinking diddling requested
	SetBrightBG();
}


// Set bright background / blink foreground
void SetBrightBG( void )
{
	register unsigned int uBright;

	if (gpIniptr->BrightBG == 2)
		return;

	// set bright/blink background (have to reverse the meaning, because
	//   0=intensity & 1=blinking for BIOS call)
	uBright = (gpIniptr->BrightBG == 0);
_asm {
	xor	ax,ax
	mov	es,ax
	mov	ax,es:[0465h]		; look for 3x8 register status word
	and	ax,020h			; bit 5 = bright or blink bit
	mov	cl,5			; bug in MSC requires us to do this
	shr	al,cl			;   in two steps
	cmp	ax,uBright		; if already the same, do not
	je	no_toggle		;   set bit again
	mov	ax,1003h
	mov	bx,uBright
	int	10h			; toggle blink/intensity bit
no_toggle:
}
}


int PASCAL keyparse( char _far *keystr, int klen )
{
	extern void _far _KeyParse( void );

	int rval = -1;

	// under DOS, just call the ASM routine
_asm {
	mov	cx,klen
	push	ds
	push	si
	lds	si,keystr
	call	_KeyParse
	pop	si
	pop	ds
	jbe	KP_DONE
	mov	rval,ax
KP_DONE:
}
	return rval;
}


// resets mouse - returns non-zero if mouse installed
int MouseReset( void )
{
	if ( gpIniptr->Mouse == 1 )
		fMousePresent = 1;
	else if ( gpIniptr->Mouse == 2 )
		fMousePresent = 0;
	else {
_asm {
		mov	ax, 0
		int	033h
		mov	fMousePresent, ax
	}
}
	return fMousePresent;
}


// turn on mouse cursor
void MouseCursorOn( void )
{
	if ( fMousePresent ) {
_asm {
		mov	ax, 1
		int	033h
}
	}
}


// turn off mouse cursor
void MouseCursorOff( void )
{
	if ( fMousePresent ) {
_asm {
		mov	ax, 2
		int	033h
}
	}
}


// get mouse position & button state
void GetMousePosition( int *nRow, int *nCol, int *nButton )
{
	*nButton = *nRow = *nCol = 0;
	if ( fMousePresent ) {
_asm {
		push	si
		push	bx
		mov	ax, 3
		int	033h
		mov	si, nRow
		mov	[si], dx
		mov	si, nCol
		mov	[si], cx
		mov	si, nButton
		mov	[si], bx
		pop	bx
		pop	si
}
		*nRow >>= 3;
		*nCol >>= 3;
	}
}


// move the mouse to the specified row/column
void SetMousePosition( int nRow, int nCol )
{
	if ( fMousePresent ) {

		nRow <<= 3;		// adjust for pixel count
		nCol <<= 3;
_asm {
		mov	ax, 4
		mov	cx, nCol
		mov	dx, nRow
		int	033h
}
	}
}

