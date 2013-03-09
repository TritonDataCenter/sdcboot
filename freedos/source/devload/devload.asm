; .....................PROGRAM HEADER FOR PROJECT BUILDER....................

;       TITLE   DEVLOAD         to load device drivers from command line.
;       FORMAT  COM
;       VERSION 3.25
;       CODE    80x86
;       BUILD   NASM -o devload.com in 3.21/newer ***
;       AUTHOR  David Woodhouse

;        (C) 1992, 1993 David Woodhouse.
;	Patches for 3.12 to 3.16, 3.17 - 3.19: 2004 to 2005, 2007 by Eric Auer
;	Patch for 3.20: 2007 by Alexey and Eric Auer
;       Patch for 3.21: 2008 by Eric Auer


;  Devload - a tool to load DOS device drivers from the command line
;        Copyright (c) 1992-2011 David Woodhouse and Eric Auer
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License along
;   with this program; if not, write to the Free Software Foundation, Inc.,
;   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


; EXPLANATION...
;       The program first relocates itself to the top of the available memory
;       and then loads the driver below itself. If the driver loads happily, it
;       is then executed. If it asks for memory to be reserved for it, it is
;       linked into the device chains and the program TSRs keeping the required
;       amount of memory, otherwise a normal exit occurs.


; ............................ALTERATION LIST................................


; Version 1.0            12/3/92
;       Basics, not user-friendly, only supports character devices so far.
;                       13/2/92
;       Change to using EXEC (4Bh) function to load - now loads .EXE files
;       Take length to TSR from device return, not file length.
;       Don't TSR if not required.


; Version 1.1            17/3/92
;       Complete rewrite of initialisation routine.
;       Allows for more than one device per file.
;       Help message added.
;       EXEC failure now explains error codes, rather than just giving no.
;                       19/3/92
;       Print error message and exit if version < 4.
;       Loads device at PSP+6, not PSP+10h (overlay FCBs and command tail).
;       Stack moved down by 8 paras, not truncated to 80h bytes.
;                       20/3/92
;       Release environment block before TSR.


; Version 2.0            21/3/92
;       Use INT 21h, function 53h. Can now load block devices.
;       Use segment in break address, don't assume same as driver segment.
;       Ask whether to terminate if can't install any blocks.
;       Disable ctrl-break.
;                       22/3/92
;       Check sector size before installing block devices.
;       Disable break with INT 1Bh, as well as INT 23h - stops ^C appearing.
;               (taken out in v2.1) - causes problems if driver changes it.
;       Don't use INT 10h - all output via INT 21h - can be redirected.
;       Print drive letter with block header address.
;       Print LastDrive message after installing block headers, not before.
;       Change program name in arena header to device filename (for MEM.EXE).
;       Support for DOS 3 added (now works with at least DOS 3.1 onwards).
;       Print driver's load address.


; Version 2.1            24/3/92
;       Bug fix - drivers requesting memory offset FFF1 - FFFF now works OK.
;                       25/3/92
;       Now uses func 60h to expand filename before printing it.
;       Bug fix - changing INT 1B lost vector if driver altered it, so don't.
;       Display INT vectors changed by driver.
;                       26/3/92
;       Converts params to upper case before passing to driver, like DOS does.
;       Also adds space after filename if no parameters given.
;       Change data at end to ?? rather than 00 - smaller .COM file.


; Version 2.2            26/10/92
;       When no blocks installed, checks whether INT vectors changed before
;       asking whether to terminate. Still not foolproof, but better.
;       Cosmetic fix - `$' now comes after CR/LF on `None.' for INT vectors.
;       Bug fix - Check all INT vectors (200h words, not 200 words!)


; Version 3.0            11/4/93 - 20/4/93
;       Complete rewrite from scratch.
;       Only relocate if going to try EXEC - saves losing F3.
;       Use path to find driver if not in specified directory.
;       Convert to .EXE program.
;       Relocate PSP to top of memory as well as code and stack.
;       Release environment before requesting memory for driver.
;       Don't even attempt to load driver if not enough memory available.
;       Use highest memory required return.
;       Link drivers in correct order.
;       Add /Q (quiet mode) option.
;       Disperse comments ad nauseum.
;                       21/4/93
;       Add /V (verbose mode) option.
;       Move lastdrive report to end.
;       Move abort request to end.
;       Add count of character devices installed.
;       Put entry point after LASTBYTE - smaller relocated code.


; Version 3.1            18/2/95
;       Bug fix - push AX before fileerr if EXEC fails.
;       Check whether found item is directory when looking for driver file.

; Version 3.11           12/2/96
;       just add email address prior to uploading.

; Version 3.12 25/3/2004 - patches by Eric Auer
;	using COM file format again
;	compiles with free Arrowsoft ASM 2.0 now
;	use e.g. free Borland Turbo C 2.01 TLINK /t as linker
;	added DOS 5+ style memory size info passing to driver
;	changed /H from alias-for-/? to load-into-umb, in other words:
;	devload can now "devloadhigh" drivers (limited)...
;	TODO: should probably do something to help MEM with the program name?

; Version 3.13 12/5/2004 - patches by Eric Auer and Erwin Veermans
;	the /Q (quiet) option now has a stronger effect

; Version 3.14 9/7/2005 - patches by Eric Auer
;	fixed a bug in block device loading: number of units in the
;	driver header is now set to the value returned by init
;	... also changed some message texts to help UPX compression

; Version 3.15 16/7/2005 - patches by Eric Auer
;	the /Q flag now suppresses even the "Driver staying resident"
;	message and the message which tells whether /H (UMB) worked.
;	Inspired by Erwin Veermans' patches.

; Version 3.16 4/11/2005 - contributed patches and some cleaning
;	improved TASM compile (normal DEVLOAD compiler is ArrowASM),
;	fixed the "UMB needs DOS 5+" version check, improved texts.
;	comment style: added spaces after ; where ; starts a comment.
;	Added some "short" and "ds:" as suggested by Diego Rodriguez.
;	DR DOS version check based on research by Diego Rodriguez.

; Version 3.17 9/5/2007  - Eric: more RAM for DPB if DOS 7+ (FAT32)

; Version 3.18 11/5/2007 - Alexey: use magic CX / DX to build ext DPB
;                          (int 21.53), use true instead of setver
;                          version if DOS 5+, provide make.bat file.
;                        - Eric: Tuned messages to get 3069 byte .com
;                          (with UPX 2.02d --8086 --best)

; Version 3.19 2/8/2007  - Eric: must also set overlay relocation factor
;                          otherwise drivers in EXE format will crash
;                          Also tune error messages, use generic text
;                          for unlikely messages. Shorten help a bit.

; Version 3.20 13/9/2007 - Alexey: added EDR-DOS support
;                          (EDR DOS WiP 17.6.2007 and newer ONLY!)
;                          added fmake.bat & readme.txt

; Version 3.21 25/6/2007 - Eric: Converted to NASM with help of NoMySo
;                          script by Michael Devore, removed fmake...
;                          (ASM / NASM pick other direction variant in
;                          mov/xor/cmp/or/add/sub reg,reg machine code)
;                          WantedParas handling: check how much RAM/UMB
;                          driver needs at least. Tuned messages again.
;                          GNU GPL info (see infradead.org) added here.

; Version 3.22 14/7/2011 - Jeremy: if error testing umb size don't fall
;                          through and do double allocations, 2nd failing

; Version 3.23 02/8/2011 - Jeremy: don't overwrite CDS in use by non-block
;                          driver

; Version 3.24 04/8/2011 - Jeremy: improve handling holes in and lack of
;                          free CDS better, add /D option to specify
;                          which CDS slot to begin free search from -
;                          i.e. what drive letter to assign device
;                          Set exit # (errorlevel) to drive# assigned 

; Version 3.25 05/8/2011 - Jeremy: fix bug where initial CDS not checked,
;                          adjust return codes to account for char devices

; .............................IMPROVEMENT IDEAS.............................


;       Load into Upper Memory Blocks (like 'DEVICEHIGH='.)
;		*** solved that in 3.12 ***
;       Work from batch files like CONFIG.SYS.
;       Change size of LASTDRIVE array by reallocating.
;       Add support for SETVER.EXE.


; ................................DEFINES....................................

STACKLEN        equ     200h	; *** only used in .exe version ***
; SMALLESTDRIVER  equ     100h	; alloc. min 4k (100h paras) for driver

QuietFlag       equ     80h
VerboseFlag     equ     40h
AutoFlag        equ     20h
UMBFlag		equ	10h	; *** added 2004 ***

; ............................CODE (at last).................................

SEGMENT	CSeg	; xxx	public  ALIGN=1  CLASS=CODE

        org     100h	; *** was 0 for .exe version ***


; ............CODE TO BE EXECUTED ONCE RELOCATED TO TOP OF MEMORY............


Main0:	jmp Main	; *** get right entry point for .com version ***


        ; DS:TopCSeg, ES:TopPSPSeg

        ; Get old PSP segment, store new PSP segment.

relocated:      push    es
                mov     es,[PSPSeg]
                pop     WORD [PSPSeg]

        ; Push segment of environment.

        ; DS:TopCSeg, ES:PSPSeg

                push    word [es:002Ch]

        ; Release old PSP segment.	*** seems to trash DEBUG ***

                mov     ah,49h
                int     21h
                jnc     relPSPok

        ; Failed to release PSP - print error.

                mov     dx,RelPSPErrMsg
                call    PrintError
                mov     ah,9
                int     21h
                mov     dx,CrLfMsg
                mov     ah,9
                int     21h

        ; Release environment segment.

        ; DS:TopCSeg, ES:CSeg

relPSPok:       pop     es
                mov     ah,49h
                int     21h
                jnc     relenvok

        ; Failed to release environment - print error.

                mov     dx,RelEnvErrMsg
                call    PrintError
                mov     ah,9
                int     21h
                mov     dx,CrLfMsg
                mov     ah,9
                int     21h

relenvok:       push    cs
                pop     es

				; *** added UMB alloc strategy selection ***
                mov     ax,5800h	; get current alloc strat
                int     21h
                mov     [OldAllocStrat],ax

		mov	bp,1		; default: no special UMB link state

		test	byte [ModeFlag], UMBFlag
		jz	lowload

				; *** added because of UMB alloc strat ***
                mov     ax,5801h	; set alloc strat: UMB only
                mov     bx,40h		; (8xh is try UMB first, low 2nd)
					; (4xh is only UMB, 00h is low only)
					; (x0h first, x1h best, x2h last fit)
                int     21h
                jc      lowload         ; TODO print message, no UMB available
		mov	ax,5802h	; get UMB link state
		int	21h
                xor     ah, ah
		mov	bp,ax		; store link state (AL is 1 if linked)
		mov	ax,5803h	; set UMB link state (!)
		mov	bx,1		; make UMBs part of the DOS mem chain!
		int	21h
                jc      lowload         ; assume no UMB available
;		mov	bx,0fffh	; never desire > 64k of UMBs
		mov	bx,7fffh	; ask unrealistic size UMB
		mov	ah,48h
		int	21h

	; this should always fail - add check if it really did here?
	; BX now tells us how much we could have gotten - in UMB space

		cmp	bx,[WantedParas]	; enough UMB free?
		ja	sizeok		; then get as much as we can
                mov     ax,5801h	; set new alloc strat: UMB first
                mov     bx,80h		; (8xh is try UMB first, low 2nd)
		int	21h

lowload:			; *** end of added code ***

        ; Find out how much memory is available by asking for stupid amounts.

                mov     ah,48h
                mov     bx,0FFFFh	; ask unrealistic size, 1 MB
                int     21h

        ; In MS-DOS version 6 and below, this will always fail, but check
        ; the return status just in case it does give what we asked for.

                jnc     grablowok	; esp. useful for UMB loading!

        ; BX now holds the maximum amount of memory available.
        ; Don't install if less than [minimum required] available.

                cmp     bx,[WantedParas]	; SMALLESTDRIVER
                ja      sizeok			; enough: get all we can
                mov     bx,[WantedParas]	; else: get all we need
						; (...and expect to fail)

        ; Attempt to grab memory for driver.

sizeok:         mov     ah,48h
                int     21h
                jnc     grablowok

				; *** added because of UMB alloc strat ***
                push    ax              ; store error code for alloc
                mov     ax,5801h	; set (here: restore) alloc strat
                mov     bx,[OldAllocStrat]
		mov	bh,0		; sanitize
		int	21h
		test	bp,1		; UMBs originally part of chain?
		jnz	keepumbs
		mov	ax,5803h	; set UMB link state (!)
		mov	bx,0		; remove UMBs from chain again
                int     21h
                pop     ax              ; restore error code from alloc
keepumbs:				; *** end of added part ***

        ; Failed to grab memory - print error and exit.

                mov     dx,GrabLoErrMsg
                jmp     allocerr

        ; Store segment of device.

grablowok:      mov     [DvcSeg],ax
		mov	WORD [DvcSeg+2],ax	; ALSO adjust relocation! 8/2007
                mov     [BlockSize],bx

				; *** added because of UMB alloc strat ***
                mov     ax,5801h	; set (here: restore) alloc strat
                mov     bx,[OldAllocStrat]
		mov	bh,0		; sanitize
		int	21h
		test	bp,1		; UMBs originally part of chain?
		jnz	keepumbs2
		mov	ax,5803h	; set UMB link state (!)
		mov	bx,0		; remove UMBs from chain again
                int     21h
keepumbs2:				; *** end of added part ***

        ; Disable Ctrl-Break.

                mov     dx,BreakHandler
                mov     ax,2523h
                int     21h

        ; Copy interrupt vectors for later comparison.

        ; DS:TopCSeg, ES:TopCSeg
                xor     bx,bx
                mov     ds,bx
        ; DS:0000, ES:TopCSeg
                mov     si,bx
                mov     di,IntVectors
                mov     cx,200h
                rep     movsw

        ; Parse parameters in same way as SYSINIT does.

                mov     ds,[cs:PSPSeg]

        ; DS:TopPSPSeg, ES:TopCSeg

        ; Find end of filename in DI.

                mov     di,[cs:NamePtr]
                add     di,[cs:NameLen]

        ; Find end of command line in BX.

                mov     bl,byte [80h]
                add     bx,81h

        ; Compare them.

                cmp     bx,di
                ja      parmsgiven

        ; If they are the same, no parameters were given, so add a space.

                mov     byte [di],' '
                inc     bx

        ; Append CrLf to line.

parmsgiven:     mov     word [bx],0A0Dh

        ; Print 'Filename: ' if not in Quiet Mode.

                push    cs
                pop     ds

;               test    ModeFlag,QuietFlag	; *** phase error! ***
                test    byte [ModeFlag],QuietFlag	; ***
                jnz     noprintfname

                mov     dx,FNameMsg
                mov     ah,9
                int     21h

        ; Make filename ASCII$ instead of ASCIIZ for printing.

                mov     di,NameBuffer
                mov     dx,di
                mov     cx,80h
                xor     al,al
                repnz   scasb
                dec     di
                mov     byte [di],'$'

        ; Print filename.

                int     21h

        ; Restore to ASCIIZ.

                mov     byte [di],0

        ; Print CrLf after filename.

                mov     dx,CrLfMsg
                int     21h

        ; Load driver file into memory.

        ; DS:TopCSeg, ES:TopCSeg

noprintfname:   mov     dx,NameBuffer
                mov     bx,DvcSeg
                mov     ax,4B03h	; load overlay
                int     21h
                jnc     loadedok

        ; Print 'EXEC failure'.

                mov     dx,NotLoadMsg
                push    ax

        ; Restore error code.

fileerr:        pop     ax

        ; Print error message and number, get offset of cause in DX.

allocerr:       call    PrintError

        ; Indicate error (driver not loaded) in exit code
prexit:
                mov     al,255

        ; Print final message.

okexit:         push    ax              ; save exit code that is in AL
                mov     ah,9
                int     21h
                pop     ax

        ; Exit program.

exit:           mov     ah,4Ch
                int     21h

; ...........................................................................

        ; DS:TopCSeg, ES:TopCSeg

        ; Check whether to print load address.

loadedok:       test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintladdr

        ; Print 'Load address:'

                mov     dx,LoadAddrMsg
                mov     ah,9
                int     21h

        ; Print segment of driver.

                mov     ax,[DvcSeg]
                call    PHWord

        ; Print ':0000'

                mov     dx,Colon0Msg
                mov     ah,9
                int     21h

        ; DS:TopCSeg, ES:TopCSeg

        ; Get pointer to 'invar' (list of lists)
        
noprintladdr:   mov     ah,52h
                int     21h

        ; Store for later use.

                mov     [InvarOfs],bx
                mov     [InvarSeg],es

        ; DS:TopCSeg, ES:InvarSeg

        ; Fetch nBlkDev and LastDrive from 'invar' (list of lists)

                mov     ax,[es:bx+20h]
                mov     word [nBlkDev],ax               ; *** updates nBlkDev & LastDrive

                ; Determine 1st free CDS entry
                push	bx
                push	es

                ; Get pointer to LASTDRIVE array (Current Directory Structure - CDS).
                les     bx,[es:bx+16h]

		; Calculate offset in CDS array of next free entry (>= current # of block devices)
                mov     al,[LastDrUsed]         ; [LastDrUsed] init to drive to begin search with
                mov     ah,[LDrSize]
                mul     ah
                ; loop until free entry is found
CDSinuse:
                add     bx,ax                   ; update pointer into CDS array
                ; check that we don't exceed CDS array bounds (> LASTDRIVE)
                mov     al,[LastDrive]
                cmp     al,[LastDrUsed]
                jbe     nofreeCDS
                ; test flags (CDS[BX].flags) in current CDS entry to see if in use
                test    byte [es:bx+44h], 0C0h
                jz      CDSfound
                ; increment to next CDS entry
                inc     byte [LastDrUsed]
                mov     al,[LDrSize]
                cbw
                jmp     CDSinuse

        ; No more PATH items or no PATH segment, so print error and exit.

nofreeCDS:      push    cs
                pop     ds
                mov     dx,NoFreeCDSMsg
                jmp     prexit
                
                
CDSfound:
                pop     es
                pop     bx

        ; Fetch max. sector size from 'invar' (list of lists)

                mov     ax,[es:bx+10h]
                mov     [SecSize],ax

                push    es
                pop     ds

        ; Trace device chain from 'invar'. (list of lists)

        ; Point DS:DI to first block header.

                mov     si,bx

        ; Point to next block header.

notlast:        lds     si,[si]

        ; Point to pointer within block header to the next one.

                add     si,word [cs:NextBlHOfs]	; ASM to NASM: add 'cs:'

        ; Loop while not at the end of the chain.

                cmp     word [si],byte -1	; ASM optimizes 0FFFFh...
                jnz     notlast

        ; Point back at beginning of last block header in chain.

                sub     si,word [cs:NextBlHOfs]	; ASM to NASM...

        ; Store for later use.

                mov     [cs:ChainEndOfs],si
                mov     [cs:ChainEndSeg],ds

        ; DS:DvcSeg, ES:InvarSeg

        ; Point ES:BX to device after NUL device.

                les     bx,[es:bx+22h]

        ; Point DS:SI to new device (DvcSeg:0000).

                mov     ds,[cs:DvcSeg]
                xor     si,si

        ; DS:DvcSeg, ES:OldDvcSeg

        ; Install all devices in chain.

anoth_dvc:      call    InstallDevice
                jnz     anoth_dvc

        ; Print LASTDRIVE error message if necessary.

                push    cs
                pop     ds

                cmp     byte [LDrErrMsg],'0'
                jz      noldrerr
                mov     dx,LDrErrMsg
                mov     ah,9
                int     21h

        ; Calculate size of driver to keep.

noldrerr:       mov     ax,[BlHEndOfs]
                add     ax,0Fh
                rcr     ax,1
                mov     cl,3
                shr     ax,cl

                add     ax,[EndSeg]

                sub     ax,[DvcSeg]

        ; Store size of driver to keep.

                push    ax

        ; Check whether anything installed, offer abortion if not.

                mov     ax,word [BlocksDone]
                or      ax,ax
                jnz     somedone

        ; Nothing installed - check whether Auto Mode.

                test    byte [ModeFlag],AutoFlag	; *** byte ptr ***
                jnz     somedone

        ; If it didn't want to stay anyway, don't ask.

                pop     cx
                push    cx
                jcxz    somedone

        ; Check whether any INT vectors changed.

                xor     di,di
                mov     es,di

        ; DS:TopCSeg, ES:0000

                mov     si,IntVectors
                mov     cx,200h
                rep     cmpsw
                jnz     somedone

        ; Nothing installed, so give option of aborting.

                mov     dx,AskEndMsg
                mov     ah,9
                int     21h

        ; Get response from keyboard.

                mov     ah,8
badkey:         int     21h

        ; If it's an extended character code, get the second byte and try again.

                or      al,al
                jnz     realchar
                int     21h
                jmp     badkey

        ; If it's valid, act upon it, else loop for another.

realchar:       cmp     al,'N'
                jz      nokill
                cmp     al,'n'
                jz      nokill
                cmp     al,'Y'
                jz      kill
                cmp     al,'y'
                jnz     badkey

        ; Response was yes, so set length required to zero.

kill:           pop     bx
                xor     bx,bx
                push    bx

                ; mov     bx,DvcSeg
                ; mov     EndSeg,bx
                ; xor     bx,bx
                ; mov     BlHEndOfs,bx


        ; Print the key pressed.

nokill:         mov     ah,02h
                mov     dl,al
                int     21h

        ; Print CrLf afterwards.

                mov     dx,CrLfMsg
                mov     ah,9
                int     21h

        ; Print Lastdrive and LastDrUsed if blocks done and Verbose Mode.

somedone:       push    cs
                pop     ds

                test    byte [BlocksDone],0FFh	; *** byte ptr ***
                jz      noprintldrmsg

                test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintldrmsg

                mov     al,byte [LastDrUsed]
                mov     ah,byte [LastDrive]

                mov     byte [LDMsgA], byte 'A'-1	; explicit byte 3.16
                mov     byte [LDMsgB], byte 'A'-1	; explicit byte 3.16

                add     byte [LDMsgA],al
                add     byte [LDMsgB],ah

                mov     dx,LastDrMsg
                mov     ah,9
                int     21h

        ; If not Quiet Mode, print number of devices installed.

noprintldrmsg:  test    byte [ModeFlag],QuietFlag	; *** byte ptr ***
                jnz     noprintnuminst

        ; Get driver keep size into CX, don't print installed message if zero.

                pop     cx
                push    cx
                jcxz    noprintnuminst

        ; Print number of blocks installed, if any.

                mov     bl,[BlocksDone]
                or      bl,bl
                jz      noblocks

                add     [NumBlInstMsg],bl
                mov     dx,NumBlInstMsg
                mov     ah,9
                int     21h

                mov     dx,NumInstMsgA
                cmp     bl,1
                jnz     blnoplural
                inc     dx
blnoplural:     int     21h

        ; Print number of character devices installed, if any.

noblocks:       mov     bl,[CharsDone]
                or      bl,bl
                jz      noprintnuminst

                add     [NumChInstMsg],bl
                mov     dx,NumChInstMsg
                mov     ah,9
                int     21h

                mov     dx,NumInstMsgA
                cmp     bl,1
                jnz     chnoplural
                inc     dx
chnoplural:     int     21h

        ; Insert new nBlkDev into 'invar'. (list of lists)

noprintnuminst: les     bx,[Invar]

                mov     al,[nBlkDev]
                mov     byte [es:bx+20h],al

        ; Restore driver size in paragraphs.

                pop     bx

        ; Test whether to print driver size.

                test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintsize

        ; Print 'Size of driver in paras:'.

                mov     dx,SizeMsg
                mov     ah,9
                int     21h

                mov     ax,bx
                call    PHWord

                mov     dx,FullStopMsg	; 'h.',cr,lf
                mov     ah,9
                int     21h

noprintsize:    or      bx,bx
                jnz     lengthnotzero

        ; Length of driver is zero, so exit now.

                mov     dx,NotLoadedMsg
                jmp     prexit  ; *** 

        ; Check whether it fits in the allocated block.

lengthnotzero:  cmp     bx,[BlockSize]	; *** was cmp ax,... ***
                jna     drvrfits
                mov     dx,TooBigMsg
                mov     ah,9
                int     21h

        ; Change memory allocation on lowest block in memory.

drvrfits:       mov     es,[DvcSeg]
                mov     ah,4Ah
                int     21h
                jnc     allocok

        ; Print 'allocation error'.

                mov     dx,Reduce2ErrMsg
                call    PrintError
                mov     ah,9
                int     21h
                mov     dx,Reduce2ErrMsga
                mov     ah,9
                int     21h

        ; Check whether to print INT vectors changed.

allocok:        test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintints

        ; Print 'Interrupt vectors changed:'

                mov     dx,IntChangeMsg
                mov     ah,9
                int     21h

        ; Print all INTs changed.

                xor     bx,bx
                mov     es,bx
                mov     di,bx
                mov     si,IntVectors
                mov     cx,200h

        ; Print no comma before the first INT number.

                mov     dx,CommaMsg-1

        ; Compare INT vectors with copy taken earlier.

loopints:       rep     cmpsw

        ; If we stopped at the end, leave the loop.

                jcxz    lastdiff

        ; Flag that at least one was changed.

                or      bl,1

        ; Print 'h, ' between INT numbers.

                mov     ah,9
                int     21h

        ; Check whether it was offset or segment that was different.

                mov     ax,di
                dec     ax
                test    ax,2
                jnz     notofs

        ; If was the offset, so don't bother checking the segment.

                add     di,byte 2	; ASM to NASM: explicit byte
                add     si,byte 2	; ASM to NASM: explicit byte
                dec     cx

        ; Print interrupt number.

notofs:         shr     ax,1
                shr     ax,1
                call    PHByte

        ; After the first one, print 'h, ' before the rest.

                mov     dx,CommaMsg
                jmp     loopints

        ; Print either full stop or 'None.'

lastdiff:       mov     dx,FullStopMsg

        ; Test flag to see whether any were changed.

                or      bl,bl
                jnz     notnochanges

        ; None were changed, so print 'None.' instead of a full stop.

                mov     dx,NoneMsg

notnochanges:   mov     ah,9
                int     21h

        ; DS:TopCSeg, ES:0000

        ; Link from NUL device.

noprintints:    les     bx,[Invar]
                add     bx,byte 0022h	; ASM to NASM: explicit byte
                mov     ax,[NewDrvOfs]
                mov     [es:bx],ax
                mov     ax,[NewDrvSeg]
                mov     [es:bx+2],ax

                push    cs
                pop     es

        ; DS:TopCSeg, ES:TopCSeg

        ; Find last backslash in filename.

                mov     di,NameBuffer+80h
                mov     al,'\'
                std
                mov     cx,0080h

                repnz   scasb	; NDISASM shows repnz as db 0xf2 here?
                cld
                add     di,byte 2	; ASM to NASM: explicit byte

        ; Point to arena header / MCB of driver segment.

                mov     ax,[DvcSeg]
                dec     ax
                mov     es,ax

        ; Set driver segment to self-ownership.

                inc     ax
                mov     word [es:1],ax

        ; DS:TopCSeg, ES:DvcSeg-1

        ; Move name into arena header / MCB

                mov     si,di
                mov     di,8
                mov     cx,8
movname:        lodsb
                cmp     al,2eh
                jz      fill0s
                cmp     al,0
                jz      fill0s
                stosb
                loop    movname
                jmp     short stayexit		; optimization 3.16

fill0s:         xor     al,al
                rep     stosb

stayexit:       
                mov     al, [ExitCode]                  ; set exit code
                test    byte [ModeFlag],QuietFlag	; *** new 3.15
                jnz     stay2exit			; new 3.15
		mov     dx,StayingMsg
                jmp     okexit				; exit and show text
stay2exit:	jmp	exit				; new 3.15



; .....................break handler......................................

BreakHandler:   iret                    ; well, that didn't take long!

; ....................PHWord...........................

;       IN:     AX      word to be printed
;       OUT:    nothing (print word as hex string)
;       LOST:   nothing


PHWord:         push    ax
                xchg    al,ah
                call    PHByte
                mov     al,ah
                call    PHByte
                pop     ax
                retn

; ...............PHByte...........

;       IN      AL      byte to printed
;       OUT:    nothing (print byte as hex string)
;       LOST:   nothing

PHByte:         push    ax
                mov     ah,al
                shr     al,1
                shr     al,1
                shr     al,1
                shr     al,1
                call    PHNibble
                mov     al,ah
                call    PHNibble
                pop     ax
                retn

; .............PHNibble................

;       IN:     AL      nibble to be printed
;       OUT:    nothing (print hex digit via int 21 function 2)
;       LOST:   nothing

PHNibble:       push    dx
                push    ax
                and     al,0Fh
                add     al,'0'
                cmp     al,'9'
                jna     ph1
                add     al,'A'-'9'-1
ph1:            mov     ah,02h
                mov     dl,al
                int     21h
                pop     ax
                pop     dx
                retn


; .......................PrintError..........................................

;       IN:     AX      Error message to explain.
;               DX      Offset of error message.
;               DS      CSeg
;       OUT:    DX      Offset of error cause message.
;       LOST:   AX
;               BX

        ; Print first message.

PrintError:     push    ax
                mov     ah,9
                int     21h
                pop     ax

        ; Print error number.

                call    PHWord

        ; If over 9, set it to slot 0 which has "ErrUnknown"

                cmp     ax,9
                jb      notover9
                xor     ax,ax

        ; Look up offset of error message.

notover9:       mov     bx,ErrTable
                shl     ax,1
                add     bx,ax
                mov     dx,[bx]	; 3.16: explicit segment
                retn


; ............................InstallDevice..................................

;       IN:     DS:SI   address of new driver header
;               ES:BX   address of old driver header

;       OUT:    DS:SI   address of next driver header
;               ES:BX   address of new driver header
;               ZERO    set if last driver in file


        ; Store device addresses.

InstallDevice:  mov     [cs:NewDrvOfs],si
                mov     [cs:NewDrvSeg],ds
                mov     [cs:OldDrvOfs],bx
                mov     [cs:OldDrvSeg],es

                push    cs
                pop     ds

; ***   ; Print CrLf to keep display tidy.

        ; DS:TopCSeg, ES:OldDvcSeg

; ***           mov     dx,offset CrLfMsg
; ***           mov     ah,9
; ***           int     21h

        ; Set up request header.

        ; Insert next block device number.

                mov     al,[LastDrUsed]
                mov     byte [RqHdr+16h],al

        ; Insert command number 0, INIT.
	; Use subunit number 0, as DI1000DD block checks even for INIT

		xor	ax,ax
                mov     word [RqHdr+2],ax	; command, subunit

        ; Insert default end of driver address = start of driver.
	; *** changed to default to "available paras * 10h" for ***
	; *** DOS 5.0+ style memory size passing to driver ***
	; (clipping the value to 64k for now, feels more compatible)
	; (alternative would be to tell EndSeg:0 instead of DvcSeg:xxxx)

                mov     ax,[DvcSeg]
                mov     word [RqHdr+10h],ax	; probably most compatible
		mov	ax,[EndSeg]		; *** <new code> ***
		sub	ax,[DvcSeg]		; find max number of paras
		cmp	ax,1000h		; more than 64kby?
		jb	tellsmall
		mov	ax,0fffh		; max reported value
tellsmall:	shl	ax,1			; convert paras to bytes
		shl	ax,1
		shl	ax,1
		shl	ax,1
		or	ax,0ch			; round up to 0xxxch
                mov     word [RqHdr+0Eh],ax	; *** </new code> ***
; ***		mov     word ptr [RqHdr+0Eh],0	; *** old code ***

        ; Insert default no blocks (number of units: 0) in driver.
                
                mov     byte [RqHdr+0Dh],0

        ; Insert pointer to copy of command line.

                mov     ax,[PSPSeg]
                mov     word [RqHdr+14h],ax
                mov     ax,[NamePtr]
                mov     word [RqHdr+12h],ax

                push    cs
                pop     es

        ; DS:TopCSeg, ES:TopCSeg

        ; Store registers (don't count on driver to keep them).

                push    si

        ; Set ES:BX to point to RqHdr (for DvcStrat call.)

                mov     bx,RqHdr

        ; Set up return addresses on stack.

                mov     ds,[cs:NewDrvSeg]

        ; DS:NewDrvSeg, ES:TopCSeg
                
        ; Push far address of DEVLOAD.

                push    cs
                mov     ax,after_int
                push    ax

        ; Push far address of dvc_int.

                push    ds
                push    word [si+8]

        ; Push far address of dvc_strat

                push    ds
                push    word [si+6]

        ; Pass control to dvc_strat, which RETFs to dvc_int, which
        ; in turn RETFs to after_int.

BREAKPOINT2:    ; retf		; *** Arrowsoft ASM screws this ***
		db 0cbh		; *** manually inserted RETF is byte 0cbh

        ; Restore registers.

after_int:      pop     si

	; *** new 7/2005: copy number of units ("blocks") to device
	; *** header if nonzero and block device. DOS kernel does
	; *** the same, as not all devices set their own header...
                test    byte [si+5],80h	; +4 is attr, 8000 is char
		jnz	ischardev
                mov     al,byte [cs:RqHdr+0Dh]	; number of units
		or	al,al
		jz	zerounits
		mov	byte [si+0Ah],al
	; Device header is: far pointer to next device,
	; word attributes, two near pointers to dvc_strat and dvc_int,
	; then for block devices [byte number of units, 7 chars name]
	; and for char devices [8 chars name], names 00 or space padded.
	; name is optional for block- but important for char-devices!
zerounits:
ischardev:
	; *** end of new 7/2005 code

        ; Increase count of character devices if it is one.

                test    byte [si+5],80h
                push    cs
                pop     ds
                jz      notchardev
                inc     byte [CharsDone]	; even if init failed for THIS part?
					; (device drivers can be multi-part)

; ***   ; Print CrLf after driver.

notchardev:
; ***           mov     dx,offset CrLfMsg
; ***           mov     ah,9
; ***           int     21h

        ; Get offset of end of driver.

                mov     ax,word [RqHdr+0Eh]

        ; Convert to paragraphs (rounded up.)

                add     ax,0Fh
                rcr     ax,1
                mov     cl,3
                shr     ax,cl

        ; Add segment of end of driver.

                add     ax,word [RqHdr+10h]

        ; Compare with previous value of EndSeg

                cmp     ax,[EndSeg]
                jb      endset

        ; EndSeg has increased - this is only a problem if blocks are
        ; already installed.

                test    byte [BlocksDone],0FFh	; *** byte ptr ***
                jz      oktogrow

        ; Some block headers are already at EndSeg - can't change it now.
        
                mov     dx,BadIncMsg
                mov     ah,9
                int     21h
                jmp     short endset	; optimization 3.16

        ; No block headers done yet - change EndSeg.
                
oktogrow:       mov     [EndSeg],ax

        ; DS:TopCSeg

        ; Check number of units in driver. Skip if none.
	; (CHAR devices are supposed to have left our 0 in this place)

endset:         mov     ch,[RqHdr+0Dh]	; explicit segment in 3.16
                or      ch,ch
                jnz     yesunits

        ; No units in this device, so skip the next section.

                jmp     nounits

        ; Zero count of block number in this device.

yesunits:       xor     cl,cl

        ; Point ES:BP to new block header location.

                les     bp,[BlHEnd]

        ; Point DS:BX to BPB pointer array from driver.

                lds     bx,[RqHdr+12h]

        ; Point DS:SI to next BPB and increase pointer.

nextblk:        mov     si,[bx]	; explicit segment in 3.16
                inc     bx
                inc     bx

        ; Check sector size.

                mov     ax,[si]	; explicit segment in 3.16
                cmp     ax,[cs:SecSize]
                jna     secsizeok
                jmp     secsizeerr

secsizeok:      mov     al,[cs:LastDrUsed]
                mov     byte [cs:BlHdrMsgA], byte 'A'	; explicit byte 3.16
;		nop					; mimick ASM
                add     [cs:BlHdrMsgA],al
        
        ; Check lastdrive.

                cmp     al,[cs:LastDrive]
                jnz     ldrok
                jmp     ldrerr

        ; Increase count of block devices and update to next free CDS entry

ldrok:

                inc     byte [cs:nBlkDev]
                inc     byte [cs:LastDrUsed]            ; point to next available drive *** TODO fixme

                ; If 1st block device loaded, store drive letter for returned exit code
                test    byte [cs:ExitCode], 0
                jnz     notfirst
                mov     byte [cs:ExitCode], al          ; al=LastDrUsed where A=0,...,Z=25
                inc     byte [cs:ExitCode]              ; A=1, ..., Z=26
                
notfirst:

        ; Store absolute block no. and block no. in device.

                mov     ah,cl
                inc     cl
                mov     [es:bp],ax

        ; Store pointer to BPB ptr array.

                push    ds
                push    bx

        ; Point DS:BX to last block header in chain.

                lds     bx,[cs:ChainEnd]

        ; Make it point to the new one.

                add     bx,word [cs:NextBlHOfs]	; ASM to NASM: explicit cs:
                mov     [bx],bp
                mov     [bx+2],es
                sub     bx,word [cs:NextBlHOfs]	; ASM to NASM: explicit cs:

                push    cs
                pop     ds

        ; Store new pointer to end of block header chain.

                mov     [ChainEndOfs],bp
                mov     [ChainEndSeg],es
                
        ; Print address of new block header if Verbose Mode.

                test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintblhmsg

                mov     dx,BlHdrMsg
                mov     ah,9
                int     21h
                
                mov     ax,es
                call    PHWord
                mov     ah,02h
                mov     dl,3ah	; colon
                int     21h
                mov     ax,bp
                call    PHWord

                mov     dx,CrLfMsg
                mov     ah,9
                int     21h

        ; Get pointer to LASTDRIVE array.

noprintblhmsg:  lds     bx,[Invar]
                lds     bx,[bx+16h]

        ; Calculate offset in array of entry for this drive.

                mov     al,[cs:LastDrUsed]
                dec     al
                mov     ah,[cs:LDrSize]
                mul     ah
                add     bx,ax

        ; Insert 'valid' flag.

                mov     byte [bx+44h],40h

        ; Insert pointer to block header for this drive.

                mov     word [bx+45h],bp
                mov     word [bx+47h],es

        ; Restore pointer to BPB pointer array.

                pop     bx
                pop     ds

        ; Insert pointer to device into block header.

                mov     ax,[cs:NewDrvOfs]
                add     bp,word [cs:NextBlHOfs]	; ASM to NASM: explicit cs:
                mov     [es:bp-6],ax
                mov     ax,[cs:NewDrvSeg]
                mov     [es:bp-4],ax

        ; Insert 'BPB needs rebuilding' flag into block header.

                mov     byte [es:bp-1],0FFh
                
        ; Insert 'End of chain' into block header.

                mov     word [es:bp],0FFFFh
                sub     bp,word [cs:NextBlHOfs]	; ASM to NASM: explicit cs:

        ; Expand BPB into block header
        ; *** Extended in DEVLOAD 3.18: use magic CX/DX ***
		push	cx            ; Save registers
;		push    dx            ;
		mov	cx, 4558h     ; Signature for calling
		mov	dx, 4152h     ; FAT32 extension
                mov     ah,53h        ; convert BPB to DPB
                int     21h
;		pop	dx            ; Restore registers
		pop	cx            ;

        ; Point ES:BP to location of next block header.

                add     bp,[cs:BlHSize]

        ; Store location of next block header.

                mov     [cs:BlHEndOfs],bp

        ; Increase count of blocks installed.

                inc     byte [cs:BlocksDone]

        ; Loop if more blocks in this device to install.

nxtblkchk:      cmp     cl,ch
                jz      nounits
                jmp     nextblk

        ; Sector size too big - print error and fail to install this block.

secsizeerr:     push    cs
                pop     ds
                mov     dx,SSizeErrMsg
                mov     ah,9
                int     21h
                inc     cl
                jmp     nxtblkchk

        ; Lastdrive too small - signal error and don't install any more.

ldrerr:         push    cs
                pop     ds
                sub     ch,cl
                add     [LDrErrMsg],ch

        ; Finished installing units, or was none to install.

nounits:        push    cs
                pop     ds

        ; Print init return status IF Verbose Mode.

                test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintinitret

                mov     dx,InitRetMsg
                mov     ah,9
                int     21h
                mov     ax,word [RqHdr+3]	; explicit segment in 3.16
                call    PHWord

                mov     dx,FullStopMsg	; 'h.',cr,lf
                mov     ah,9
                int     21h

        ; DS:TopCSeg

        ; Set up pointers.

noprintinitret: les     bx,[OldDrv]
                lds     si,[NewDrv]

        ; DS:NewDvcSeg, ES:OldDvcSeg

        ; Find location of next driver in file.

                mov     ax,[si+2]
                push    ax

                mov     ax,[si]
                push    ax

        ; Link NewDrv to OldDrv.

                mov     [si],bx
                mov     [si+2],es

        ; Restore next driver in file address to AX:SI

                pop     si
                pop     ax

        ; Point ES:BX to just installed driver.

                les     bx,[cs:NewDrv]

        ; Check whether to change segment or use same seg for next driver.

                cmp     ax,0FFFFh
                jz      nosegchange

        ; Segment is different - add value onto old segment and put into DS.

                mov     cx,ds
                add     cx,ax
                mov     ds,cx

        ; Set zero flag on whether this is the last driver in the file.

nosegchange:    cmp     si,byte -1	; ASM to NASM: 0FFFFh to byte -1
                retn

; .............................TEXT MESSAGES.................................

StayingMsg	db 'Driver loaded.',13,10,24h
		; db 'Driver staying resident.',13,10,24h
FNameMsg        db 'Loading: $'		; db 'Filename: $'
LoadAddrMsg     db 'Loaded at: $'
InitRetMsg      db 'Init status: $'
SizeMsg         db 'Driver size in paras: $'
WantedParasMsg	db 'Init driver size in paras: $'
IntChangeMsg    db 'Int vectors changed: $'
NoneMsg         db 'None.',13,10,24h
CommaMsg        db 'h, $'
FullStopMsg     db 'h.',13,10,24h
Colon0Msg       db ':'
NotStayMsg      db '0000'
CrLfMsg         db 13,10,24h
LastDrMsg       db 13,10,'Last used drive: '
LDMsgA          db 'A:',13,10,'LASTDRIVE: '
LDMsgB          db 'A:',13,10,24h
BlHdrMsg        db 'DPB for '
BlHdrMsgA       db 'A: at $'
NumBlInstMsg    db '0 drives$'
NumChInstMsg    db '0 char device$'
NumInstMsgA     db 's installed.',13,10,24h
LDrErrMsg       db '0 drives skipped - LASTDRIVE overflow.',13,10,24h
AskEndMsg       db 13,10,'No drives or Ints - unload (Y/N) ? $'
NoFreeCDSMsg    db 'Error: free drive letter not found, increase LASTDRIVE',13,10,'$' 
SSizeErrMsg     db 'Sector size too large.',13,10,24h
NotLoadedMsg    db 'Driver not loaded',13,10,'$'

        ; Error messages.

ReduceErrMsg    db "Error: Can't reduce memory allocation ($"
RelEnvErrMsg    db "Error: Can't release ENV ($"
RelPSPErrMsg    db "Error: Can't release PSP ($"
GrabHiErrMsg    db "Error: Can't grab memory to relocate ($"
GrabLoErrMsg    db "Error: Can't grab memory to load driver ($"
Reduce2ErrMsg   db "Error: Can't resize memory allocation ($"
		; 'Driver grew after block header setup.'
BadIncMsg       db "Error: Block driver overflowed memory allocation.",13,10
		; error message continues with Reduce2ErrMsga
Reduce2ErrMsga  db "Better reboot now.",13,10,24h
NotLoadMsg      db 13,10,"Error: EXEC failed ($"
TooBigMsg       db "Error: Driver overflowed memory allocation."
		db 13,10,24h

        ; Error cause messages.

Err1    	db 'h - invalid request)',13,10,24h
Err2            db 'h - File not found)',13,10,24h
Err3            db "h - Dir not found)",13,10,24h
Err5            db 'h - Access denied)',13,10,24h
Err7            db 'h - MCB bad)',13,10,24h ; MCB destroyed, Arena header bad
Err8            db 'h - Out of memory)',13,10,24h
Err9            db 'h - MCB not found)',13,10,24h
; ***           db ' PLEASE INFORM THE AUTHOR!',13,10,24h
; ErrB            db 'h - Format invalid)',13,10,24h
		; FreeDOS only returns error 0b, DE_INVLDFMT, if you
		; try to use DosExec with mode&7f not in 0, 1, 3,
		; in other words if you use int 21.4b with bad AL
ErrUnknown      db 'h'
BadSwitchMsg2   db ')',13,10,24h


ErrTable        dw      ErrUnknown	; no error
                dw      Err1    	; function nr. bad / unsupported
                dw      Err2		; file not found
                dw      Err3		; path / dir not found
                dw      ErrUnknown	; <-- too many files open
                dw      Err5		; access denied (to file / dir)
                dw      ErrUnknown	; invalid (file) handle
                dw      Err7		; MCB bad (has no M, Z... header)
                dw      Err8		; out of memory
                dw      Err9		; MCB addr invalid
;                 dw      ErrUnknown	; env bad (> 32k long, in exec)
;                 dw      ErrUnknown ; ErrB	; can happen in int 21.4b

; ............................PROGRAM DATA...................................

LDrSize         db      58h,0	; size of block in LastDrive array.
BlHSize         dw      0021h	; size in paras of parameter block.
NextBlHOfs      db      19h,0	; offset in parameter block of ptr to next one.

BlocksDone      db      00	; no. of blocks installed.
CharsDone       db      00	; no. of character devices installed.

DvcSeg          dw	0	; ? parameter block for EXEC function.
                dw      0000	; i.e. segment, relocation factor.

ModeFlag        db      00	; command line switch flags

BlHEnd:
BlHEndOfs       dw      0000
EndSeg          dw	0	; ? segment, end of required memory.

PSPSeg          dw	0	; ?

PathPtr         dd      0
EmptyPath	dw	0

NameBuffer      times	80h	db	0	; resb      80h
                  
nBlkDev         db      0       ; resb  1       ; Note: nBlkDev & LastDrive
LastDrive       db      0       ; resb  1       ; must be together as loaded from LoL together
LastDrUsed      db      2                       ; default to C:, may be overridden by cmd line option

ExitCode        db      0       ; default to 0 for char devices, 1=A to 26=Z for 1st block device loaded

OldAllocStrat   dw	0	; resw	1	; DOS allocation strategy
BlockSize       dw	0	; resw	1
WantedParas	dw	0	; minimum paragraphs or RAM to load driver

NamePtr         dw	0	; resw	1	; pointer to start of name.
NameLen         dw	0	; resw	1

Invar:		; list of lists - see int 21 function 52
InvarOfs        dw	0	; resw	1
InvarSeg        dw	0	; resw	1

ChainEnd:
ChainEndOfs     dw	0	; ? last device parameter block in chain.
ChainEndSeg     dw	0	; resw	1

SecSize         dw	0	; resw	1

NewDrv:
NewDrvOfs       dw	0	; ? storage for InstallDevice routine.
NewDrvSeg       dw	0	; resw	1

OldDrv:
OldDrvOfs       dw	0	; resw	1
OldDrvSeg       dw	0	; resw	1

RqHdr           times	20h	db 0	; resb      20h

FileBuffer	equ	IntVectors+300h	; 100h bytes overlap (20h used now)
IntVectors      times	200h	dw 0	; ? storage for interrupt vectors,
                        		; for checking whether they've changed.

        ; Marker to signal last byte that needs relocation.

LASTBYTE        equ     $

; ................DATA WHICH ISN'T NEEDED AFTER RELOCATION...................

SignOnMsg	db 'DEVLOAD v3.25 - load DOS device drivers '
		db '- license' ; db '- free -'
		db ' GNU General Public License 2',13,10
		db '(c) 1992-2011 David Woodhouse, Eric Auer '
		db '(FreeDOS.org)',13,10
		db 13,10,24h
%if 0
		db 'DEVLOAD v3.21 (C) 1992-1996 David Woodhouse '
                db ' <dwmw2@infradead.org>',13,10
		db 'Eric Auer 2004-2008'
		db ' <eric*coli.uni-sb.de> License: GNU GPLv2',13,10
                db ' Loads device drivers.',13,10
; kind of obvious...:	db ' Needs DOS 3 or newer.'
		db 13,10,24h
%endif

HelpMsg1	db 'Usage:    DEVLOAD [switches] filename [params]',13,10
                db 'Emulates: DEVICE=filename [params] in CONFIG.SYS',13,10
                db 13,10,'Switches:',13,10
                db '      /? - display this help message.',13,10
                db '      /H - try to load driver to UMB.',13,10
                db '      /Q - quiet mode.',13,10
                db '      /V - verbose mode.',13,10
                db '      /A - auto-mode (see docs).',13,10
                db '      /D - drive letter, eg. /DS installs to S: or later.',13,10
		db 13,10
		db 'DEVLOAD is free software. It comes with NO waranty.',13,10
                db 'Debug hints: self-reloc @ $'
HelpMsg2        db ', driver exec @ $'

umbmsg		db 'Trying to use UMB.',13,10,'$'
noumbmsg	db 'No UMB: DOS 5 or newer needed.',13,10,'$'

BadSwitchMsg    db 'Error: Bad switch ($'
; NoFileMsg	db 'DEVLOAD /? shows help.',13,10,'$'
FileNoExistMsg  db "Error: Can't open file ($"
BadVerMsg       db 'Error: DOS 3 or newer needed.',13,10,'$'


; ........MAIN PROGRAM ENTRY POINT - SITUATE ABOVE LASTBYTE BECAUSE..........
; .....IT DOESN'T NEED TO BE KEPT WHEN RELOCATING TO THE TOP OF MEMORY.......

        ; Set up segment registers.

Main:           push    cs
                pop     ds
                push    cs
                pop     es
                cld

        ; Get PSP segment.

                mov     ah,62h
                int     21h
                mov     [cs:PSPSeg],bx

; ***   ; Print sign on message. <-- moved further below

; ***           mov     dx,offset SignOnMsg
; ***           mov     ah,9
; ***           int     21h

        ; Check DOS version.

                mov     ax,3000h
                int     21h
                cmp     al,3
                ja      okver
                jz      ver3

        ; Version before 3.0, so print error and exit.

badver:		mov     dx,BadVerMsg
		jmp     prexit

        ; Version 3.x, so change variables to correct values.
	; ... check for DR DOS first, idea of Diego Rodriguez, 3.16

ver3:		; DR DOS up to 7.00 "are DOS 3.31" but pre-DR-6.0 fail:
		; int 21.4452 --> if no carry, and if ah=10, then we
		; have DR DOS kernel revision AL... ah=14 is multiuser.
		; Diego reports kernel 71h working, 70h is untested
		; Palm DOS, older kernels are known not to work for us.
		; Kernel BDOS 1071 is DR DOS 6.0 in the 03/1993 update.
		; AH is actually 2 nibbles: 0/1 8080/8086 high, and
		; 0/1/2/4 for CP/M, MP/M, CP/Net, multiuser resp...!

	; From RBIL: Caldera OpenDOS 7.01 is Novell DOS 7 rev 10 plus
	; minor changes (lost patches), 7.02 recovers patched up to
	; rev 15 plus some re-written patches by Matthias Paul 1997...
	; BDOS 106F/1070 of 1992/1993 are around ViewMAX/3 GUI (more
	; Win than GEM, later released under GPL) and multitasking,
	; including an unpublished Novell/Apple "MacOS 7.1 for DOS"...
	; Finally, version 72 is Novell DOS and 73 is OpenDOS 7.02,
	; later also DR DOS 7.02 and 7.03 of 1999 use that BDOS value.

	; BDOS 1071 is the only DR DOS which reports MS DOS 3.xx
	; which actually works with DEVLOAD. Older ones do not work.
	; Newer ones do not hit this test anyway: They report DOS 6.

		stc
		push	dx		; preserve DX
		mov	ax,4452h	; "DR" - get DR DOS revision
		int	21h
		pop	dx		; usually will be equal to AX
		jc	realver3	; error means: not a DR DOS
		cmp	ah,10h		; single user DR DOS?
		jnz	badver		; reject CP/M, 8080, MP/M, CP/Net
		cmp	al,71h		; first tested-to-work kernel
		jb	badver		; 72h+ report MS version 6...

; DOS CDS (current directory structure) array has LASTDRIVE entries of
; 58h bytes each, 51h for DOS 3.x ... DOS Block headers are the DPB
; (drive parameter block) items, 20h bytes in DOS 3.x, 21h in 4.0-6.0
; FAT32 DPB start like FAT1x ones, but are 1ch bytes longer ... (3.17)
realver3:	mov     byte [LDrSize],51h
		mov     byte [BlHSize],20h 
		mov     byte [NextBlHOfs],18h
		jmp	short classicver

okver:		; Check FAT32 compatibility: VER >= 7.01
		; (new in 3.17, extended in 3.18 and in 3.20)
		cmp	al,5	 ; new 3.17
		jb	classicver
		push	ax       ; Save DOS version from int 21.3000
		push    bx       ; Save OEM ID      from int 21.3000
		mov	ax,3306h ; Get true DOS version
		int	21h
		mov	cl,8     ; Swap BH and BL
		ror	bx,cl    ; 
		cmp	bx,0701h ; VER >= 7.01 ?
                pop	bx       ; Restore DOS version from int 21.3000
		pop	ax       ; Restore OEM ID      from int 21.3000 
		jb	classicver		

	; FAT32 aware version: larger DPB, but same DPB pointer / CDS size
		mov	ch, 3Dh    ; Set BlHSize (new in 3.20)
		; Check EDR-DOS presence (added in 3.20)
		; This code is required because EDR-DOS uses non-typical
		; FAT32 DPB (it has 65 b size instead of 61 b)
		jne	stdFAT32   ; If VER != 7.01 - it is not EDR-DOS
		cmp	bx, 0EE00h ; 0EE00h is EDR-DOS OEM ID
		jne	stdFAT32   ;
		cmp	ax, 00006h ; EDR-DOS always returns ver 6.00
		jne	stdFAT32   ; after calling function int 21.3000
		mov	ch, 41h    ; Increase DPB size for EDR-DOS (added in 3.20)
stdFAT32:	mov	byte [BlHSize], ch ; Set FAT32 DPB size (3Dh or 41h)

classicver:
        ; Check command line.

                mov     ds,[PSPSeg]
                xor     bh,bh
                mov     bl,byte [80h]
                or      bx,bx
                jnz     cmdlineexists

        ; No parameters given - print error and exit.

nofilename:
		jmp	help
		; before 3.21: print SignOnMsg prexit with NoFileMsg

        ; Command line exists - convert to all upper case.

cmdlineexists:  mov     si,81h
                mov     cx,bx
toupperloop:    lodsb
                cmp     al,'a'
                jb      notlower
                cmp     al,'z'
                ja      notlower
                xor     al,20h
                mov     [si-1],al
notlower:       loop    toupperloop

        ; Check whether filename present.

                mov     si,0081h
                add     bx,si

        ; DS:SI--> Start of command line.
        ; DS:BX--> End of command line.

getloop1:       lodsb

        ; If passed end of command line, exit loop.

                cmp     si,bx
                ja      nofilename

        ; Loop while whitespace.

                cmp     al,' '
                jz      getloop1
                cmp     al,9
                jz      getloop1

        ; Found non-whitespace, point back at it.

                dec     si

        ; DS:SI --> first non-whitespace char on command line.

        ; Get current switch char (usually '/').

                mov     ax,3700h
                int     21h

        ; Check whether first char on command line is a switch.

                cmp     [si],dl
                jz      isswitch
                jmp     noswitch

        ; Load switch and check it.

isswitch:       lodsw
                cmp     ah,'?'
                jz      help
                cmp     ah,'H'
                jz      umb	; *** was help ***
                cmp     ah,'Q'
                jz      quiet
                cmp     ah,'A'
                jz      auto
                cmp     ah,'V'
                jz      verbose
                cmp     ah,'D'
                jz      driveletter

        ; Unrecognised switch - print error and exit.

unknownswitch:  push    ax

                push    cs
                pop     ds

       ; Print sign on message before error message
                mov     dx,SignOnMsg
                mov     ah,9
                int     21h


                mov     dx,BadSwitchMsg
                mov     ah,9
                int     21h

                pop     dx
                mov     ah,2
                int     21h
                mov     dl,dh
                int     21h

                mov     dx,BadSwitchMsg2
                jmp     prexit

        ; Print help message.

help:           push    cs
                pop     ds

       ; Print sign on message before showing the help screen
                mov     dx,SignOnMsg
                mov     ah,9
                int     21h


                mov     dx,HelpMsg1
                mov     ah,9
                int     21h
                mov     ax,BREAKPOINT1
                call    PHWord
                mov     dx,HelpMsg2
                mov     ah,9
                int     21h
                mov     ax,BREAKPOINT2
                call    PHWord
                mov     dx,CrLfMsg
                jmp     prexit

        ; Set verbose mode flag.

verbose:        or      byte [cs:ModeFlag],VerboseFlag
                and     byte [cs:ModeFlag],~ QuietFlag
                jmp     short switchloop	; optimization 3.16

        ; Set automatic mode flag.

auto:           or      byte [cs:ModeFlag],AutoFlag
                jmp     short switchloop	; optimization 3.16

        ; Set quiet mode flag.

quiet:          or      byte [cs:ModeFlag],QuietFlag
                and     byte [cs:ModeFlag],~ VerboseFlag
                jmp	short switchloop	; optimization 3.16

        ; Set initial drive letter to assign device to (or 1st available after this one)
driveletter:
                lodsb           ; get drive letter, spaces before it are not supported e.g. /DS
                sub     al,65   ; convert letter to 0 based #, cmd line upcased above so -'A'
                mov     [CS:LastDrUsed], al
                jmp     short switchloop

        ; Set load into UMB (devicehigh) mode flag.

umb:            push	bx		; *** added DOS version check ***
		push	cx
		push	ax
		push	dx
		mov	ax,3000h	; get DOS version AL.AH (.BH.BL.CX)
		int	21h
		mov	dx,noumbmsg
		cmp	al,5		; at least version 5 ? (3.15 tested AH, bug)
		jb	umbskip		; else no UMBs avail
		xor	dx,dx
		or      byte [cs:ModeFlag],UMBFlag
umbskip:	push	ds
		mov	bx,cs
		mov	ds,bx
                test    byte [ModeFlag],QuietFlag	; *** new 3.15
		jnz	umbquiet			; new 3.15
		or	dx,dx
		jz	umbquiet
		mov	ah,9		; show message at ds:dx
		int	21h
umbquiet:						; new 3.15
		pop	ds
		pop	dx
		pop	ax
		pop	cx
		pop	bx		; *** end of added part ***

                jmp     short switchloop	; optimization 3.16

        ; Skip to next space.

switchloop:     lodsb
                cmp     si,bx
                jna     switchloop1
                jmp     nofilename

switchloop1:    cmp     al,9
                jz      outswitchloop
                cmp     al,' '
                jnz     switchloop

        ; Point back at first space and go back to getloop1 to skip
        ; to either next switch or to filename.

outswitchloop:  dec     si
                jmp     getloop1

        ; Store pointer to start of pathname.

noswitch:       push    si
                mov     bp,si

        ; Find pointer to actual 8-char filename and end of pathname.

getloop2:       lodsb
                cmp     al,'\'
                jz      backsl
                cmp     al,'/'
                jnz     nobacksl

        ; Move pointer to after backslash into BP.

backsl:         mov     bp,si

        ; Break out of loop if space, tab, CR or LF found.

nobacksl:       cmp     al,' '
                jz      outloop2
                cmp     al,9
                jz      outloop2
                cmp     al,13
                jz      outloop2
                cmp     al,10
                jz      outloop2

        ; Check whether end of command line reached. Loop if not.

                cmp     si,bx
                jna     getloop2

        ; DS:SI-2 --> last char of pathname.
        ; DS:BP --> first char in filename.

        ; Calculate length of filename.

outloop2:

       ; *** Print sign on message now, AFTER parsing command line ***
                test    byte [ModeFlag],QuietFlag	; ***
                jnz     noprintsignon
		push ax
		push dx
                mov     dx,SignOnMsg	; (DS -> CS already)
                mov     ah,9
                int     21h
                test    byte [ModeFlag],UMBFlag		; *** new 3.15
		jz	noumbflag			; new 3.15
		mov	dx,umbmsg
		mov	ah,9
		int	21h
noumbflag:	pop dx
		pop ax
noprintsignon:

                mov     [es:NamePtr],bp
                dec     si
                mov     cx,si
                sub     si,bp
                mov     [es:NameLen],si

        ; Restore pointer to start of pathname.

                pop     si

        ; Check whether file specified contains path.

                cmp     si,bp
                jz      notpathname

        ; Set PathPtr to point to zero - simulate no PATHs left.

; ???                mov     word ptr es:PathPtr,offset DvcSeg+2
                mov     word [es:PathPtr],EmptyPath	; 8/2007
                mov     word [es:PathPtr+2],cs

        ; Start with default directory.

notpathname:    sub     cx,si
                mov     di,NameBuffer

        ; Use default directory or one specified first time, not PATH.

                jmp     entrypoint

; ...........................................................................

        ; Filename doesn't exist as specified - try using PATH.

        ; Check whether we've already got a pointer to PATH.

allpathloop:    lds     si,[PathPtr]
                or      si,si
                jnz     pathfound

        ; Not yet, so find PATH segment.

                mov     ds,[cs:PSPSeg]
                mov     bx,word [002Ch]

        ; Check whether it exists.

                or      bx,bx
                jnz     envsegexists

        ; No more PATH items or no PATH segment, so print error and exit.

filenoexist:    push    cs
                pop     ds
                mov     dx,FileNoExistMsg
                jmp     fileerr

        ; Store PATH segment in local pointer.

envsegexists:   mov     ds,bx
                mov     word [cs:PathPtr+2],bx

        ; Scan environment for 'PATH='
                                     
envloop1:       lodsb
                cmp     al,0
                jz      filenoexist
                cmp     al,'P'
                jnz     nextenvvar1
                lodsb
                cmp     al,'A'
                jnz     nextenvvar1
                lodsb
                cmp     al,'T'
                jnz     nextenvvar1
                lodsb
                cmp     al,'H'
                jnz     nextenvvar1
                lodsb
                cmp     al,'='
                jnz     nextenvvar1
                jmp     short pathfound		; optimization 3.16

        ; Not 'PATH=', so skip to next environment variable.

nextenvvar:     lodsb
nextenvvar1:    or      al,al
                jnz     nextenvvar
                jmp     envloop1

        ; Store file error message.

pathfound:      push    ax

        ; Skip spaces at start of this PATH item.

pathfoundloop:  lodsb
                cmp     al,' '
                jz      pathfoundloop
                cmp     al,9
                jz      pathfoundloop

        ; DS:SI-1 --> first non-whitespace in PATH item.

        ; If we've reached the end of the PATH statement, error and exit.

                cmp     al,0
                jz      filenoexist

        ; Forget file error message - we'll try again.

                add     sp,byte 2	; ASM to NASM: explicit byte

        ; Store start of this PATH item + 1.

                push    si

        ; Find end of this PATH item.

pathloop1:      lodsb
                cmp     al,0
                jz      endpath
                cmp     al,';'
                jnz     pathloop1

        ; Store start of next PATH item.

endpath:        mov     word [cs:PathPtr],si

        ; If last one, point back at the terminating NULL.

                or      al,al
                jnz     ismorepaths
                dec     word [cs:PathPtr]

        ; Calculate length of this PATH item.

ismorepaths:    mov     cx,si
                pop     si
                sub     cx,si
                dec     si

        ; Copy PATH item to NameBuffer.

                mov     di,NameBuffer
                rep     movsb

        ; Add backslash if necessary.

                push    cs
                pop     ds

                cmp     byte [di-1],'\'
                jz      alreadybacksl
                mov     al,'\'
                stosb

        ; Copy filename after PATH item.

alreadybacksl:  mov     si,[NamePtr]
                mov     cx,[NameLen]

                mov     ds,[PSPSeg]

entrypoint:     rep     movsb

        ; Store terminating NULL.

                xor     al,al
                stosb

        ; Check whether file exists by attempting to get attributes.

                push    cs
                pop     ds

                mov     dx,NameBuffer
                mov     ax,4300h
                int     21h
                jnc     foundfilename
                jmp     allpathloop

        ; Found file entry, but is it a directory?

foundfilename:  test    cl,10h
                jz      okfilename

        ; If it is, make the error code into `file not found'
        
                mov     ax,2
                jmp     allpathloop

	; set error code to our own file read error one

filereaderror:
		mov	ax,1		; self-defined error code
		jmp	allpathloop

	; check file header to know how much RAM the driver wants

okfilename:	mov	ax,3d00h	; open file ds:dx for read
		int	21h
		jc	filereaderror
		mov	bx,ax		; handle
		mov	ax,3f00h	; read file
		mov	cx,1ch		; just the header
		mov	dx,FileBuffer	; abuse as file buffer
		int	21h
		jc	filereaderror
		cmp	ax,1ch
		jb	filereaderror
		cmp	word [FileBuffer],'MZ'
		jz	exesize		; (if not EXE-SYS: plain-SYS)
		mov	ax,4202h	; seek to end, handle BX
		xor	cx,cx
		xor	dx,dx
		int	21h
		jc	filereaderror
		; file size is now in DX:AX
		push	ax
		mov	ax,3e00h	; close file, handle BX
		int	21h		; no error handling :-p
		pop	ax
		jmp	convertsize

exesize:	mov	ax,3e00h	; close file, handle BX
		int	21h
		mov	ax,[FileBuffer+4] ; pages, rounded up (+2: fraction)
		xor	dx,dx
		mov	bx,512		; page size
		mul	bx
		mov	bx,[FileBuffer+10] ; min extra paras (+12: max extra)
		mov	cl,4
		shl	bx,cl
		add	ax,bx
		adc	dx,byte 0	; size is now in DX:AX

convertsize:	add	ax,byte 15
		adc	dx,byte 0
		and	dx,15		; use size modulo 1 MB, as 21.4b
		mov	bx,16		; convert to paras
		div	bx
		mov	[WantedParas],ax

	; show information about file size / header, if verbose:

		test    byte [ModeFlag],VerboseFlag	; *** byte ptr ***
                jz      noprintwp
                mov     dx,WantedParasMsg	; 'Minimum RAM'
                mov     ah,9
                int     21h
                mov     ax,[WantedParas]	; Min paras to load driver
                call    PHWord
		mov	dx,FullStopMsg		; 'h',cr,lf
		mov	ah,9
		int	21h
noprintwp:

        ; File exists - expand filename using function 60h.

		mov     si,NameBuffer
                mov     di,si
                mov     ah,60h
                int     21h

        ; Get old allocation strategy.

                mov     ax,5800h
                int     21h
                mov     [OldAllocStrat],ax

        ; Reduce main allocation.

        ; DS:CSeg, ES:CSeg

                mov     bx,LASTBYTE+10Fh	; resident part and PSP
                add     bx,STACKLEN	; stack
                mov     cl,4
                shr     bx,cl
                mov     cx,bx
                mov     es,[PSPSeg]
                mov     ah,4ah
                int     21h
                jnc     reduceok

        ; Failed to reduce memory allocation, so print error and exit.

                mov     dx,ReduceErrMsg
                jmp     allocerr

        ; Set allocation strategy to highest fit.

reduceok:       mov     ax,5801h
                mov     bx,2		; *** low memory last fit ***
                int     21h

        ; Request enough at top of mem for PSP + DEVLOAD + STACK.

                mov     bx,cx
                mov     ah,48h
                int     21h
                pushf
                mov     es,ax

        ; Reset allocation strategy to old value.

        ; DS:CSeg, ES:TopPSPSeg

                mov     ax,5801h
                mov     bx,[OldAllocStrat]
                int     21h

        ; Check whether grabbed memory OK.

                popf
                jnc     nograbhierr

        ; Failed to grab memory, so print error and exit.

                mov     dx,GrabHiErrMsg
                jmp     allocerr

        ; Make new PSP at top of memory.

        ; DS:CSeg, ES:TopPSPSeg

nograbhierr:    mov     ds,[PSPSeg]
                mov     si,word [2]	; explicit segment in 3.16 (avoid TASM problem)
                mov     dx,es
                mov     ah,55h
                int     21h

        ; Fix parent PSP record in new PSP.

                mov     ax,[16h]
                mov     [es:16h],ax

        ; Move program to top of memory.

                mov     cx,LASTBYTE+80h+0Fh
                and     cx,0FFF0h
                shr     cx,1
                mov     di,80h	; *** why is that 80h? command line? ***
                mov     si,di
                rep     movsw

        ; Make segment at top of memory self-owned.

                push    cs
                pop     ds

                mov     ax,es	; target segment
                dec     ax
                mov     ds,ax	; mcb for target segment
                mov     word [1],es

                push    cs
                pop     ds

        ; Make PSP at top of memory current.	; *** moved this up here ***

                mov     bx,es
                mov     ah,50h
                int     21h

        ; Calculate location of stack at top of memory.

                mov     bx,LASTBYTE+10Fh	; resident part + PSP
                mov     cl,4
                shr     bx,cl
                mov     ax,es
                add     ax,bx

        ; Change to stack at top of memory.
	; *** originally happened BEFORE the int 21.50 call, problem...
	; (the stack overwrites this code after a while!)

		cli			; *** added ***
                mov     ss,ax
	; *** SP was 0 + stacklen in the .exe case ***
		mov	sp,STACKLEN-4	; *** needed for .com version ***


        ; Transfer control to top of memory via RETF.
	; *** layout there is: relocated PSP, copied code, stack
	; (problem: .com version breaks some assumptions here :-/)

                mov     ax,es
;               add     ax,10h	; *** not for .com version! ***
                mov     ds,ax
                push    ax
                mov     ax,relocated
                push    ax
		sti		; *** added ***
BREAKPOINT1:    ; ret far	; *** Arrowsoft ASM screws up retf / ret far
		db 0cbh		; *** manually encoded RETF is byte 0cbh





; *** removing stack segment to create a .com version ***
; SSeg    segment stack   para    'STACK'
;
;        org     0
;
;        db      STACKLEN dup (?)
; SSeg            ends

;       end	; *** can be Main for .exe version ***


