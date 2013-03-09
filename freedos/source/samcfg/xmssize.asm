
	org 0x100	; a com program

	cld
	mov si,0x81	; get command line args
args:	lodsb
	cmp al,'-'	; delta mode?
	jz deltamode	; DELTA MODE!
	cmp al,' '
	jz args
	cmp al,9
	jz args
	cmp al,13	; CR at end of line?
	jb showhelp
	cmp al,'0'	; digit?
	jb showhelp
	cmp al,'9'	; digit?
	ja showhelp

percentmode:
	sub al,'0'	; al is now the highest digit
	mov ah,al	; AH is first digit
pml:	lodsb		; next digit
	cmp al,' '
	jz pml
	jb pmsingle	; all digits collected
	sub al,'0'
	jc showhelp
	cmp al,9
	ja showhelp
	aad		; convert AH AL from BCD to binary
pml2:	push ax		; our percentage
	lodsb		; check if line ends here
	cmp al,' '
	pop ax
	jz pml2
	jb gopercent	; assume that below ' ' means "end of line, okay"
	jmp short showhelp

pmsingle:		; percent value is in 0 to 9 range
	mov al,ah
	mov ah,0
gopercent:
	jmp percentcalc	; PERCENT MODE!

	; -------------------------------

showhelp:
	mov dx,helpmsg
	mov ah,9	; show text
	int 0x21
	mov ax,0x4c00	; return with errorlevel 0
	int 0x21

	; -------------------------------

deltamode:
	xor cx,cx	; sum
dml:	lodsb
	cmp al,' '	; skip over all spaces
	jz dml
	jb deltacalc	; below ' ' is assumed to be end of line
	sub al,'0'
	jc showhelp
	cmp al,9
	ja showhelp
	mov ah,0	; AX is the digit
	mov bx,10	; the factor
	xchg cx,ax	; fetch old sum
	mul bx		; now DX:AX is 10 * sum
	add ax,cx	; add the digit (ignore high part)
	cmp ax,4096	; 4 GB are 4096 MB, normal HIMEM has that limit...
	ja showhelp	; abort if trying to subtract very high values
	mov cx,ax	; update sum
	jmp short dml	; loop to collect more digits

deltacalc:		; calculate free XMS minus AX Megabytes
	mov ax,cx	; use the sum
	push ax
	call testxms	; check amount of available XMS in kbytes
	pop ax
	mov bx,1024	; the factor
	xor dx,dx
	mul bx		; ... user meant MBytes, but testxms returns kBytes
			; result is in DX:AX
	sub [xmssize],ax	; subtract requested amount
	sbb [xmssize+2],dx	; from 32bit value
	jc underflow		; less than nothing left? bad!
	mov bx,1024
	mov ax,[xmssize]	; fetch the left amount
	mov dx,[xmssize+2]	; ... 32bit again
	cmp dx,bx		; too much?
	ja overflow
	div bx			; convert to MBytes again: result AX, remainder DX

	; -------------------------------

gotvalue:
	cmp ax,255
	ja overflow
	call showresult

returnvalue:
	push ax
	mov dx,okaymsg
	mov ah,9	; show text
	int 0x21
	pop ax
	mov ah,0x4c	; exit with errorlevel from our calc results
	int 0x21

overflow:
	mov dx,overmsg
	mov ah,9	; show text
	int 0x21
	mov ax,0x4cff	; clip errorlevel to 255
	int 0x21

underflow:
	mov dx,undermsg
	mov ah,9	; show text
	int 0x21
	mov ax,0x4c00	; clip errorlevel to 0
	int 0x21
	
	; -------------------------------

percentcalc:		; calculate AX percent of free XMS
	push ax
	call testxms	; check amount of available XMS in kbytes
	pop ax
	mov cx,ax		; wanted amount of percents
	mov ax,[xmssize]	; fetch value
	mov dx,[xmssize+2]	; ... which is 32bit
	or dx,dx	; more than 64 mb?
	jnz bigpercent	; then use 386 specific processing
	mul cx		; multiply by percentage
	shr dx,1	; pre-divide by 2
	rcr ax,1
	shr dx,1	; now we have pre-divided by 4
	rcr ax,1
	mov bx,25600	; convert to Mbytes units: divide by (25*1024)
	div bx		; result AX (remainder in DX)
	jmp gotvalue	; we have a result

bigpercent:		; only happens on 386+ systems...
	mov eax,[xmssize]
	movzx ecx,cx
	mul ecx		; multiply by percentage
	mov ebx,102400	; convert to Mbyte units: divide by 1024*100
	div ebx		; result in EAX (remainder in EDX)
	jmp gotvalue	; we have a result

	; -------------------------------

showresult:		; patch result, max 255, as text into okaymsg
	push ax
	cmp al,10
	jb smallresult
	cmp al,100
	jb normalresult
	mov byte [hundred],'1'	; 100s digit
	sub al,100
	cmp al,100
	jb normalresult
	mov byte [hundred],'2'
	sub al,100
normalresult:
	aam		; AL to AH:AL
	xchg al,ah	; AH is 1s digit now, AL is 10s digit
	add ax,'00'	; convert to text
showdone:
	mov [ten],ax
	pop ax
	ret

smallresult:
	add al,'0'	; convert to text
	mov ah,' '	; pad with a trailing space
	jmp short showdone

	; -------------------------------

testxms:	; check if there is XMS, return value in [xmssize]
	mov ax,0x4300
	int 0x2f
	cmp al,0x80
	jz havexms

noxms:	mov dx,noxmsmsg
	mov ah,9	; show string at DX
	int 0x21
	ret

havexms:
	mov ax,0x4310	; get entry point
	push es
	int 0x2f
	mov [xmsptr],bx
	mov [xmsptr+2],es
	pop es

	mov ah,0	; get XMS version AH.AL (and BH.BL, and DX=HMA state)
	call far [xmsptr]
	cmp ah,3	; AH is major version, AL is minor
			; supposed to be in BCD but FreeDOS HIMEM disagrees
	jae newxms
	mov ah,8	; query amount of free XMS AX (and total size DX)
	mov bl,0	; preset to no error 
	call far [xmsptr]
	or bl,bl	; any error?
	jnz noxms
	or ax,ax	; anything free?
	jz noxms
	mov [xmssize],ax
	ret

newxms:			; only reached on 386+ PCs with XMS 3 drivers
			; actually HIMEM 3.03-3.07 report XMS 3 on 286 :-!
	mov ah,0x88	; query amount of free XMS 3 EAX (and total size EDX)
	mov bl,0	; (... and highest phys address ECX)
	call far [xmsptr]
	or bl,bl	; any error?
	jnz noxms
	or eax,eax	; anything free?
	jz noxms
	mov [xmssize],eax
	ret

	; -------------------------------

xmsptr	dd 0		; pointer to XMS handler
xmssize	dd 0		; amount of free XMS in kbytes

	; -------------------------------

noxmsmsg	db "No XMS present",13,10,"$"
undermsg	db "Delta bigger than free XMS size, returning 0!",13,10,"$"
overmsg		db "Result is above 255 MB, returning 255.",13,10,"$"
okaymsg		db "Result: "
hundred		db " "
ten		db "00 MBytes, returned in errorlevel",13,10,"$"

helpmsg	db "Public Domain XMS sizer tool  by Eric Auer 2005",13,10
	db "Usage:    XMSSIZE -delta  or  XMSSIZE percent",13,10
	db "Ranges:   delta 0 to 4096     percent 1 to 99",13,10
	db "Returns:  A value in MByte units, rounded down,",13,10
	db "          as errorlevel, the range is 0 to 255.",13,10
	db 13,10,"Examples:",13,10
	db "XMSSIZE -42 returns 'free XMS - 42 MBy'",13,10
	db "XMSSIZE 42  returns '42% of free XMS'",13,10
	db 13,10,"Example, needs FreeCOM:",13,10
	db "XMSSIZE 30",13,10
	db "MYCACHE /SIZE=%errorlevel%M",13,10
	db 13,10,"Trick to avoid big results:",13,10
	db "XMSSIZE 3",13,10
	db "MYCACHE /SIZE=%errorlevel%0M",13,10,"$"

