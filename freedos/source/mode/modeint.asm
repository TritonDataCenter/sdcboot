; Code for printer redirection and infinite retries, based on:
;   MODE -- Mode setting utility for Free-DOS
;   Written for the Free-DOS project.
;   (c) Copyright 1994-1995 by K. Heidenstrom. ... GPL 2 ...
;   Internet: kheidens@actrix.gen.nz
;   Snail: K. Heidenstrom c/- P.O. Box 27-103, Wellington, New Zealand

old17	dd 0	; old interrupt 0x17 vector

redirconfig	db 0,0,0,0
	; for LPT1..LPT4 (although BIOS only supports 1..3),
	; config is one byte each: Bits are ("?" bits ignored):
	; ? ? RETRY ?  ? X X X
	; the X bits combine to a number 0..7, 0 meaning no redirect
	; and 1..4 meaning send LPTn data to COMx. RETRY set means
	; infinite retry. Other values mean redirect to NUL for now.
	; Good point: config can be ASCII text.
	; redirect LPTn=COMm - LPTn does not need to exist physically.

; Above: data part, 8 bytes. Below: code part, about 120 bytes :-).
; STARTING with our int 17h handler!

; ================================================================


; INT 17: AH is the function number, which may be 0 (send character),
; 1 (initialise port), or 2 (get status).  DX is the port number,
; normally 0-2 for LPT1-3...


myInt17:	cmp	dx,4
		jae	noInt17		; only LPT1..4 processed
		cmp	ah,2
		ja	noInt17		; only hook functions 0..2

		push	bx		; *** BX
		mov	bx,dx		; for this port...
		mov	bl,[cs:redirconfig+bx]	; ...get control bits
		mov	bh,bl
		and	bx,2007h	; BH: persistence, BL: target

		or	bl,bl		; Redirected?
		jnz	IsRedirected	; If so

		; not redirected, only need to handle retry for function 0

		cmp	ah,0		; send char?
		jnz	noInt17x	; otherwise, no special action
		or	bh,bh		; persistent?
		jz	noInt17x	; If not, just go to old int 17h handler


		; try to send until no timeout reported

PersistLoop:	xor	ah,ah		; send char
		pushf			; call normal...
		call	far [cs:old17]	; ...int 17 handler.
		test	ah,1		; timeout occurred?
		jnz	PersistLoop	; if yes, retry (infinite)
					; (abort on Ctrl-Break !?)

myInt17donex:	pop	bx		; *** BX
		iret			; done!

noInt17x:	pop	bx		; *** BX
noInt17:	jmp	far [cs:old17]	; jump to original handler

		; ---- special handlers follow ----

RedirToNUL:	mov	ah,90h		; just report "nice" status.
		jmp	short myInt17donex


		; If redirected, translate status. Functions:
		; 0 send, 1 init (ignored, only reports status),
		; 2 report status.

IsRedirected:	cmp	bl,4		; Special (NUL) redirection?
		jnb	RedirToNUL

RedirNormal:	push	dx		; ***   DX
		mov	dl,bl		; target port number + 1
		dec	dx		; adjust
		test	ah,ah		; Check function number
		jnz	getStatus	; init / status: get status

		; send the char to SERIAL port now

SerialPersist:	mov	ah,1		; Function to send character (serial)
		int	14h		; SERIAL I/O
		test	ah,80h		; everything fine?
		jz	getStatus	; then we are done.
		or	bh,bh		; Persistent behaviour?
		jnz	SerialPersist	; if yes, retry.
		mov	bh,80h		; remember the timeout


getStatus:	push	ax		; ***      AX
		mov	ah,3		; fetch SERIAL status bits
		int	14h		; Request serial port status
		or	bh,ah		; OR together timeout facts
		mov	ah,10h		; return "SEL"
		jns	noSerialTimeout	; TIMEOUT?
		or	ah,1		; set "TIMEOUT" if AH bit 7
noSerialTimeout:			; "TIMEOUT" was set.
		test	al,10h		; CTS?
		jz	noSerialReady
		or	ah,80h		; set "READY" if AL bit 4
noSerialReady:				; "CTS" was set.
		mov	bh,ah		; save status
		pop	ax		; ***      AX
		mov	ah,bh		; restore status

		pop	dx		; ***   DX
		jmp short myInt17donex

		; AL bit 4, CTS -> READY
		; AH bit 7, TIMEOUT -> TIMEOUT
		; other bits in AH:
		;   TIMEOUT TXshiftREADY TXholdREADY BREAK
		;     FrameError ParityError OverrunError RXdataREADY
		; other bits in AL:
		;   CARRIER RING DataSetReady ClearToSend
		;     deltaCARRIER endRING deltaDSR deltaCTS
		; printer status bits in AH:
		;   READY nACK PaperEmpty SEL  IOError ? ? TIMEOUT
		; (PaperEmpty and SEL together mean "no printer")
		; we return:
		;   READY 0 0 1  0 0 0 TIMEOUT

; ================================================================

; Extra comments on MODE: 115200/Baud is uart divisor...
; outb port+3, 0x80 (LCR: set DivisorLatchAccessBit)
; outb port,   divisorLO     then     outb port+1, divisorHI
; outb port+3, 0x?? (LCR: high bit 0, low bits specify settings)

