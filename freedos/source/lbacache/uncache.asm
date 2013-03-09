 ; This file is part of LBAcache, the 386/XMS DOS disk cache by
 ; Eric Auer (eric@coli.uni-sb.de), 2001-2005.

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




; this is a silly UN-loader:
; It scans through the memory for instances of HDCACHE$ device
; headers, and if they have an XMS handle, it closes the handle
; after disabling the driver.
; If in addition the link to a COMCACHE signature is found, the
; MCB size is reduced to save some RAM.
; If - third case - even int 0x13 is found to be still pointing
; to the device, the oldint vector is restored and the RAM is
; completely freed.
; if an I option rather than an U option is given, will only
; show information and not unload the instances.


; new 01/2002: the F flush option: flush drives A..F ("eject media").
; new 01/2002: uncache is now part of the cache binary :-)
; new 08/2002: no waiting inside status display, dec/percent display
; new 11/2002: more messages and flushes A..J, no longer very suitable
;              for standalone (infolist uses labels, not numbers now).
; new 11/2002: two column output, redirectable, fixed percentage bug.
;              also worked on some crash: resizing MCBs changes owner
;              made output layout generally nicer :-)
;              Had to move messages IN FRONT because they may not be
;              at offset > 0x2000: the offset high bits are FLAGS for
;              pmessage... Could squeeze those flags, though. *******
;              Removed ability to run STANDALONE completely.
; new 08/2003: far more human readable output, more decimal output.
;              Changed pmessage to use only 2 high bits as flags, not 3, so
;              offsets can be up to 0x3fff now. Unfolding bit fields, too.
; new 05/2004: short STAT statistics output, related to INFO. ZERO to
;              clear statistics. Mute INFO stuff for non-INFO topics.

; -------------------------------------------------------------

crlfmsg21	db 13,10,"$",0
pmtabmsg21	db 9,"$",0

flushmsg	db "Flushing... (nothing else)$"
		; no linebreak as pmessage already does that!
flushsepmsg	db " * $"
flushedmsg	db 13,10
		db "Sent eject media command to BIOS drives 0,1,0x80..0x87",13,10
		db "- to tell all loaded LBAcaches to flush themselves.$"

noxmsmsg	db "No XMS driver: no LBAcaches! $"
yesxmsmsg1	db "Searching for LBAcaches... $"
yesxmsmsg2	db "XMS pointer is now:  0x$"
curri13msg	db   "Int 0x13 vector is: 0x$"

keymsg		db "Scanning for more caches...$"
donemsg		db "Done for all LBAcaches.$"

foundmsg	db "Found signature at segment    0x$" ; *hex*
xmsmsg		db "Used XMS handle number: 0x$" ; *hex*
xmsinfomsg	db "XMS handle has size (in kB) $"

infolist1	dw rdhit,rhitmsg+0x8000,  rdmiss,rmissmsg+0x8000
		dw wrhit,whitmsg+0x8000,  wrmiss,wmissmsg+0x8000
		dw 0,0

infolist2	dw hint,tabinf1+0x4000, oldvec,oldi13msg+0x8000
; %%%		dw havelba,hlbamsg+0x2000,drvselmask,dvselmsg+0x2000
		dw 0,0
		; *** NEW 11/2002 must match positions in datahead
		
rhitmsg		db "read  hits (sectors): $"
rmissmsg	db "read  misses: $"
whitmsg		db "write hits (sectors): $"
wmissmsg	db "write misses: $"

tabinf1 	db "table offset (hex.):    0x$"
oldi13msg	db "stored  int 0x13  vector: 0x$"

		; Beautified this display 5/2004
dvsel1msg	db "Caching first physical harddisk.$"
dvselnmsg	db "Caching first "
dvselnpatch	db "? physical harddisks.$"
dvselmsg	db "Cached disks 0x8?,  ?="
dvselfield	db "01234567$"
		; Beautified this display 5/2004
alllbamsg	db "[LBA enabled]"
fdanypatch	db " Caching floppy "
fdlistpatch	db "A: B:$"	; new 5/2004
hlbamsg		db "LBA found for disks 0x8?, ?="
hlbafield	db "01234567$"
dummycachemsg	db "This cache caches NO drives!$"

	; we get log2 of this information in 2a: hi(tabsz) 2b: lo(tabsz)
tabinf2a	db "sectors per bin in table: $"
tabinf2b	db "bytes per bin in table: $"

hrdmsg		db "total MegaBytes read:     $"
hwrmsg		db "total MegaBytes written:  $"
hhitmsg		db "--> percentage of hits:  $"

twocolumns	db 0	; *** 11/2002 start with one column
		; *** also new 11/2002: string lengths tuned :-)
		; low bit: 0=crlf 1=tab after message
		; bit 1: 0=nothing 2=toggle low bit after message

unhookmsg	db "Int 0x13 vector is now again 0x$" ; *hex*
dangerchainmsg	db "Int 0x13 CANNOT be rechained.",9
		db "DOS RAM will NOT be freed.$"

dangerchain	db 0	; *** 11/2002: is one if we have met any
			; *** non-unchainable instance before!
oldpsp		dw 0	; *** 11/2002: needed to work around the
			; *** DOS oddity that resizing makes you
			; *** own the MCB (so that it will be
			; *** freed when YOU terminate...). Sigh!

statclearedmsg	db "Statistics of this instance cleared.$"

comsegmsg	db "LBAcache.com found, PSP segment:  0x$"
reducemsg	db "Reducing MCB to .... paras (hex.) 0x$"
removemsg	db "Removing LBAcache.com from RAM.$"
mcbfailmsg	db "MCB operation failed, trying UMB variant$"

xmserrmsg	db "XMS problem, function.error is: 0x$"
xmsptr		dd 0	; pointer to the XMS handler
xmswhat		db 0	; function code

unflag		db 0	; what to do: 1:unload/stop 2:info 4:flush/sync
			; 8:stat 16:zero_stats 64:only show help
		; * see the %define-d ????flag values in cmdline.asm! *
currint13	dd 0	; current value of int 0x13 vector


; -------------------------------------------------------------

showwimp:
	mov si,dx	; show a message before leaving
	call pmessage	; see below :-)
leavethis:
	pop ds
	pop es
	pop eax
	popa
	ret

uncache:		; always called from the cache
	pusha
	push eax
	push es
	push ds

	mov ah,[cs:args]
	test ah,HELPflag
	jnz leavethis
	test ah,31	; only continue if anything to do
	jz leavethis	; bits: 1 stop (remove all caches with
	mov bx,cs	; the signature and a nonzero XMS handle)
	mov ds,bx	; 2 info (stop should also set bit 1 !)
	mov es,bx	; 4 sync (flush/empty) all caches

; -------------------------------------------------------------

	; .386 (nasm does not have this "enable 80386 code" at)
	; (all, 80386 code is always allowed. Be careful your-)
	; (self thus not to "jz near ..." or so on an 8086... )

	; I know, no 386 presence check. pah. The STANDALONE
	; version is no longer maintained. UNCACHE now is part
	; of LBACACHE itself and gets run AFTER the 386 check...

confirmed:			; ok, so we have a mission now!
	mov [cs:unflag],ah	; store the type of our mission

	test ah,SYNCflag	; SYNC is special: needs no scanning.
	jz confnorm
	mov si,flushmsg
		call pmessage	; message only, no AL AX EAX shown
	mov ax,0x0d00
		int 0x21	; DOS "disk reset" - writes back dirty
				; buffers. And flushes some caches :-)
	mov ax,0x4600		; eject media (int 13 extensions)
	mov dl,0		; A:
		int 0x13	; no error checking: we only care for
	mov ax,0x4600		; the side effect of flushing the cache
	mov dl,0		; B:
		int 0x13
	mov ax,0x4600
	mov dl,0x80		; C:..J:
flushloop:
	push dx
		int 0x13
	pop dx
	inc dl

	push dx
	push ax
	mov dx, flushsepmsg	; separator to split the feedback from
	mov ah,9		; the resident LBAcache; "progress bar".
	int 0x21		; DOS string output
	pop ax
	pop dx

	cmp dl,0x87		; *** flush C:..J: (0x80..0x87)
	jbe flushloop
	mov al,0
	mov dx,flushedmsg
		jmp showwimp	; we flushed (and maybe ejected) A..F 

; -------------------------------------------------------------

confnorm:
	mov ax,0x4300
	int 0x2f	; XMS install check
	cmp al,0x80
	jz xmsfound
	mov dx,noxmsmsg
	jmp showwimp	; no XMS - no caches can exist either

xmsfound:
	push es
	mov ax,0x4310
	int 0x2f	; get the pointer to the xms far call
	mov [cs:xmsptr],bx
	mov ax,es
	mov [cs:xmsptr+2],ax
	shl eax,16
	mov ax,bx
	pop es

	mov byte [cs:twocolumns],3	; start with 2 column mode + tab!

	mov si,yesxmsmsg1		; show message only
		call pmessage		; XMS found, scanning may begin

	test byte [cs:unflag],INFOflag
	jz uc_suppr1
	mov si,yesxmsmsg2+0x8000	; show message and EAX
		call pmessage		; XMS found, scanning may begin
	mov byte [cs:twocolumns],3	; start with 2 column mode + tab!
uc_suppr1:


	push word 0
	pop es			; ES will change soon anyway
	mov eax,[es:0x4c]	; current int 0x13 vector
	mov [cs:currint13],eax	; store for later
	test byte [cs:unflag],INFOflag
	jz uc_suppr2
	mov si,curri13msg+0x8000	; show message and EAX
		call pmessage
uc_suppr2:

; -------------------------------------------------------------

	mov bp,0x70	; assume no success before this...

findme:
	mov es,bp
	push eax
	mov eax,[es:0x100]
	inc eax			; to avoid having pure MAGIC in next line
	cmp eax,MAGIC+1		; scan for the magic value
	pop eax
	jnz findon
	mov ax, [es:xmshandle]	; XMS handle
	or ax,ax
	jz findon		; no XMS handle - must be already off
	push eax
	mov eax,[es:xmsvec]	; XMS pointer for this one
	cmp eax,[cs:xmsptr]
	pop eax
	jz down1	; XMS pointers match
findon:
	jmp nope	; go on scanning

down1:
	test byte [cs:unflag],STOPflag	; UNLOAD requested?
	jz noshutdown
	mov word [es:running],2	; SHUT DOWN driver
noshutdown:

	mov byte [cs:twocolumns],2	; *** 2 column mode + crlf!

	test byte [cs:unflag],INFOflag
	jz uc_suppr3
	push ax
	mov si,foundmsg+0x4000	; *offset* - show message and AX
	mov ax,bp
		call pmessage	; show segment
	pop ax

; ----------------------

	mov si,xmsmsg+0x4000	; *offset* - show message and AX
		call pmessage	; show handle
uc_suppr3:

	mov dx,ax	; handle
	  push dx	; **save
	mov ah,0x0e	; INFO handle
		call doxmscall
	jnc xmsok1
	  pop dx	; **restore1
xmswimp:
	jmp instancedone	; wimp out

xmsok1:
	mov ax,dx	; size in kb
		; + STATflag because even non-tech STAT should show this:
	test byte [cs:unflag],INFOflag + STATflag
	jz uc_suppr4
	mov si,xmsinfomsg+0x8000	; *offset*, show message and EAX
		movzx eax,ax		; clear high half (max 64 MB) <<<
		call hex2dec		; convert to decimal <<<
		call pmessage
uc_suppr4:
	  pop dx	; **restore2

	test byte [cs:unflag],STOPflag	; do we UNLOAD ?
	jz nofreexms
refreexms:
	mov ah,0x0a	; FREE handle
		call doxmscall
	jnc xmsfreeok

	cmp bl,0xab	; failed because locked? NEW 6/2005
	jnz xmswimp
	mov ah,0x0d	; UNLOCK handle
		call doxmscall
	jnc refreexms	; try again (loop is usually left after 1 round)
	jmp xmswimp	; (end of NEW 6/2005 stuff)

xmsfreeok:
	mov word [es:xmshandle],0	; remove XMS handle ID -> mark
				; driver as uninteresting for UNCACHE
nofreexms:

; ----------------------

	; infolist1 shows read and write hit counts: for INFO and STAT.
	test byte [cs:unflag],INFOflag + STATflag
	jz infodone1

	mov bx,infolist1	; a list of what we want to show in dec
infoloop1:
	mov si,[cs:bx]		; where to read
	or si,si
	jz infodone1
	mov eax,[es:si]		; read a value from driver
	inc bx
	inc bx
	mov si,[cs:bx]		; select a message
	test si,0x8000		; 32 bit?
	jnz info32list1
	movzx eax,ax		; mask to 16 bit otherwise
	test si,0x4000		; 16 bit?
	jnz info16list1
	xor ah,ah		; mask to 8 bit otherwise
info16list1:
info32list1:
		call hex2dec	; convert EAX to packed BCD
		call pmessage	; show the message and AX/EAX
	inc bx
	inc bx
	jmp short infoloop1	; go on with next info

infodone1:

; ----------------------

	; Next is the quite technical table description.
	test byte [cs:unflag],INFOflag
	jz infodone2

tablehints:			; decode table property description bits
	mov ax,[es:tabsz]	; READ VALUE FROM DRIVER
	and ax,0x0f0f		; ignore undefined bits
	push cx
	push ax
	mov cl,ah		; log2 of sectors per bin
	xor eax,eax
	inc eax
	shl eax,cl
		call hex2dec	; convert to packed BCD
	mov si,tabinf2a+0x4000	; show a 4 digit value
		call pmessage	; show the message and AX
	pop ax
	mov cl,al		; log2 of bytes per bin
	xor eax,eax
	inc eax
	mov ax,1
	shl eax,cl
		call hex2dec	; convert to packed BCD
	mov si,tabinf2b+0x4000	; show a 4 digit value
		call pmessage	; show the message and AX
	pop cx

; ----------------------

	; This shows more table information and the stored int 13 vector.
	mov bx,infolist2	; a list of what we want to show in hex
infoloop2:
	mov si,[cs:bx]		; where to read
	or si,si
	jz infodone2
	mov eax,[es:si]		; read a value from driver
	inc bx
	inc bx
	mov si,[cs:bx]		; select a message
		call pmessage	; show the message and AX/EAX
	inc bx
	inc bx
	jmp short infoloop2	; go on with next info

infodone2:

; ----------------------

	; This lists the cached and LBA enabled drives.
; ***	test byte [cs:unflag],INFOflag + STATflag
; ***	jz bitshowdone

cachebitshow:			; new 5/2004: trap simple cases
	mov al,[es:drvselmask]	; 1 bit/disk, lsb is 0x80
	mov si,fdanypatch	; if no harddisk cached
	cmp al,1		; trigger "first harddisk" message
	jz cachebit1st		; exactly 1 harddisk?
	jb lbafornoneshow	; 0 disks? then just do the floppy info

	mov ah,'2'		; I think a loop would not be smaller...
	cmp al,3
	jz cachebitndisk
	inc ah
	cmp al,7
	jz cachebitndisk
	inc ah
	cmp al,15
	jz cachebitndisk
	inc ah
	cmp al,31
	jz cachebitndisk
	inc ah
	cmp al,63
	jz cachebitndisk
	inc ah
	cmp al,127
	jz cachebitndisk
	inc ah
	cmp al,255
	jz cachebitndisk

	mov si,dvselfield	; worst case: show bit field
	mov bx,dvselmsg
		call bitshow	; fill field
	jmp short lbabitshow

cachebit1st:
	mov si,dvsel1msg	; "caching first physical harddisk"
		call pmessage
	jmp short lbabitshow

cachebitndisk:
	mov [cs:dvselnpatch],ah	; patch in ASCII digit
	mov si,dvselnmsg	; "first N harddisks" message
		call pmessage

lbabitshow:
	mov al,[es:havelba]	; 1 bit/disk, lsb is 0x80
	cmp al,[es:drvselmask]	; do ALL selected drives have LBA?
	jz lbaforallshow	; then show "LBA enabled" + floppy info
	mov si,fdanypatch
	or al,al		; does NONE of the drives have LBA?
	jz lbafornoneshow	; then just show floppy cache info
	mov si,hlbafield
	mov bx,hlbamsg
	mov byte [cs:twocolumns],2	; *** 2 column mode + crlf!
		call bitshow	; fill field
	jmp bitshowsdone

lbaforallshow:
	mov si,alllbamsg	; all cached drives have lba
lbafornoneshow:
	; *** new 5/2004: Show floppy cache status ***
	; Not reached if you have several disks and only SOME have LBA
	; ... this is a layout decision to keep the output shorter.
	mov ax,[es:fddstat]	; hi byte: cacheeable lo byte: cached
	test al,3		; any floppy cache enabled?
	jz noflopshow
	test al,1		; A: floppy cache enabled?
	jnz doflopashow
	mov word [cs:fdlistpatch],'  '	; zapp "A:" string
doflopashow:
	test al,2		; B: floppy cache enabled?
	jnz lbaflopshow
	mov byte [cs:fdlistpatch+2],'$'	; zapp "B:" string
	jmp short lbaflopshow
noflopshow:
	mov byte [cs:fdanypatch],'$'	; zapp message about floppy
	cmp byte [es:drvselmask],0	; no harddisks either?
	jnz lbaflopshow			; else okay
	mov si,dummycachemsg		; nothing cached at all
lbaflopshow:
	mov byte [cs:twocolumns],2	; *** 2 column mode + crlf!
		call pmessage
	jmp bitshowsdone


; ----------------------

bitshow:
	push bx			; the message to show
	mov bl,'0'		; drive 0x80
	mov ah,1		; drive 0 corresponds to LSB

showbitloop:
	test al,ah		; this drive?
	jnz showthisbit
	mov byte [cs:si],'_'	; nope
	jmp short shownextbit
showthisbit:
	mov byte [cs:si],bl
shownextbit:
	shl ah,1		; next drive bit
	inc bl			; next drive description
	inc si			; next field slot
	cmp bl,'8'
	jb showbitloop		; more drives to check

	pop si
		call pmessage	; show the message
	ret

; ----------------------

bitshowsdone:

	; Show cache throughput in MBytes and cache hit percentage.
	test byte [cs:unflag],INFOflag + STATflag
	jz near humanreadabledone

; *** 8/02: added human readable display
	push edx
	push ebx

humanreadable:
	mov eax,[es:rdhit]	; read hits (unit: 512 byte sectors)
		push eax	; * save hits for percent
	xor edx,edx
	add eax,[es:rdmiss]	; read misses added
	adc edx,edx
		push eax	; * save (low part of) sum for percent

	mov ebx,2048
	div ebx			; transform (sum!) to MBytes
	xor edx,edx
		call hex2dec	; transform to packed BCD
	mov si,hrdmsg+0x4000	; human readable read message, 16bit
	test eax,0xffff0000	; need 32bit?
	jz hrd16
hrd32:	xor si,0xc000		; 16->32bit display size
				; (flags 0x4000 instead of 0x8000)
hrd16:		call pmessage

		pop eax		; * total
	mov ebx,eax
		pop eax		; * hits
		push ebx	; * total (>= hits)
	mov ebx,100		; for percent *** BUG fixed
	xor edx,edx
	mul ebx			; *** BUG was: did use edx=100
		pop ebx		; * total
				; now ebx is an access count > 0
	or ebx,1		; *** lazy "rounding"

				; EDAX=100*hits EBX=(hits+misses)|1
	div ebx			; calculate percent

	cmp eax,100		; clip to 100%
	jb hrd99
hrd100:	mov eax,100
	jmp short hrdmax
hrd99:		aam		; AAM: ah=al div 10, al=al mod 10
		shl ah,4
		or al,ah	; now we have packed BCD
		xor ah,ah	; clear high byte
hrdmax:
	mov si,hhitmsg+0x4000	; percentage message, 16bit <<<
		call pmessage

; ----------------------

humanreadable2:
	mov eax,[es:wrhit]	; write hits (unit: 512 byte sectors)
		push eax	; save for percent
	xor edx,edx
	add eax,[es:wrmiss]	; read misses added
	adc edx,edx
		push eax	; save (low part) for percent
	mov ebx,2048
	div ebx			; transform to MegaBytes
	xor edx,edx
		call hex2dec	; transform to packed BCD
	mov si,hwrmsg+0x4000	; human readable write message, 16bit
	test eax,0xffff0000	; need 32bit?
	jz hwr16
hwr32:	xor si,0xc000		; 16->32bit display size
				; (flags 0x4000 instead of 0x8000)
hwr16:		call pmessage
		pop eax		; * total
	mov ebx,eax
		pop eax		; * hits
		push ebx	; * total
	mov ebx,100		; for percent *** BUG fixed
	xor edx,edx
	mul ebx			; *** BUG was: did use edx=100
		pop ebx		; * total
				; now ebx is an access count > 0
	or ebx,1		; *** lazy "rounding"

				; EDAX=100*hits EBX=(hits+misses)|1
	div ebx			; calculate percent

	cmp eax,100		; clip to 100%
	jb hwr99
hwr100:	mov eax,100
	jmp short hwrmax
hwr99:		aam		; AAM: ah=al div 10, al=al mod 10
		shl ah,4
		or al,ah	; now we have packed BCD
		xor ah,ah	; clear high byte
hwrmax:
	mov si,hhitmsg+0x4000	; percentage message, 16bit <<<
		call pmessage

	pop ebx
	pop edx

; ----------------------

humanreadabledone:
	test byte [cs:unflag],ZEROflag	; should we ZERO statistics?
	jz nozeroinstance

	xor eax,eax			; zap those statistics
	mov [es:wrmiss],eax
	mov [es:wrhit],eax
	mov [es:rdmiss],eax
	mov [es:rdhit],eax
	mov si,statclearedmsg		; tell that we zapped
		call pmessage

nozeroinstance:
	test byte [cs:unflag],STOPflag	; do we UNLOAD ?
	jz near instancedone	; if not, we are done with the instance

	mov byte [cs:twocolumns],0	; *** 11/2002 if yes, change
					; *** to one column mode!
			; (could check if we are in left column here)

shrinkit:
	mov ax,es		; driver segment
	cmp ax,[cs:currint13+2]	; current int 13 vector segment
	jz fullremove		; int 13 still in driver
	mov eax,[es:oldvec]	; old int 13 vector...
	cmp eax,[cs:currint13]
	jnz partremove		; IF Z: int 13 same as oldint stored
				; in driver, so it was not even hooked

	test byte [cs:dangerchain],1	; *** BUGFIX 11/2002: if the
				; *** removed instance points to yet
				; *** ANOTHER removed instance, we
	jz fullremove		; *** would be doomed!
	mov si,dangerchainmsg	; warning message, 0 bit.
		call pmessage
	jmp short partremove

; ----------------------

fullremove:
	mov eax,[es:oldvec]	; int 13 vector before driver took it
	push es
	  push word 0
	  pop es
	mov [es:0x4c],eax	; un-hook the int 13 vector !!
	pop es
	mov [cs:currint13],eax	; update our notion of int 13

	mov si,unhookmsg+0x8000	; message and EAX
		call pmessage	; tell that we have unhooked

; ---	xchg bx,bx
; ---	xchg bx,bx
; ---	xchg bx,bx

	mov ax,0		; now we can FREE the used memory,
	jmp comremove		; unless the driver was loaded as sys.

; ----------------------

				; we cannot unhook int 0x13, but
partremove:			; we can still reduce the used size,
				; unless the driver was loaded as sys.

	or byte [cs:dangerchain],1	; *** NEW 11/2002 remember
					; *** that we came along this
					; *** cannot-unhook-case!

	mov ax,[es:tsrsize]	; where to end the TSR after shrink
	add ax,15
	shr ax,4		; paragraphs - NEEDED BELOW
comremove:

	mov bx,es		; the DOS allocated segment of this
				; Very simple since 5/2004: equal to
				; segment where the MAGIC was found.
	; In older versions, the DOS segment was for the COM overhead
	; while the magic string was found in the SYS/pseudo-sys
	; segment somewhat later. The S/S segment used 0-based offsets
	; ... the 5/2004 version just uses normal .com "org 0x100" ...

	push ax
	mov ax,bx		; segment of com
		mov si,comsegmsg+0x4000	; show message and AX
		call pmessage
	pop ax

		mov si,removemsg	; show message only

	or ax,ax		; will new size be 0 paragraphs?
	jz vanishing

; ---	or byte [cs:dangerchain],128	; *** NEW 11/2002

	; before 5/2004, we now calculated AX = 1+size+SYSSEG-COMSEG
	; which is AX = 1 + size + ES - BX. Now we simply have ES=BX
	; so we can reduce to AX = size + 1 paragraphs.
	inc ax
		mov si,reducemsg+0x4000	; show message and AX

vanishing:
		call pmessage	; tell that we are reducing


	mov cx,0x1149	; start by assuming FREE (seg bx)
			; CH is the XMS function, CL the DOS one
	or ax,ax
	jz mcbme	; shrink/remove seg bx to ax paragraphs
	mov cx,0x124a	; resize, do not free

	; *** DOS oddity workaround, as recommended by FreeDOS
	; *** kernel people: If you resize a MCB, you automatically
	; *** own it. Solution: Pretend being the instance to
	; *** be resized (so that it stays owner of its own MCB).

; ---	test byte [cs:dangerchain],128	; *** NEW 11/2002: COM flag
; ---	jz nopsptrick1
	push ax
 	push bx
; //	mov ah,0x62	; *** get current PSP segment
; //	int 0x21	; *** (of this non-resident LBAcache instance)
; //	mov [cs:oldpsp],bx	; *** save it
	mov [cs:oldpsp],cs	; *** It is CS in this COM file! [5/2004]
		pop bx	; *** restore the PSP address of "our victim"
		push bx
	mov ah,0x50	; *** SET your process ID / PSP addr to seg BX
	int 0x21	; *** "be the instance that is resized"
	pop bx
	pop ax
; --- nopsptrick1:

mcbme:	push es
	push bx
	push ax
	mov es,bx		; segment
	mov bx,ax		; new size (only used if resizing)
	mov ah,cl		; the function to do
		int 0x21	; resize/free MCB at segment ES
	pop ax
	pop bx
	pop es

	pushf

	or ax,ax	; // Did we "shrink" to zero size?
	jz nopsptrick2	; // 5/2004: COM flag gone, use THIS instead!
; ---	test byte [cs:dangerchain],128	; *** NEW 11/2002: COM flag
; ---	jz nopsptrick2
	push ax
	push bx
	mov bx,[cs:oldpsp]	; *** back to original value
	mov ah,0x50	; *** SET your process ID / PSP addr to seg BX
	int 0x21	; *** "be yourself" again (for int 21.4c...)
	pop bx
	pop ax
; ---	and byte [cs:dangerchain],127	; *** clear "is com" flag
	nopsptrick2:

	popf

	jnc instancedone	; if it has worked, done with this!

	mov si,mcbfailmsg	; Only reason: this was no MCB
		call pmessage	; MCB operation failed

	; trying UMB mechanism as a second possibility
	; still, segment is BX and new size is AX
	; if DOS=UMB is active, DOS is supposed to have
	; converted all UMB to MCB, but we will see...

	mov dx,bx	; segment
	mov bx,ax	; new size (if resize)
	mov ah,ch	; the XMS function we figured out above
		call doxmscall	; XMS UMB free/resize
	; jc umbfailed... - the user already knows from the XMS error

; -------------------------------------------------------------

; *** Wait removed 8/02
instancedone:
	mov si,keymsg		; only show message
		call pmessage	; tell that we are done ; *** waiting
; ***	mov ah,0
; ***		int 0x16	; wait for a keypress
	
nope:	inc bp			; signature not found
	cmp bp,0xa000	; skip seg 0xa000 ... 0xc1ff in search
	jnz nskip
	mov bp,0xc200	; no VGABIOS would be < 16k, but Xdosemu
			; has an 8k VGABIOS ... http://www.dosemu.org/
nskip:	cmp bp,0xf000	; no we will not check the ROM and HMA area!
	jnz near findme	; go on searching

leavethis0:
	mov si,donemsg		; only show message
		call pmessage	; say goodbye...
	mov al,0		; errorlevel 0
	jmp leavethis

; -------------------------------------------------------------


pmessage:		; show message at (si and 0x3fff) and maybe
	push dx		; value of ax or eax (ax if si test 0x4000 nz,
	push eax	; eax if si test 0x8000 nz), and then CRLF
	push ds		; *** NEW 11/2002: twocolumns may skip CRLF

	  push cs	; *** if on, every other CRLF will be a TAB
	  pop ds	; *** IF you turn twocolumns OFF (and ~2),
	  push ax	; *** AND the "test ... 1" is ZR, then you
			; *** have to CRLF before the next string.
	mov dx,si
	and dx,0x3fff	; mask out our flags (offset is max 16k)
	mov ah,9
		int 0x21

	  pop ax

	test si,0xc000	; any bytes to show? <<<
	jz pmdone	; if none, done

	  mov dl,cl	; save CL
	  push ax
	mov ax,si
	shr ax,12	; how many nibbles to show
	and al,12	; only 4/8/12, which is    <<<
			; AX EAX ? (2, 4, 6 bytes) <<<
	mov dh,al	; as a counter
	mov cl,8	; max 8 nibbles
	sub cl,dh	; how many nibbles not to show
	shl cl,2	; nibbles -> bits
	  pop ax
	shl eax,cl	; make EAX "left-bound"

	push si
	and si,0x3fff	; mask out our flags (offset is max 16k)
scanhx:	inc si		; find end of string
	cmp byte [si],'$'
	jnz scanhx
	cmp word [si-2],'0x'	; did string end with "0x"?
	pop si
	mov cl,'*'	; *** do not suppress leading zeroes for HEX
	jz showeaxlp
	mov cl,'0'	; *** start suppressing leading zeroes for DEC

showeaxlp:
	rol eax,4	; move digit to show in lowest pos
	  push ax
	and al,0x0f
	add al,'0'	; hex -> ascii
	cmp al,'9'
	jbe nothex
	add al,7	; 'A'-('9'+1)
nothex:	cmp al,cl	; *** leading zero to be suppressed?
	jnz notzero
	mov al,'_'	; *** SUPPRESS leading zero!
	jmp short waszero
notzero:
	mov cl,'*'	; *** stop suppressing leading zeroes
waszero:
	mov dl,al
	mov ah,2
%ifdef REDIRBUG
	call BUGTTY
%else
		int 0x21	; show a char
%endif
	  pop ax
	cmp dh,2	; last digit to follow now?
	jnz notend
	mov cl,'*'	; *** do not ever suppress last digit...
notend:	dec dh
	jnz showeaxlp	; on to the next digit

	  mov cl,dl	; restore CL

pmdone:	mov dx,crlfmsg21	; *offset*

	test byte [cs:twocolumns],1	; TAB instead of CRLF ?
	jz pmdonecrlf
	mov dx,pmtabmsg21
pmdonecrlf:
	mov ah,9
		int 0x21
	test byte [cs:twocolumns],2	; toggle mode on?
	jz pmnotoggle
	xor byte [cs:twocolumns],1	; toggle TAB <-> CRLF
pmnotoggle:

	pop ds
	pop eax
	pop dx
	ret

suppzero	db '0'	; char of which leading instances are suppressed

; -------------------------------------------------------------

hex2dec:		; take a longint eax and convert to packed BCD
	push ebx	; overflow handled (0xffffffff has 10 digits)
	push ecx
	push edx
	push edi
	xor edx,edx
	xor ecx,ecx	; shift counter
	xor edi,edi	; result
	mov ebx,10
h2dlp:	div ebx		; remainder in edx, 0..9
	shl edx,cl	; move to position
	or edi,edx	; store digit
	xor edx,edx	; make edx:eax a proper 64bit value again
	or eax,eax	; done?
	jz h2dend
	add cl,4	; next digit
	cmp cl,32	; digit possible?
	jae h2doverflow	; otherwise overflow
	jmp short h2dlp
		
h2dend:	mov eax,edi	; return packed BCD
	pop edi
	pop edx
	pop ecx
	pop ebx
	ret

h2doverflow:
	mov edi,0x99999999
	jmp short h2dend

; -------------------------------------------------------------

doxmscall:		; do an XMS call, with error handling
	mov [cs:xmswhat],ah	; for the error message
	call far [cs:xmsptr]
	or ax,ax
	jz xmserror		; ax=1 if ok
	clc			; ok
	ret

xmserror:			; ax was 0, error code in BL
	cmp bl,0xab		; if XMS locking error, suppress message:
	jz xmsmuteerror		; handled by caller, so no message needed
	push ax
	push si
	mov al,bl		; error code
	mov ah,[cs:xmswhat]	; verbose :-)
		mov si,xmserrmsg+0x4000	; *offset*, show message and AX
		; AH is not very important, but we show it anyway
		call pmessage
	pop si
	pop ax
xmsmuteerror:
	stc		; error
	ret

; -------------------------------------------------------------

