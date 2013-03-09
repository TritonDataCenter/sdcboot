

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


// IOFMT.C - Replacement I/O routines for 4xxx family
//   (c) 1993 - 1999 Rex C. Conn  All rights reserved

#include "product.h"

#include <io.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "4all.h"

extern int _pascal _fmtout(int, char _far *, const char _near *, va_list);
extern int _pascal _fmtin(const char _far *, const char _near *, va_list);


// replace the RTL sscanf()
int _cdecl sscanf( const char *source, const char *fmt, ... )
{
	va_list arglist;

	va_start( arglist, fmt );
	return ( _fmtin( source, fmt, arglist ));
}


// far source version of sscanf
int _cdecl sscanf_far( const char _far *source, const char *fmt, ... )
{
	va_list arglist;

	va_start( arglist, fmt );
	return (_fmtin( source, fmt, arglist ));
}


// replaces the RTL sprintf()
int _cdecl sprintf( char *dest, const char *fmt, ... )
{
	va_list arglist;

	va_start( arglist, fmt );
	return (_fmtout( -1, dest, fmt, arglist ));
}


// far target version of sprintf()
int _cdecl sprintf_far( char _far *dest, const char *fmt, ... )
{
	va_list arglist;

	va_start( arglist, fmt );
	return ( _fmtout( -1, dest, fmt, arglist ));
}


// convert integer to ASCII
void _fastcall IntToAscii( int nVal, char *szBuf )
{
	sprintf( szBuf, FMT_INT, nVal );
}


// fast printf() replacement function, with handle specification
int _cdecl qprintf( int handle, const char *format, ... )
{
	va_list arglist;

	va_start( arglist, format );
	return ( _fmtout( handle, 0L, format, arglist ));
}


// fast printf() replacement function (always writes to STDOUT)
int _cdecl printf(const char *format, ...)
{
	va_list arglist;

	va_start( arglist, format );
	return ( _fmtout( STDOUT, 0L, format, arglist ));
}


// colorized printf()
int _cdecl color_printf( int attrib, const char *format, ... )
{
	register int length;
	va_list arglist;
	char dest[256];
	int row, column;

	va_start( arglist, format );

	if ( attrib != -1 ) {

		length = _fmtout( -1, dest, format, arglist );

		// get current cursor position
		GetCurPos( &row, &column );

		// write the string & attribute
		WriteStrAtt( row, column, attrib, dest );

		SetCurPos( row, column+length );
	} else
		length =_fmtout( STDOUT, 0L, format, arglist );

	return length;
}


// write a string to STDOUT
int _fastcall qputs( LPTSTR string )
{
	return (_write( STDOUT, string, strlen( string ) ));
}


// print a CR/LF pair to STDOUT
void crlf( void )
{
	_write( STDOUT, gszCRLF, 2 );
}


// write a character (no buffering) to the specified handle
void _fastcall qputc( int handle, unsigned char c )
{
	_write( handle, &c, 1 );
}

