; TICKLE - floppy cache tickler / read ahead
; public domain by Eric Auer eric -at- coli.uni-sb.de 2003-2004.
; To compile with http://nasm.sf.net, do: nasm -o tickle.com tickle.asm

; Use this only with an int 13 floppy read cache loaded, like
; LBAcache with the FLOP command line option. Tickle should then
; "tickle" the cache and speed up floppy access then. Without loaded
; (floppy) cache, floppy access will be slowed down by tickle!

%define BUFSECT 18	; size of tickle track buffer in sectors
	; Tickle will refuse to load if the buffer wraps a linear 64k
	; boundary. Recommended value: 18. Minimum value: 16 (convention!)

%define ticklelist 0x88	; status buffer for tickling 80 * (2*4) bits
	; list which keeps track which tracks have been tickled
	; (We store this in the *PSP* now - 5/2004 - to save RAM!)
%define FDTRACKS 83	; how many floppy tracks to allow for tickling

	; *** suggested: TSIZE 8 or 16, TMAX 3 or 5. (5/2004) ***
	; For small cluster sizes, use small TSIZE values.
	; TSIZE must not be bigger than BUFSECT!
	; For CHS, only TSIZEs of 4, 8 and 16 make sense.
	; For LBA, use TSIZEs of 4, 8 or 16, or use BUFSECT=TSIZE=32.
%define TSIZE 8		; how many sectors to read ahead on harddisk
	; MUST be a power of two! A read of this size is made before
	; before the originally intended read, to load the cache.
	; Read-aheads which wrap tracks will NOT be tickled in CHS!
%define TMAX 4		; reads which are longer than this will not
	; trigger tickling on harddisk. Should be << TSIZE.

%define LBAhdTICKLE 1	; undefine to remove LBA harddisk handling (27.8.2003)
%define CHShdTICKLE 1	; undefine to remove CHS harddisk handling (31.7.2003)
%define RECHECK_CHS 1	; undefine to make CHS tickling 8086 compatible. Less
	; effective but saves some RAM if LBA support is not compiled in.

; LBAhdTICKLE and CHShdTICKLE share a table area: Worst case result is
; that when you *mix* CHS and LBA access, more or fewer read-ahead
; attempts than intended will take place. Saves some RAM. Harddisk
; read-ahead improved (added "recent access" table) in 5/2004.
; Moved the floppy track tickle list table into PSP to save RAM.

; ====================================

	org 100h	; a .com file
start:	jmp install

instdone:		; this final installer part clears the buffer
			; and overwrites the setup code by doing that!
	mov bx,[cs:ticklebufp]	; track buffer
	mov cx, (BUFSECT * 512)	; "heap" usage
	xor ax,ax
clbuf:	mov [cs:bx],al
	inc bx
	loop clbuf
	;
	mov cl,4
	mov ax,3100h	; no errors
	mov dx,[cs:ticklebufp]	; pointer to track buffer
	add dx,(BUFSECT * 512) + 15
			; size of resident code + PSP + "heap"
	shr dx,cl	; get size in paragraphs
	int 21h		; go TSR
	

i13old		dd 0	; pointer to old int 13 handler
ticklestat	db 0	; status of last int 13 call
blockedflop	db 0ffh	; floppy drive for which TICKLE is disabled (0..3)
ticklebufp	dw ticklebuf	; pointer to data buffer

; - dummyddpt	db 0,0,0,0, 1,0,0,0, 0,0,0,0, 0	; empty dummy DDPT
; - lastDI	dw 0	; for debugging
; - lastES	dw 0	; for debugging

ticklebugmsg1	db 13,10,"TICKLE: Odd "	; no ,0 here! 8/2004
tickledrivechr	db 'A'
ticklebugmsg1b	db ": -> DDPT[4]=",0
ticklebugmsg2	db " i13.8 sec=",0
ticklebugmsg3	db " i13.2 cx=",0
ticklebugmsg4	db 13,10,"Tell eric[at]coli.uni-sb.de"
		db 13,10,0

hexdigits	db "0123456789abcdef"

; ====================================

instcheck:			; offer our 8k+ buffer to others!
	cmp bx,'le'
	jnz nnntickle		; AX='ck' but rest differs -> chain
	cmp eax,'tick'		; only use 386 if pretty sure
	jnz nnntickle
	mov bx,cs
	mov es,bx		; buffer SEGMENT
	mov bx,[cs:ticklebufp]	; buffer OFFSET
	mov eax,'haha'		; laugh tickle laugh
	iret			; install check done

; ====================================

i13new:	cmp ax,'ti'	; magic request? (EAX=tick BX=le) <<< HANDLER >>>
	jz instcheck	; EAX tick means AX ti, not ck! (fixed 14.8.2004)
	cmp dl,3	; first 4 floppy drives?
	jbe isfloppy

%ifdef LBAhdTICKLE CHShdTICKLE	; if either of them is possible...
	cmp dl,80h	; a harddisk maybe?
patchplace:		; write 0ebh (jmp short) here -> no harddisk tickle
	jb nnntickle	; not handled by us
	jmp isharddisk
%endif

nnntickle:
	jmp far [cs:i13old]	; *** CHAIN to original handler ***

isfloppy:
	cmp dl,[cs:blockedflop]	; TICKLE blocked for this drive?
	jz nnntickle
	cmp ah,2	; read?
	jnz ntickle
	cmp ch,FDTRACKS	; only tickle first 80 cylinders
	jae ntickle
	cmp dh,0	; first head?
	jnz ytickle
	cmp cx,0001	; do NOT tickle boot sector: reading the
	jnz ytickle	; boot sector can mean an update of the

ntickle:		; right disk but wrong place or already tickled
	pushf
	call far [cs:i13old]	; *** CALL normal handler now ***
	jnc nerror
	cmp ah,6		; disk change?
	jnz nchanged
	jmp diskchange		; flush list, return disk change
nchanged:
	stc
nerror: push bp			; optimized code from Arkady :-)
        mov bp,sp		; BP IP CS callerflags
        rcr byte [bp+6],1	; carry->msb, callercarry->carry, ...
        rol byte [bp+6],1	; ourcarry->lsb, ...
        pop bp			; callercarry set to ourcarry :-)
        iret			; the safest way to return from int!


ytickle:		; current geometry...
	push ax
	push bx
	mov bh,0
	mov bl,ch	; cylinder
	call getmask	; get bit for this drive / head
	test [cs:ticklelist+bx],al	; already tickled that one?
	pushf
	or [cs:ticklelist+bx],al	; will tickle that one
	popf
	pop bx
	pop ax
	jnz ntickle	; already tickled that one
; -	jmp tickle	; *** do the actual tickling ***

; ====================================

tickle: mov byte [cs:ticklestat],0
	push es
	push bx
	push ax
	push cx

	push cx
	push dx
	mov byte [cs:tickledrivechr],dl
	push di
; -	; sometimes int 13.8 does not set ES:DI at all...
; -	mov di,cs
; -	mov es,di
; -	mov di,dummyddpt	; if BIOS does not set ES:DI...
	xor ax,ax
	mov es,ax
	les di,[es:78h]		; INT 1e vector (DDPT)
	; -
	mov ax,855h		; get drive parameters ("55" unused)
	; int 13h
	pushf
	call far [cs:i13old]	; *** get drive parameters
	mov ah,[es:di+4]	; DDPT (int 1e) sectors / track
; -	mov [cs:lastES],es
; -	mov [cs:lastDI],di
	pop di
	pop dx
	mov al,cl	; 6 lsb are "sectors per cylinder" value
	pop cx
	jc ticklebug	; do not tickle if that did not work
	and al,3fh	; max. sectors per cylinder for drive
	jz ticklebug	; do not tickle if no geometry known
	cmp ah,8
	jb ticklebug	; current sect. / cyl. at least 8 ?
	cmp ah,al
	ja ticklebug	; int 1e +4 (current sectors per cylinder) ok?
	mov al,ah	; prefer "current" over "max" sect. / cyl.
	; ES:DI AX BX CX DX returned by int 13.8: CX DX is geometry,
	; ES:DI int 1e (floppy params), AX 0, BL drive type
	cmp al,BUFSECT	; track longer than our buffer?
	jbe nmaxed
	mov al,BUFSECT	; then tickle only first e.g. 18 sectors
nmaxed: mov ah,2	; read to tickle
	mov bx,cs
	mov es,bx
	mov bx,[cs:ticklebufp]
	mov cl,1	; from first sector on
	; DX and CH are still user values
	pushf
	call far [cs:i13old]	; *** call original handler and tickle
	jnc ticklenostat
	mov [cs:ticklestat],ah
ticklenostat:
ticklenobug:
	pop cx
	pop ax
	pop bx
	pop es
	cmp byte [cs:ticklestat],6	; disk changed?
	jz diskchange
	jmp ntickle			; (other errors are ignored)

; ====================================

ticklebug:
	mov cx,ax
	add byte [cs:tickledrivechr],'A'
	mov bx,ticklebugmsg1
	call printstring
	mov al,ch
	call printAL
	mov bx,ticklebugmsg2
	call printstring
	mov al,cl
	call printAL
	mov bx,ticklebugmsg3
	call printstring
	pop cx
	push cx
	mov al,ch
	call printAL
	mov al,cl
	call printAL
	mov bx,ticklebugmsg4
	call printstring
	jmp short ticklenobug

; ====================================

printstring:
	mov al,[cs:bx]
	cmp al,0
	jz printed
	inc bx
	int 29h	; fast DOS TTY
	jmp short printstring
printed:
	ret

printAL:
	push bx
	push ax
	shr al,1
	shr al,1
	shr al,1
	shr al,1
	and ax,15
	mov bx,ax
	mov al,[cs:hexdigits+bx]
	int 29h	; fast DOS TTY
	pop ax
	push ax
	and ax,15
	mov bx,ax
	mov al,[cs:hexdigits+bx]
	int 29h	; fast DOS TTY
	pop ax
	pop bx
	ret

; ====================================

diskchange:
	push ax
	call getmask	; get bit for this drive / head
	mov ah,al
	xor dh,1	; other head
	call getmask
	or al,ah
	xor dh,1	; normal head again
	not al		; invert mask (clear bits)
	push bx
	mov bx,ticklelist
ticklelistclear:
	and byte [cs:bx],al
	inc bx
	cmp bx,ticklelist+FDTRACKS
	jb ticklelistclear
	pop bx
	pop ax
	;
	mov ah,6		; disk change
	push bp
	mov bp,sp		; stack: BP IP CS flags
	or byte [bp+6],1	; stc on caller flags
	pop bp
	iret			; the safest way to return from int

; ====================================

getmask:		; get bit corresponding to drive/head
	push cx
	mov cl,dl	; drive
	add cl,cl
	test dh,1	; head
	jz side0
side1:	inc cl
side0:	mov al,1
	shl al,cl
	pop cx
	ret

; ====================================
; ==== HARDDISK-ONLY CODE FOLLOWS ====
; ====================================

	; --- ---
endpoint:		; everything from here on can be overwritten
			; if no harddisk tickling is activated
	; --- ---

chaindisk:
	jmp nnntickle	; *** chain to original handler ***

isharddisk:

%ifdef LBAhdTICKLE
%ifdef CHShdTICKLE
%define HDTARGET hdtickletest
%else
%define HDTARGET chaindisk
%endif
	test dl,[cs:lbatickleflag]
	jz HDTARGET
	cmp ah,42h	; LBA read?
	jnz HDTARGET
	cmp dl,80h	; known drive?
	jb HDTARGET
	cmp dl,87h	; known drive?
	ja HDTARGET
	jmp lbatickle	; *** go and tickle it! ***
%undef HDTARGET
%endif

; ====================================

%ifdef CHShdTICKLE
hdtickletest:
	test dl,[cs:hdtickleflag]
	jz chaindisk
	cmp ah,2	; CHS read?
	jnz chaindisk	; only use original handler otherwise
	; *** go and tickle in CHS mode! ***

chstickle:
	push ax
	test cl,(TSIZE-1)	; << only tickle EVERY 8 starting position
	jnz nohdgeo	; << (simplified, not checking cylinder / head!)
	cmp al,TMAX
	ja nohdgeo	; << only tickle reads SHORTER than ... sectors

	call recent_chs_check	; already read ahead this area before?
	jc nohdgeo		; if recently tickled, don't retickle.

	push bx
	push cx
	push dx
	push es
	push di
	mov ah,8		; get drive parameters
		push ax		; save AL
	; int 13h
	pushf
	call far [cs:i13old]	; *** get drive parameters
		pop ax		; restore AL
	mov ah,cl	; ignore AH, use CL geometry part...
	pop di		; ignore "int 1e"
	pop es		; ignore "int 1e"
	pop dx		; GEOMETRY
	pop cx		; GEOMETRY
	pop bx		; ignore "floppy type"
	jc nohdgeo
	and ah,63	; find "sectors per track"
	jz nohdgeo	; 0 is impossible

	mov al,cl	; starting sector
	and al,63	; use only "sector" value
	add al,TSIZE	; << going to tickle EIGHT sectors
	cmp al,ah	; still well in range?
	ja nohdgeo	; otherwise, do not tickle

dochstickle:
	push es
	push bx
	mov ax,0200h + TSIZE	; << READ 8 sectors in order to tickle
	mov bx,cs
	mov es,bx
	mov bx,[cs:ticklebufp]
	; int 13h
	pushf
	call far [cs:i13old]	; *** read into tickle buffer
				; (ignore any errors)
	pop bx
	pop es

nohdgeo:
	pop ax
	jmp nnntickle	; let original handler do original job


recent_chs_check:	; check CX / DX whether this place has been
			; tickled recently
%ifdef RECHECK_CHS
	push ax
	push ebx
	xor ebx,ebx

	mov bx,cx	; fold down CX DX into some 8 bit "hash"
	ror bx,4
	xor bx,dx
	rol bx,4
	xor bl,bh			; simple "hash"
	and bx,63			; 64 table entries!

	xor ax,ax
	cmp [cs:hdd_hist+ebx*4],cx	; recently used CX?
	jnz rchsc1
	cmp [cs:hdd_hist+2+ebx*4],dx	; recently used DX?
	jnz rchsc1
	inc ax				; flag: recently tickled!
rchsc1:	mov [cs:hdd_hist+ebx*4],cx	; update table
	mov [cs:hdd_hist+2+ebx*4],dx	; update table
	pop ebx
	or ax,ax
	pop ax
	jz rchscN
rchscY:	stc		; already tickled, do not re-tickle
	ret
%endif	; RECHECK_CHS

rchscN:	clc		; not recently tickled
	ret

%else	; else not CHShdTICKLE
%undef RECHECK_CHS	; if no CHS tickling, RECHECK_CHS is never used
%endif


; ====================================


%ifdef LBAhdTICKLE

lbatickle:	; tickle LBA harddisk read
	push eax
	push ebx
	push ds
	push si
	cmp word [ds:si+2],TMAX	; did user try to read more than 3 sectors?
	ja nolbatickle		; if yes, no read ahead is done
	mov eax,[ds:si+12]	; did user try to read sector beyond 2 TB?
	or eax,eax
	jnz nolbatickle		; ignore those!
	mov eax,[ds:si+8]	; the sector that the user tried to read
	test al,(TSIZE-1)	; << sector number a multiple of 8?
	jnz nolbatickle		; only tickle in one of 4 cases!
	movzx ebx,dl		; which drive?
	and bl,7fh		; *** disk size table[0] is for disk 0x80
	add eax,byte TSIZE		; << we want to read that far
	jbe nolbatickle		; CY or ZR -> beyond 4G sectors
	cmp eax,[cs:disksizes+ebx*4]
	jae nolbatickle		; beyond end of disk
	sub eax,byte TSIZE	; << rewind to where read started

	call recent_lba_check	; already read ahead this area before?
	jc nolbatickle		; if recently tickled, don't retickle.

dolbatickle:
	mov si,diskaddrpacket	; our disk address packet
	push cs
	pop ds			; our disk address packet
	;
	push cs				; segment of buffer pointer
	push word [cs:ticklebufp]	; offset of buffer pointer
	pop dword [cs:lbabufp]		; store to disk address packet
	;
	mov word [cs:lbacnt],TSIZE	; << read ahead 8 sectors
	mov [cs:lbapos],eax	; ...starting with this sector
	; (not reinitialized: size word, high 32 bit of sector number)
	mov ah,42h		; LBA read
	pushf			; simulate interrupt
	call far [cs:i13old]	; *** read into tickle buffer
				; (ignore any errors)
nolbatickle:
	pop si
	pop ds
	pop ebx
	pop eax
	jmp nnntickle	; let original handler do original job


recent_lba_check:	; check EAX / DL whether this place has been
			; tickled recently
	push ax
	push ebx

	mov bl,dl	; drive number, use only 3 low bits
	and bl,7
	xor al,bl	; xor it into the sector number
	mov ebx,eax	; fold down EAX into 8 bit "hash"
	shr ebx,16	; ax = eax_hi
	xor bx,ax	; hi16 xor lo16 -> 16bit intermediate value
	xor bl,bh	; lo8 xor hi8 of 16bit -> 8 bit value
	rol bl,1
	and bx,63			; 64 table entries!

	cmp [cs:hdd_hist+ebx*4],eax	; recently used EAX?
	stc				; ~flag: not recently tickled
	jnz rlbac1			; if not, suggest tickling
	clc				; ~flag: recently tickled!
rlbac1:	cmc				; turn ~flag into flag
	mov [cs:hdd_hist+ebx*4],eax	; update table
	pop ebx
	pop ax
	ret
%define RECHECK_CHS	; allocate hdd_hist buffer
%endif

; ====================================

%ifdef RECHECK_CHS	; if hdd_hist buffer is used
	align 4
hdd_hist:
	times 64 dd 0	; "history hash", 64 entries, 32bit each
			; each can be either for CHS or for LBA.
			; Mixing both formats is pretty harmless here!
%endif

%ifdef CHShdTICKLE
hdtickleflag	db 0	; 0x80 to enable CHS harddisk tickling
%endif

%ifdef LBAhdTICKLE
lbatickleflag	db 0	; 0x80 to enable LBA harddisk tickling
whichdrive	db 0	; temp. variable used during drive detection
;
disksizes	dd 0,0,0,0,0,0,0,0	; disk sizes in sectors
;
diskaddrpacket	dw 10h	; size
lbacnt		dw 8		; sector COUNT
lbabufp		dw ticklebuf	; buffer pointer offset
lbabufpseg	dw 0		; buffer pointer segment
lbapos		dd 0,0		; which sector
%endif


	align 16
ticklebuf	; then data buffer for tickling,
		; BUFSECT * 512 bytes (e.g. 18 * 512)

; ====================================

install:
	push bx			; (make SP wrap 0x10000 -> 0xfffe)
	mov ax,sp		; .com file: SP tells heap size
				; (if > 64k heap, AX is 0xfffe now)
	pop bx			; (SP returns to normal value)
	mov bx,[cs:ticklebufp]	; track buffer
	add bx, (BUFSECT * 512)	; "heap" usage
	cmp ax,bx		; enough heap free?
	ja enoughram		; if yes, continue
	mov dx,lowram		; tell that we have too little heap
	jmp errexit		; exit with error message

enoughram:
	push es
	mov es,[cs:2ch]		; environment segment
	mov ah,49h		; release memory
	int 21h			; DOS call ...
	pop es

cpucheck:
%imacro	FLAGCHECK 0
	pushf		; save
	push ax		; test value
	popf		; try to set flags
	pushf		; check what happened
	pop ax		; test results
	popf		; restore
%endmacro

	xor ax,ax	; try to zero all flags
	mov bx,0f000h	; bit mask for high bits
	FLAGCHECK
	and ax,bx	; high bits...
	cmp ax,bx	; ...stuck to 1? 80186 or older!
	jz oldcpu
			; next assumption: 286
	mov ax,bx	; try to set all high bits
	FLAGCHECK
	test ax,bx	; all stuck to 0? 80286!
	jnz goodcpu	; ... else it is a 386 or newer CPU

oldcpu:	mov dx,need386
	jmp errexit

goodcpu:

	mov si,81h
	cld
parseon0:
	lodsb
	cmp al,' '
	jb parsed0
	cmp al,'?'	; help requested?
	jnz parseon0
	mov dx,helpmsg
	mov ah,9
	int 21h		; show help screen
	mov ax,4c00h
	int 21h		; exit, no error
parsed0:

searchmemdisk:
	mov cx,4
	mov dl,0	; A:
smdloop:
	call memdiskcheck
	jz nmemdiskfound
	mov al,[blockedflop]
	cmp al,0ffh	; already blocked one?
	jz ymemdiskfound
	mov dx,dualmemdisk
	jmp errexit	; cannot handle more than one virtual floppy

ymemdiskfound:
	mov [blockedflop],dl	; store drive number
	push dx
	add dl,'A'
	mov [memdiskletter],dl
	mov dx,memdiskmsg	; tell about memdisk
	mov ah,9
	int 21h			; show memdisk detection info
	pop dx
nmemdiskfound:
	inc dx
	loop smdloop

searchtickle:
	mov eax,'tick'
	mov bx,'le'
	push es
	int 13h			; TICKLE install check, returns pointer ES:BX
	pop es			; (to tickle buffer, defined to be 8k or more)
	cmp eax,'haha'
	jnz nticklefound
	mov dx,alreadyloaded
	jmp errexit	; cannot load twice

nticklefound:


%ifdef CHShdTICKLE
	mov si,81h
	cld
parseon:
	lodsw
	dec si
	cmp al,' '
	jb parsed
	and ax,0xdfdf	; lower case -> upper case
	cmp ax,'HD'
	jz chstickleon
	cmp ax,'CH'	; alternative syntax
	jnz parseon
chstickleon:
	mov byte [ds:hdtickleflag],0x80
	mov dx,hdwarning
	mov ah,9
	int 21h		; show warning: Harddisk CHS tickle enabled!
parsed:
%endif


%ifdef LBAhdTICKLE
	mov si,81h
	cld
parseon2:
	lodsw
	dec si
	cmp al,' '
	jnb nparsed2
	jmp parsed2
nparsed2:
	and ax,0xdfdf	; lower case -> upper case
	cmp ax,'LB'
	jnz parseon2
	mov byte [ds:lbatickleflag],0x80
	; mov [ds:lbabufpseg],cs	; set buffer segment
	; (now done in resident code whenever int 13.4x is called)
	mov dx,lbawarning
	mov ah,9
	int 21h		; show warning: Harddisk LBA tickle enabled!

	mov dl,80h-1	; first drive - 1
	mov [ds:whichdrive],dl
nextlbacheck:
	mov dl,[ds:whichdrive]
	inc dl		; next drive
	cmp dl,88h	; all drives done?
	jnz nextlbc
	jmp alllbachecked
nextlbc:
	mov [ds:whichdrive],dl

	mov ah,41h	; LBA install check for drive DL
	mov bx,55aah
	int 13h		; LBA/extensions supported for this drive?
	jc nolbabios
	cmp bx,0aa55h
	jnz nolbabios
	test cx,1		; normal LBA supported? (func. 42-44h,47h,48h)
	jz nolbabios
	mov dl,[ds:whichdrive]
	mov ah,48h		; get drive parameters to DS:SI
	mov si,driveparambuf
	mov word [ds:si],1ah	; tell size of buffer, 1.x version is enough.
	mov word [ds:si+2],0	; Phoenix/Dell bug workaround
	int 13h			; query drive parameters
	jc nolbabios
	; buffer contents: dw size, dw flags, dd CYL, dd HEAD, dd SEC/TRACK
	; ... and at offset 10h: dq SIZE. Finally, dw bytes per sector.
	cmp word [driveparambuf+18h],200h
	jnz nolbabios			; not 512 bytes per sector
	mov eax,[driveparambuf+14h]	; 386+
	or eax,eax			; 386+
	jnz nolbabios			; ignore drives of more than 2 TB
	mov eax,[driveparambuf+10h]	; 386+
	sub dl,80h			; subtract list offset
	movzx ebx,dl			; current drive (386+)
	mov [ds:disksizes+4*ebx],eax	; store size (386+)
nolbabios:
	jmp nextlbacheck

alllbachecked:
parsed2:
%endif


dmacheck:

	mov dl,80h
	mov bx,ticklebuf	; normal buffer location
%ifdef LBAhdTICKLE
	test dl,[cs:lbatickleflag]
	jnz biggertsr
%endif
%ifdef CHShdTICKLE
	test dl,[cs:hdtickleflag]
	jnz biggertsr
%endif
%ifdef LBAhdTICKLE CHShdTICKLE	; if either of them compiled in
smallertsr:			; no harddisk tickling active
	mov bx,endpoint + 15	; can move buffer to earlier place
	and bx,0fff0h		; align 16
	mov byte [cs:patchplace],0ebh	; replace "jmp if no harddisk"
				; by "jmp always" (disable harddisk tickle)
%endif
biggertsr:
	mov [cs:ticklebufp],bx	; store selected buffer location

real_dmacheck:
	mov cl,4
	mov ax,cs
	shl ax,cl
	add ax,bx	; buffer pointer
	push ax		; 16 lsb of linear address of buffer start
	mov ax,cs
	shl ax,cl
	add ax,bx	; buffer pointer
	add ax, (BUFSECT * 512)	; end of track buffer
	; 16 lsb of linear address of buffer end
	pop cx		; (pushed as ax above)
	sub ax,cx	; end-start
	test ax,ax
	js wrongpoint	; end-start negative? bad sign...

	mov dx,normalmsg
	mov ah,9	; show string
	int 21h
	;
	mov al,0		; size
	mov ah,13		; eof char (CR)
	mov [cs:0x80],ax	; zapp command line arguments
	mov bx,ticklelist	; zapp list of tickled tracks
	mov al,0
ticklelistinit:
	mov [cs:bx],al
	inc bx
	cmp bx,ticklelist+FDTRACKS
	jb ticklelistinit
	;
	mov ax,3513h	; get old int 13 vector
	int 21h
	mov [cs:i13old],bx
	mov [cs:i13old+2],es
	;
	mov ax,2513h	; set new int 13 vector
	mov dx,cs
	mov ds,dx
	mov dx,i13new	; new handler
	int 21h
	;
	mov ax,cs
	mov ds,ax
	mov es,ax
	jmp instdone	; SUCCESS - we can activate the TSR

wrongpoint:
	mov dx,wrongmsg

errexit:
	mov ah,9	; show string
	int 21h
	mov dx,abortmsg
	mov ah,9	; show string
	int 21h
	mov ax,4c01h	; end with error
	int 21h

; ====================================

memdiskcheck:			; test if drive DL is a memdisk
	pushad
	push es
	mov eax,454d0800h	; magic1 + AH=8 (get geometry)
	mov ecx,444d0000h	; magic2
	push dx
	mov edx,53490000h	; magic3 +
	pop dx			; ... drive number in DL
	mov ebx,3f4b0000h	; magic4
	int 13h			; BIOS DISK API
	shr eax,16		; ignore AX
	shr ebx,16		; ignore BX
	shr ecx,16		; ignore CX (geometry C/S)
	shr edx,16		; ignore DX (geometry H in DH)
	xor bp,bp		; no memdisk
	cmp ax,4d21h		; magic5
	jnz nomemdisk
	cmp cx,4d45h		; magic6
	jnz nomemdisk
	cmp dx,4944h		; magic7
	jnz nomemdisk
	cmp bx,4b53h		; magic8
	jnz nomemdisk
	inc bp			; memdisk found

	; ES:DI now points to a data structure:
	; dw bytes, dw version, dd disk address in RAM,
	; dd size in sectors, dd pointer to commandline,
	; dd old int 13h vector, dd old int 15h vector
	; dw old [40h:13h] low memory size value

	; We cannot prevent people from uninstalling MEMDISK later,
	; but we could block the MEMDISK install check to prevent
	; detection / removal of MEMDISK by not-yet-loaded programs.

nomemdisk:
	or bp,bp		; memdisk found? then return NZ
	pop es
	popad
	ret

; ====================================

helpmsg	db "TICKLE read-ahead for LBAcache by Eric Auer 2003-2006",13,10
	db "By eric[at]coli.uni-sb.de, License GPL (www.gnu.org)",13,10
	db "Version: 31aug2006, options: /? shows this help",13,10
%ifdef CHShdTICKLE
	db "         /CHS old harddisk read-ahead on",13,10
%endif
%ifdef LBAhdTICKLE
	db "         /LBA new harddisk read-ahead on",13,10
%endif
	db "Floppy read-ahead is always on",13,10
	db "Load LBACACHE with FLOP option enabled first!",13,10
	db "Do not LOADHIGH TICKLE, risk of data loss / DMA problems!",13,10
	db "$"

normalmsg	db "Read-ahead TICKLEr TSR loaded, 9k buffer.",13,10
		db "$"

memdiskmsg	db "No read-ahead for virtual MEMDISK floppy "
memdiskletter	db "A:",13,10,"$"
		; *** memdisks have wrong DDPT, by the way! ***

wrongmsg	db "TICKLEr TSR 9k buffer crosses 64k DMA boundary,",13,10
		db "please load to another point in RAM.",13,10,"$"
lowram		db "Out of memory - try a bigger UMB or load low",13,10,"$"
need386		db "TICKLE can only be used on 386 or newer CPUs",13,10,"$"
dualmemdisk	db "Second virtual MEMDISK floppy found??",13,10,"$"
alreadyloaded	db "TICKLE already resident in RAM",13,10,"$"

abortmsg	db "Not going TSR - aborting.",13,10,"$"
%ifdef CHShdTICKLE
hdwarning	db "/CHS old harddisk read-ahead enabled.",13,10,"$"
%endif
%ifdef LBAhdTICKLE
lbawarning	db "/LBA new harddisk read-ahead enabled.",13,10,"$"
driveparambuf:	; we put drive parameter data here (during init only)
%endif

