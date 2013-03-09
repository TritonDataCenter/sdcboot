	; Public domain disk finder tool, for example to do:
	;     FINDDISK +TURBODSK
	; to change to the first tdsk drive in C:-Z: range.
	; See the help message for more information, or just
	; try the tool :-). Compiler: nasm.sourceforge.net,
	; To compile, do: nasm -o finddisk.com finddisk.asm

%define SAFEMODE 1	; set to 1 to suppress critical errors
	; To trap critical error: e.g. install int 24 handler which
	; returns AL=0/1/2/3 for ignore/retry/abort-int23/fail.
	; Luckily, Int 22-24 are restored from PSP at program exit.

%define BUGGY_FCB 1	; set to 1 if the kernel ignores the search
	; pattern for LABEL searches, seems to be a common problem?
	; This feature eats ca. 20 of our precious disk space, grrr.

%define UPCASE 0	; set to 1 to force the search pattern to
	; upper case. Better for lazy users, but will spoil the
	; processing of WinXP labels which can be mixed case. If
	; you have UPCASE set to 1 and want to search "WinXP-C",
	; you would have to use the search pattern "W??XP-C".

	org 100h
start:
%if SAFEMODE
	mov ax,2524h	; install int 24 critical error handler
	mov dx,i24handler
	int 21h
%endif
	mov si,81h	; start of command line
cline:	mov ax,[si]
	inc si
	cmp al,9	; ignore tab
	jz cline
	cmp al,' '	; ignore spaces
	jz cline
	jb help		; end of commandline reached but no label yet
	cmp al,'+'	; jump-there flag
	jnz noplus
	mov byte [change],1
	jmp short cline
noplus:	cmp ah,':'	; drive spec?
	jnz nodrv
	and al,0dfh	; crude upcase
	sub al,'A'
	jc help		; invalid drive
	cmp al,'Z'-'A'
	ja help		; invalid drive
	inc ax		; (AL is part of AX)
	mov [drive],al	; 1-based
	mov byte [scandrv],0	; disable scan mode
	inc si		; skip the ":"
	jmp short cline
nodrv:	cmp al,'/'	; user tried to do /? or similar
	jnz main	; else assume that the label starts here


help:	mov ah,9	; show string
	mov dx,helpmsg
	int 21h
	mov ax,4c00h	; return with errorlevel 0
	int 21h


%if SAFEMODE
i24handler:
	mov al,3	; "fail" - make the call which triggered a
	iret		; critical error fail, without user prompt.
%endif


main:	mov di,what	; we reached the first place which is neither
			; space nor + nor X: - the label!
clabel: cmp al,'*'	; big wildcard?
	jz fill		; then pad search pattern with ?s
%if UPCASE
	cmp al,'a'	; gotta upcase first?
	jb okchar	; else keep copying
	cmp al,'z'	; only upcase a-z, of course
	ja okchar
	sub al,'a'-'A'	; make it upper case
%endif
okchar:	stosb		; store char to [di], inc di
	lodsb		; fetch next char from [si], inc si
	cmp al,' '	; end of line?
	jb scan		; then use search pattern :-)
	cmp di,what+11	; label about to get too long?
	jb clabel	; else keep copying
	jmp short help	; too long label is an error

fill:	mov al,'?'
	stosb		; store char to [di], inc di
	cmp di,what+11	; already done?
	jb fill		; else keep filling


	; all variables are set now, time to start scanning.
scan:		mov ah,11h	; FCB "find"
	mov dx,fcb	; our search structure
	int 21h
	or al,al	; found any match?
	jz found	; then we are done!
baddrv:	cmp byte [scandrv],0	; scan mode?
	jz fail		; else we are done
	inc byte [drive]
	cmp byte [drive],26+1	; Z: reached?
	jbe scan	; else keep scanning

fail:	mov ah,9	; show string: no drive with such label found
	mov dx,nomsg
	int 21h
	mov ax,4c00h	; nothing found, exit with errorlevel 0
	int 21h

found:
%if BUGGY_FCB
	mov di,80h+what-fcb	; result FCB of the search
	mov si,what		; pattern
	mov cx,11		; size of both patterns
fcheck:	lodsb			; found /
	mov ah,[di]		; / wanted
	inc di
	cmp al,'?'		; wildcard always matches
	jz fokay
	cmp al,ah		; otherwise, really check
	jnz baddrv		; not a real match :-(
fokay:	loop fcheck
%endif
	; we could now do int 21.2f and read the structure at ES:BX
	; to display the found label: The structure either starts
	; with only a drive number byte, 1 based, or with a -1 byte
	; followed by 5 reserved bytes, an attribute byte and then the
	; drive number byte. After that, 8+3 chars filename follow...

	mov al,[drive]
	push ax		; save for later
	add al,'A'-1	; convert 1 based drive to "A" based letter
	mov [letter],al	; plug into message
	mov ah,9	; show string
	mov dx,okmsg
	int 21h
	cmp byte [change],0	; should we change to that drive?
	jz nogo		; else skip

godrv:	pop dx		; restore drive number to DL (part of DX)
	push dx		; save it again for later
	dec dx		; convert from 1 based to 0 based drive
	mov ah,0eh	; change to drive
	int 21h		; (... and ignore errors)

nogo:	pop ax		; restore drive number
	mov ah,4ch	; exit with errorlevel from drive number
	int 21h


change	db 0		; nonzero if change-there selected
scandrv	db 1		; nonzero if scanning all C:-Z:


helpmsg:
	db "Public Domain disk finder by EA 2005",13,10
	db 13,10
	db "Usage:",13,10
	db "  FINDDISK [X:][+]LABEL",13,10
%if UPCASE
	db "Search LABEL on drives C:-Z:",13,10
%else
	db "Search LABEL on drives C:-Z:, case-sensitive.",13,10
%endif
	db "  X:  check only 1 drive",13,10
	db "  +   go to the found drive",13,10
%if SAFEMODE	; save some disk space if SAFEMODE is on ;-)
	db "  ? * wildcards, e.g. DO?-BI*",13,10
%else
	db "  ? * wildcards, e.g. DO?-BIN*",13,10
%endif
%if UPCASE
	db "Errorlevel:",13,10
	db "  1,2,... A:,B:,... matches",13,10
	db "  0 "
nomsg	db "search failed",13,10	; overlap w/ help saves space
%else
	db "Returns:",13,10
	db "  1,4,... A:,D:,... matches",13,10
	db "  0 "
nomsg	db "no match",13,10		; overlap w/ help saves space
%endif
	db "$"


okmsg	db "echo Match on",13,10
letter	db "X:",13,10,"$"	; linebreak before the "X:" string is
			; useful if you redirect the output to a bat file


fcb	db -1, 0, 0, 0, 0, 0, 8	; -1 is "extended", 8 is "label"
drive	db 3		; 1... means "A:"... Scan starts at C:
what	db "           "	; blank-padded search term,
	;   12345678abc		; can contain ? wildcards.




%if 0			; old safemode code
	...
	mov ax,6900h	; get extra info, like ser no, from boot sector
	mov bl,[drive]	; 1-based drive number
	mov dx,snbuf	; target buffer for the - not yet used - info
	int 21h
	jnc okdrv	; no error? fine!
	cmp al,5	; error "no ext BPB found" is acceptable
	jnz baddrv	; else avoid this drive
okdrv:	...

snbuf	dw 0	; buffer for "get disk ser no", first word: "level",
	; should be 0, dword ser no, 11 bytes label-from-boot-sector,
	; 8 bytes filesystem id string from boot sector.
	; "get disk ser no" produces no critical error prompt.
	;     this buffer will overwrite the start of the help message,
	;     but at that time, we already know that we show no help.

%endif
