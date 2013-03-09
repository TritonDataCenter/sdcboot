

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


;    WIN95.ASM -- 4DOS long filename replacement functions
;
;    Functions:
;	_chdir
;	_dos_getfileattr
;	_dos_setfileattr
;	_mkdir
;	_open
;	remove (_unlink)
;	rename
;	_rmdir
;	_sopen
;
;	Win95AckClose()
;	Win95EnableClose()
;	Win95DisableClose()
;	Win95QueryClose()
;	Win95GetTitle()
;
;    LFN Functions:
;	 FindClose
;	 FindFirstFile
;	 FindNextFile
;	 GetLastError
;
;    Assemble with MASM 6:
;	 ml -c win95.asm
;
;==============================================================================

	 name  win95
	 .model MEDIUM, c

byp	 equ   <byte ptr>
wp	 equ   <word ptr>
dwp	 equ   <dword ptr>

ENABLE_LFN equ 2
ENABLE_SFN equ 4


	include	product.asm
	include	inistruc.asm

;    WIN32_FIND_DATA structure:

fd	struct
  dwFileAttributes   dd ?
  ftCreationTime     dq ?
  ftLastAccessTime   dq ?
  ftLastWriteTime    dq ?
  nFileSizeHigh	     dd ?
  nFileSizeLow	     dd ?
  dwReserved0	     dd ?
  dwReserved1	     dd ?
  cFileName	     db 260 dup (?)
  cAlternateFileName db 14 dup (?)
fd	ends


	 .data

	 extrn _fmode:word
         extrn fWin95LFN:word

	 extrn _umaskval:word
	 extrn _nfile:word
	 extrn _osfile:byte

	extrn _doserrno:word

lasterr  dw    0


	 .code	MISC_TEXT

DEBUG	=	0
	include	DEBUG.ASM	; debugging code


Win95EnableClose proc
	 mov   dx, 1
	 jmp   W95Op		; join common code with Win95DisableClose
Win95EnableClose endp

Win95DisableClose proc
	 mov   dx, 0
W95Op::
         mov   ax, 0168Fh
         int   2Fh
         ret			; returns 0 if successful

Win95DisableClose endp


Win95QueryClose proc
         mov   ax, 0168Fh
	 mov   dx, 0100h
         int   2Fh
         ret
Win95QueryClose endp


Win95AckClose proc
         mov   ax, 0168Fh
	 mov   dx, 0200h
         int   2Fh
         ret
Win95AckClose endp


;------------------------------------------------------------------------------
;
;    int _chdir(char *dirname);
;    int _mkdir(char *dirname);
;
;------------------------------------------------------------------------------

_mkdir	 proc  uses si, dirname:ptr byte
	 mov   ax, 7139h	  ; fcn 7139: make directory (long name)
	 jmp   dirop		  ; join common code with _chdir
_mkdir	 endp

_chdir	 proc  uses si, dirname:ptr byte
	 mov   ax, 713Bh	  ; fcn 713B: change directory (long name)

dirop::
	 mov   dx, dirname	  ;   ..
	 mov   cl, al		  ; save function code

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
         je    chdir_old	  ;if not go do old call

	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case fcn not supported
	 int   21h		  ; issue long filename form of request
	 jnc   chdir_good	  ; skip ahead if success
	 cmp   ax, 7100h	  ; did it fail because fcn not supported?
	 jne   chdir_bad	  ; if not, error

chdir_old:
	 mov   ah, cl		  ; reissue 8.3 form of request
	 int   21h		  ;   ..
         jc    chdir_bad

chdir_good:
         xor   ax,ax
         jmp   short chdir_bye

chdir_bad:
         mov   _doserrno, ax
         mov   ax, -1
chdir_bye:
         ret

_chdir	 endp


;------------------------------------------------------------------------------
;
;    int _dos_getcwd( int disk, char *dirname );
;
;------------------------------------------------------------------------------

_dos_getcwd proc	uses bx dx si, disk:word, dirname:ptr byte
	mov	dx, disk
	mov	si, dirname

        cmp     fWin95LFN, 0       ; Win95 LFNs enabled?
	je	gcd_old

	mov	ax, 7147h	  	;fcn 7147: get current directory (long name version)
	stc				;set carry in case not supported
	int	21h
	jnc	gcd_good		;skip if call succeed in long-name form
	cmp	ax, 7100h	  	;check for bona fide error
	jne	gcd_bad			;skip if so
	;
gcd_old:	mov	ah,047h		;reissue as 8.3 call
	int	21h
	jc	gcd_bad
	;
gcd_good:	xor	ax,ax
	jmp	short gcd_bye
	;
gcd_bad:	mov	_doserrno, ax
	;
gcd_bye:	ret
	;
_dos_getcwd endp


;------------------------------------------------------------------------------
;
;    unsigned _dos_getfileattr(char *filename, unsigned *attrib);
;
;------------------------------------------------------------------------------

_dos_getfileattr proc uses bx si, filename:ptr byte, attrib:ptr byte

	 mov   ax, 7143h	  ; fcn 7143: get/set attributes (long name)
	 xor   bl, bl		  ; BL = 0 to indicate "get" fcn

	 mov   dx, filename

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	 je    getatt_old	  ;if not go do old call

	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long filename form of request
	 jnc   getatt_good	  ; skip ahead on success
	 cmp   ax, 7100h	  ; is fcn supported?
	 jne   getatt_bad	  ; if not, we've got error

getatt_old:
	 mov   ax, 4300h	  ; fcn 4300: get attribute
	 int   21h		  ; issue 8.3 form of request
	 jc    getatt_bad	  ; skip ahead if error

getatt_good:
	 mov   bx, attrib
	 mov   [bx], cx		  ; store attribute byte from CX
	 xor   ax, ax		  ; indicate successful call
         jmp   short getatt_bye

getatt_bad:
         mov   _doserrno, ax
         mov   ax, -1
getatt_bye:
         ret

_dos_getfileattr endp


;------------------------------------------------------------------------------
;
;    unsigned _dos_setfileattr(char *filename, unsigned attrib);
;
;------------------------------------------------------------------------------

_dos_setfileattr proc uses bx si, filename:ptr byte, attrib:word

	 mov   ax, 7143h	  ; fcn 7143: get/set attributes (long name)
	 mov   bl, 1		  ; BL = 1 to indicate "set" fcn
	 mov   cx, attrib	  ; CX = new attributes for file

	 mov   dx, filename

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	 je    setatt_old	  ;if not go do old call

	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long filename form of request
	 jnc   setatt_good	  ; skip ahead on success
	 cmp   ax, 7100h	  ; is fcn supported?
	 jne   setatt_bad	  ; if not, we've got error

setatt_old:
	 mov   ax, 4301h	  ; fcn 4301: set attribute
	 int   21h		  ; issue 8.3 form of request
         jc    setatt_bad

setatt_good:
         xor   ax, ax
         jmp   short setatt_bye

setatt_bad:
         mov   _doserrno, ax
         mov   ax, -1
setatt_bye:
         ret

_dos_setfileattr endp


;------------------------------------------------------------------------------
;
;    int remove(char *filename);
;
;------------------------------------------------------------------------------

	 public remove
remove:

_unlink  proc  uses si, filename:ptr byte

	 mov   dx, filename

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	 je    unlink_old		;if not go do old call

	 mov   ax, 7141h	  ; fcn 7141: delete file
         xor   cx, cx
	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long filename form of request
	 jnc   unl_good		  ; skip ahead if okay
	 cmp   ax, 7100h	  ; is fcn supported?
	 jne   unl_bad

unlink_old:
	 mov   ah, 41h		  ; issue 8.3 form of request
	 int   21h
         jc    unl_bad

unl_good:
         xor   ax, ax
         jmp   short unl_bye

unl_bad:
         mov   _doserrno, ax
         mov   ax, -1
unl_bye:
         ret

_unlink  endp


;------------------------------------------------------------------------------
;
;    int rename(char *oldname, char *newname);
;
;------------------------------------------------------------------------------

rename	 proc  uses si, oldname:ptr byte, newname:ptr byte

	 push  di		  ; save working register

	 mov   dx, oldname
         push  ds
         pop   es
	 mov   di, newname

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	 je    rename_old		;if not go do old call

	 mov   ax, 7156h	  ; fcn 7156: long name form of rename
	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long filename form of rename
	 jnc   ren_good		  ; skip ahead on success
	 cmp   ax, 7100h	  ; is fcn supported?
	 jne   ren_bad

rename_old:
	 mov   ah, 56h		  ; resissue 8.3 form of request
	 int   21h
         jc    ren_bad

ren_good:
         xor   ax, ax
         jmp   short ren_bye

ren_bad:
         mov   _doserrno, ax
         mov   ax, -1
ren_bye:
         pop   di
         ret

rename	 endp


;------------------------------------------------------------------------------
;
;    int _rmdir(char *dirname);
;
;------------------------------------------------------------------------------

_rmdir	 proc  uses si, dirname:ptr byte

	 mov	dx, dirname

         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	 je    rd_old		  ;if not go do old call

	 mov   ax, 713Ah	  ; fcn 713A: remove directory (long name)
	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long name version of request
	 jnc   rd_good		  ; skip if no error
	 cmp   ax, 7100h	  ; did it fail because it's not supported?
	 jne   rd_bad		  ; if not, handle error

rd_old:
	 mov   ah, 3Ah		  ; reissue 8.3 form of request
	 int   21h
         jnc   rd_good

rd_bad:
         mov   _doserrno, ax
         mov   ax, -1
         jmp   short rd_bye

rd_good:
         xor   ax, ax
rd_bye:
         ret

_rmdir	 endp


;------------------------------------------------------------------------------
;
;    int _open(char *filename, int oflag [, int pmode]);
;    int _sopen(char *filename, int oflag, int shflag [, int pmode]);
;
;------------------------------------------------------------------------------

;    Open flags (from fcntl.h):

_O_RDONLY      equ 0000h
_O_WRONLY      equ 0001h
_O_RDWR        equ 0002h
_O_APPEND      equ 0008h

_O_CREAT       equ 0100h
_O_TRUNC       equ 0200h
_O_EXCL        equ 0400h

_O_TEXT        equ 4000h
_O_BINARY      equ 8000h

;    Sharing flags (from share.h):

_SH_COMPAT     equ 00h
_SH_DENYRW     equ 10h
_SH_DENYWR     equ 20h
_SH_DENYRD     equ 30h
_SH_DENYNO     equ 40h

;    Permission flags (from stat.h):

_S_IWRITE      equ 0080h
_S_IREAD       equ 0100h

_sopen	 proc  uses bx si, filename:ptr byte, oflag:word, shflag:word, pmode:word
	 local fileflag:byte, junk:byte
	 xor   bx, bx
	 mov   bl, byp shflag	  ; pickup sharing arg
	 mov   ax, pmode	  ; copy optional permission setting
	 mov   shflag, ax	  ;   down to where _open can find it
	 jmp   open_1		  ; join common code with _open
_sopen	 endp

_open	 proc  uses bx si, filename:ptr byte, oflag:word, pmode:word
	 local fileflag:byte, junk:byte
	 xor   bx, bx		  ; share flags: compatibility mode

open_1::
           xor   cx, cx         ; default pmode to 0 for wacky network clients
                                ; that don't ignore it when they should
	 mov   ax, oflag	  ; pickup open flags
	 mov   fileflag, 0	  ; assume binary access
	 mov   dx, 01h		  ; open if exists, fail if doesn't

;    Select binary or text. If none specified, use the global setting

	 test  ax, _O_BINARY	  ; is binary specified?
	 jnz   open_2		  ; if yes, okay
	 test  ax, _O_TEXT	  ; how about text?
	 jnz   @F		  ; if yes, also okay
	 test  _fmode, _O_BINARY  ; is default to use binary?
	 jnz   open_2		  ; if yes, skip ahead
@@:
	 mov   byp fileflag, 80h  ; set flag for text access

;    Compose open mode and action flags for eventual 21/716C

open_2:
	 and   al, 3		  ; preserve open mode 0, 1, or 2
	 or    bl, al		  ; merge with sharing flags left in BL

	 test  ax,_O_TRUNC	  ; truncate existing file?
	 jz    @F		  ; if not, skip
	 mov   dl, 02h		  ; yes. set action flag accordingly
@@:
	 test  ax, _O_CREAT	  ; might we create a missing file?
	 jz    open_3		  ; if not, skip ahead

	 or    dl, 10h		  ; yes. set action flag
	 test  ax, _O_EXCL	  ; fail open if file exists?
	 jz    @F		  ; if not, action is okay
	 and   dl, not 0Fh	  ; yes. modify action accordingly
@@:
	 mov   cx, pmode	  ; get permission mode argument
	 call  _cXENIXtoDOSmode   ; convert to DOS format
         mov   pmode, cx

;    Setup and issue long filename extended open request. The
;    registers have already been setup as follows:
;	 BH (flags) = 0
;	 BL = open mode and sharing flags
;	 CX = attributes for new file
;	 DH (reserved) = 0
;	 DL = action to take if file exists or doesn't exist

open_3:
         cmp   fWin95LFN, 0       ; Win95 LFNs enabled?
	  je   old_style		;if not go do old call

	 mov   si, filename	  ;   ..

	 mov   ax, 716Ch	  ; fcn 716C: extended open (long filename)
	 stc			  ; set carry in case not supported
	 int   21h		  ; issue long filename open request
	 jnc   success		  ; skip ahead if it succeeds
	 cmp   ax, 7100h	  ; did it fail because fcn not supported?
	 jne   failure

;    Long filename request isn't supported, so reissue the request using the
;    DOS 2+ Create or Open functions

old_style:
	 test  word ptr oflag, _O_EXCL	  ; no overwrite existing file?
         jz    test_truncate

         mov   ax, 04300h	; Check if file already exists
         mov   dx, filename
         int   21h
         jc    old_create	; No - create it!
         mov   ax, 80		; Error - File Exists
         jmp   short failure

test_truncate:
	 test  word ptr oflag, _O_TRUNC	  ; might we create a missing file?
         jnz   old_create

         mov   ah, 3Dh            ; open file
         mov   al, bl
         mov   cx, pmode
         mov   dx, filename
	 int   21h
         jnc   success

	 test  word ptr oflag, _O_CREAT	  ; might we create a missing file?
         jz    failure

old_create:
         mov   ah, 3Ch            ; create file
         mov   cx, pmode
         mov   dx, filename
	 int   21h
	 jnc   success

failure:
	 stc			  ; force carry flag on
         mov   _doserrno, ax
         mov   ax, -1
         ret

;    Here when open succeeds. AX is currently the file handle. Use 21/4400
;    to see if we're dealing with a device or a disk file.

success:
	 mov   bx, ax		  ; save file handle in BX
	 mov   ax, 4400h	  ; fcn 4400: get device information
	 int   21h
	 and   dl, 80h		  ; keep "device" flag only
	 shr   dl, 1		  ; shift to position
	 or    fileflag, dl	  ; merge with text-access flag

	 test  dl, dl		  ; is it a device?
	 jnz   setflag		  ; if yes, don't look for ^Z

;    If we're opening a disk file for text access, see if file ends
;    in a ^Z EOF marker. If so, truncate the file just before it.
;    We only do this for RDWR access, though I'm not sure why.

	 test  fileflag, 80h	  ; text access?
	 jz    setflag		  ; if not, no problem here
	 test  oflag, _O_RDWR	  ; read-write access?
	 jz    setflag		  ; if not, there won't be any confusion

	 mov   cx, -1		  ; CX:DX = file position (-1)
	 mov   dx, cx		  ;   ..
	 mov   ax, 4202h	  ; fcn 4202: seek relative to EOF
	 int   21h		  ; position file to end

	 neg   cx		  ; convert -1 to +1

	 lea   dx, junk 	  ; DS:DX -> 1-byte data buffer
	 mov   ah, 3Fh		  ; fcn 3F: read file
	 int   21h		  ; read last byte of file

	 test  ax, ax		  ; test return code
	 jz    @F		  ; skip ahead if nothing read
	 cmp   junk, 1Ah	  ; or if byte isn't a ^Z
	 jne   @F		  ;   ..

	 neg   cx		  ; seek back to EOF-1
	 mov   dx, cx		  ;   ..
	 mov   ax, 4202h	  ;   ..
	 int   21h		  ;   ..

	 xor   cx, cx		  ; set data count = 0 to truncate file
	 mov   ah, 40h		  ; fcn 40: write file
	 int   21h		  ; issue request to truncate before ^Z
@@:
	 xor   cx, cx		  ; seek back to start of file
	 xor   dx, dx		  ;   ..
	 mov   ax, 4200h	  ;   ..
	 int   21h		  ;   ..

;    If we've opened a disk file, see if it's read-only

setflag:
	 test  fileflag, 40h	  ; is this a device?
	 jnz   setflag_1	  ; if yes, that's a good assumption

	 mov   dx, filename
	 mov   ax, 7143h	  ; fcn 7143: get/set attribute
	 push  bx		  ; save file handle
	 xor   bl, bl		  ; BL = 0 to indicate "get"
	 xor   si, si             ; SI = 0 (no attribute match)
	 stc			  ; set carry in case not supported
	 int   21h		  ; go get file attribute
	 pop   bx		  ; restore file handle in any case
	 jnc   @F		  ; skip ahead if no error

	 mov   ax, 4300h	  ; reissue 8.3 form of request
	 int   21h		  ;   ..
@@:
	 test  cl, 1		  ; is file read-only?
	 jz    @F		  ; if yes, okay
	 or    fileflag, 10h	  ; yes. remember that
@@:
	 test  oflag, _O_APPEND   ; opening for append?
	 jz    setflag_1	  ; if not, okay
	 or    fileflag, 20h	  ; yes. remember that too

;    Save the appropriate internal access flag for this file:
;	 80    text-mode access
;	 40    file is really a device
;	 20    append to disk file
;	 10    read-only disk file
;	 01    file is open

setflag_1:
	 cmp   bx, _nfile	  ; is file handle within range of table?
	 jae   toomany		  ; if not, error
	 mov   cl, fileflag	  ; compose flag byte
	 or    cl, 1		  ; indicate file is open
	 mov   _osfile[bx], cl	  ; store flags

	 mov   ax, bx		  ; set file handle for normal return
	 ret			  ; return to external caller

toomany:
	 mov   ah, 3Eh		  ; fcn 3E: close file
	 int   21h		  ; issue close request
	 mov   ax, 1800h	  ; set internal error code
	 jmp   failure		  ; exit with error

_open	 endp


_cXENIXtoDOSmode proc
	 mov   ax, _umaskval	  ; get mask from _umask
	 not   ax		  ; invert because of negative sense of mask
	 and   ax, cx		  ; AX = bits we won't keep
	 xor   cx, cx		  ; clear those bits in CX
	 test  ax, _S_IWRITE	  ; is write permission allowed?
	 jnz   @F		  ; if yes, okay
	 or    cl, 1		  ; no. set read-only attribute
@@:
	 ret			  ; return to caller

_cXENIXtoDOSmode endp


;------------------------------------------------------------------------------
;
;    HANDLE WINAPI FindFirstFile(LPTSTR lpszPath, WORD uMode, LPWIN32_FIND_DATA lpFindData, unsigned *fUnicode )
;
;------------------------------------------------------------------------------

FindFirstFile proc far pascal uses bx si di, lpszPath:dword, uMode:word, lpFindData:dword, fUnicode:ptr byte

	 mov   lasterr, 0	  ; assume call will succeed

	 push  ds		  ; save DS across call (1)
	 mov   ax, 714Eh	  ; fcn 714E: find first file (long name version)
	 lds   dx, lpszPath	  ; DS:DX -> search pattern
	 les   di, lpFindData	  ; ES:DI -> results buffer
	 mov   cx, uMode	  ; CX = attribute mask: files or directories
	 xor   si, si             ; SI = 0 (single attribute match is enough)

	 stc			  ; set carry in case fcn not supported
	 int   21h		  ; issue DOS request
	 jnc   goback
	 mov   lasterr, ax	  ; save error code
	 xor   ax, ax		  ; indicate failure by returning null handle
goback:
	 mov   bx, fUnicode
	 mov   [bx], cx		  ; store Unicode conversion flag
	 pop   ds		  ; restore DS (1)
	 ret			  ; return with find-file handle in AX

FindFirstFile endp


;------------------------------------------------------------------------------
;
;    BOOL WINAPI FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindData, unsigned *fUnicode )
;
;------------------------------------------------------------------------------

FindNextFile proc far pascal uses bx si di, hFindFile:word, lpFindData:dword, fUnicode:ptr byte
	 mov   lasterr, 0	  ; assume call will succeed

	 mov   ax, 714Fh	  ; fcn 714F: find next file (long name version)
	 mov   bx, hFindFile	  ; BX = find-file handle
	 les   di, lpFindData	  ; ES:DI -> results buffer
	 xor   si, si             ; SI = 0 (single attribute match is enough)

	 stc			  ; set carry in case fcn not supported
	 int   21h		  ; issue DOS request
	 jnc   success		  ; skip ahead if success

	 mov   lasterr, ax	  ; save last error
	 xor   ax, ax		  ; return FALSE to indicate no more matches
	 jmp   goback		  ; done

success:
	 mov   ax, 1		  ; return TRUE to indicate one more match
	 mov   bx, fUnicode
	 mov   [bx], cx		  ; store Unicode conversion flag
goback:
	 ret			  ; return to caller

FindNextFile endp


;------------------------------------------------------------------------------
;
;    BOOL WINAPI FindClose(HANDLE hFindFile)
;
;------------------------------------------------------------------------------

FindClose proc far pascal uses ds bx si di, hFindFile:word

	 mov   lasterr, 0	  ; assume call will succeed

	 mov   ax, 71A1h	  ; fcn 71A1: find close (long name version)
	 mov   bx, hFindFile	  ; BX = find-file handle
	 xor   si, si             ; SI = 0 (no attribute match)

	 stc			  ; set carry in case fcn not supported
	 int   21h		  ; issue DOS request
	 jnc   success		  ; skip ahead if success

	 mov   lasterr, ax	  ; save last error
	 xor   ax, ax		  ; return FALSE to indicate failure
	 jmp   goback		  ; done
success:
	 mov   ax, 1		  ; return TRUE to indicate success
goback:
	 ret			  ; return to caller

FindClose endp


;------------------------------------------------------------------------------
;
;    BOOL WINAPI Win95GetTitle(char * pszTitle)
;
;------------------------------------------------------------------------------

Win95GetTitle proc far pascal uses ds si di, pszTitle:word

         mov	ax, 168Eh
         mov	di, pszTitle
         mov	cx, 255
         mov	dx, 2
         int	2Fh
	 ret			  ; return to caller

Win95GetTitle endp


;------------------------------------------------------------------------------
;
;    BOOL WINAPI Win95SetTitle(char * pszTitle)
;
;------------------------------------------------------------------------------

Win95SetTitle proc far pascal uses ds di, pszTitle:word

         mov	ax, 168Eh
         mov	di, pszTitle
         mov	dx, 0
         int	2Fh
	 ret			  ; return to caller

Win95SetTitle endp


;------------------------------------------------------------------------------
;
;    INT WINAPI Win95GetFAT32Info(char *pszDrive, FAT32 *pFAT32, INT nFAT32Size)
;
;------------------------------------------------------------------------------

Win95GetFAT32Info proc far pascal uses bx si di es, pszDrive:word, pFAT32:word, nFAT32Size:word

	push	ds		;set buffer segment
	pop	es
	mov	di, pFat32	;get buffer address
	mov	cx, nFAT32Size	;and size
	push	cx		;save them
	push	di
	xor	al, al
	rep	stosb		;clear the buffer
	pop	di		;restore size and address
	pop	cx
	mov	dx, pszDrive	;point to drive name
	mov	ax, 7303h		;extended get free space function
	int	21h
	mov	ax, 0		;assume OK
	 jnc	Fat32Done		;continue if OK
	inc	ax		;return non-zero (error)
Fat32Done:
	ret
Win95GetFAT32Info endp


;------------------------------------------------------------------------------
;
;    WORD GetLastError()
;
;------------------------------------------------------------------------------

GetLastError proc far pascal
	 mov   ax, lasterr	  ; get last error code
	 ret			  ; return to caller
GetLastError endp

	 end

