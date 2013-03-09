 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2004.

 ; LBAcache is free software; you can redistribute it and/or modify
 ; it under the terms of the GNU General Public License as published
 ; by the Free Software Foundation; either version 2 of the License,
 ; or (at your option) any later version.

 ; LBAcache is distributed in the hope that it will be useful,
 ; but WITHOUT ANY WARRANTY; without even the implied warranty of
 ; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ; GNU General Public License for more details.

 ; You should have received a copy of the GNU General Public License
 ; along with LBAcache; if not, write to the Free Software Foundation,
 ; Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 ; (or try http://www.gnu.org/licenses/licenses.html at www.gnu.org).

; LBAcache - a hard disk cache based on XMS, 386 only, 
; and aware of the 64bit LBA BIOS Int 13 Extensions.
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2004



	; Some of the status messages use meep: This uses
	; int 21 while running < 1 and int 10 tty otherwise.

	; parse the command line arguments

	; recognizes words:
	; / -> skipped
	; digits -> set size in units of half ksectors
	;     Or if value has > 2 digits, use unit 2 sectors
	;     which is 1 kbyte (new 5/2004)
	; BUF digits -> as digits
	; DRV digits -> enable caching only for some harddisks
	;     Here 0..7 means BIOS disk number 0x80..0x87.
	;     DRV NULL means disable caching for harddisks.
	; TUNA -> enable (classic, slow) fully associative cache
	;     (better allocation / more hits, but slow for big caches)
	;     true if dispatch.asm tuneflags & 1 (7/2004)
	; TUNW -> enable (classic) "alloc on write" - useful if
	;     freshly written data will be read back later, but
	;     actual disk writes are always done at once. Can mean
	;     more cache trashing if you write a lot without reading
	;     it back (shortly) after writing.
	;     true if dispatch.asm tuneflags & 2 (7/2004)
	; TUNS -> force local stack to be in low RAM (< 0xa000:0),
	;     needed for int 13.8 to work with SCSI + EMM386 (!).
	;     true if dispatch.asm tuneflags & 4 (7/2004)
	; COOL -> all sectors which are accessed after this command are
	;     frozen / locked into the cache (given high stickiness,
	;     which has only limited effect without TUNA) (8/2004)
	; HEAT -> "frozen" property is removed from all accessed
	;     sectors after this command (8/2004). Alias: MELT.
	; TEMP -> return to normal operation, stop modifying stickiness
	;     (and show a count of sticky entries at the moment) (8/2004)
	;
%define HELPflag 64
	; HELP or HLP or ? or syntax error -> show help message
	; FLOP -> enable floppy caching for those of (A: B:)
	;     that have a change line (change detection)

	; new arguments to trigger former uncache functionality:
%define SYNCflag 4
	; SYNC -> flush (empty) the stack for 0,1,0x80..0x87
%define INFOflag 2
	; INFO -> show some informations and statistics
%define STATflag 8
	; STAT -> just show statistics (easier to understand)
%define STOPflag 1
	; STOP -> remove all instances as completely as possible
	; if one of those is given, the cache will not stay TSR,
	; but only do the requested task.
%define ZEROflag 16
	; ZERO -> reset statistics counters to zero [new 5/2004].

	; defaults are using the size from datahead.asm sectors
	; and to cache all drives 0x80..0x87

args	dw 0	; Bits: see ...flag defines above.
		; Any non-zero value also means "do not go TSR".

parsecommandline:
	push eax
	push si
	cld

; -------------

	push di
	mov di,0x81-2	; command line starts at 0x81
	mov ax,'X '	; nonspace + space, for the space compressor.
	stosw		; prepend this (overwrites [0x7f] and [0x80])
	mov si,di	; store transformed text at same place
	mov byte [0xff],0	; force string termination
clnextchar:
	lodsb		; read a char
	cmp al,'a'	; upcase while copying
	jb clnoupcase
	cmp al,'z'
	ja clnoupcase
clupcase:
	sub al,32	; 'a'-'A'
clnoupcase:
	cmp al,10	; DOSsy EOF (sys - obsolete) ?
	jz endcmdline
	cmp al,13	; DOSsy EOF (com/exe) ?
	jz endcmdline
	cmp al,0	; other EOF ?
	jz endcmdline

	cmp al,9
	jnz clnomod	; <- ugh.. jNz of course!
	mov al,' '	; convert tab to space
clnomod:
	cmp al,' '	; compress N spaces to one
	jnz clnospc
	cmp [di-1],al	; previous also a space?
	jz clsqueezespc	; if yes, do not store this space
clnospc:
	stosb		; store possibly modified char
clsqueezespc:
	jmp short clnextchar

endcmdline:
	cmp byte [di-1],' '	; remove the max 1
	jnz clnotrail	; (compressed) trailing space
	mov byte [di-1],0
clnotrail:
	mov al,0
	stosb		; add that EOF mark!
	pop di

; --------------

	mov si,0x81	; really parse the pre-parsed string now

clnextword:
	mov eax,[cs:si]	; think big!
	inc si			; parse on... (skip " " etc.)
	cmp al,0	; eof?
	jz near clparsedone

	cmp al,'/'	; "/option" is the same as "option"
	jz clnextword

	cmp al,' '	; space?
	jz clnextword	; skip over space
	cmp al,'?'	; "?" help request?
	jz near clshowhelp
	cmp eax,'HELP'	; HELP request?
	jz near clshowhelp
			; *** new 7/2004
	cmp eax,'TUNA'
	jnz clntuna		; full associativity enable?
	or byte [cs:tuneflags],1
	jmp near clskip4	; consume keyword
clntuna:
	cmp eax,'TUNW'
	jnz clntunw		; alloc on write enable?
	or byte [cs:tuneflags],2
	jmp near clskip4	; consume keyword
clntunw:
	cmp eax,'TUNS'
	jnz clntuns		; low stack enable?
	or byte [cs:tuneflags],4
	jmp near clskip4	; consume keyword
clntuns:
			; *** end new 7/2004
	call cltemperature	; new 8/2004
	cmp eax,'FLOP'
	jz clflop	; FLOP floppy cache enable?
	cmp eax,'INFO'
	jz clinfo	; INFO (uncache) show info and statistics?
	cmp eax,'STAT'
	jz clstat	; STAT (uncache) show statistics?
	cmp eax,'SYNC'
	jz clsync	; SYNC (uncache) flush and sync caches?
	cmp eax,'STOP'
	jz clstop	; STOP (uncache) remove all caches?
	cmp eax,'ZERO'
	jz clzero	; ZERO (uncache) zero statistics counts?

; ---	cmp byte [cs:si+4-1],0	; end right after our 4 chars?
; ---	jz cltowrongarg	; at eof while only arg expecting arg possible
clnonfin:
	cmp eax,'BUF '	; "BUF digits" behaves just as "digits"
	jz clbuf0
	cmp eax,'DRV '
	jz near cldrvsel	; DRV definition list
				; which drives to cache?
	cmp al,'0'	; numeric argument?
	jb cltowrongarg	; else syntax error - none of the valid args
	cmp al,'9'	; numeric argument?
	ja cltowrongarg	; else syntax error - none of the valid args
	jmp clbufsize	; numeric arguments determine cache size.

cltowrongarg:
	dec si		; show full arg
	jmp clwrongarg	; reject any other arguments

; --------------

clbuf0:	mov ax,0	; BUF is just a dummy keyword now
	dec si		; BUF has only 3 letters, not 4 as the others.
	jmp short clcommon4
clsync: mov ax,SYNCflag
	jmp short clcommon4
clstop: mov ax,STOPflag
	jmp short clcommon4
clinfo: mov ax,INFOflag + STATflag	; stats AND technical info
	jmp short clcommon4
clstat: mov ax,STATflag
	jmp short clcommon4
clzero:	mov ax,ZEROflag
clcommon4:
	or [cs:args],ax	; store the flag change
clskip4:
	add si,4-1	; skip the keyword
clhx:	jmp clnextword

; --------------

clflop:
	add si,4-1			; skip the "FLOP" keyword
	or word [cs:fddstat],3		; wish: cache A: -and- B:
					; findgeom may disable this later

; - Setup will show a message anyway!	; in setup if no good drives found!
; -	push si
; -	mov si,clfddmsg			; tell that floppies will be cached
; -		call strtty		; show string
; -	pop si
	jmp clnextword

; --------------

clbufsize:			; numeric argument:
	dec si			; rewind SI to point to 1st digit.
	push bx			; will hold the converted number
	xor bx,bx

clbufszloop:
	lodsb
	cmp al,' '		; end of argument or buffer?
	jbe clbufszdone
	sub al,'0'		; convert to binary
	cmp al,9		; a valid digit?
	jbe clbufdigok
	dec si			; adjust error message
clbufszbug:
	pop bx			; restore BX!
	jmp clwrongarg		; report syntax error

clbufszbugval:
	pop bx
	push si
	mov si,sizerangemsg
		call strtty
	pop si
	jmp clwrongarg

clbufdigok:
	mov ah,0		; (!)
	cmp bx,(32768+9)/10	; will become > 32767 now
	jae clbufszbugval
	push ax
	push dx			; protect DX from MUL...!
	mov ax,10		; multiply old value by 10
	xor dx,dx
	mul bx
	mov bx,ax		; update intermediate result
	pop dx
	pop ax
	add bx,ax		; add the new digit as lowest one
	jmp short clbufszloop	; scan for more digits

clbufszdone:
	dec si			; rewind to space / eof char again!
	or bx,bx	; *** easter egg value 0 temporarily disabled ***
	jz clbufszbugval	; syntax error!
	cmp bx,99		; less than 3 digits?
	ja clbufink
clbufin256kunits:		; handle old style argument
	xchg bl,bh		; multiply by 256
clbufink:			; convert to kilobytes (* 2 sectors)
	or bx,bx		; overflow if > 0x8000 kbytes
	js clbufszbugval	; syntax error!
	add bx,bx		; 2 sectors are 1 kbyte
	mov [cs:sectors],bx	; store new cache size
	pop bx
	jmp clnextword

; --------------

cldrvsel:			; cache only drives in (list)
	add si,4-1		; list starts here, skip "DRV "
	xor ax,ax
	mov [cs:drvselmask],ax	; assume NO drives cached,
	mov eax,[cs:si]
	cmp eax,'NULL'		; special keyword: cache no drives!
	jnz cldslp
	add si,4+1		; skip over "NULL" plus one more
				; (because we have a dec si below)
	jmp short cldsdone

cldslp:				; add drives from user list
	mov al,[cs:si]
	inc si			; parse on
	cmp al,' '
	jbe cldsdone		; list ends here
	cmp al,'0'
	jb cldswrong		; complain if not a digit in 0..7 range
	cmp al,'7'
	jbe cldsok		; 0..7 digit
cldswrong:
	dec si			; adjust error pointer
	jmp clwrongarg		; syntax error!

cldsok:	sub al,'0'		; digit to byte
	push cx
	mov cl,al
	mov ax,1
	shl ax,cl		; *** LSB=0x80 .. MSB=0x8F
	pop cx
	or [cs:drvselmask],ax	; activate caching for this drive
				; *** ... if cacheable!
	jmp short cldslp	; go on with list

cldsdone:			; done. Now show a drive list message.
	mov ax,[cs:drvselmask]
	push si			; save cmd line pointer
	push bx

	test ax,0x00ff		; ANY drive?
	jnz clsomedrives
clnodriveatall:
	mov si,clnodrvmsg
		call strtty	; DRV NULL selected...
	jmp short cllisteddrives
clsomedrives:
	mov si,cldrvmsg
		call strtty
	mov si,ax		; bitmask
	mov al,'0'		; name of lowest bit is "0" or "80"
cldrivedisploop:
	test si,1		; this bit set???
	jz clnotthisdrive
	push si
	mov [cs:cldrvnummsg+1],al	; patch into message
	mov si,cldrvnummsg
		call strtty	; show a string like "80 "
	pop si
clnotthisdrive:
	shr si,1		; next bit
	inc al			; next name
	cmp al,'7'		; all drives done?
	jbe cldrivedisploop	; loop

cllisteddrives:
	mov si,crlfmsg
		call strtty	; CRLF
	pop bx
	pop si
	dec si			; take back the last inc si, make SI
				; point to the space after the arg.
	jmp clnextword		; parse on

; --------------

cltemperature:			; added 8/2004: COOL HEAT TEMP
	push cx
	xor cx,cx		; temperature change
	cmp eax,'TEMP'		; no change?
	jz cltempcall
	inc cx			; is it heating?
	cmp eax,'HEAT'
	jz cltempcall
	cmp eax,'MELT'		; just an alias, same function
	jz cltempcall
	dec cx
	dec cx			; or is it cooling?
	cmp eax,'COOL'
	jz cltempcall
clnotemp:
	pop cx
	ret

cltempcall:
	mov [cs:cltempmsg+2],eax	; put command name into message
	mov eax,'tick'
	push ebx
	mov ebx,'temp'
	push dx
	mov dx,4		; affect no real drive if no cache loaded
	int 0x13		; send to all caches: temp change CX
		mov ax,dx	; result DX
		sub ax,4	; remove bias DL value again
	pop dx
	pop ebx
		push word cltempmsg
		call meep
	pop cx
	push si
		mov si,crlfmsg
		call strtty
	pop si
	pop ax			; we do not RET but JMP: clean up stack.
	jmp clstat		; skip over 4 chars and activate the
				; statistics as if "STAT" was given.

cltempmsg	db 13,10,"???? function sent. Result: ",0

; --------------

clwrongarg:			; we did not understand this one!
	push si
	mov si,clrejmsg
		call strtty	; complain about syntax
	pop si
	call strtty		; show unparsed rest of command line
clwadone:
	mov si,crlfmsg
		call strtty	; CRLF
				; wrong argument -> show help and exit!

clshowhelp:			; show help - on user request or after error
	mov si,clhelpmsg
		call strtty	; show help message
	or word [cs:args],HELPflag	; remember not to go TSR!

clparsedone:			; we are done with parsing the command line
	pop si
	pop eax
	ret

; --------------

cldrvmsg	db "Caching drives: ",0
	; LSB stands for 0x80, next bit for 0x81, and so on.
cldrvnummsg	db "80 ",0	; new 4/2004
clnodrvmsg	db "No harddisks cached.",0

; - clfddmsg	db "Caching floppies.",13,10,0
	; see above - fddstat word - and dispatch.asm and setup.asm

clrejmsg	db "Syntax error: ",0
sizerangemsg	db "Size must be 1..99 (1/4 MB units) or 128..32767 (kbytes)."
		db 13,10,0

clhelpmsg:	; changed 7/2003 based on a suggestion by Aitor S. Merino
		; simplified again 5/2004.
	db 13,10,"To load:     LBACACHE  [size] [DRV list] [FLOP] [TUNA] [TUNW]",13,10
; ***	db "To get help: LBACACHE  [HELP] [/?]",13,10
; ---	db 13,10
; ---	db "Load options:",13,10
	db "  size      Specifies the cache size in kbytes of XMS. Default: 2048.",13,10
	db "            Maximum cache size is 32767, less if not enough DOS RAM free.",13,10
	db "  DRV list  Use to override harddisk autodetection. See lbacache.txt!",13,10
	db "  DRV NULL  Use this to suppress harddisk caching completely.",13,10
	db "  FLOP      Enable the floppy cache (A: and B:, autodetected). To speed up",13,10
	db "            floppy use, load TICKLE, too! Please report if FLOP has bugs.",13,10
	; *** new 7/2004
	db "  TUNA      Tune: fully Associative mode (slow but maybe more cache hits).",13,10
	db "  TUNW      Tune: alloc on Write mode (sometimes better, sometimes worse).",13,10
	db "  TUNS      Tune: move local stack to LOW RAM area (needed for SCSI).",13,10
	; *** end new 7/2004
	db 13,10
	db "When loaded: LBACACHE  [INFO] [SYNC] [STOP] [STAT] [ZERO]",13,10
; ---	db "Options which send commands to all resident LBAcache instances:",13,10
	db "  INFO      Show cache statistics and tech details.",13,10
	db "  STAT      Show cache statistics (hit/miss count unit: sectors).",13,10
	db "  ZERO      Reset statistics counters.",13,10
	db "  SYNC      Empty the cache contents.",13,10
	db "  STOP      Shutdown and unload (XMS and DOS RAM are freed again).",13,10
	db 0

