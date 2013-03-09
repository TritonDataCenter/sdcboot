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

; Check out the CHS version as well (limited to 8 GB,
; uses less DOS memory, and wimps out on LBA write)...

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

pendingwrite	dw 0	; so a read will end up before or after
			; a multi sector write, not in middle of it
rwbusy		dw 0	; better be too picky and allow NO nesting.

nesterr	db 'LBACACHE: int13 nesting',7,0

; ---------------------------------------------------------------

; lbageneric and chsgeneric are the wrappers for the write call,
; writemain is the generic loop to write disk and cache.
; lbawrite and hdwrite are the entry points from the dispatcher.

%include "write.asm"

; ---------------------------------------------------------------

	; that file contains CHStoLBA, which converts CXDH to EAX
	; in context DL, and LBAtoCHS, which does the same in the
	; other direction (converts EAX to CXDH in context DL).
	; Relies on the [geometry] table!
	; LBAtoCHS will return a sector number of 0 and a cylinder
	; number of 1023 on overflow (thus, CX will be 0xffc0).
	; All registers and flags are preserved.

%include "chs2lba.asm"

; ---------------------------------------------------------------

; lbaread and hdread are the entry points from the dispatcher.
; readmain is the main read from disk or cache loop.
; for read from disk, we have readfromdisk... which can do CHS
; if no LBA is detected, but defaults to LBA.

%include "read.asm"


