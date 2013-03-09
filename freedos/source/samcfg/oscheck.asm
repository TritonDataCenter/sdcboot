; An improved tool to read the boot sector of ?: for all FAT
; filesystems and store it into a file with fixed bootsect.bin
; name. Displays status and returns very informative errorlevels
; as it can analyze boot sector contents and root directory
; contents to predict if the disk is bootable and which OS is
; installed. Public Domain, based on the SAVEBS (8/2003) Public
; Domain tool. This is OSCHECK, written by Eric Auer 7/2004.

; Make OSCHECK binary: Make: nasm -o oscheck.com oscheck.asm

; Update 24.7.2004: disk access flag and better messages for
; non-FAT (e.g. not formatted) disks and boot sectors which do
; no look like loading any kernel.
; Update 19.8.2004: SYSLINUX detection (LDLINUX.SYS string).
; Update 12.1.2005: save to file only if /S option given.

%define BIGMSG 0

	org 0x100	; a COM program

start:	mov bx,0x81	; command line
scan:	mov ax,[bx]
	inc bx		; scan on
	cmp al,13	; EOF?
	jz scanned2
	cmp al,' '	; skippable?
	jz scan
	cmp al,9	; skippable?
	jz scan
	cmp al,'/'	; skippable?
	jz scan
	cmp al,'?'	; help wanted?
	jz help
	cmp ah,13	; EOF next?

scanned2:
;	jz scanned
	jz help		; if no drive letter given, show help message

	and al,0xdf	; upcase
	cmp ah,':'	; X: ?
	jnz scan2	; else scan on, unless /S
	sub al,'A'	; convert to drive number
	cmp al,'Z'-'A'
	jna gotdrive	; show help if invalid drive letter,
			; otherwise stop analyzing command line now.

help:	mov dx,helpMsg
	call SHOWSTRING
	mov ax,0x4cff	; leave, errorlevel -1
	int 21h

scan2:	cmp al,'S'		; option /S? (must come BEFORE drive spec!)
	jnz help		; otherwise: illegal option
	mov byte [cs:savej+1], 0	; zap the jmp offset, so the
	jmp short scan			; save step is no longer skipped
				; ... then scan on ...

gotdrive:
	mov [drive],al	; store new drive selection

scanned:
	mov al,[drive]	; 0 based
	add al,'A'
	mov [drvletter],al

	mov ax,0x3000	; DOS version
	int 21h
%if 1
	cmp al,4	; DOS 4.x+?
	jb noaccessflag
%endif
	cmp al,7	; DOS 7.x+?
	jb nolock
	mov ax,0x440d	; IOCTL
	mov cx,0x084a	; LOCK
	mov bl,[drive]	; NOT 0 based but...
	inc bx		; 1 based
	mov bh,0	; lock level (full)
	int 21h
nolock:

%if 1
accessflag:
	; DOS 4+ int 21.440d.bl=drive+1.cx=0867/4867.dsdx->structure
	; get access flag. Structure: 2 bytes, 0, flag. special: 0.
	; if CY then error AL ... error 1/16 is "not so serious".
	; if no error, and current flag not already on, set it:
	; int 21.440d.bl=drive+1.cx=0847/4847.dsdx->structure
	; with structure: flag++, special 0. Same error checking.
	; Structure: db func, flag (allows access to unformatted)
	mov ax,440dh
	mov bl,[drive]
	inc bx
	mov cx,0847h		; FAT16 variant, just SET it. Too
	mov dx,access		; lazy / tight on space to GET it 1st.
	int 21h
; ---	ret			; We are evil and do not even RESET
; ---				; the flag later.
noaccessflag:
%endif


	mov ah,0x0d	; reset drives
	int 21h
	mov ah,0x32	; reset that drive
	push ds		; DS/BX changed by the call
	mov dl,[drive]	; NOT 0 based but...
	inc dx		; 1 based
	int 21h
	pop ds		; restore DS

	mov dx,messageSmall
	call SHOWSTRING
	mov al,[drive]	; 0 based
	mov cx,1	; 1 sector
	mov dx,0	; 1st sector
	mov bx,buffer	; buffer (in DS)
	int 25h		; DOS read sector (call far)
			; all data registers may be changed now!
	jc i25err
	xor ax,ax	; no error
i25err:			; status AH: 80 timeout 8 dmafail 2/4 sec not found
	popf		; as int 25 is actually a call far
	cmp ax,207h	; > 64k sectors?
	jz bigdisk
	mov al,ah	; remember status
	or al,al	; any problem?
	jz savebuf	; else save buffer
	or al,0x80
	mov dx,diskMsg
	jmp quitMSG	; abort

bigdisk:		; other call style for > 64k sector drives
%if BIGMSG
	mov dx,messageBigger
	call SHOWSTRING
%endif
	mov al,[drive]	; 0 based
	mov cx,-1	; special flag
	mov bx,bigparams
	mov [bufseg],ds
	int 25h		; DOS read sector (call far)
			; all data registers may be changed now!
	jc i25err2
	xor ax,ax	; no error
i25err2:		; status AH: 80 timeout 8 dmafail 2/4 sec not found
	popf		; as int 25 is actually a call far
	cmp ax,207h	; FAT32 drive?
	jz biggerdisk
	mov al,ah	; remember status
	or al,al	; any problem?
	jz savebuf	; else okay
	or al,0x80
	mov dx,diskMsg
	jmp quitMSG

biggerdisk:
	mov dx,messageFAT32
	call SHOWSTRING
	mov ax,7305h	; FAT32 disk access
	mov cx,-1
	mov dl,[drive]	; NOT 0 based but...
	inc dx		; ...1 based
	mov si,0	; flags: LSB=1 means write, 0 read, etc.
	mov bx,bigparams
	int 21h
	jnc savebuf

	mov dx,fat32errMsg
	or al,0x80
	jmp short quitMSG

savebuf:
	call ANALYSIS
savej:	jmp short notsaving	; will be patched away if /S is given

saving:	mov dx,savingMsg
	call SHOWSTRING
	mov ah,0x3c	; CREATE (or overwrite) file
	mov dx,filename
	mov cx,0x20	; attributes: only "archive" set
	int 0x21
	jnc noquit

quitF:	mov dx,fileMsg	; file access error
	or al,0x40
	jmp quitMSG

noquit:	mov bx,ax	; remember file handle
	mov ah,0x40	; write handle BX
	mov cx,0x200	; size 1 sector
	mov dx,buffer	; buffer (in DS)
	int 0x21
	jc quitF
	mov ah,0x3e	; close handle BX
	int 0x21
	jc quitF

notsaving:
	mov dx,doneMsg
	call SHOWSTRING
	jmp short quitgood

quitMSG:		; show error description, then generic error message
	push ax
	push ds
	mov ax,cs
	mov ds,ax
	call SHOWSTRING
	pop ds
	pop ax

quit:	push ax		; quit with generic error message
	mov dx,errorMsg
	call SHOWSTRING
	pop ax
	jmp short quitbad	; error code is errorlevel

quitgood:
	mov al,[status]	; ANALYSIS result is errorlevel
quitbad:
	push ax
	mov ax,0x3000	; DOS version
	int 21h
	cmp al,7	; DOS 7.x+?
	jb nounlock
	mov ax,0x440d	; IOCTL
	mov cx,0x086a	; UNLOCK
	mov bl,[drive]	; NOT 0 based but...
	inc bx		; 1 based
	mov bh,0	; lock level (full)
	int 21h
nounlock:
	pop ax

	mov ah,0x4c
	; exit with error code AL
	int 0x21



noboot:	mov [status],al	; belongs to ANALYSIS below
SHOWSTRING:		; called from EVERYWHERE around here!
	mov ah,9
	int 21h
	ret



ANALYSIS:		; analyze buffer, return result in status byte
	mov dx,noBOOTmsg
	mov al,1			; prepare for "no BOOT"
	cmp word [buffer+0x1fe],0xaa55	; signature
	jnz noboot
	mov dx,noFATmsg
	mov al,2			; prepare for "no DOS"
	cmp word [buffer+0x0b],512	; sector size
	jnz noboot
	cld
	mov si,buffer+3			; OEM ID string
	mov di,checkbuf
	mov cx,8
	rep movsb
	mov ax,0x0a0d	; CRLF
	stosw
	mov al,"$"
	stosb
	mov dx,oemMsg
	call SHOWSTRING
	mov byte [status],3		; prepare for "any DOS"

	mov si,kernellist		; list of kernels
	xor bp,bp			; index
namefind:
	mov al,[si]
	or al,al			; end of list?
	jnz checkone
	mov dx,noLOADmsg		; no known kernel string
	call SHOWSTRING			; probably dummy boot sector
					; "this is no bootable disk"
	ret				; then we are done

checkone:
	mov di,buffer+0x40		; do not scan first part
checkon:
	mov cx,11			; name length
	push si
	push di
	repz cmpsb			; string compare
	pop di
	pop si
	or cx,cx
	jz foundone
	inc di				; move search window
	cmp di,buffer+0x1fe-11		; last useful position
	jbe checkon
	inc bp				; count
	add si,12			; try next string
	jmp short namefind

foundone:
	lea ax,[bp+0x10]		; "found kernel name N"
	mov [status],al

	mov dx,kernelMsg
	call SHOWSTRING
	mov dx,si
	call SHOWSTRING
	mov dx,crlfMsg
	call SHOWSTRING

	mov al,[drive]
	add al,'A'
	mov di,checkbuf
	stosb
	mov ax,':\'
	stosw

	mov cx,8		; copy first part, w/o trailing spaces
cpnam1: lodsb
	cmp al,' '
	jz skipn1
	stosb
skipn1:	loop cpnam1

	cmp byte [si],' '	; extension present?
	jz skipex		; else no dot
	mov al,'.'		; do not forget the DOT...
	stosb

	movsb			; 1st char of extension
	lodsb			; 2nd
	cmp al,' '
	jz skipex
	stosb
	lodsb			; 3rd
	cmp al,' '
	jz skipex
	stosb

skipex:	mov al,'$'		; end marker for string

	push di
	stosb
	mov dx,checkMsg
	call SHOWSTRING
	mov dx,checkbuf
	call SHOWSTRING
	mov dx,crlfMsg
	call SHOWSTRING
	pop di

	mov al,0		; end marker for findfirst
	stosb

	mov ax,0x4e00		; findfirst
	mov cx,0x87		; allowed attrib: shareable, (~x), (~arch),
				; ~dir, ~label, system, hidden (readonly)
	mov dx,checkbuf		; filename
	int 21h
	jc nosuchfile

	mov al,[status]
	add al,0x10		; found kernel name in boot sector AND
	mov [status],al		; a corresponding file on disk!

	mov dx,yeskernelMsg
	call SHOWSTRING
	ret

nosuchfile:
	mov dx,nokernelMsg
	call SHOWSTRING
	ret



drive		db 2		; C: (0 based)
status		db 0		; 1 no 55aa, 2 no DOS
access		db 0,1		; disk access structure: 0, flag.

nokernelMsg	db "No such kernel file.",13,10,"$"
yeskernelMsg	db "Found kernel.",13,10,"$"
kernelMsg	db "String: $"
checkMsg	db "Checking:",13,10,"$"
oemMsg		db "OEM ID: "	; continues on next line
checkbuf	db "x:\????????.***",0
filename	db "bootsect.bin",0

bigparams	dd 0		; sector number: 1st sector
		dw 1		; read 1 sector
		dw buffer	; buffer offset
bufseg		dw 0		; buffer segment

messageSmall	db "Trying to read boot sector of "
drvletter	db "C:",13,10,"$"

%if BIGMSG
messageBigger	db "Using 32+ MB style access...",13,10,"$"
		; no "reliable" message, because bigger
		; FAT16 drives *can* be accessible through
		; the interface for small drives as well.
%endif

messageFAT32	db "FAT32!?",13,10,"$"
savingMsg	db "Saving boot sector to bootsect.bin ...",13,10,"$"
doneMsg		db "done!"	; continued in next line
crlfMsg		db 13,10,"$"

noBOOTmsg	db "INVALID boot sector!",13,10,"$"
noFATmsg	db "No FAT disk!?",13,10,"$"
noLOADmsg	db "No boot sector?",13,10,"$"

diskMsg		db "Disk read failed.",13,10,"$"
errorMsg	db "Check errorlevel.",13,10,"$"
	; 0 "okay", 0x40.. "file error", 0x80.. "disk error"
fileMsg		db "File (over)write error.",13,10,"$"
fat32errMsg	db "FAT32 disk access error.",13,10,"$"

kernellist:
fdoskern	db "KERNEL  SYS$"
		; years ago that was ipl.sys and kernel.exe
metakern	db "METAKERNSYS$"

msdoskern1	db "IO      SYS$"
msdoskern2	db "MSDOS   SYS$"	; second stage, not in Win9x
win9xkern	db "WINBOOT SYS$"
winntkern	db "NTLDR      $"

pcdoskern1	db "IBMBIO  COM$"
pcdoskern2	db "IBMDOS  COM$"	; also for DRDOS/OpenDOS

syslinuxkern	db "LDLINUX SYS$"	; SYSLINUX boot loader
endoflist	db 0
	; TODO: find all "*.SYS" and "*.COM" variants, too

	; PS: DPMI/DOS extender of Win3.x/3: win386.exe Win9x: vmm32.sys
	; ... and krnl386.exe, with special handshake with win.com ...
	; ... then user.exe gdi.exe system/network/display/keyboard.drv
	; Win32: kernel32/user32/gdi32/advapi32.dll wrappers around Win16
	; ... shell: progman (Win3.xx) / explorer (Win9x)

	align 16
buffer:		; buffer is after "all" messages

helpMsg	db "Free OSCHECK by Eric Auer 2004 - Usage: OSCHECK [/S] X:",13,10
	db "/S write boot sector of X: to bootsect.bin",13,10
	db 13,10
	db "Errorlevel depends on X: analysis:",13,10
	db "0 aborted, 1 not bootable, 2 not DOS, 3 unknown DOS",13,10
	db "16+N boot sector N found,",13,10
	db "32+N boot sector N and matching kernel found",13,10
	db "64+  could not save boot sector to file,",13,10
	db "128+ could not read boot sector,",13,10,13,10
	db "Values for N:",13,10
	db "0 FreeDOS, 1 MetaKern, 2/3 MS-DOS/Win9x/WinNT,",13,10
	db "4 Win9x, 5 WinNT, 6/7 PC-DOS/DR-DOS, 8 SYSLINUX",13,10
	db "$"	; errorlevel 255: help displayed

