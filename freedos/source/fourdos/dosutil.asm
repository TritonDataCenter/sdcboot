

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          title     DOSUTIL - Assembler utilities for 4DOS

          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          (C) Copyright 1988 - 1999 Rex Conn & J.P. Software
          All Rights Reserved

          Original Author:    Rex Conn
          Rewritten by:       Tom Rawson, J.P. Software, 11/9/88
          Rewritten again:    Rex Conn 8/89 - 5/90, 3/91, 9/91-10/91, 11/92

          These 4DOS support routines provide fast screen I/O and other
          functions for 4DOS.

          } end description

          .model    medium

          ;
          ; Includes
          ;
          include   product.asm         ;product equates
          include   trmac.asm           ;general macros
          include   4dlparms.asm        ;loader parameters
          include   inistruc.asm        ;INI file structures and macros
          include   dvapi.inc           ;DESQview calls

EF_ID     equ       0200000h		; ID bit in EFLAGS register

MAXLINE   equ       256                 ;max command line length for dos_box

BIOS_RAM  segment at 40h
          org	87h
info	db	?
BIOS_RAM  ends

	.data

          extrn     __osmajor:byte      ; DOS major version number
          extrn     __osminor:byte	; DOS minor version number
          extrn     _gchMajor:byte	; "Real" version numbers
          extrn     _gchMinor:byte
          extrn     _gnOSFlags:word     ; O/S status flags
          extrn     _gpIniptr:word      ; INI file data pointer
          extrn     _gszCmdline:byte    ; parser's command line buffer

emm_name  db        'EMMXXXX0'          ; name of EMS driver
StatusNDP dw        ?                   ; address of NDP control word

SaveESP   dd        ?
int24seg  dw        ?			; saved INT 24h vector
int24off  dw        ?
rmargin   dw        ?                   ; display right margin
StatusVid dw        ?                   ; address of 6845 status register,
BIOSOutput dw       0			; send output through BIOS flag

BIOSRow   db        ?
BIOSCol   db        ?
BIOSPage  db        ?
BIOSColor db        ?

          ;
          ; Table to translate last 5 bits of return from INT 21
          ; function 4452h to DR-DOS version number -- 3 bytes per
          ; entry:  return value limit, minor version, major version.
          ;
DRVerTab  db        02h, 3, 40          ;1062 or below is DR-DOS 3.40
          db        04h, 3, 41          ;1064 or below is DR-DOS 3.41
          db        05h, 5, 00          ;1065 is DR-DOS 5.00
          db        11h, 6, 00          ;1071 or below is DR-DOS 6.00
          db        1Fh, 7, 00          ;107F or below is Novell DOS 7.00

	;
	; In DOS, references are in various segments; code is in MISC_TEXT
	;

SERV_TEXT segment   word public 'CODE'  ; define server segment
          extrn     ErrMsg:far          ; error message table scanner
          extrn     CritTAdr:dword      ; critical error text address
          extrn     ServErrs:byte       ; text address
SERV_TEXT ends                          ; just here for references, no data


_TEXT     segment   word public 'CODE'  ;define normal code seg for externals
          extrn     _HoldSignals:far	;break control
          extrn     _EnableSignals:far
          extrn     __write:far	;interface to C write routine
_TEXT     ends

	.code	MISC_TEXT  
          assume    cs:MISC_TEXT, ds:DGROUP, es:nothing, ss:nothing

calldosW	equ	int 21h		; call dos via INT 21

MSG_SEG	equ	SERV_TEXT		; use SERV_TEXT for messages


	;==================================================================
	;
	;  OPERATING SYSTEM INTERFACE ROUTINES (except files)
	;
	;==================================================================

	;
          ; QuerySwitchChar - get the current DOS switch character
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QuerySwitchChar,noframe,far

          mov       ax,3700h
          calldosW
          cmp       al,0FFh		; function supported?
          jne       got_switchar
          mov       dl,'/'		; default to forward slash
got_switchar:
          mov       al,dl

          exit

          ;
          ; QueryVerifyWrite - check disk write verify flag
          ;

          entry     QueryVerifyWrite,noframe,far

          mov       ah,054h
          calldosW
          xor       ah,ah

          exit


          ;
          ; SetVerifyWrite - set disk write verify flag
          ;
          ; On entry:
          ;         Arguments on stack:
          ;           unsigned int VerifyFlag
          ;

          entry     SetVerifyWrite,argframe,far

          argW      VerifyFlag

          mov       ax,VerifyFlag
          mov       ah,02Eh		; set VERIFY state
          xor       dx,dx
          calldosW

          exit


          ;
          ; SetCodePage - set the current code page
          ;
          ; On entry:
          ;         int lpt_num = printer number (1-n)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     SetCodePage,argframe,far

          Argw      codepage

          push      bx
          mov       bx,codepage
          mov       ax,06602h
          calldosW
          jc        cp_error
          xor       ax,ax
cp_error:
          pop       bx
          exit


          ;
          ; CheckForBreak - check for a ^C / ^Break
          ;
          ; On exit:
          ;         AX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     CheckForBreak,noframe,far

          push      bx
          mov       ah, 0Bh
          int       21h
          pop       bx
          exit


          ;
          ; DUMMY24 - temporary replacement for INT 24h handler
          ;

          even
dummy24   proc      far

          mov       ax,3		; fail the call
          iret

dummy24   endp


          ;
          ; DosError - turn the DOS critical error handler on/off
          ;
          ; On entry:
          ;         Argument on stack:
          ;           int nErrorflag:   disable if 2, enable if 1
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     DosError,argframe,far

          ArgW      nErrorFlag

          push      ds

          mov       ax,nErrorFlag
          cmp       ax,2
          jne       reset_popup

	  mov	    al,24h
	  calldos   GETINT		; save former INT 24h vector
	  mov	    int24off,bx
	  mov	    int24seg,es

          loadseg   ds,cs
          assume    ds:@curseg	;fix assumes
          mov       dx,offset dummy24
          mov       al,24h		; set dummy INT 24h vector
          calldos   SETINT
          jmp       short DEExit

reset_popup:
          assume    ds:DGROUP		;fix assumes
          mov       dx,int24off
          mov       ax,int24seg
          mov       ds,ax
          assume    ds:nothing          ; fix assumes
          mov       al,24h		; reset INT 24h vector
          calldos   SETINT
DEExit:
          pop       ds
          assume    ds:DGROUP		; fix assumes

          exit


          ;
          ; GETERROR - Get a DOS error message
          ;
          ; On entry:
          ;         Argument on stack using pascal calling convention:
          ;            int errnum:      error number
          ;            char *buf:       error output buffer
          ;
          ; On exit:
          ;         AX = address of error message (buf argument)
          ;         CX destroyed
          ;         All other registers and interrupt state unchanged
          ;

          entry     GetError,argframe,far   ;set up entry point

          argW      buf                 ;buffer address
          argW      errnum              ;error number

          pushm     bx,si,di,ds         ;save registers

          mov       di,buf              ;get buffer address
          push      ds                  ;save DS for buffer segment
          loadseg   es,MSG_SEG,ax	;get message segment address
          lds       si,es:CritTAdr      ;get critical error table address
          assume    ds:nothing          ;fix assumes
          pop       es                  ;get buffer segment in ES
          mov       ax,errnum           ;get error number
          call      ErrMsg              ;get critical error message
          xor       al,al               ;get null
          cld                           ;move goes forward
          stosb                         ;store null at end of message
          mov       ax,buf              ;set return value
          popm      ds,di,si,bx         ;restore registers
          assume    ds:DGROUP		;fix assumes

          exit                          ;all done


          ;
          ; GetDOSVersion - Get real DOS version information
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         __osmajor, __osminor, and _gnOSFlags set for this DOS
          ;           version
          ;         AX, CX, DX, ES destroyed
          ;         All other registers and interrupt state unchanged
          ;

DOS_DR    equ       80h                 ;DR-DOS bit in _gnOSFlags high byte
DOS_OS2   equ       40h                 ;OS/2 bit in _gnOSFlags high byte
                                        ;  machine

          entry     GetDOSVersion,noframe,far

          pushm     bx,si,di,bp         ;save registers

          calldos   VERSION             ;get DOS version
          mov       bptr __osmajor,al   ;save major version
          mov       bptr __osminor,ah   ;save minor version
          mov       bptr _gchMajor,al   ;save true major version
          mov       bptr _gchMinor,ah   ;save true minor version
          mov       _gnOSFlags,0        ;clear OS flags
          mov       ax,4452h            ;get DR-DOS inquiry flag
          calldos                       ;get DR-DOS version
          mov       bx,ax               ;copy result
          and       bx,0FBE0h           ;ignore low 5 bits
          cmp       bx,1060h            ;is it 106xh or 107xh?
           jne      ChkOS2              ;if not go look for OS/2 1.x
          bset      <bptr _gnOSFlags+1>,DOS_DR  ;set DR-DOS flag
          and       ax,01Fh             ;check real version
          mov       bx,offset DRVerTab  ;point to table
          ;
DRVTLoop: cmp       al,[bx]             ;check if we found the version
           jbe      DRGotVer            ;if so go on
          add       bx,3                ;skip to next table entry
          jmp       short DRVTLoop      ;and loop back
          ;
DRGotVer: mov       bx,1[bx]            ;get real version number
          jmp       short StoreVer      ;and go on
          ;
ChkOS2:   mov       bl,bptr _gchMajor   ;get current major version
          cmp       bl,10               ;is it OS/2 1.x?
          je        AdjOS2              ;if so skip DOS 5 check and do
                                        ;  OS/2 adjustment
          xor       bx,bx               ;clear BX for 3306 call
          mov       ax,3306h            ;check DOS 5, get real version
          calldos                       ;do the call
          or        bx,bx               ;was it DOS 5?
           jz       GVDone              ;if not go on
          mov       bptr __osmajor,bl   ;save major version
          mov       bptr __osminor,bh   ;save minor version
          or        _gnOSFlags,dx       ;save flags

StoreVer: mov       bptr _gchMajor,bl   ;save true major version
          mov       bptr _gchMinor,bh   ;save true minor version

AdjOS2:   mov       bh,10               ;get 10 for compare and divide
          cmp       bl,bh               ;is it OS/2?
           jb       GVDone              ;if not get out
          bset      <bptr _gnOSFlags+1>,DOS_OS2   ;set OS/2 flag
          mov       al,bl               ;copy version
          xor       ah,ah               ;clear high byte
          div       bh                  ;divide by 10
          mov       bptr _gchMajor,al   ;save as real version

;FIXME - switch 2.30 to 3.0, 2.40 to 4.0

GVDone:   popm      bp,di,si,bx         ;restore registers

          exit                          ;all done


          ;
          ; reset_disks - flush buffers & reset disk drives
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;

          entry	reset_disks,noframe,far

          mov       ah,0Dh
          calldosW
          exit


          ;
          ; SDFlush -- Flush SMARTDRV and compatible disk caches
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, CX, DX, ES destroyed
          ;         All other registers and interrupt state unchanged
          ;

          entry     SDFlush,noframe,far

        push    bx
	push	bp		;save register for SMARTDRV / NCACHE bug
	;
SDLoop:	mov	ax, 4A10h	;SMARTDRV code
	xor	bx, bx		;install check / status function
	int	2Fh		;is SMARTDRV there?
	cmp	ax, 0BABEh	;if so it returns BABE
	 jne	SDDone		;if not there get out
	or	cx, cx		;any dirty blocks?
	 jz	SDDone		;if not get out
	mov	ax, 4A10h	;SMARTDRV code
	mov	bx, 1		;commit function
	int	2Fh		;commit dirty blocks
	jmp	SDLoop		;loop (may not all commit on one call)
	;
SDDone:	pop	bp		;restore register for SMARTDRV / NCACHE bug
	pop	bx
          exit                  ;all done


	;==================================================================
	;
	;  OPERATING SYSTEM INTERFACE ROUTINES (files)
	;
	;==================================================================

          ;
          ; ForceDelete - Force a non-recoverable delete in OS/2 2.x
          ;
          ; On entry:
          ;         Arguments on stack:
          ;           char *Target
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     ForceDelete,argframe,far

          argW      Target

          pushm     bx,di,si

          mov       ax,06400h
          mov       bx,0CBh		; ordinal
          mov       cx,0636Ch
          mov       dx,Target
          calldosW
          jc        FDError
          xor       ax,ax
FDError:
          popm      si,di,bx

          exit                          ; all done


          ;
          ; SetIOMode - get raw or cooked mode on file handle
          ;
          ; On entry:
          ;         Arguments on stack:
          ;           unsigned int fd:   file handle
          ;           int mode - 0 for binary, 1 for ASCII
          ;

          entry     SetIOMode,argframe,far

          argW      mode
          argW      fd

	  push	    bx
          mov       ax,04400h
          mov       bx,fd
          calldosW
          xor       dh,dh
          or        dl,020h		; binary (raw) mode
          mov       ax,04401h
          mov       bx,fd
          cmp       word ptr mode,0
          je        raw_mode
          and       dl,08Fh		; ASCII mode
raw_mode:
          calldosW
	  pop	    bx
          exit


          ;
          ; _dos_createEA - create a new file with Extended Attributes
          ;
          ; On entry:
          ;         int flag
          ;		if == 0, we're about to display the prompt
          ;		if == 1, we're about to accept keyboard input
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     _dos_createEA,argframe,far

          Argw      eaop
          Argw      attribute
          Argw      outfile
          Argw      outname
          Argw      infile
          Argw      inname

          push      bx
          push      di
          push      si

          push      ds
          pop       es
          mov       di,eaop		; ES:DI = EAOPT structure
          mov       si,[inname]		; DS:SI = ASCIIZ filename
          mov       ax,05702h
          mov       bx,infile
          mov       cx,000Ch
          mov       dx,0004h		; get all EAs
          calldosW
          jc        ea_copy_error

          mov       si,[outname]	; DS:SI = ASCIIZ filename
          mov       ax,06C01h		; open file with EAs
          mov       bx,00100010b 	; read/write
          mov       cx,attribute
          mov       dx,00010010b
          calldosW
ea_copy_error:
          pop       si
          pop       di
          jc        ea_copy_bye

          mov       bx,outfile
          mov       [bx],ax		; save file handle
          xor       ax,ax
ea_copy_bye:
	  pop	    bx
          exit


          ;
          ; QueryIsConsole - test to see if Handle is the console (redir test)
          ;
          ; On exit:
          ;         AX = 0 if redirected, 1 if at the console
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryIsConsole,argframe,far

          argW      Handle

          push	    bx
          mov       ax,04400h		; IOCTL call for console status
          mov       bx,Handle
          calldosW
          mov       ax,0
          jc        not_console
          and       dx,083h
          cmp       dx,083h
          jne       not_console
          inc	    ax

not_console:
          pop	    bx
          exit
	

	;==================================================================
	;
	;  HARDWARE / BIOS INTERFACE ROUTINES (except video)
	;
	;==================================================================

          ;
          ; QueryMouseReady - test for mouse driver
          ;
          ; On exit:
          ;         AX = 0 if no mouse, 1 if present
          ;

          entry     _QueryMouseReady,noframe,far

          push      bx
          mov       ax,015h
          xor       bx,bx
          int       033h		; if mouse loaded, BX != 0
          xor       ax,ax
          or        bx,bx
          jz        no_mouse
          mov       ax,1
no_mouse:
          pop       bx
          exit


          ;
          ; QueryDriveRemovable - test if specified drive is removable
          ;
          ; On entry:
          ;         Argument on stack:
          ;           int drivespec:   drive to check (1 - A, 2 - B, etc.)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryDriveRemovable,argframe,far

          ArgW      drivespec

          push      bx
          mov       bx,drivespec

          mov       ax,04408h		; IOCTL check if block dev is removable
          calldosW
          jc        is_fixed		; error - assume fixed
          or        al,al		; 0 - removable, 1 - fixed
          jne       is_fixed
is_removable:
          mov       ax,1		; set removable flag
          jmp       short dr_bye
is_fixed:
          xor       ax,ax		; clear removable flag
dr_bye:
          pop       bx
          exit


          ;
          ; QueryDriveReady - test if specified drive is ready
          ;
          ; On entry:
          ;         Argument on stack:
          ;           int drivespec:   drive to check (1 - A, 2 - B, etc.)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryDriveReady,argframe,far

          ArgW      drivespec

          push      bx
          mov       ax, 2
          push      ax
          call      DosError		; disable error popup

          mov       dx, drivespec
          mov       ah, 36h
          calldosW
          cmp       ax, 0FFFFh
          jne       no_error

ready_error:
          xor       ax, ax
          jmp       short not_ready
no_error:
          mov       ax, 1
not_ready:
          push      ax			; save result (!= 0 if successful)

          mov       ax, 1
          push      ax
          call      DosError		; enable error popup

          pop       ax
          pop       bx
          exit


          ;
          ; QueryDriveExists - test if specified drive exists
          ;
          ; On entry:
          ;         Argument on stack:
          ;           int drivespec:   drive to check (1 - A, 2 - B, etc.)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryDriveExists,argframe,far

          argW      drivespec

          push      bx
          mov       bx,drivespec
          xor       dx,dx		; clear attribute word
          mov       ax,04409h		; IOCTL check if block dev is remote
          calldosW
          jnc       d_e_exists		; no error - assume drive exists
          cmp       al,15
          jne       d_e_exists		; if not "invalid drive" assume exists
          xor       ax,ax		; clear exists flag
          jmp       short d_e_bye
d_e_exists:
          mov       ax,1		; set exists flag
d_e_bye:
          pop       bx
          exit


          ;
          ; QueryDriveRemote - test if specified drive is remote
          ;
          ; On entry:
          ;         Argument on stack:
          ;           int drivespec:   drive to check (1 - A, 2 - B, etc.)
          ;
          ; On exit:
          ;         AX = -1 for invalid drive, 0 for local, 1 for remote
          ;         DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryDriveRemote,argframe,far

          ArgW      drivespec

          push      drivespec
          call      QueryDriveExists

          or        ax,ax		; if AX = 0, drive doesn't exist
          jz        is_remote

          and       dx,1000h		; test remote bit
          jnz       is_remote
          xor       ax,ax
is_remote:

          exit

          ;
          ; QueryIsCDROM - test if specified drive is a CD-ROM
          ;
          ; On entry:
          ;         Argument on stack:
          ;           char *drivespec:   drive to check (1 - A, 2 - B, etc.)
          ;
          ; On exit:
          ;         AX = -1 for invalid drive, 0 for local, 1 for remote
          ;         BX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryIsCDROM,argframe,far

          ArgW      driveptr

          push      bx
	mov	bx,driveptr		;point to drive string
	mov	cl,[bx]		;get drive letter
	cmp	cl,'a'		;lower case?
	jb	cd_drive		;if not go on
	cmp	cl,'z'		;check high end
	ja	cd_drive		;if not LC go on
	sub	cl,32		;make it upper case
cd_drive:
	sub	cl,'A'		;convert to drive value
	xor	ch,ch		;clear high byte
	mov	ax,0150Bh		;MSCDEX call
	xor	bx,bx
	int	2Fh			;returns AX=0 if drive not supported
	cmp	bx,0ADADh
	je	is_cdrom
	xor	ax,ax
is_cdrom:
	or	ax,ax
	jz	cd_bye
	mov	ax,1
cd_bye:
          pop       bx
          exit

          ;
          ; QueryPrinterReady  - test if the specified printer is ready
          ;
          ; On entry:
          ;         int lpt_num = printer number (1-n)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryPrinterReady,argframe,far

          argW      lpt_num

          push      bx
          mov	ah,02h
          mov	dx,lpt_num
          int	17h			; get printer status
          cmp	ah,090h
          mov	bl,ah
          mov	ax,1
          cmp	bl,090h		; if printer ready ah == 0x90
          je	lpt_return
          xor	al,al
lpt_return:
          pop       bx
          exit

          ;
          ; GET_CPU - get cpu type
          ;
          ; On exit:
          ;    AX = 86 if an 8088/8086
          ;        186 if an 80186/80188
          ;        200 for a NEC V20/V30
          ;        286 if an 80286
          ;        386 if an 80386
          ;        486 if an 80486
          ;        586 if a Pentium
          ;        686 if a Pentium Pro
          ;    CX destroyed, all other registers unchanged, interrupts on
          ;

          entry     _get_cpu,noframe,far

CPUID     MACRO
          db	0Fh		; hardcoded opcode for CPUID instruction
          db	0A2h
ENDM

          push      bx

          pushf				; put flags register onto the stack
          xor       ax,ax
          push      ax			; put AX on the stack
          popf				; bring it back in flags
          pushf				; try to set bits 12 thru 15 to a zero
          pop       ax			; get back Ur flags word in AX
          and       ax,0F000h		; if bits 12 thru 15 are set, then it's
          cmp       ax,0F000h		;   an 808x, 8018x or NEC V20/V30
          je        old_chip

          mov       ax,07000h		; set FLAG bits 12 thru 14 - NT, IOPL
          push      ax			; put it onto the stack
          popf				;   and try to put 07000H into flags
          pushf				; push flags again
          pop       ax			;   and bring back AX for a compare
          and       ax,07000h		; if bits 12 thru 14 are set
          mov       ax,286		;   then it's an 80386
          jz        cpu_bye

try_386:
          .386

          mov       ecx,esp		; save ESP
          mov       SaveESP,ecx
          and       esp,0FFFFFFFCh	; zero lower 2 bits to avoid
                                        ;   AC fault on 486
          pushfd			; save EFLAGS
          pushfd			; push EFLAGS
          pop       eax			; EAX = EFLAGS
          mov       edx,eax		; EDX = EFLAGS
          xor       eax,40000h		; toggle AC bit(bit 18) in EFLAGS
          push      eax			; push new value
          popfd				; put it in EFLAGS
          pushfd			; push EFLAGS
          pop       eax			; EAX = EFLAGS
          and       eax,40000h		; isolate bit 18 in EAX
          and       edx,40000h		; isolate bit 18 in EDX
          cmp       eax,edx		; are EAX and EDX equal?
          je        short got_386	;   yes - it's a 386

; it's a 486 or Pentium

          pushfd			; get current flags
          pop       eax
          mov       edx,eax
          xor       eax,EF_ID		; try to toggle ID bit
          push      eax
          popfd
          pushfd			; get new EFLAGS
          pop       eax

          and       eax,EF_ID		; if we can't toggle ID, it's a 486
          and       edx,EF_ID
          cmp       eax,edx
          je        short got_486

          mov       eax,1
          CPUID
          and       ax,0F00h		; mask everything except family field
          cmp       ax,0500h		; get Family field (5=Pentium)
          jb        got_486

          cmp       ax, 500h
          jbe       got_586

          mov       ax,686
          jmp       short restore_486
got_586:
          mov       ax,586
          jmp       short restore_486

got_486:
          mov       ax,486
          jmp       short restore_486

got_386:
          mov       ax,386		; it's an 80386

restore_486:
          popfd                         ; restore EFLAGS
          mov       ecx,SaveESP
          mov       esp,ecx		; restore ESP
          jmp       short cpu_bye

          .8086

old_chip:
          mov       ax,0FFFFh		; load up AX
          mov       cl,33		; this will shift 33 times if it's an
					;   8088/8086, or once if 80188/80186
          shl       ax,cl		; Shifting 33 should zero all bits
          mov       ax,186
          jnz       short cpu_bye

not_186:
          xor       al,al		; set ZF
          mov       al,40h		; mul on NEC does NOT affect ZF
          mul       al			;   but on 8086/88, ZF gets thrown
          jz        got_nec
          mov       ax,86		; it's an 8088/8086
          jmp       short cpu_bye

got_nec:
          mov       ax,200		; it's a NEC V20/V30
cpu_bye:
          popf
          pop       bx

          exit


          ;
          ; GET_NDP - get ndp type
          ; Must assemble with the /R option set in MASM
          ;
          ; On exit:
          ;    AX = 0 if no ndp
          ;        87 if an 8087
          ;        287 if an 80287
          ;        387 if an 80387
          ;
          ;  All other registers unchanged, interrupts on
          ;

          entry     _get_ndp,noframe,far

          fninit			; try to initialize NDP
          mov       byte ptr StatusNDP+1,0  ; clear memory byte
          fnstcw    StatusNDP		; put control word in mem
          mov       ah,byte ptr StatusNDP+1
          cmp       ah,03h		; if AH is 3, NDP is present
          je        chk_87		;   found NDP
          xor       ax,ax		; clear AX to show no NDP
          jmp       short bye_ndp

; 'got an 8087 ?
chk_87:
          and       StatusNDP,NOT 0080h	; turn ON interrupts (IEM=0)
          fldcw     StatusNDP		; load control word
          fdisi				; turn OFF interrupts (IEM=1)
          fstcw     StatusNDP		; store control word
          test      StatusNDP,0080h	; if IEM=1, NDP is 8087
          jz        chk287
          mov       ax,087
          jmp       short bye_ndp
chk287:
          finit				; set default infinity mode
          fld1				; make infinity
          fldz				;     by dividing
          fdiv				;         1 by zero !!
          fld       st			; now make negative infinity
          fchs
          fcompp			; compare two infinities
          fstsw     StatusNDP		; if, for 8087 or 80287
          fwait				; 'til status word is put away
          mov       ax,StatusNDP	; get control word
          sahf				; put AH into flags
          mov       ax,0287
          jz        short bye_ndp
          mov       ax,0387		; must be a 80387
bye_ndp:
          exit

          ;
          ; GET_EXPANDED - get amount of expanded memory
          ;
          ; On entry:
          ;         Argument on stack:
          ;           unsigned int *emsfree:   pointer to free ems page count
          ;
          ; On exit:
          ;         AX = pages (16K) of expanded memory
          ;         DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     get_expanded,argframe,far

          argW      emsfree             ;one argument, point to free page cnt

          pushm     bx,si,di            ;save registers
          mov       al,67h              ;get EMS interrupt number
          calldos   GETINT              ;get interrupt vector in es:bx
          lea       si,emm_name         ;point to driver name for comparison
          mov       di,10               ;address of name field in dev header
          mov       cx,8                ;comparison length
          cld                           ;move forward
          repe      cmpsb               ;does driver name match?
          jne       noems               ;no, no EMS
          callems   UPCNT               ;get ems page counts
          or        ah,ah               ;any error?
          jnz       noems               ;if error, return no ems
          mov       si,emsfree          ;get address of free page count
          mov       [si],bx             ;store free page count
          mov       ax,dx               ;get total page count
          jmp       short gxret         ;and return
noems:
          xor       ax,ax               ;clear total page count
gxret:
          popm      di,si,bx            ;restore registers
          exit

          ;
          ; GET_EXTENDED - get amount of extended memory
          ;
          ; On exit:
          ;         AX = # of 1K blocks of extended memory
          ;         All other registers unchanged, interrupts on
          ;

          entry     _get_extended,noframe,far

          cmp       __osmajor,10
          jae       got_88
          call      _get_cpu
          cmp       ax,286		; is it an 86/88/186/188/V20/V30?
          jb        got_88		;   yup, so no extended mem
          mov       ax,08800h           ; get interrupt number
          int       15h
          cmp       ah,080h
          jb        got_ext
got_88:
          xor       ax,ax		; probably not really extended!
got_ext:
          exit

          ;
          ; GET_XMS - get amount of XMS memory
          ;
          ; On entry:
          ;       int *hma_status
          ;
          ; On exit:
          ;         AX = # of 1K blocks of extended memory
          ;         DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     get_xms,varframe,far

          argW      hma_state
          varD      XMSControl		; address of XMS driver
          varend

          push      bx
          call      _get_cpu
          cmp       ax,286		; is it an 86/88/186/188/V20/V30?
          jb        not_286

          mov       ax,XMSTEST
          int       2Fh
          cmp       al,XMSFLAG
          je        got_xms
not_286:
          xor       ax,ax
          jmp       short no_xms
got_xms:
          mov       ax,XMSADDR
          int       2Fh
          mov       word ptr [XMSControl],bx
          mov       word ptr [XMSControl+2],es

          mov       ah,1		; get HMA availability
          mov       dx,0FFFFh
          call      dword ptr [XMSControl]

          mov       ah,al
          mov       al,bl		; save error code
          or        ah,ah
          jz        no_hma

          push      ax
          mov       ah,2		; release the HMA
          call      dword ptr [XMSControl]
          pop       ax
no_hma:
          mov       bx,hma_state
          mov       [bx],ax

          mov       ah,8		; query free extended memory
          call      dword ptr [XMSControl]
          mov       ax,dx               ; copy total free to AX
no_xms:
          pop       bx
          exit

          ;
          ; BIOS_KEY - get key from the BIOS (for LIST /S)
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     _bios_key,noframe,far

          push      bx
          xor       ax,ax
          int       16h
          mov       bh,ah
          xor       ah,ah
          or        al,al
          jnz       not_special
          mov       al,bh		; set extended scan code
          add       ax,0100h
not_special:
          pop       bx
          exit                          ; all done

          ;
          ; DosBeep - beep the speaker with the specified tone & duration
          ;
          ; On entry:
          ;       int frequency
          ;       int duration (in 1/18th second increments)
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     DosBeep,argframe,far

          argW      duration            ; length of time (in 1/18th increments)
          argW      freq                ; frequency (in Hz)

          push      bx
          sti
          xor       bx,bx		; clear result flag
          mov       ax, 040h		; point ES to ROM BIOS data area
          mov       es, ax
          mov       cx,freq
          cmp       cx,20		; anything less than 20 Hz creates
          jb        beep_get_time	;   a divide overflow

          mov       dx,012h
          mov       ax,034DCh
          div       cx
          push      ax
          mov       al, 10110110b	; select 8253
          out       43h, al
          jmp       $+2
          pop       ax
          out       42h, al		; low byte of divisor
          jmp       $+2
          xchg      ah, al
          out       42h, al		; high byte of divisor
          jmp       $+2

; Wait for desired duration by monitoring time-of-day 18 Hz clock
          in        al, 61h		; get current value of control bits
          jmp       $+2
          or        al, 3
          out       61h, al		; turn speaker on

beep_get_time:
          mov       dx, es:[06Ch]       ; save original tick count
beep_loop:
          sti		   	; because programs keep doing CLIs!!
          cmp       word ptr duration,1 ; if only waiting 1 tick, don't check
          je        no_char		;   for a key

          mov       ah,0Fh		; this silly kludge is to work around
          int       10h			;   a bug in the OS/2 2.0 DOS box

          mov       ah,01h		; check for ^C or ^BREAK
          int       16h
          jz        no_char
          or        ax,ax
          jz        beep_ctrlc		; ^BREAK waiting
          cmp       al,3		; ^C waiting?
          jne       no_char
beep_ctrlc:
          mov       bx,3
          jmp       short beep_off

no_char:
	xor	bx, bx		; force 0 return if we are done
          mov       ax, es:[06Ch]
          sub       ax, dx              ; get time elapsed
          cmp       ax,duration         ; now check duration
          jae       beep_off            ; loop if not there yet

          cmp       word ptr freq,20	; check if DELAY'ing
          jae       beep_loop
          cmp       word ptr duration,6	; don't give up timeslice for short wait
          jbe       beep_loop
          mov       ax,1680h
          int       2Fh			; give up Win / OS/2 timeslice
          cmp       al,80h
          jne       beep_loop
          int       28h			; 2Fh 1680h not supported; call DOS idle
          jmp       short beep_loop

beep_off:
          cmp       cx,20
          jb        beep_bye

          in        al,61h
          jmp       $+2
          and       al,11111100b	; turn speaker off
          out       61h, al

beep_bye:
          mov       ax,bx
          pop       bx
          exit
	
	;==================================================================
	;
	;  HARDWARE / BIOS INTERFACE ROUTINES (video)
	;
	;==================================================================

          ;
          ; GetCurPos - get the cursor position
          ;
          ; On entry:
          ;       int *row
          ;       int *column
          ;
          ; On exit:
          ;         BX, DX, destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     GetCurPos,argframe,far

          argW      gc_column
          argW      gc_row

          push      bx
          push      bp			; for PC1 bug
          mov       ah,0Fh
          int       10h			; get current page into BH
          mov       ah,03h
          int       10h
          pop       bp
          xor       ax,ax

          mov       al,dh
          mov       bx,gc_row
          mov       [bx],ax

          mov       al,dl
          mov       bx,gc_column
          mov       [bx],ax

          pop       bx
          exit

          ;
          ; SetCurPos - position the cursor
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row
          ;            int column
          ;
          ; On exit:
          ;         AX, BX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     SetCurPos,argframe,far

          argW      s_column
          argW      s_row

          push      bx
          mov       ah,0Fh		; get video mode (for page # in BH)
          push      bp			; save BP for PC1 BIOS bug
          int       10h
          pop       bp
          mov       dh,s_row
          mov       dl,s_column
          mov       ah,2
          int       10h
          pop       bx

          exit

          ;
          ; GETVIDEOMODE - get the video adapter and monitor type
          ;
          ; On exit:
          ;         Returns monitor type:
          ;            0 = monochrome
          ;            1 = cga
          ;            2 = ega w/mono monitor
          ;            3 = ega w/color monitor
          ;            4 = vga w/mono monitor
          ;            5 = vga w/color monitor
          ;
          ;         AX, BX, CX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     GetVideoMode,noframe,far

          push      bx
          push      bp			; save BP for old BIOS bug
          mov       ax,01A00h		; function 1A returns active adapter
          int       10h			; al will return as 1a if supported
          cmp       al,01ah
          jnz       no_dc
          cmp       bl,7		; monochrome VGA?
          jz        mono_vga
          cmp       bl,8		; color VGA?
          jz        color_vga
          mov       bl,4		; color EGA?
          jz        color_ega
          mov       bl,5		; monochrome EGA?
          jz        mono_ega

no_dc:
          mov       ah,12h		; Get information about the EGA
          mov       bl,10h
          int       10h
          cmp       bl,10h		; if it came back as 10h (no EGA),
          je        invalid		;   skip next test

          push      ds
          mov       ax,BIOS_RAM		; BIOS RAM area
          mov       ds,ax
          assume    ds:BIOS_RAM
          mov       bl,info		; get information byte
          pop       ds
          assume    ds:DGROUP

          test      bl,8		; is the EGA active (bit 3 == 0)?
          jnz       invalid
          cmp       bh,1		; monochrome monitor?
          jnz       color_ega
mono_ega:
          mov       ax,2
          jmp       short got_video
color_ega:
          mov       ax,3
          jmp       short got_video
mono_vga:
          mov       ax,4
          jmp       short got_video
color_vga:
          mov       ax,5
          jmp       short got_video

invalid:
          mov       ah,0Fh		; get video mode
          int       10h
          cmp       al,7
          mov       ax,1
          jnz       got_video
          xor       ax,ax
got_video:
          pop       bp
          pop       bx

          exit

          ;
          ; GetCellSize - get the cursor cell size
          ;
          ; On entry:
          ;       int cursor_type
          ;
          ; On exit:
          ;         AX, BX, DX, destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     GetCellSize,argframe,far

          argW      cursor_type

          push      bx
          push      bp			; for PC1 bug
          mov       ah,0Fh
          int       10h			; get current page into BH
          push      ax
          mov       ah,03h
          int       10h
          pop       ax

          cmp       ch,020h		; if start line >= 32, reset it
          jae       is_invisible
          cmp       cl,5		; check for cursor in the middle of
          jb        is_invisible	;   the character cell
          cmp       ch,cl		; if start > end, something is wacky!
          jbe       not_invisible
is_invisible:
          mov       cx,0607h		; wake up!
not_invisible:
          cmp       al,7		; get video mode & test for BIOS bug on
          jne       not_mono		;   old PC - mono mode & cursor 6,7
          cmp       cx,0607h
          jne       not_mono
          mov       cx,0B0Ch		; set mono cursor to 11,12
not_mono:
          mov       ax,cursor_type	; set start & end rows by rounding
          mul       cl			;   off the value in CursO or CursI
          add       ax,49
          mov       dl,100
          div       dl
          mov       ch,cl
          sub       ch,al

          mov	    ax,cx		; return shape in AX
          pop       bp
          pop       bx

          exit

          ;
          ; GetAtt - get normal & inverse attributes at cursor position
          ;
          ; On entry:
          ;       int *normal
          ;       int *inverse
          ;
          ; On exit:
          ;         DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     GetAtt,argframe,far

          argW      inverse
          argW      normal

          push      bx
          push      bp			; for PC1 bug
          mov       ah,0Fh
          int       10h			; get current page into BH
          mov       ah,8
          int       10h			; get character & attribute
          pop       bp

          mov       al,ah		; save normal
          xor       ah,ah
          or        al,al		; if black on black, change to white
          jnz       not_black
          mov       al,7
not_black:
          mov       bx,normal		; save normal attribute
          mov       [bx],ax

          and       ax,077h		; remove blink & intensity bits
          mov       cl,4		; ROR 4 to get inverse
          ror       al,cl

          mov       bx,inverse
          mov       [bx],ax		; save inverse attribute

          pop       bx
          exit

          ;
          ; Scroll - scroll or clear the window
          ;
          ; On entry:
          ;         Arguments are:
          ;            int upper row
          ;            int left column
          ;            int lower row
          ;            int right column
          ;            int scroll mode (-1 = down, 0 = clear window, 1 = up)
          ;            int attribute
          ;
          ; On exit:
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     Scroll,argframe,far

          argW      w_attrib
          argW      w_mode
          argW      w_rcol
          argW      w_lrow
          argW      w_lcol
          argW      w_urow

          push      bx
          mov       ch,w_urow
          mov       cl,w_lcol
          mov       dh,w_lrow
          mov       dl,w_rcol
          mov       bh,w_attrib

          cmp       word ptr w_mode,0
          jge       scroll_up
          mov       ax,0700h
          sub       ax,w_mode
          jmp       short scroll_it
scroll_up:
          mov       ah,06h
          mov       al,w_mode
scroll_it:
          pushm     bp, ds, es, di, si
          int       10h
          popm      si, di, es, ds, bp

          pop       bx
          exit

          ;
          ; SetBorderColor - set the border color
          ;
          ; On entry:
          ;         Arguments are:
          ;            int b_color
          ;
          ; On exit:
          ;         AX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     SetBorderColor,argframe,far

          argW      b_color

          push      bx
          push      bp			; save BP for old PC BIOS bug
          mov       ah,0Bh		; first try it for CGA / MCGA
          mov       bx,b_color
          int       10h

          mov       ax,1001h		; EGA/VGA use different registers
          mov       bx,b_color
          mov       bh,bl
          int       10h
          pop       bp
          pop       bx

          exit

          ;
          ; SetLineColor - Rewrite the line attributes directly
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row
          ;            int column
          ;            int length
          ;            int sa_attrib
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;
          entry     SetLineColor,argframe,far

          argW      la_attrib
          argW      la_length
          argW      la_column
          argW      la_row

          pushm     bx,di

          mov       dh,la_row           ; get row
          mov       dl,la_column        ; get column
          call      initpchr            ; initialize the display routines

          cmp       BIOSOutput,0	; BIOS output requested?
          je        SLCNoBios
          call      GetCursor		; save current cursor location
          push      dx
SLCNoBios:
          mov       ax, la_attrib	; set attribute
          mov       cx, la_length
          call      chg_colors

          cmp       BIOSOutput,0	; BIOS output requested?
          je        SLCExit
          pop       dx
          call      SetCursor		; restore cursor location
SLCExit:
          popm      di,bx

          exit

          ;
          ; SetScrColor - Rewrite the screen attributes directly
          ;
          ; On entry:
          ;         Arguments are:
          ;            int sa_attrib
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;
          entry     SetScrColor,argframe,far

          argW      sa_attrib
          argW      sa_Columns
          argW      sa_Rows

          pushm     bx, di

          xor       dx,dx		; start at 0,0
          call      initpchr		; screen buffer returned in ES:DI

          cmp       BIOSOutput,0	; BIOS output requested?
          je        SSCNoBios
          call      GetCursor		; save current cursor location
          push      dx

SSCBios:
          mov       ax,sa_attrib        ; set attribute
          mov       cx,sa_Columns
          call      chg_colors
          inc       BIOSRow
          mov       dx,sa_Rows
          cmp       dl,BIOSRow
          jae       SSCBIOS

          pop       dx
          call      SetCursor		; restore original cursor position

          jmp       short SSCExit

SSCNoBios:
          mov       cx,sa_Rows
          inc       cx
          mov       ax,sa_Columns
          mul       cx
          mov       cx,ax		; CX = total words in screen buffer

          mov       ax,sa_attrib        ; set attribute
          call      chg_colors
SSCExit:
          popm      di,bx

          exit

          ;
          ; ReadCellStr - read the characters and attributes at the 
          ;   specified location for the specified length
          ;
          ; On entry:
          ;         Arguments are:
          ;            char _far *rc_string
          ;            int rc_length
          ;            int rc_row
          ;            int rc_column
          ;
          ; On exit:
          ;         Returns nothing
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     ReadCellStr,argframe,far

          argW      rc_column
          argW      rc_row
          argW      rc_length
          argD      rc_string

          pushm     bx, ds, di, si

          mov       dh,rc_row
          mov       dl,rc_column
          call      initpchr

          mov       cx,rc_length
;          shr       cx,1                ; convert from byte to cell count

          cmp       BIOSOutput,0	; BIOS output requested?
          je        prloop

; BIOS code!

          les       di,rc_string

          push      bp			; save BP for old PC BIOS bug

          call      GetCursor		; save current cursor location
          push      dx
rbloop:
          mov       dh,BIOSRow
          mov       dl,BIOSCol
          call      SetCursor

          mov       ah,8
          mov       bh,BIOSPage
          int       10h			; get char & att at cursor

          mov       es:[di],ax
          add       di,2
          inc       BIOSCol
          loop      rbloop

          pop       dx
          call      SetCursor		; reset saved current cursor location

          pop       bp
          jmp       short rc_exit

prloop:
          mov       dx,StatusVid        ; get 6845 status register address
          lds       si,rc_string
rloop:
          or        dx,dx               ;  check for mono, EGA, or VGA
          jz        readit              ; if so, skip retrace wait

rtend:    in        al,dx               ; wait for horizontal retrace end
          shr       al,1                ; check retrace bit
          jc        rtend               ; wait for it to go off
          cli                           ; no interrupts while we wait for it 
                                        ;  to go back on
rtstart:  in        al,dx               ; wait for horizontal retrace start
          shr       al,1                ; check retrace bit
          jnc       rtstart             ; wait for it to go on

readit:   mov       ax,es:[di]          ; read from display
          sti                           ; enable interrupts again
          mov       ds:[si],ax
          add       si,2
          add       di,2
          loop      rloop

rc_exit:
          popm      si, di, ds, bx

          exit

          ;
          ; WriteCellStr - write the characters and attributes at the 
          ;   specified location for the specified length
          ;
          ; On entry:
          ;         Arguments are:
          ;            char _far *fptr
          ;            int length
          ;            int row
          ;            int column
          ;
          ; On exit:
          ;         Returns nothing
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     WriteCellStr,argframe,far

          argW      wc_column
          argW      wc_row
          argW      wc_length
          argD      wc_string

          pushm     bx, ds, di, si

          mov       dh,wc_row
          mov       dl,wc_column
          call      initpchr

          mov       cx,wc_length
;          shr       cx,1		; convert from byte to cell count

          cmp       BIOSOutput,0	; BIOS output requested?
          je        pwcloop

; BIOS output code!
          call      GetCursor		; save current cursor location
;          push      dx

          les       di,wc_string

          push      bp			; save BP for old PC BIOS bug
wcbloop:
          push      cx
          mov       dh,BIOSRow
          mov       dl,BIOSCol
          call      SetCursor

          mov       bx,es:[di]		; get character & attribute to write
          mov       al,bl
          mov       bl,bh		; attribute
          mov       bh,BIOSPage
          mov       ah,09h
          mov       cx,1
          int       10h			; write char & att at cursor

          add       di,2
          inc       BIOSCol
          pop       cx
          loop      wcbloop

          pop       bp

;          pop       dx
;          call      SetCursor		; restore cursor location
          jmp       short wcs_exit

pwcloop:
          mov       dx,StatusVid        ; get 6845 status register address
          lds       si,wc_string
wcloop:
          mov       bx,ds:[si]		; get character to write
          or        dx,dx               ;  check for mono, EGA, or VGA
          jz        wcwrite             ; if so, skip retrace wait

wcend:    in        al,dx               ; wait for horizontal retrace end
          shr       al,1                ; check retrace bit
          jc        wcend               ; wait for it to go off
          cli                           ; no interrupts while we wait for it 
                                        ;  to go back on
wcstart:  in        al,dx               ; wait for horizontal retrace start
          shr       al,1                ; check retrace bit
          jnc       wcstart             ; wait for it to go on

wcwrite:
          mov       es:[di],bx          ; write to display
          sti                           ; enable interrupts again
          add       di,2
          add       si,2
          loop      wcloop

wcs_exit:
          popm      si, di, ds, bx

          exit

          ;
          ; WriteChrAtt - write one character and attribute
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row
          ;            int column
          ;            int attribute
          ;            int character
          ;
          ; On exit:
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     WriteChrAtt,argframe,far

          argW      wc_char
          argW      wc_attrib
          argW      wc_column
          argW      wc_row

          pushm     bx, di

          mov       dh,wc_row
          mov       dl,wc_column
          call      initpchr

          cmp       BIOSOutput,0	; BIOS output requested?
          je        wc1
          call      GetCursor		; save current cursor location
          push      dx
wc1:
          mov       ah,wc_attrib
          mov       al,wc_char
          call      putchar

          cmp       BIOSOutput,0	; BIOS output requested?
          je        wca_exit
          pop       dx
          call      SetCursor		; reset saved current cursor location

wca_exit:
          popm      di, bx

          exit


          ;
          ; WriteStrAtt - write a string & attribute directly to display memory
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row:         row for output
          ;            int column:      column for output
          ;            int attribute:   attribute output
          ;            char *string:    string to display
          ;
          ; On exit:
          ;         AX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     WriteStrAtt,argframe,far

          argW      string              ;string address
          argW      attrib              ;display attribute
          argW      ocolumn             ;column number
          argW      orow                ;row number

          pushm     bx,si,di		;save registers

          mov       dh,orow             ;get row
          mov       dl,ocolumn          ;get column
          call      initpchr            ;initialize the display routines
          mov       si,string           ;point to string

          cmp       BIOSOutput,0	; BIOS output requested?
          je        ploop
          call      GetCursor		; save current cursor location
          push      dx
ploop:
          lodsb                         ;get a character to display
          or        al,al               ;end of string?
          jz        pbye                ;if so we're done
          mov       ah,attrib           ;get attribute
          call      putchar             ;display the character
          jmp       short ploop
pbye:
          cmp       BIOSOutput,0	; BIOS output requested?
          je        ws_exit
          pop       dx
          call      SetCursor		; reset saved current cursor location
ws_exit:
          popm      di,si,bx            ;restore registers

          exit


          ;
          ; WriteCharStrAtt - write characters & attribute directly to display
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row:         row for output
          ;            int column:      column for output
          ;            int length:	length of "string"
          ;            int attribute:   attribute output
          ;            char *string:    string to display
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     WriteCharStrAtt,argframe,far

          argW      string              ;string address
          argW      attrib              ;display attribute
          argW      olength		;string length
          argW      ocolumn             ;column number
          argW      orow                ;row number

          pushm     bx,si,di		;save registers

          mov       dh,orow             ;get row
          mov       dl,ocolumn          ;get column
          call      initpchr            ;initialize the display routines

          mov       si,string           ;point to string
          mov       cx,olength

          cmp       BIOSOutput,0	; BIOS output requested?
          je        qloop
          call      GetCursor		; save current cursor location
          push      dx
qloop:
          lodsb                         ;get a character to display
          mov       ah,attrib           ;get attribute
          push      cx
          call      putchar             ;display the character
          pop       cx
          loop      qloop

          cmp       BIOSOutput,0	; BIOS output requested?
          je        wcsa_exit
          pop       dx
          call      SetCursor		; reset saved current cursor location
wcsa_exit:
          popm      di,si,bx            ;restore registers

          exit

          ;
          ; WriteVStrAtt - quick-print a string and attribute vertically
          ;   directly to display memory
          ;
          ; On entry:
          ;         Arguments are:
          ;            int row:         row for output
          ;            int column:      column for output
          ;            int attribute:   attribute output
          ;            char *string:    string to display
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;
          entry     WriteVStrAtt,argframe,far

          argW      v_string            ;string address
          argW      v_attrib            ;display attribute
          argW      v_column            ;column number
          argW      v_row               ;row number

          pushm     bx, si, di          ;save registers

          mov       dh, v_row           ;get row
          mov       dl, v_column        ;get column
          call      initpchr            ;initialize the display routines

          mov       si, v_string        ;point to string
          cmp       BIOSOutput,0	; BIOS output requested?
          je        vloop
          call      GetCursor		; save current cursor location
          push      dx
vloop:
          lodsb                         ;get a character to display
          or        al,al               ;end of string?
          jz        vbye                ;if so we're done
          mov       ah,v_attrib         ;get attribute
          call      putchar             ;display the character
          inc       BIOSRow
          dec       BIOSCol		;putchar INC'd BIOSCol

          cmp       BIOSOutput,0	; BIOS output requested?
          jne       vloop

          add       di,rmargin		; go to next line
          add       di,rmargin
          sub       di,2		; at the same column
          jmp       short vloop
vbye:
          cmp       BIOSOutput,0	; BIOS output requested?
          je        vs_exit
          pop       dx
          call      SetCursor		; reset saved current cursor location
vs_exit:
          popm      di, si, bx          ;restore registers

          exit

          ;
          ; WriteTTY - write a string to the display via BIOS output
          ;
          ; On entry:
          ;         Arguments are:
          ;            char *string:    string to display
          ;
          ; On exit:
          ;         AX, BX, CX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     WriteTTY,argframe,far

          argW      string              ; string address

          push      bp			; for PC1 bug
          pushm     bx, si
          mov       si,string

          mov       ah,0Fh
          int       10h			; get current page into BH
wloop:
          lodsb				; get next character
          or        al,al
          jz        write_bye
          mov       ah,0Eh
          mov       bl,7
          int       10h
          jmp       short wloop
write_bye:
          popm      si, bx
          pop       bp

          exit


          ;
          ; INITPCHR - Initialize direct video I/O
          ;
          ; On entry:
          ;         DH = screen row
          ;         DL = screen column
          ;
          ; On exit:
          ;         ES:DI = location in display buffer of specified row
          ;                 and column
          ;         AX, BX, DX, ES destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     initpchr,noframe,,local   ;set entry point

          push      bp			; save BP for old PC BIOS bug

          mov       StatusVid,0         ;clear wait for retrace (CGA) address

          xor       ax,ax               ;get zero
          mov       es,ax               ;point es to ROM BIOS data area
          mov       ax,es:[044Ah]       ;get line length
          cmp       ax, 0
          jne       ValidColumns
          mov       ax, 80
          mov       bx,_gpIniptr        ;point to INI data
          cmp       [bx].I_Columns,0
          jz        ValidColumns
          mov       ax, [bx].I_Columns
ValidColumns:
          mov       rmargin,ax          ;set right margin to line length

          mov       BIOSRow,dh
          mov       BIOSCol,dl

          mov       bx,_gpIniptr
          cmp       [bx].I_BIOS,0	; BIOS output requested?
          je        NoInitBIOS

          mov       BIOSOutput, 1
          mov       ah,0Fh
          int       10h			; get current page into BH
          mov       BIOSPage,bh
          jmp       short init_exit

NoInitBIOS:
          mov       BIOSOutput, 0
          mul       dh                  ;line length * row = total row bytes
                                        ;  in ax
          xor       dh,dh               ;clear high byte so dx = column
          add       ax,dx               ;add the number of columns
          add       ax,ax               ;* 2 for attribute byte
          add       ax,es:[044Eh]       ;add offset into screen buffer
          push      ax			;save the cursor position offset

          call      GetVideoMode
          cmp       ax, 2
          jae       vga

          mov       ax,0B000h           ;assume mono screen
          cmp       byte ptr es:[0449h],7  ;check video mode
          je        ipdone              ;if mode 7, it's mono & we're done
          mov       ax,es:[0463h]       ;address of 6845 video controller
          add       ax,6                ;offset for status register
          mov       StatusVid,ax        ;save status register address
vga:
          mov       ax,0B800h           ;set for cga/ega/vga

ipdone:   mov       es,ax               ;copy video segment
          xor       di,di               ;set DI to hardware video offset

          ;
          ; Is DESQView loaded?  If so, get shadow video RAM buffer (if any)
          ;   make SHADOW call to DESQview to set window buffer address 
          ;   in ES:DI -- leaves ES:DI alone if no shadow buffer
          ;
          mov       bx,_gpIniptr
          cmp       [bx].I_DVMode,0	; Is DV loaded?
          je        not_dv
          cmp       __osmajor,10	; if OS/2 or NT, DV not loaded!
          jae       not_dv

          mov       ah,0FEh
          int       10h

not_dv:   pop       ax                  ;get back old cursor offset
          add       di,ax               ;add to video buffer offset

init_exit:
          pop       bp
          exit


          ;
          ; PCHAR - display a character directly to video memory
          ;
          ; On entry:
          ;         AH = attribute
          ;         AL = character to print
          ;         ES:DI = segment:offset for video memory
          ;
          ; On exit:
          ;         ES:DI = updated video memory location 
          ;         AX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          EVEN
          entry     putchar,noframe,,local  ;set entry point

          xor       bx,bx
          mov       cx,1                ;print 1 character
          mov       BIOSColor,ah	; save color for BIOS output w/tabs
          cmp       al,9                ;check for tab
          jne       doprint             ;not tab, go display it
          mov       bl,BIOSCol          ;tab  - get current column number
          and       bx,07h              ;take lower three bits
          mov       cx,8                ;set tab width
          sub       cx,bx               ;get number of blanks
          mov       al,' '              ;display that many blanks
doprint:
          mov       bl,BIOSCol  	;get current column
          cmp       bx,rmargin          ;to right of right margin?
          jae       pchar_bye           ;  yup - exit

          mov       bx,_gpIniptr
          cmp       [bx].I_BIOS,0	; BIOS output requested?
          je        NoBIOS

          push      bp
          push      cx

          push      ax
          mov       dh,BIOSRow
          mov       dl,BIOSCol
          call      SetCursor
          pop       ax			; restore character & attribute

          mov       bh,BIOSPage		; page 0?
          mov       bl,BIOSColor	; attribute
          mov       cx,1
          mov       ah,9
          int       10h

          pop       cx
          pop       bp

          jmp       short noprint

NoBIOS:
          mov       dx,StatusVid        ;get 6845 status register address
          or        dx,dx               ; check for monochrome, EGA, or VGA
          jz        writeit             ;if so, skip retrace wait
          mov       bx,ax               ;save character and attribute

hrtend:   in        al,dx               ;wait for horizontal retrace end
          shr       al,1                ;check retrace bit
          jc        hrtend              ;wait for it to go off
          cli                           ;no interrupts while we wait for it 
                                        ;  to go back on
hrtstart: in        al,dx               ;wait for horizontal retrace start
          shr       al,1                ;check retrace bit
          jnc       hrtstart            ;wait for it to go on
          mov       ax,bx               ;get back character & attr
writeit:
          stosw                         ;write to display
          sti                           ;enable interrupts again

noprint:
          inc       BIOSCol             ;increment column counter
          loop      doprint             ;check for more chars to print

pchar_bye:
          exit


          ;
          ; CHG_COLORS - change attributes directly in video memory
          ;
          ; On entry:
          ;         AL = attribute
          ;         CX = Number of character cells to modify
          ;         ES:DI = segment:offset for video memory
          ;
          ; On exit:
          ;         ES:DI = updated video memory location 
          ;         AX, BX, CX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     chg_colors,noframe,,local   ;set entry point

          mov       bx,_gpIniptr
          cmp       [bx].I_BIOS,0	; BIOS output requested?
          je        chg_direct

          push      bp			; save BP for old PC BIOS bug
          mov       BIOSColor,al
          mov       dh,BIOSRow
          mov       dl,BIOSCol
chgbloop:
          push      cx
          push      dx
          call      SetCursor

          mov       ah,8
          mov       bh,BIOSPage
          int       10h			; get char & att at cursor

          mov       bl,BIOSColor	; retrieve attribute to BL
          mov       cx,1
          mov       bh,BIOSPage
          mov       ah,9
          int       10h			; write char & att at cursor

          pop       dx
          inc       dx
          pop       cx
          loop      chgbloop

          pop       bp
          jmp       short chc_exit       

chg_direct:
          mov       dx,StatusVid        ; get 6845 status register address
c_attr:
          inc       di			; skip the character
          or        dx,dx               ; check for monochrome, EGA, or VGA
          jz        cw_attr             ; if so, skip retrace wait
          mov       bl,al
c_hrtend:
          in        al,dx               ; wait for horizontal retrace end
          shr       al,1                ; check retrace bit
          jc        c_hrtend            ; wait for it to go off
          cli                           ; no interrupts while we wait for it 
                                        ;  to go back on
c_hrtstart:
          in        al,dx               ; wait for horizontal retrace start
          shr       al,1                ; check retrace bit
          jnc       c_hrtstart          ; wait for it to go on
          mov       al,bl               ; get back attribute
cw_attr:
          stosb                         ; write attribute to display
          sti                           ; enable interrupts again
          loop      c_attr		; continue for rest of line / screen
chc_exit:
          exit


          ;
          ; GetCursor - save the current cursor position
          ;
          ; On exit:
          ;         DX = current cursor position
          ;         AX, BX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     GetCursor,noframe,,local   ;set entry point

          push      cx
          mov       ah,3		; save current cursor location
          mov       bh,BIOSPage
          int       10h
          pop       cx

          exit


          ;
          ; SetCursor - set the cursor position
          ;
          ; On entry:
          ;         DX = cursor position
          ; On exit:
          ;         AX, BX, DX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     SetCursor,noframe,,local   ;set entry point

          mov       ah,2		; set cursor location
          mov       bh,BIOSPage
          int       10h

          exit


	;==================================================================
	;
	;  MISCELLANEOUS ROUTINES
	;
	;==================================================================

          ;
          ; NOAPPEND - dummy routine to keep APPEND from crashing 4DOS
          ;

          entry     noappend,noframe,far

          mov       ax,0FFFFh
          exit


          ;
          ; CriticalSection - Start or end critical section under
          ;                   multitaskers and task switchers
          ;
          ; On entry:
          ;         Arguments on stack:
          ;           unsigned int CritFlag       0 = end, 1 = start
          ;
          ; On exit:
          ;         AX, BX, DX, destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     CriticalSection,argframe,far

          argW      CritFlag            ;1 to start, 0 to end

          push      bx
          mov       bx,_gpIniptr        ;point to INI data
          cmp       [bx].I_WMode,1      ;running under Windows?
           jbe      CFChkDV             ;if not check DV
          mov       ax,1681h            ;get begin critical section value
          cmp       wptr CritFlag,0     ;start?
           jne      CFWinGo             ;if so go on
          inc       ax                  ;bump to end value
          ;
CFWinGo:  int       2Fh                 ;call Windows, then drop through
          ;
CFChkDV:  cmp       [bx].I_DVMode,1     ;running under DESQview?
           jbe      CFDone              ;if not we're done
          mov       ax,DVC_BEGINC       ;get begin critical section value
          cmp       wptr CritFlag,0     ;start?
           jne      CFDVGo              ;if so go on
          mov       ax,DVC_ENDC         ;get end value
          ;
CFDVGo:   int       15h                 ;call DESQView
          ;
CFDone:
          pop       bx
          exit                          ; all done


          ;
          ; DecodeMsg - Decode and display a "secure" message
          ;
          ; On entry:
          ;   DecodeMsg(int MsgNum, (char *)MsgBuf)
          ;
          ; On exit:
          ;   AX = message buffer address
          ;   BX, CX, DX, ES destroyed
          ;   Other registers preserved, interrupt state unchanged
          ;

          entry     DecodeMsg,argframe,far
          ;
	argW	MsgBuf		;message buffer address
          argW      MsgNum              ;message number
          ;

          pushm     bx,si,di,ds         ;save registers

          loadseg   es,ds               ;set destination segment
          mov       ah,bptr MsgNum+1    ;make things confusing (this gets 0)
          mov       cl,ah               ;copy the 0
          loadseg   ds,MSG_SEG,bx     	;get server segment address
	assume	ds:MSG_SEG	;fix assumes
          mov       al,bptr MsgNum      ;get real message number + 36
          xchg      ah,al               ;put it in ah
          mov	di,MsgBuf           ;get output buffer address
          add       cl,4                ;get rotation count of 4
          mov       si,offset MSG_SEG:ServErrs  ;get error table address
          ror       ax,cl               ;slide things around
          sub       ax,(2400h shr 4)    ;adjust for offset of 36
          shr       ax,cl               ;slide result into place, now AX is
                                        ;  message number and hopefully we
                                        ;  confused the casual hacker a bit
          mov       es:[di],ah          ;confuse a little more
          add       al,193              ;add signon message base
          push      di                  ;save destination address
          call      ErrMsg              ;dump message into buffer
	xor	al,al		;get terminating NUL
	stosb			;terminate the string
	pop	ax		;get back buffer address
          popm      ds,di,si,bx         ;restore registers
	assume	ds:DGROUP		;fix assumes
          exit


          ;
          ; SAFE_APPEND - turn off DOS's default unsafe APPENDs (stripping
          ;    an explicit pathname)
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         All other registers unchanged, interrupts on
          ;

          entry     safe_append,noframe,far	;set up entry point

          push      bx
          mov       ax,0B706h		; get APPEND state
          xor       bx,bx
          int       02Fh
          test      bx,1		; test for APPEND active
          jz        no_append
          cmp       __osmajor,20
          jb        not_os2
          or        bh,00100000b	; OS/2 has the /PATH:OFF backwards!
          jmp       short set_append
not_os2:
          and       bh,11011111b	; set /PATH:OFF
set_append:
          mov       ax,0B707h
          int       02Fh
no_append:
          pop       bx
          exit

          ;
          ; QueryKSTACK - Return 1 if KSTACK is loaded
          ;
	  ; On entry:
	  ;	No requirements
	  ;
          ; On exit:
          ;         AX = 1 if KSTACK loaded, 0 if not
          ;         All other registers unchanged, interrupts on
          ;

          entry     QueryKSTACK,noframe,far	;set up entry point
          push      bx
	  mov	    ax,0D44Fh
	  xor	    bx,bx
	  int	    2Fh
	  cmp	    ax,44DDh		; AX = 44DD if KSTACK is loaded
	  jne	    no_kstack
	  mov	    bx,1
no_kstack:
	  mov	    ax,bx
          pop       bx
          exit


	;
	; QueryClipAvailable - Return 1 if clipboard is available
	;
	; On entry:
	;	No requirements
	;
	; On exit:
	;	AX = 1 if clipboard is available, 0 if not
	;	All other registers and interrupts unchanged
	;

	entry	QueryClipAvailable,noframe,far	;set up entry point
	mov	ax, 1700h 		;does the current session support
	int	2Fh			;  reading the clipboard?
	sub	ax, 1700h 		;if so it returns  AX != 1700h
	je	QCADone			;if not we're done (AX = 0)
	mov	ax, 1			;yes it does
QCADone:
        exit


	;
	; OpenClipboard - Open the clipboard
	;
	; On entry:
	;	No requirements
	;
	; On exit:
	;	Clipboard opened
	;	All registers and interrupts unchanged
	;

	entry	OpenClipboard,noframe,far	;set up entry point
	mov	ax, 1701h
	int	2Fh			;open the clipboard
	exit


	;
	; CloseClipboard - Close the clipboard
	;
	; On entry:
	;	No requirements
	;
	; On exit:
	;	Clipboard closed
	;	All registers and interrupts unchanged
	;

	entry	CloseClipboard,noframe,far	;set up entry point
	mov	ax, 1708h
	int	2Fh			;close the clipboard
	exit

	;
	; QueryClipSize - Return clipboard size
	;
	; On entry:
	;	Clipboard must be available per QueryClipAvailable
	;	  and opened by OpenClip
	;
	; On exit:
	;	DX:AX = size of clipboard
	;	All other registers and interrupts unchanged
	;

	entry	QueryClipSize,noframe,far	;set up entry point
	mov	ax, 1704h
	mov	dx, 1		;OEM text format
	int	2Fh		;get size of cliboard data in DX:AX
	exit

	;
	; ReadClipData - Read data from the clipboard
	;
	; On entry:
	;	Clipboard must be available per QueryClipAvailable
	;	  and opened by OpenClip
	;	Arguments on stack:
	;	  char _far *lpMemory	pointer to buffer
	;
	; On exit:
	;	All registers and interrupts unchanged
	;

	entry	ReadClipData,argframe,far	;set up entry point
	argD	lpMemory		;buffer address

	mov	ax, 1705h 	;read data
	les	bx, lpMemory	;data pointer
	mov	dx, 1		;OEM text format
	int	2Fh		;read the data
	exit

	;
	; SetClipData - Write data to the clipboard
	;
	; On entry:
	;	Clipboard must be available per QueryClipAvailable
	;	  and opened by OpenClip
	;	Arguments on stack:
	;	  char _far *lpMemory	pointer to buffer
	;	  long uClipSize	size of data to transfer
	;
	; On exit:
	;	All registers and interrupts unchanged
	;

	entry	SetClipData,argframe,far	;set up entry point
	argD	uClipSize	 	;bytes to transfer
	argD	lpMemory		;buffer address

	mov	ax, 1702h
	int	2Fh			; empty the clipboard
	les	bx, lpMemory		; get data address
	dload	cx, si, uClipSize	; get buffer size
	mov	dx, 7			; OEM text format
	mov	ax, 1703h
	int	2Fh			; set clipboard data
	mov	ax, 1708h
	int	2Fh			; close clipboard
	exit

@curseg	ends

          end

