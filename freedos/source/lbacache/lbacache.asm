 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2008.

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
 ; Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 ; (or try http://www.gnu.org/licenses/licenses.html at www.gnu.org).

; LBAcache - a hard disk cache based on XMS, 386 only, 
; and aware of the 64bit LBA BIOS Int 13 Extensions.
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2006

; CHS-only old version is no longer supported. This can do CHS, too.


%define VERSION '07apr2008'	; *** introduced 7/2004 ***


; %define PRETENDER 1
;	^- do NOT modify int 0x13 because we have a bug :-(
;	(flag introduced 24.11.2001)
; %define DBGsp 1	; print SP at some occasions

%ifdef DBGsp
%imacro STACKDEBUG 0	; 0 parameters (1 would be referred %1...)
	push ax
	mov ax,sp
	inc ax
	inc ax
		push word colonmsg
		call meep	; warn
	pop ax
%endmacro
%else
%define STACKDEBUG nop
%endif

; WARNING: In nasm mov ax,label and mov ax,[label] are same sometimes
; (???) use mov ax,[seg:label] to access memory AT label !?
; (only a problem for sources, not for destinations)

	org 0x100	; COM/XMS version of an hard disk read cache

		; inspired by PC Magazine dcache.com (which was
		; COM/EMS version and only 1 drive, max 256 MBy drive)
	; LBACACHE written by Eric Auer 10/2001 <eric@coli.uni-sb.de>
        ; (based on 5/96 sys with 386 driver for 850 MB HDD also by EA)
	; (old versions of this cache in 10/2001 were called HDDCACHE)

; noW implemented: floppy cache
;                  int 0x13 LBA extensions support
;                  command line arguments
;                  all in one binary (TSR, unTSR, info, flush)
;
; not planned:     useability with EMS and 8086 (use DCACHE there),
;                  DOS rather than int 0x13 cache,
;                  write cache (may be offered as a separate tool).   
;
; further notes:   CD-ROM cache is now available as separate CDRcache!
;
; See history.doc to learn about the version history of LBAcache!

; include the initial jump and quite a bit of global data...
; this must be FIRST as it contains stuff that will stay in RAM
; if the driver is disabled but complete unloading does not work

%include "datahead.asm"

	; .386 (nasm only has BITS 16 and BITS 32 directives for)
	; (the CS type,  but cannot limit itself to x86 code...!)

; -------------------------------------------------------------------

; include the main NEWI13 dispatcher which calls
; hdread and hdwrite which now come in two versions,
; one for LBA and one for CHS - {hd,lba}{read,write}
; we can return from those to either enderr, endok, or oldint
; for now, several functions cause the cache to be flushed or shut
; down completely - no support for flushing only one drive yet!
; also contains callold, which calls the original int 0x13
; FLOPPY: disk change detection and similar things are done here

%include "dispatch.asm"

; -------------------------------------------------------------------

	; main R/W handling functions
	; for CHS: hdwrite and hdread
	; es:bx is buffer, cx/dh location, dl drive, al size
	; for LBA: lbawrite and lbawrite
	; dl is drive, ds:si points to a structure of:
	; B 0x10 (0x18 to allow a 64bit flat pointer)
	; B 0
	; W number of sectors (also used for a return value:
	;   number of sucessfully read/written sectors)
	; D DOS pointer to buffer (or -1 to use flat pointer)
	; Q sector number
	; Q optional flat pointer
	; (we do NOT handle the flat pointer or sector numbers
	;  longer than 32bit, the dispatcher checks this!)

	; FLOPPY: relies on flushing on disk change already
	; handled in dispatch.asm above. Only change: floppy forces CHS

%include "rwfunc.asm"

; -------------------------------------------------------------------

	; XMS helper functions
	; copytoxms copies one sector from es:bx to XMS slot AX
	; copytodos copies one sector from XMS slot AX to es:bx

%include "xmscopy.asm"

; -------------------------------------------------------------------

	; more helper functions (using int 0x10):
	; meep: makes a beep or shows the message pointed to by
	;       a word on stack (if the word is not 0...),
	;       then shows AX and returns, taking the word from
	;       the stack again. Saves all regs including flags.
	; showal: shows AL as hex. Trashes lots of registers.

%include "meepdisp.asm"

; -------------------------------------------------------------------

	; status table handling functions

	; old format was - per entry - B LLLLLLDD (lru and drive),
	; B DH W CX, location and drive passed in CX/DH and DL.
	; new format is - per entry - D sector low, B drive, B LRU,
	; W reserved (could be used to support more than 4 G of
	; sectors, which will be drives of more than 4 TB, or to
	; support any other fancy feature, for example a linked bin
	; list for chains of adjacent sectors).
	; location and drive are passed in EAX and DL.

	; Input is sector number EAX, output (xms) bin AX
	; findbin:  finds a bin for a location (stc if not found),
        ; newbin:   allocates a new bin for a given location,    
        ;           flushing old bins if needed.
        ;           (main bin selection "intelligence" !)  
	; flush:    marks all cache buffers as empty, resets table
	;           ... 10/2003: spares frozen bins (warns / sets CY)
        ; flushone: empties all slots for drive DL only 
	;           ... 10/2003: spares frozen bins (warns / sets CY)
        ; telltabsize: (returns carry on error)
        ;          tells in AX how big a table for AX sectors will be
	; New in 10/2003:
	; freezebin: marks bin AX as "dirty buffer" (unflushable)
	; meltbin: marks bin AX as normal data buffer again

%include "binsel.asm"

; -------------------------------------------------------------------

	; end of install code, needs to be resident (because of flush)

resinst:
inittable:
	mov di,table 		; *offset*
	mov al,0x20
inittab:
	mov [cs:di],al		; fill table (and possibly stack) with " "
	inc di
	cmp di,[cs:tsrsize]	; changed from "localsp" 7/2004
	jb inittab

		call flush	; mark cache as empty, initialize table
				; -> cache size must be known!

quitpop:		; this is where a failure ends...
	pop ds		; restore all regs and return
	popa

	; .8086 (nasm cannot limit itself to a certain CPU, so)
	; (we must simply take care of only using 8086 code in)
	; (this section ourselves... nasm DOES have BITS 16 / )
	; (and BITS 32 directives to set the current CS type. )

quitinst:
	mov ax,[ds:tsrsize]	; now the value tells how big the TSR
	cmp ax,eoftsr		; will be in first place.
	jae reallytsr
	mov ax,0x4c01		; no need to stay in RAM, just exit.
	int 0x21		; exit with errorlevel 1.

reallytsr:
	mov dword [ds:0x100],MAGIC-1	; write magic value (in 2 steps)
	inc dword [ds:0x100]		; (to make instance scannable)
	mov word [ds:tsrsize],eoftsr	; from now on, this value tells
		; the uncache module how much it might shrink the MCB
		; when complete unload / stop fails.
	mov word [running],1	; mark init as done
	add ax,byte 15		; round up
	mov cl,4
	shr ax,cl
	mov dx,ax
	mov ax,0x3100		; stay in RAM as a TSR
	int 0x21		; exit with errorlevel 0.

; -------------------------------------------------------------------

fish2err:
	db 'LBACACHE shutdown: INT 13.',7,0
fisherr	db 'LBACACHE flush: INT 13.',7,0
wrerr	db 'LBACACHE flush: write error.',7,0
; rderr	db 'LBACACHE: read error.',7,0
xmserr	db 'LBACACHE: XMS error.',7,0
; fullerr	db 'LBACACHE: scaling ',7,0


hello	db 'LBAcache disk read cache for XMS + 386, '
	db   'E. Auer <eric@coli.uni-sb.de> 2001-2006',13,10
	db 'License: GPL 2. '	; *** remember to keep year range updated

align 4,db ' '	; align with spaces :-) [changed 5/2004 from 8 to 4]

	; ********************************************

table:		; THIS is where our table -starts-, or in other words,
		; THIS is where the resident code -ends- !!!
		; this + table size -> first free byte position.

	; ********************************************

	db      'Up to 8 harddisks, 2 floppy, LBA / CHS. Version: '
	db	VERSION
	db '.',13,10,0


	; the rest after <table> will be overwritten by our (aligned)
	; status table...

	; ( old format: first byte is (DL && 3) || (importance << 2) )
	; ( next byte is DH, then we have the CX word                )
	; ( newer format: dword sector, byte drive, byte lru, word   )

	; 1/2002 format:  dword sector0, byte drive, byte lru, word
	; bitmask (bitmask: allows ..16 sectors per slot, default _4_)
	; Using default of _8_ sectors per slot since 11/2002.
	; Possible write cache format: 2 byte-sized bitmasks, one for
	; "used" and one for "dirty". Max 8 sectors per slot then.

; -------------------------------------------------------------------

	; main install and setup routine follows (starts with
	; 8086 compatible code to detect 386, then does a check
	; for the existence of XMS, allocates the cache there,
	; and stores the drive geometry information for drives
	; 0x80 .. 0x83 for CHS <-> LBA conversion
	; install: is the entry point.
	; jumps to resinst: at the end.
	; FLOPPY: added detection of change lines to know which
	; drives we should dare to cache.

%include "setup.asm"

; -------------------------------------------------------------------

	; parse command line, display it...
	; input: pointer esbx to "that device structure"

        ; recognizes words:
        ; digit -> set size in units of ksectors*
        ; BUF digit -> as digit
        ; DRV letters -> enable caching only for letters    
        ;     (0..7, deprecated variant C..J, unknown are ignored) 
        ; ?:\anything -> ignored (e.g. the first argument in
        ;     .sys mode will be c:\path\to\lbacache.sys)
        ; HELP or HLP or ? -> show help message
        ; anything else -> error message

        ; defaults are using the size from datahead.asm sectors
        ; and to cache all drives 0x80..0x87
	; FLOPPY: added FLOP keyword to enable floppy read cache
	; for drives enumerated by setup.asm, See also dispatch.asm .

%include "cmdline.asm"

; -------------------------------------------------------------------

	; the whole uncache binary is now a part of the lbacache
	; binary. see uncache.asm for what it does (it handles the
	; INFO, SYNC and STOP command line arguments)...
	; Accessed by a near call to "uncache", no regs changed.
	; The call reads out [cs:args]. No XMS handle may be present
	; when calling uncache!

%include "uncache.asm"

; -------------------------------------------------------------------

	; .8086 (as said above, nasm does not have this)
	; (directive, so make sure you do not use >8086)
	; ( code  in  the  lines  below  ->  by  hand! )

			; obvious: do NOT use this after it is
			; overwritten by the table - use meep instead

			; *** changed to use int 0x21, because strtty
			; *** is only used during setup and should be
			; *** redirectable (11/2002)
strtty: push ax		; *** Print out a string during init, via DOS
	push dx		; *** was bx
strt2:	; *** mov bx,7	; print out some text string using bios
        mov ah,2	; *** was 0x0e
			; *** int 21.2 is not reliably redirected,
			; *** using int 21.6 (-> dl must not be 0xff)
        mov al,[cs:si]
	inc si		; so we need not care for std/cld as w/ lods!
        or al,al
        jz short sttend
	mov dl,al	; *** for DOS the char must be in DL
%ifdef REDIRBUG
	call BUGTTY
%else
	int 0x21	; *** was 0x10
%endif
        jmp short strt2	; YUCK, was to strtty - stack bomb!
sttend: pop dx		; *** was bx
	pop ax
	ret

%ifdef REDIRBUG
BUGTTY:	push ds		; Workaround a FreeDOS kernel bug 11/2002:
	push dx		; only strings, not chars are ">" and "|"
	mov ax,cs	; redirectable (FreeDOS kernel 2027)
	mov ds,ax
	mov [ds:bugttybuf],dl
	mov dx,bugttybuf
	mov ah,9
	int 0x21	; print (single char) string...
	pop dx
	pop ds
	ret
bugttybuf	db "*$"	; sigh...
%endif

