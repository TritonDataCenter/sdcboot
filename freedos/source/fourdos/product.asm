

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          ;
          ; Include file to set product and platform values
          ; for 4DOS / 4OS2 / 4PM / 4WIN / 4NT / etc. assembler files
          ;
          ;
          ; Product defaults
          ;
_4DOS     =         0                   ;1 if assembling for 4DOS
_4OS2     =         0                   ;1 if assembling for 4OS2
_4NT      =         0                   ;1 if assembling for 4NT
_TCMD16   =         0                   ;1 if assembling for TCMD/16
_TCMD32   =         0                   ;1 if assembling for TCMD/32
_TCMDOS2  =         0                   ;1 if assembling for TCMD/OS2
	;
_PRDBITS	=	0		;product bit flags
_PRDGERM	equ	100		;bit for German version
_PRDRT	equ	200		;bit for runtime version
_PRDBETA	equ	300		;bit for beta version
          ;
          ; Platform defaults
          ;
_DOS      =         0                   ;1 if assembling for DOS
_OS2      =         0                   ;1 if assembling for OS/2
_NT       =         0                   ;1 if assembling for Win32
_PM       =         0                   ;1 if assembling for OS/2 PM
_WIN      =         0                   ;16/32 if assembling for Win16/32
_FLAT     =         0                   ;1 if assembling for OS/2 or Windows
                                        ;  flat (32-bit) memory model
          ;
          ;
_MASM6_   =         1                   ;for Spontaneous Assembly
          ;
          ;
          ifndef    _BETA
_BETA     =         0
          endif
	;
	;
	ifdef	__4DOSRT
__4DOS	=	1
_RT	=	1
	endif
	;
	;
	ifdef	__4NTRT
__4NT	=	1
_RT	=	1
	endif
	;
	;
	ifdef	__TCMD32RT
__TCMD32	=	1
_RT	=	1
	endif
          ;
          ;
          ifndef    _RT
_RT       =         0
          endif
          ;
          if	_RT
_BP       =         0
	else
_BP	=	1
          endif
          ;
          ;
          ifdef     __4DOS
_4DOS     =         1
_DOS      =         1
          endif
          ;
	;
          ifdef     __4NT
_4NT      =         1
_NT       =         1
_FLAT     =         1
          endif
	;
	;
          ifdef     __TCMD32
_4NT      =         1
_TCMD32   =         1
_NT       =         1
_WIN      =         32
_FLAT     =         1
          endif
          ;
          ;
	;
	;
          if        _FLAT
          .386
CODE32    segment dword use32 public 'CODE'
CODE32    ends
DATA32    segment dword use32 public 'DATA'
DATA32    ends
CONST32   segment dword use32 public 'CONST'
CONST32   ends
BSS32     segment dword use32 public 'BSS'
BSS32     ENDS
          assume    cs:FLAT, ds:FLAT, ss:FLAT, es:FLAT  
          endif

