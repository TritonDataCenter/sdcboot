; Public domain FAT32 kernel support detection and per-drive
; FAT (12-32) type detection, < 512 bytes, by Eric Auer 2003.
; To assemble: nasm -o whichfat.com whichfat.asm
;
; Usage to detect kernel support: WHICHFAT
; result is returned in errorlevel:
; 0 has FAT32 kernel / 1 has no FAT32 kernel
;
; Usage to detect FAT type of a drive letter: WHICHFAT x:
; result is returned in errorlevel:
; 0 could not be detected / 1 invalid drive letter
; 12 drive is FAT12 / 16 drive is FAT16 / 32 drive is FAT32

	org 100h

	; *** entry point ***
start:
	; *** lastdrive detection ***
	mov ah,19h	; get current drive
	int 21h
	mov [curdrv], al
	mov ah,0eh	; set drive
	mov dl,al	; which drive, 0 based
	int 21h
	mov [drvcnt], al	; lastdrive: 5 = last letter E etc.

	; *** kernel FAT32 detection ***
	mov ax,7300h	; get some FAT32 flag
	; FreeDOS 2032a actually does not support THIS FAT32
	; function yet, but it returns "invalid function" if
	; FAT32 is supported at all and ax=7300 otherwise.
	mov dl,0	; current drive (can be any FATxx drive)
	mov cl,1	; dirty buffers flag
	int 21h
	cmp ax,7300h	; simply unimplemented? (no carry set then!)
	jz fat16kernel	; other error code (AL) would mean FAT32 there
	mov byte [ktype], 32	; kernel can do FAT32
	mov word [kernelmsgnum],"32"	; change message, too.
fat16kernel:

	; *** command line parsing ***
	cld
	xor cx,cx
	mov si,80h	; command line
	lodsb
	mov cl,al	; length of command line
	mov al,ch
cmdlp:	mov ah,al	; previous char
	lodsb
	cmp al,9
	jz cmdlp
	cmp al,' '
	jz cmdlp
	jb xcore	; control char? then end of line. Return
			; kernel abilities if no drive letter yet.
	cmp al,'a'	; lower case?
	jb caseok
	sub al,'a'-'A'	; else fix
caseok:	cmp al,':'	; just saw a drive letter?
	jz dparse
	cmp al,'?'	; help request?
	jnz cmdlp	; else ignore
	jmp helpme	; show help message

	; *** check a certain drive ***
dparse:	mov al,ah	; look back into drive letter
	sub al,'A'	; turn into 0 based drive number
	cmp al,drvcnt
	jae nodrv	; cannot be a valid drive!
	mov [chkdrv],al	; store for later checks
	inc al		; change to 0=default, 1=a: ... numbering!
	mov ah,36h	; check free space (similar to int 21.1c)
	mov dl,al	; check free space on that drive
	int 21h
	cmp ax,-1	; AX=SecPerClust BX=FreeClust CX=ByPerSec
			; DX=ClustOnDrive (max 2GB-32k reported!)
	jz nodrvx	; "invalid drive" (RBIL) or only non-FAT?
			; could do CD-ROM check: device / 21.4402
			; or int 2f.1510.drive(0based)CX.es:bx request
	cmp dx,4085	; FAT12: must be < 4085 (<= ???) clusters
	jb fat12	; (FAT16 would be < 65525)
	; cmp dx,65525	; ... if above, FAT32 ...
	cmp byte [ktype],32	; FAT32 enabled kernel?
	jnz fat16	; else it must be FAT16, no further checks.
	mov al,[ds:chkdrv]
	add al,'A'
	mov [drvnam],al	; create "A:\" type string
	mov ax,7303h	; FAT32: get extended free space
	mov cx,30h	; size of buffer
	mov di,spcbuf	; pointer to buffer in ES
	mov dx,drvnam	; pointer to local/net drive spec string, DS!
	int 21h		; returns CY=0, AL=0 if no FAT32 support
	jc nodrv	; did not work. Strange.
	mov dx,[di+22h]	; high word of unadjusted total cluster count
	mov ax,[di+20h]	; low  word of unadjusted total cluster count
	or dx,dx
	jnz fat32	; > 64k? Must be FAT32.
	cmp ax,65525	; > FAT16 limits? Must be FAT32.
	jae fat32
	jmp short fat16	; else it is FAT16.

	; *** drive might be CD-ROM (only doing *CDEX 2.0+ check) ***
nodrvx:	mov ax,150bh	; CD-ROM 2.0: drive check
	xor cx,cx
	mov cl,chkdrv	; which drive? 0 for A: etc.
	int 2fh		; multiplexer: *CDEX in this case.
	cmp ax,0adadh	; any CDEX there?
	jnz nodrv
	or ax,ax
	jz nodrv	; AX is 0 if no CD-ROM drive
	mov dx,cdromdrive
	mov al,0	; return "not FAT"
	jmp short dmsg
	
	; *** kernel abilities returned ***
xcore:	mov dx,kernelmsg
	mov al,0	; 0 for FAT32
	cmp byte [ktype], 32
	jz dmsg		; it is true: FAT32
	mov al,1	; 1 for at most FAT16
	jmp short dmsg

	; *** drive letter was out of range ***
nodrv:	mov dx,nosuchdrive
	mov al,1	; return "no drive"
	jmp short dmsg

	; *** return FAT type of this drive ***
fat12:	; mov dx,"12"	; (default)
	mov al,12
	jmp short fatyy	; (not fatxx...)
fat16:	mov dx,"16"
	mov al,16
	jmp short fatxx
fat32:	mov dx,"32"
	mov al,32
fatxx:	mov [fatdrivenum],dx
fatyy:	mov dx,fatdrivemsg
	jmp short dmsg

	; *** help screen ***
helpme:	mov dx,helpmsg
	mov al,2
	jmp short dmsg

	; *** common exit stuff ***
dmsg:	push ax
	mov ah,9	; show string$
	int 21h
	pop ax
done:	mov ah,4ch	; exit with errorlevel AL
	int 21h

	; *** variables ***
ktype	db 16		; maximum supported FAT type for this kernel
curdrv	db 0		; active drive number when called
chkdrv	db 0		; which drive to check, 0=a: etc.
drvcnt	db 1		; number of last possible drive letter
			; (may have invalid ones in between!)
drvnam	db "C:\",0	; for int 21.7303

	; *** messages ***
nosuchdrive	db "No such local drive!",13,10,"$"
cdromdrive	db "CD-ROM or DVD drive!",13,10,"$"
fatdrivemsg	db "Drive is FAT"
fatdrivenum	db "12.",13,10,"$"	; patched as needed
kernelmsg	db "Kernel limit: FAT"
kernelmsgnum	db "16.",13,10,"$"	; patched to 32 dynamically

helpmsg	db "WHICHFAT [x:]",13,10
	db "Returns: generic:",13,10
	db 9,"0=FAT32 kernel, 1=FAT16 kernel 2=/? found."
	db 13,10
	db "For 1 drive:",13,10
	db 9,"0 not FAT, 1 no local drive, ",13,10
	db 9,"12 FAT12, 16 FAT16, 32 FAT32.",13,10
	db "$"

spcbuf	dw 0	; on return: size of returned structure
	dw 0	; request for extended free space data version 0
	; following at offsets 4, 8, ...:
	; [04h] SecPerClust, [08h] ByPerSec,    [0ch] FreeClust
	; [10h] TotalClust,  [14h] FreeSect*,   [18h] TotalSect*
	; [1ch] FreeClust*,  [20h] TotalClust*, [24h] reserved
	; [28h] reserved. (* is "without adjustment for compression")

