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
 ; Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 ; (or try http://www.gnu.org/licenses/licenses.html at www.gnu.org).

; LBAcache - a hard disk cache based on XMS, 386 only,
; and aware of the 64bit LBA BIOS Int 13 Extensions.
; GPL 2 software by Eric Auer <eric@coli.uni-sb.de> 2001-2004


next:	jmp install	; 3 bytes
	nop		; 1 byte
	; (above first 4 bytes will be overwritten by magic value!)
%define MAGIC 0x1bacac43	; "LBACACHE" in leet spelling

; ---------------------------------------------------------------

; All the stuff up to and including TSRSIZE should not change in
; structure, because external tools rely on this data being there!
; The magic values are the name at offset 0x0a and the xms handle
; right after that being HDCACHE$ and not-zero respectively.
; LBAcache INFO wants havelba and drvselmask to stay at their places, too.

xmshandle	dw 0	; our XMS handle (also for unloading)
xmsvec		dd 0	; call far pointer for XMS services

geometry:
	db 63,255	; for the hard disks 0x80 .. 0x87:
	db 63,255	;   first byte is max sector number,
	db 63,255	;   second byte is max head number
	db 63,255	; *** NEW: 8 drives supported!

	db 63,255
	db 63,255
	db 63,255
	db 63,255
ageometry       dw 0x112        ; *** geometry of A: 
bgeometry       dw 0x112        ; *** geometry of B: 
			; *** remember that UNCACHE has to be adjusted
			; to reflect the different layout! (infolist)
			; now using labels instead of numbers :-)

rdhit	dd 0	; those 4 counters offer some statistics
rdmiss	dd 0
wrhit	dd 0
wrmiss	dd 0

hint	dw table	; pointer to table (hint for debuggers)
tabsz	dw 8	; hint for debuggers: size of table entries
		; high byte is how far sectors must be shifted right
		; to get number of table entries!

sectors	dw 4096		; how many sectors to cache (512 bytes
			; XMS and {4 bytes CHS / 8 bytes LBA version}
			; DOS RAM for each sector) (also for debuggers)
			; NEW binsel.27.11.2001 uses only 8 bytes for
			; each 1<<N sectors, e.g. for 4 sectors :-)

oldvec	dd 0		; the old INT 0x13 VECTOR (also for unloading)


running	dw 0		; 1 means "init already done"
			; 2 means "disabled" (for the dispatcher)

tsrsize	dw eoftsr	; to know what has to stay in RAM.
			; During init: how much has to stay in RAM
			; to activate the cache. After that: How much
			; has to stay in RAM when the cache is shut down
			; and deactivated (if complete unload fails).

havelba	dw 0		; 1 means LBA BIOS found, 0 means CHS only
			; for rwfunc (actually only for read: When a
			; real disk read is needed, determine whether
			; it should be done with LBA or with CHS).
			; *** NEW semantics: bitfield, like drvselmask
			; 2008: only low byte used, MSB of high byte set
			; to indicate "in DOSEMU" (ignore floppy change)

drvselmask	dw -1	; LSB for disk 0x80 etc. - 1 means cached
			; *** NEW: also a bitfield now. Only low byte used.

; ---------------------------------------------------------------

int13new:
	test word [cs:running],1
	jz int13off			; handler disabled?
	jmp NEWI13			; the dispatcher (386 code...)
int13off:
	jmp far [cs:oldvec]		; chain to old handler

	; all stuff up to this line has to stay in RAM in case we were
	; loaded as SYS or in case int 13 was changed in the meantime!

eoftsr:	; just a label to know where the important part ends

; ---------------------------------------------------------------

