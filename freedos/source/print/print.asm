;
;
;   Print, a backgound file printing utility.
;   Copyright (C) 1998	James B Tabor
;
;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation; either version 2 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;
TITLE	PRINT.ASM
;
;
;   Purpose:	To print in background. To load files from a queue to
;		be printed out concurrently. Very basic in format. No
;		big code breaking, just basic stuff. May be M$_Dos
;		compatible with PRINT.XXX.
;
;   Created:		    4-SEP-1992		    James B. Tabor
;
;
;   Modified:		    3-JUN-1999		    James B. Tabor
;
;		Bad command string operations.
;
;   Modified 9/2007 Eric Auer: TIME_TOGO reduced from 18 to 2 to
;   get 9x higher printing speed. Added comments about the MS PRINT
;   options /S:ticks /M:ticks /U:ticks /Q:nn /B:size /D:dev ...
;
;   This PRINT here only takes /1 /2 /3 as "/D:lpt1" etc equivalent
;   options or a filename to print a file. Please use the free PRINTQ
;   tool for "print file", "print /t" (clear queue incl current job),
;   "print /c" (clear queue) and "print /p file" (add file to queue).
;
;   To compile with free compilers:
;   Assemble: ArrowASM "asm print.asm ;"
;   Link to .com: Turbo C 2.01 "tlink /t print.obj"
;
TIME_TOGO	EQU	2	; for /S:ticks and/or /U:ticks?
				; /M:ticks corresponds to 40:78..40:7b
				; (defaults for MS: 8, unknown, unknown)
INIT_BUF_SIZE	EQU	256	; as in /B:size of MS, see BUF_SIZE
STACK_SIZE	EQU	192
;
;
code		segment
		assume	   cs:code,ds:code
;		ORG	02CH
; ENV_SEG		DW	?
		org	100h
start:		jmp	first
;
;
INT05IP		DW	0
INT05CP		DW	0
INT08IP		DW	0
INT08CP		DW	0
INT13IP		DW	0
INT13CP		DW	0
INT24IP		DW	0
INT24CP		DW	0
INT28IP		DW	0
INT28CP		DW	0
INT2FIP		DW	0
INT2FCP		DW	0
;
DOSS_OFS	DW	0
DOSS_SEG	DW	0
;
DOS_CRIT_OFS	DW	0
DOS_CRIT_SEG	DW	0
;
IN_INTCO	DB	0
IN_PRINT	DB	0
;		 DB	 '****'
QUEUE_FLAG	DB	0
;
RUN_CNT		DB	18		; one-time delay after install
;
PSP_SEG		DW	0
		DB	'SETONIT>'
DEVICE_P	DB	0		;DEFAULT PRN=0 (LPT1) see /D:device
		DB	'<'
;
DTA_OFS		DW	0
DTA_SEG		DW	0
;
OLDCTRL		DB	0
;
errinfoarray	dw	6 dup(0)
errinfods	dw	0
errinfoes	dw	0
		dw	3 dup(0)
;

;
SAVE_CS		DW	0
;
SAVE_SP		DW	0
SAVE_SS		DW	0
;
SAVE_F_PTR	DW	0
FIRST_PASS	DB	0
S_FF_FLAG	DB	0

;
FILE_NUM	DB	0
X_DATA		DW	0
Y_DATA		DW	0
;
FAKE:
		DB	'PRN     ',0

;
FLAG_FF		DB	0		    ;FLAG BYTE -1 OF FILE_0
FILE_0		DB	64 DUP(0)	; as in /Q:nn (1..32, default 10)
FILE_1		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
		DB	64 DUP(0)
LAST_FILE	DB	64 DUP(0)
		DB	2  DUP(0)
;
;
;
;
HANDLE		DW	0
F_S_L		DW	0
F_S_H		DW	0
F_S_LO		DW	0
F_S_HI		DW	0
;
;
;   *** Timer tick.... ***
;   int 08 entry point; check INTs 13h active and refuse to
;   run if any of them are set, or if the DOS SAFE and ErrMode flags are
;   not zero .. Otherwise call printer module ..
;
;
INT_08:
		PUSHF
		CALL	CS:DWORD PTR [INT08IP]
		CLI
		CMP	CS: BYTE PTR RUN_CNT,0
		JNZ	NO_RUNA
		CMP	CS: BYTE PTR IN_PRINT,0
		JNZ	NO_RUNB
		CMP	CS: BYTE PTR QUEUE_FLAG,0
		JZ	NO_RUNB
		PUSH	DS
		PUSH	ES
		PUSH	BX

		PUSH	CS
		POP	DS

		CMP	CS: BYTE PTR IN_INTCO,0	    ;COMBATION CHECK
		JNZ	NO_RUN

		LES	BX,CS:DWORD PTR [DOSS_OFS]
		CMP	ES: BYTE PTR [BX-1],0	    ;ErrorMode Flag ,
		JNZ	NO_RUN			    ;check for processing
						    ;critical disk error..
		CMP	ES: BYTE PTR [BX],0	    ;InDOS FLAG ...
		JNZ	NO_RUN

		MOV	CS: BYTE PTR IN_PRINT,0FFH
		CALL	RUNIT
		MOV	CS: BYTE PTR IN_PRINT,0
NO_RUN:
		POP	BX
		POP	ES
		POP	DS
NO_RUNB:
		MOV	CS: BYTE PTR RUN_CNT,TIME_TOGO	; as in /S:ticks?
NO_RUNA:
		DEC	CS: BYTE PTR RUN_CNT

;		 JMP	CS:DWORD PTR [INT08IP]
		IRET
;
;
; *** DOS Forground entry point ... ***
; Don't have to check anything here, it is safe?
; (called when DOS is waiting for user console input)
;
;
INT_28:
		CLI
		PUSHF
		CALL	CS:DWORD PTR [INT28IP]
		CMP	CS: BYTE PTR IN_PRINT,0
		JZ	NORM_INT28
		IRET
NORM_INT28:
		PUSHF
		CMP	CS: BYTE PTR RUN_CNT,0
		JNZ	NO_RUN1A
		CMP	CS: BYTE PTR QUEUE_FLAG,0
		JZ	NO_RUN1A
		PUSH	DS
		PUSH	ES
		PUSH	BX
		LES	BX,CS:DWORD PTR [DOS_CRIT_OFS]	;CRIT ERROR FLAG ADD
		CMP	BYTE PTR ES:[BX],0		;IS Dos CRIT STATE
		JNZ	NO_RUN1

		MOV	CS: BYTE PTR IN_PRINT,0FFH
		CALL	RUNIT
		MOV	CS: BYTE PTR IN_PRINT,0
NO_RUN1:
		POP	BX
		POP	ES
		POP	DS
		MOV	CS: BYTE PTR RUN_CNT,TIME_TOGO	; as in /S:ticks?
NO_RUN1A:
		POPF
		IRET
;
INT_24:
		XOR	AL,AL
		ADD	AL,3
		IRET
;
MON_I13:
		PUSHF
		INC	 CS: BYTE PTR IN_INTCO
		POPF
		PUSHF
		CALL	CS:DWORD PTR [INT13IP]
		PUSHF
		DEC	CS: BYTE PTR IN_INTCO
		POPF
		db	0cah, 2, 0	; RETF	2
;
;
;
INT_05:
		CMP	CS:BYTE PTR QUEUE_FLAG,0
		JNZ	SKIP_I_5

		PUSHF
		CALL	CS:DWORD PTR [INT05IP]
SKIP_I_5:
		IRET
;
;
;
;
;
CHK_STATUS:
		MOV	AX,0
		CLC
		CMP	CS: BYTE PTR QUEUE_FLAG,0
		JZ	EXIT_2F
		STC
		MOV	AX,8
		MOV	SI,OFFSET FAKE
		PUSH	CS
		POP	DS
		JMP	EXIT_2F
;
E_HOLD_JOBS:
		DEC	CS:IN_PRINT
		MOV	AX,0AH
		CLC
		JMP	EXIT_2F
;
HOLD_P_JOBS:
		MOV	DX,0
		PUSH	CS
		POP	DS
		INC	CS:IN_PRINT
		MOV	SI,OFFSET FILE_0
		MOV	AX,8
		CLC
		JMP	EXIT_2F
;
;
;	Emulate PRINT.XXX to load files into queue, and pointers too.
;
;
INT_2F:
		CLI
		PUSH	ES
		PUSH	DI
		PUSH	BX
		PUSH	CX
		CMP	AH,01
		JNE	SKIP_2F
		CMP	AL,0
		JE	DO_Y_INST
		CMP	AL,1
		JE	SUMMIT_Q
		CMP	AL,2
		JE	E_HOLD_JOBS
		CMP	AL,3
		JE	E_HOLD_JOBS
		CMP	AL,4		; HOLD PRN JOBS.
		JE	HOLD_P_JOBS
		CMP	AL,05		; END HOLD ON PRN JOBS.
		JE	E_HOLD_JOBS
		CMP	AL,6
		JE	CHK_STATUS
SKIP_2F:
		POP	CX
		POP	BX
		POP	DI
		POP	ES
		STI
		JMP	CS:DWORD PTR [INT2FIP]
;
DO_Y_INST:
		MOV	AL,0FFH
EXIT_2F:
		POP	CX
		POP	BX
		POP	DI
		POP	ES
		STI
		db	0cah, 2, 0	; RETF	2
;
;
;
;
SUMMIT_Q:
		CMP	CS: BYTE PTR QUEUE_FLAG,10
		JE	FILE_BUFF_FULL

		MOV	BX,DX
		MOV	AL,DS:BYTE PTR [BX]
		MOV	CS:BYTE PTR S_FF_FLAG,AL    ;GET COMMAND SPEC.
		INC	BX			    ; 0 = FORM FEED
		MOV	SI,DS: WORD PTR [BX]	    ; 1 = NO F.F.
		MOV	ES,CS: SAVE_CS
;
		MOV	DI,OFFSET FILE_0
		MOV	AL,CS:BYTE PTR FILE_NUM
		CMP	AL,0
		JE	SKIP_M
		MOV	AH,0
		MOV	BX,64
		MUL	BX
		ADD	DI,AX
SKIP_M:
		MOV	AL,CS: BYTE PTR S_FF_FLAG
		MOV	CS: BYTE PTR [DI-1],AL
		MOV	CX,0
		PUSH	SI
L_CHK:
		MOV	AL,DS:BYTE PTR [SI]	    ; CALC THE CX NUMBER
		INC	SI			    ; FOR MOVES BYTES...
		INC	CX
		CMP	CX,65
		JE	INT_2F_ERROR
		CMP	AL,0			    ; FINDING ZERO
		JNE	L_CHK
		DEC	CX
		POP	SI
;
		CLD
		REP	MOVSB
		INC	CS: BYTE PTR QUEUE_FLAG
		INC	CS: BYTE PTR FILE_NUM
		CLC
		MOV	AX,0
SKIP_M_1:
		JMP	EXIT_2F
INT_2F_ERROR:
		STC
		MOV	AX,0CH
		JMP	SHORT SKIP_M_1
;
FILE_BUFF_FULL:
		STC
		MOV	AX,8
		JMP	EXIT_2F
;
;
;
;
;
;
;
RUNIT:
		CLI
		MOV	CS: SAVE_SS,SS
		MOV	CS: SAVE_SP,SP
		MOV     SP,CS: BUF_SIZE
		ADD     SP,OFFSET BUFFER + STACK_SIZE
		AND	SP,0fffcH
		MOV	SS,CS: SAVE_CS
		STI
		PUSHF
		PUSH	ES
		PUSH	DS
		PUSH	AX
		PUSH	BX
		PUSH	CX
		PUSH	DX
		PUSH	DI
		PUSH	SI
		PUSH	BP
;
		PUSH	CS
		POP	DS
		PUSH	CS
		POP	ES
;
		PUSH	ES
		PUSH	DS
		XOR	BX,BX
		MOV	AH,59H
		INT	21H
		MOV	CS:ERRINFODS,DS
		POP	DS
		PUSH	BX
		MOV	BX,OFFSET ERRINFOARRAY
		MOV	[BX],AX
		POP	2[BX]
		MOV	4[BX],CX
		MOV	6[BX],DX
		MOV	8[BX],SI
		MOV	0AH[BX],DI
		MOV	0EH[BX],ES
		POP	ES
;
;
		PUSH	DS
		PUSH	ES
		MOV	AH,62H
		INT	21H
		XCHG	BX,CS:PSP_SEG		;SWAP PSP'S
		MOV	AH,50H
		INT	21H
		POP	ES
		POP	DS
;
		MOV	AX,3302H		; TURN OFF CTRL-C/BREAK
		MOV	DL,0			; AND GET IT TOO.
		INT	21H
		MOV	OLDCTRL,DL
;
		MOV	AX,3524H
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT24CP,DX
		MOV	CS: WORD PTR INT24IP,BX
		MOV	DX,OFFSET INT_24
		MOV	AX,2524H
		INT	21H
;
		MOV	AH,2FH			; GET/SET DTA
		INT	21H
		MOV	WORD PTR DTA_OFS,BX
		MOV	WORD PTR DTA_SEG,ES
		MOV	DX,80H			; USE COMM_LINE
		MOV	AH,1AH
		INT	21H
;
		PUSH	CS
		POP	DS
		PUSH	CS
		POP	ES
;
		CALL	DO_PRINTER
;
		PUSH	CS
		POP	DS
;
		PUSH	DS
		MOV	DS,CS:DTA_SEG
		MOV	DX,CS:DTA_OFS
		MOV	AH,1AH			; PUT BACK DTA...
		INT	21H
		POP	DS
;
		PUSH	DS
		MOV	DS,CS:INT24CP
		MOV	DX,CS:INT24IP
		MOV	AX,2524H
		INT	21H
		POP	DS
;
		MOV	AX,3301H
		MOV	DL,OLDCTRL		; PUT BREAK CHECK FLAG BACK.
		INT	21H
;
		PUSH	DS
		PUSH	ES
		MOV	AH,62H
		INT	21H
		XCHG	BX,CS:PSP_SEG		; SWAP PSP'S...
		MOV	AH,50H
		INT	21H
		POP	ES
		POP	DS
;
		MOV	AX,5D0Ah
		MOV	DX,OFFSET ERRINFOARRAY	; PUT ERRORS BACK.
		INT	21H
;
;
		POP	BP
		POP	SI
		POP	DI
		POP	DX
		POP	CX
		POP	BX
		POP	AX
		POP	DS
		POP	ES
		POPF
		CLI
		MOV	SS,CS: SAVE_SS
		MOV	SP,CS: SAVE_SP
		STI
;
		RET
;
;
;
;   ERROR BUFFER OUTPUT , IF PRINT NOT READY. THIS ROUT. WILL DO IT.
;
;
;
;
DO_ERROR_BUFFER:
		MOV	CX,WORD PTR SAVE_CX	; GET POINTERS..
		MOV	AX,WORD PTR SAVE_AX
		MOV	SI,WORD PTR SAVE_SI
		PUSH	AX
		CALL	LOOP_OUT
		POP	AX
		JC	SKIP_D_E_B
		SUB	WORD PTR F_S_L,AX
		ADD	WORD PTR F_S_LO,AX
		ADC	WORD PTR F_S_HI,0	; IF CARRY ADD TO POINTER.
		MOV	BYTE PTR ERROR_IN_PRN,0
SKIP_D_E_B:
		JMP	EXIT_PRINTER
;
;
ERROR:
		PUSH	CS
		POP	DS
		MOV	SI,OFFSET ERROR_OUT
		MOV	CX,26
		CALL	LOOP_OUT
		CALL	EXIT_P_E
		JMP	CLOSE
;
;	BACKGROUND PRINTER UTILITY.....
;
;	BASIC FORM ....
;
;
DO_PRINTER:
		CMP	BYTE PTR ERROR_IN_PRN,0
		JNZ	DO_ERROR_BUFFER

		CMP	FIRST_PASS,0		    ; FIRST PASS FLAG,
		JNZ	OPEN			    ; WORK ON ONE FILE AT A
;						      TIME.
		MOV	DI,OFFSET FILE_0
		MOV	DX,DI
		MOV	SAVE_F_PTR,DX
;
XOPEN:
		PUSH	CS
		POP	DS
		MOV	AX,3D02H
		MOV	DX,WORD PTR SAVE_F_PTR
		INT	21H
		JC	ERROR
		MOV	HANDLE,AX

		PUSH	CS
		POP	DS
		MOV	AX,4202H		; GET FILE SIZE.
		MOV	CX,0
		MOV	DX,0
		MOV	BX,HANDLE
		INT	21H
;
		MOV	CS: WORD PTR F_S_L,AX
		MOV	CS: WORD PTR F_S_H,DX
;
		MOV	AX,4200H		; RESET FILE POINTER TO TOP.
		MOV	BX,HANDLE
		MOV	DX,0
		MOV	CX,0
		INT	21H
;
		INC	FIRST_PASS
		JMP	SKIP_OPEN
;
;
ERROR_2:
		JMP	ERROR
OPEN:
		PUSH	CS
		POP	DS
		MOV	AX,3D02H
		MOV	DX,WORD PTR SAVE_F_PTR
		INT	21H
		JC	ERROR_2
		MOV	HANDLE,AX
;
;
;
		MOV	AX,4201H		; POINT TO FILE.
		MOV	BX,HANDLE
		MOV	CX,F_S_HI		; OFFSET FROM START OF
		MOV	DX,F_S_LO		; FILE....
		INT	21H
;
;
;
SKIP_OPEN:
;
		MOV	AX,CS: WORD PTR F_S_L
		CMP	AX,CS: WORD PTR BUF_SIZE	; as in /B:size
		JAE	READ
		MOV	AX,CS: WORD PTR F_S_H
		CMP	AX,0
		JZ	SMALL_FILE
		DEC	AX
		MOV	CS: WORD PTR F_S_H,AX
;
READ:
		PUSH	CS
		POP	DS
;
		MOV	AH,3FH
		MOV	BX,HANDLE
		MOV	CX,WORD PTR BUF_SIZE		; as in /B:size
		MOV	DX,OFFSET BUFFER
		INT	21H
		MOV	AX,WORD PTR BUF_SIZE		; as in /B:size
		MOV	SAVE_AX,AX
		PUSH	AX
		CALL	OUT_BUFFER
		POP	AX
		JC	CLOSE
		SUB	WORD PTR F_S_L,AX
		ADD	WORD PTR F_S_LO,AX
		ADC	WORD PTR F_S_HI,0	; IF CARRY ADD TO POINTER.
;
;
CLOSE:
		PUSH	CS
		POP	DS
		MOV	AH,3EH
		MOV	BX,HANDLE
		INT	21H
		JMP	EXIT_PRINTER
;
ERROR_1:
		JMP	ERROR
;
;
;
SMALL_FILE:
		PUSH	CS
		POP	DS
		MOV	AH,3FH
		MOV	BX,HANDLE
		MOV	CX,F_S_L
		MOV	DX,OFFSET BUFFER
		INT	21H
		MOV	AX,F_S_L
		MOV	SAVE_AX,AX
		PUSH	AX
		CALL	OUT_BUFFER
		POP	AX
		JC	CLOSE
;
		CMP	BYTE PTR FLAG_FF,01H
		JE	SKIP_NO_FF
;
		MOV	AX,1
		CALL	OUT_BUFFERX	; DO FORM FEED..
;
SKIP_NO_FF:
		PUSH	CS
		POP	DS
		MOV	AH,3EH
		MOV	BX,HANDLE
		INT	21H
;
;
EXIT_P_E:
;
		MOV	CS: F_S_LO,0
		MOV	CS: F_S_HI,0
		MOV	CS: BYTE PTR FIRST_PASS,0
		DEC	CS: BYTE PTR FILE_NUM
		DEC	CS: BYTE PTR QUEUE_FLAG
;
		CLD
		PUSH	CS
		POP	DS
		PUSH	CS
		POP	ES
		MOV	CX,9
		MOV	DI,OFFSET FILE_0
		MOV	SI,OFFSET FILE_1
		MOV	AL,BYTE PTR [SI-1]
		MOV	BYTE PTR FLAG_FF,AL
XY_LOOP:
		MOV	WORD PTR X_DATA,SI
		MOV	WORD PTR Y_DATA,DI
		PUSH	CX
		MOV	CX,64
		REP	MOVSB
		POP	CX
		MOV	SI,WORD PTR X_DATA
		MOV	DI,WORD PTR Y_DATA
		ADD	SI,64
		ADD	DI,64
		LOOP	XY_LOOP

		MOV	CX,64
		MOV	DI,OFFSET LAST_FILE ; ALWAYS ZERO LAST FILE BUFFER.
		XOR	AL,AL
		CLD
		REP	STOSB
EXIT_PRINTER:
		RET
;
;
;
;
;
OUT_BUFFERX:
		MOV	SI,OFFSET ERROR_OUT
		ADD	SI,25
		MOV	CX,AX
		JMP	LOOP_OUT
;
OUT_BUFFER:
		CLD
		MOV	SI,OFFSET BUFFER
		MOV	CX,AX
		JCXZ	NO_O_BUFF
;
LOOP_OUT:
		MOV	SAVE_SI,SI
		MOV	SAVE_CX,CX
		LODSB
		PUSH	AX
		MOV	DL,DEVICE_P	; LPT1 ...
		MOV	DH,0
		MOV	AH,2
		INT	17H
		AND	AH,29H
		POP	AX
		JNZ	ERROR_PRINT_L
		CMP	AL,26
		JE	NO_O_BUFF
		MOV	AH,0
		MOV	DH,0
		MOV	DL,DEVICE_P	; LPT1 ....
;		INT	 29H		; TEST ONLY....
		INT	17H
		LOOP	LOOP_OUT
;
NO_O_BUFF:
		CLC
		RET
;
;
;
ERROR_PRINT_L:
		MOV	BYTE PTR ERROR_IN_PRN,0FFH
		STC
		RET
;
;
;
ERROR_IN_PRN	DB	0
SAVE_CX		DW	0
SAVE_SI		DW	0
SAVE_AX		DW	0
BUF_SIZE	DW	512			; as in /B:size
;
;
;				       1	 2
;			 0  1  234567890123456789012   3  4  5
ERROR_OUT	DB	13,10,'ERROR File not found!',10,13,12
;
;		Uninitialized data starts here
;
BUFFER		LABEL	BYTE
;		DB	INIT_BUF_SIZE DUP (0)		; as in /B:size
;		DB	0, 0
;		DB	192 DUP(0)
; STACK		LABEL	BYTE
;
;
;
;		Nonresident code and data starts here
;
FIRST:

		PUSH	CS
		POP	DS
		PUSH	CS
		POP	ES
		MOV	BX,DS
		MOV	SAVE_CS,BX
		MOV	DX,OFFSET MSG_OUT
		CALL	PRINT_STRING
		MOV	AX,0100H
		INT	2FH
		CMP	AL,0FFH
		JNE	SKIP_UINSTALL
		MOV	AX,0104H	; freeze queue to check it
		INT	2FH
		CALL	ADD_FILE
		MOV	AX,0105H
		INT	2FH
		MOV	AX,4C00H
		INT	21H
SKIP_UINSTALL:
;
;	OBTAIN ADDRESS OF DOS SAFE FLAG, THE InDOS FLAG.....
;
		PUSH	ES
		MOV	AH,034H
		INT	21H
		MOV	AX,ES
		MOV	WORD PTR DOSS_SEG,AX
		MOV	WORD PTR DOSS_OFS,BX
		POP	ES
;
;	OBTAIN ADDRESS OF DOS CRITICAL ERROR FLAG, THE ErrorMode FLAG....
;	You will notice that the address for this flag is one above the
;	InDOS flag .. For MS-Dos 3.10 and above .....
;	********* GET SWAP DATA AREA *********
;
		PUSH	DS
		PUSH	SI
		MOV	AX,5D06H
		INT	21H
		MOV	AX,DS
		MOV	BX,SI
		MOV	CS: WORD PTR DOS_CRIT_SEG,AX
		MOV	CS: WORD PTR DOS_CRIT_OFS,BX
		POP	SI
		POP	DS

;
		MOV	AX,3505H
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT05CP,DX
		MOV	CS: WORD PTR INT05IP,BX
		MOV	DX,OFFSET INT_05
		MOV	AX,2505H
		INT	21H
;
;		Install our timer tick handler: Printing can start
;		after "initial one time delay" in RUN_CNT
		MOV	AX,3508H
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT08CP,DX
		MOV	CS: WORD PTR INT08IP,BX
		MOV	DX,OFFSET INT_08
		MOV	AX,2508H
		INT	21H
;
		MOV	AX,3513H
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT13CP,DX
		MOV	CS: WORD PTR INT13IP,BX
		MOV	DX,OFFSET MON_I13
		MOV	AX,2513H
		INT	21H
;
;		Install dos idle handler: Printing can start
;		when DOS is waiting for console input
		MOV	AX,3528H
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT28CP,DX
		MOV	CS: WORD PTR INT28IP,BX
		MOV	DX,OFFSET INT_28
		MOV	AX,2528H
		INT	21H
;
		MOV	AX,352FH
		INT	21H
		MOV	DX,ES
		MOV	CS: WORD PTR INT2FCP,DX
		MOV	CS: WORD PTR INT2FIP,BX
		MOV	DX,OFFSET INT_2F
		MOV	AX,252FH
		INT	21H
;
		PUSH	ES
		PUSH	DS
		MOV	AH,62H		; get current PSP (CS if COM)
		INT	21H
		MOV	CS:PSP_SEG,BX
		POP	DS

		MOV	ES,CS:[2ch]	; ENV_SEG
		MOV	AH,49H		; free memory
		INT	21H
		POP	ES
;
;
;		Add any files listed on command line to print queue
		INC	IN_PRINT
		CALL	ADD_FILE
;
;
		MOV	DX,OFFSET INSTALL
		CALL	PRINT_STRING
		MOV	DX,OFFSET PRN_DEVICE
		CALL	PRINT_STRING

;
		DEC	IN_PRINT
;		MOV	DX,OFFSET FIRST
		MOV	DX,OFFSET BUFFER
		ADD	DX,BUF_SIZE	; current buffer size
		ADD	DX,STACK_SIZE
		ADD	DX,15		; round up
		MOV	CL,4
		SHR	DX,CL		; convert to paragraphs
		MOV	AX,3100H	; stay resident
		INT	21H
;		INT	27H
;
;
PRINT_STRING:
		MOV	AH,9
		INT	21H
		RET
;
MSG_OUT		DB	13,10,'   Backgound print utility for Dos.',13,10
		DB	'   Version 1.02, by James B. Tabor.',13,10,'$'
INSTALL		DB	'   PRINT QUEUE SUBSYSTEM IS INSTALLED ',13,10,'$'
ALL_QUEUE	DB	'   NO FILE ENTERED FROM COMMAND LINE.',13,10,'$'
ALLINST		DB	'   FILE ADDED TO QUEUE.',13,10,'$'
ALLERROR	DB	'   Search First: CAN NOT FIND FILE OR PATH !',13,10,'$'
ALLERROR_1	DB	'   Search Second: CAN NOT FIND FILE OR PATH !',13,10,'$'
ERROR_S		DB	'   BUFFER FULL!',07
CRLF		DB	13,10,'$'
PRN_DEVICE	DB	'   Device to direct Print [PRN=0]: ',13,10,'$'
;			 01234567890123456789012345678901
;				   1	     2	       3
FILE_BUF	DB	128 DUP (0)
;
QUEUE_LIST	DB	0   ; MS_DOS PRINT.XXX USES THIS BYTE AS A PRIORITY
FILE_QUEUE	DD	0   ; BYTE , BUT WE ARE NOT...
;
ADD_FILE:
		PUSH	CS
		POP	DS
;
		push	cs
		pop	es
		CALL	FIX_FILE_P
		JC	EXIT_W_O
;
		CALL	CONZ_FILE
;
		MOV	DX,OFFSET FILE_BUF ;ASSUME DTA @ COMM_LINE
		MOV	AH,4EH
		MOV	CX,0
		INT	21H
		JNC	GO_SUMMIT
		MOV	DX,OFFSET ALLERROR
		AND	AX,03H
		JNZ	BADD_EXIT
		MOV	DX,OFFSET FILE_BUF
FIND_NEXT:
		MOV	AH,4FH
		INT	21H
		JNC	GO_SUMMIT
		MOV	DX,OFFSET ALLERROR_1
		CMP	AX,12H
		JNE	BADD_EXIT
		JMP	GOOD_EXIT
GO_SUMMIT:
		CALL	GETPATHFILE
		CALL	SUMMIT_T_Q
		JC	SKIP_S_T_Q_SS
		JMP	FIND_NEXT
SKIP_S_T_Q_SS:
		MOV	DX,OFFSET ERROR_S
		JMP	BADD_EXIT
;
;
GOOD_EXIT:
		MOV	DX,OFFSET ALLINST
BADD_EXIT:
		CALL	PRINT_STRING
		RET
EXIT_W_O:
		MOV	DX,OFFSET ALL_QUEUE
		JMP	BADD_EXIT
;
;
;
FIX_FILE_P:
		MOV	BX,80H
		MOV	AL,[BX]
		CMP	AL,0
		JE	E_F_F_P
;
		CALL	COMM_LINE
		MOV	AL,"/"
		REPNE	SCASB
		JNE	NEXT_CHAR
		MOV	AL,BYTE PTR [DI]
		MOV	WORD PTR [DI-1],0H
		AND	AL,33H
		MOV	BYTE PTR [PRN_DEVICE+31],AL
		AND	AL,03H
		DEC	AL
		MOV	BYTE PTR DEVICE_P,AL	; as in /D:device for MS
;
		CALL	COMM_LINE
		INC	DI	    ; SKIP OVER SPACE @ COMM_LINE.
		DEC	CX
		MOV	AL,20H
		REPNE	SCASB
		JNE	E_F_F_P
		DEC	DI
NEXT_CHAR:
		MOV	WORD PTR [DI],0
		CLC
		RET
E_F_F_P:
		STC
		RET
;
CONZ_FILE:
		PUSH	CS
		POP	DS
		PUSH	CS
		POP	ES

		MOV	SI,81H
CONZ_L:
		CMP	BYTE PTR [SI],20H
		JNE	CONZ_A
		INC	SI
		JMP	CONZ_L
CONZ_A:
		MOV	DI,OFFSET FILE_BUF
		MOV	AH,60H
		INT	21H
		RET
SUMMIT_T_Q:
		PUSH	CS
		POP	DS
		MOV	DX,OFFSET FILE_BUF_1
		MOV	BX,DS
		MOV	WORD PTR FILE_QUEUE,DX
		MOV	WORD PTR FILE_QUEUE+2,BX
		MOV	AX,0101H
		MOV	DX,OFFSET QUEUE_LIST
		INT	2FH
		RET
;
;
;
GETPATHFILE:
		MOV	DI,OFFSET FILE_BUF_1
		MOV	SI,OFFSET FILE_BUF
G_P_F_1:
		LODSB
		STOSB
		OR	AL,AL
		JNE	G_P_F_1
G_P_F_2:
		DEC	DI
		CMP	BYTE PTR [DI],'\'
		JNE	G_P_F_2
		INC	DI
		MOV	SI,80H+1EH  ; ASSUME DOS HAS SET DTA @ COMM_LINE.
		MOV	CX,13
		REP	MOVSB
		RET
;
;
;
COMM_LINE:
		MOV	DI,80H
		MOV	CL,BYTE PTR [DI]
		XOR	CH,CH
		INC	DI
		CLD
		RET
FILE_BUF_1	DB	75 DUP(?)

code		ends
		end	   start
