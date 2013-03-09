;;
;;   XMS.ASM - routines to use Extended Memory from a DOS program.
;;
;;   Copyright (C) 1999, 2000, Imre Leber.
;;
;;   This program is free software; you can redistribute it and/or modify
;;   it under the terms of the GNU General Public License as published by
;;   the Free Software Foundation; either version 2 of the License, or
;;   (at your option) any later version.
;;
;;   This program is distributed in the hope that it will be useful,
;;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;   GNU General Public License for more details.
;;
;;   You should have recieved a copy of the GNU General Public License
;;   along with this program; if not, write to the Free Software
;;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;;
;;   If you have any questions, comments, suggestions, or fixes please
;;   email me at:  imre.leber@worldonline.be
;;
;;
;;*************************************************************************
;;
;; XMS.ASM
;;
;; Routines to use Extended Memory, the HMA and upper memory from
;; a DOS program.
;;
;; NOTE: Some of these routines are translations from the XMS routines 
;;       by Cliff Rhodes to NASM by Imre Leber.
;;
;; The C version was released to the public domain by Cliff Rhodes with
;; no guarantees of any kind.
;;
;; The assembly version is hereby put under GNU General Public License by
;; Imre Leber.
;;
;;**************************************************************************
;; version: 15 juli 2000
;;

;; just to be on the save side.
%macro SaveRegs 0
push si
push di
push ds
%endmacro

%macro RestoreRegs 0
pop  ds
pop  di
pop  si
%endmacro

%macro AssignDS 0
mov  ax, XMS_DATA
mov  ds, ax
%endmacro

%define XMS_INT  002fh     ;; DOS Multiplex interrupt

segment XMS_DATA class=DATA
        
        XMSDriver DD   0
        initFlag  DB  -1

;;struct XMSRequestBlock bd.
        bd 
nbytes  dd 0    ;; Number of bytes to move
shandle dw 0    ;; Handle of source memory
soffset dd 0    ;; Offset of source in handle's memory area
dhandle dw 0    ;; Handle of destination memory
doffset dd 0    ;; Offset of destination in memory

UMBsize dw 0    ;; size of the last successfully allocated UMB.

segment XMS_TEXT class=CODE

;*********************** routines for the EMB's **************************

;==========================================================================
;===                        XMMinit (XMSinit)                           ===
;==========================================================================
;=== int  XMMinit(void); or int XMSInit(void);                          ===
;===                                                                    ===
;=== Verifies wether an eXtended Memory Manager is installed.           ===
;===                                                                    ===
;=== Returns 1 if manager found, 0 if not.                              ===
;===                                                                    ===
;=== NOTE:This function should be called before any other XMS function! ===
;==========================================================================

        global _XMSinit
        global _XMMinit
_XMSinit:
_XMMinit:
        push  bp

        SaveRegs
        AssignDS

        cmp  [initFlag], byte -1
        jne  .EndOfProc

        mov  [initFlag], byte 0

        mov  ax, 4300h                ;; Verify XMS manager present.
        int  XMS_INT
        cmp  al, 80h
        je   .next
        xor  ax, ax
        jmp  .EndOfProc

.next:
        mov  ax, 4310h                ;; Get XMS manager entry point.
        int  XMS_INT
        mov  [word XMSDriver], bx     ;; Save entry point.
        mov  [word XMSDriver+02h], es

        xor  ah, ah
        call far [dword XMSDriver]    ;; See if at least version 2.0
        cmp  ax, 0200h
        jb   .EndOfProc
        
        mov  [initFlag], byte 1
        
.EndOfProc:
   xor  ah, ah
   mov  al, [initFlag]

        RestoreRegs
        pop  bp
        retf

;=========================================================================
;===                           XMScoreleft                             ===
;=========================================================================
;=== unsigned long XMScoreleft(void);                                  ===
;===                                                                   ===
;=== Returns number of bytes available in largest free block.          ===
;=========================================================================

        global _XMScoreleft
_XMScoreleft:
        push bp
        SaveRegs
        AssignDS
        
        cmp  [initFlag], byte 0
        jne  .next

        xor  ax, ax
        xor  dx, dx
        jmp  .EndOfProc

.next:
        mov  ax, 0800h
        call far [dword XMSDriver]
        mov  bx, 1024
        mul  bx

.EndOfProc:
        RestoreRegs
        pop  bp
        retf

;==========================================================================
;===                            XMSalloc                                ===
;==========================================================================
;=== unsigned int XMSalloc(unsigned long size);                         ===
;===                                                                    ===
;=== Attempts to allocate size bytes of extended memory.                ===
;===                                                                    ===
;=== Returns handle if successful, 0 if not.                            ===
;===                                                                    ===
;=== NOTE: Actual size allocated will be the smallest multiple of 1024  ===
;===       that is larger than size.                                    ===
;==========================================================================

        global _XMSalloc
_XMSalloc:
        push bp
        mov  bp, sp

        SaveRegs
        AssignDS

        cmp  [initFlag], byte 0
        jne  .next

        xor  ax, ax
        jmp  .EndOfProc
.next:

      ;;Get the number of 1024 byte units required by size.
        mov  ax, [bp+06h]
        mov  dx, [bp+08h]

        mov  bx, 1024
        div  bx

        cmp  dx, 0
        je   .next1

      ;;Add a block for any excess.
        inc  ax

.next1:
        mov  dx, ax
        mov  ax, 0900h

        call far [dword XMSDriver]
        cmp  ax, 1
        je   .next2

        xor  ax, ax
        jmp  .EndOfProc

.next2:
        mov  ax, dx

.EndOfProc:
        RestoreRegs
        pop  bp
        retf

%if 0

;==========================================================================
;===                            XMSrealloc                              ===
;==========================================================================
;===  int XMSrealloc(unsigned int handle, unsigned long size);          ===
;===                                                                    ===
;===  Tries to expand or schrink a certain extended memory block.       ===
;===                                                                    ===
;===  Returns 1 if successful, 0 if not.                                ===
;==========================================================================

        global _XMSrealloc
_XMSrealloc:
        push bp
        mov  bp, sp
        
        SaveRegs
        AssignDS

        mov  ax, [bp+08h]
        mov  dx, [bp+0Ah]

        mov  bx, 1024
        div  bx

        cmp  dx, 0
        je   .next1

      ;;Add a block for any excess.
        inc  ax

.next1:
        mov  dx, [bp+06h]
        mov  bx, ax
        mov  ax, 0f00h
        call far [dword XMSDriver]
        
        RestoreRegs
        pop  bp
	retf

%endif

;===========================================================================
;===                            XMSfree                                  ===
;===========================================================================
;=== int  XMSfree(unsigned int handle);                                  ===
;===                                                                     ===
;=== Attempts to free extended memory referred to by handle. Returns 1   ===
;=== if successful, 0 if not.                                            ===
;===========================================================================

        global _XMSfree
_XMSfree:

        push bp
        mov  bp, sp

        SaveRegs
        AssignDS

        cmp  [byte initFlag], byte 0
        jne  .next

        xor  ax, ax
        jmp  .EndOfProc

.next:
        mov  dx, [bp+06h]
        mov  ax, 0a00h

        call far [dword XMSDriver]

.EndOfProc:
        RestoreRegs
        pop  bp
        retf

;------------------------------------------------------------------------
;---                               XMSmove                            ---
;------------------------------------------------------------------------
;--- private: XMSmove                                                 ---
;---                                                                  ---
;--- Copy memory to and from XMS.                                     ---
;---                                                                  ---
;--- in: ax: number of bytes to copy.                                 ---
;---                                                                  ---
;--- out: ax: number of bytes copied (0 if error).                    ---
;------------------------------------------------------------------------

XMSmove:
        push ax
        mov  [word nbytes],     ax
        mov  [word nbytes+02h], word 0

        mov  si, bd
        mov  ah, 0Bh
        call far [dword XMSDriver]
        pop  dx
        cmp  ax, 0
        je   .EndOfProc

        mov  ax, dx

.EndOfProc
        ret

;===========================================================================
;===                           DOStoXMSmove                              ===
;===========================================================================
;=== unsigned DOStoXMSmove(unsigned int desthandle,                      ===
;===                       unsigned long destoff, const char *src,       ===
;===                       unsigned n);                                  ===
;===                                                                     ===
;=== Attempts to copy n bytes from DOS src buffer to desthandle memory   ===
;=== area.                                                               ===
;=== Returns number of bytes copied, or 0 on error.                      ===
;===========================================================================

        global _DOStoXMSmove
_DOStoXMSmove:

        push bp
        mov  bp, sp

        SaveRegs
        AssignDS

        cmp  [initFlag], byte 0
        jne  .next

        xor  ax, ax
        jmp  .EndOfProc

.next:
        mov  [shandle], word 0

        mov  ax, [bp+06h]
        mov  [dhandle], ax

        mov  ax, [bp+08h]
        mov  [doffset], ax
        mov  ax, [bp+0Ah]
        mov  [doffset+02h], ax

        mov  ax, [bp+0Ch]
        mov  [word soffset],     ax
        mov  ax,[bp+0Eh]
        mov  [word soffset+02h], ax

        mov  ax, [bp+10h]
        call XMSmove

.EndOfProc:
        RestoreRegs
        pop  bp
        retf

;==========================================================================
;===                            XMStoDOSmove                            ===
;==========================================================================
;=== unsigned XMStoDOSmove(char *dest, unsigned int srchandle,          ===
;===                       unsigned long srcoff, unsigned n);           ===
;===                                                                    ===
;=== Attempts to copy n bytes to DOS dest buffer from srchandle memory  ===
;=== area.                                                              ===
;===                                                                    ===
;=== Returns number of bytes copied, or 0 on error.                     ===
;==========================================================================

        global _XMStoDOSmove
_XMStoDOSmove:

        push bp
        mov  bp, sp

        SaveRegs
        AssignDS
        
        cmp  [initFlag], byte 0
        jne  .next

        xor  ax, ax
        jmp  .EndOfProc

.next:
        mov  [dhandle], word 0

        mov  ax, [bp+06h]
        mov  [word doffset], ax
        mov  ax, [bp+08h]
        mov  [word doffset+02h], ax
      
        mov  ax, [bp+0Ah]
        mov  [word shandle], ax

        mov  ax, [bp+0Ch]
        mov  [word soffset], ax
        mov  ax, [bp+0Eh]
        mov  [word soffset+02h], ax

        mov  ax, [bp+10h]
        call XMSmove

.EndOfProc:
        RestoreRegs
        pop  bp
        retf

;*********************** routines for the HMA ****************************

%if 0

;==========================================================================
;===                           HMAalloc                                 ===
;==========================================================================
;=== int HMAalloc(void);                                                ===
;===                                                                    ===
;=== Allocates the HMA if it is available.                              ===
;===                                                                    ===
;=== Returns: 1 on success, 0 on failure.                               ===
;==========================================================================

        global _HMAalloc
_HMAalloc:
        SaveRegs
        AssignDS

        mov  ah, 01h
        mov  dx, 0FFFFh
        call far [dword XMSDriver]       ;; Allocate HMA.

        cmp  ax, 0
        je   .EndOfProc                  ;; exit on error.

        mov  ah, 03h
        call far [dword XMSDriver]       ;; Open gate A20.

        cmp  ax, 0                       ;; exit on success.
        jne  .EndOfProc

        mov  ah, 02h
        call far [dword XMSDriver]       ;; release the HMA on failure.
        xor  ax, ax

.EndOfProc
        RestoreRegs
        retf

;==========================================================================
;===                           HMAcoreleft                              ===
;==========================================================================
;=== unsigned HMAcoreleft(void);                                        ===
;===                                                                    ===
;=== Returns the size of the HMA in bytes.                              ===
;===                                                                    ===
;=== Remark: Only returns the right size after the HMA has been         ===
;===         succesfully allocated.                                     ===
;==========================================================================

        global _HMAcoreleft
_HMAcoreleft:

        mov  ax, 65520

        retf

;==========================================================================
;===                           HMAfree                                  ===
;==========================================================================
;=== int HMAfree(void);                                                 ===
;===                                                                    ===
;=== Deallocates the HMA.                                               ===
;===                                                                    ===
;=== Only call if the HMA has been successfully allocated.              ===
;==========================================================================

        global _HMAfree
_HMAfree:
        SaveRegs
        AssignDS
        
        mov  ah, 04h
        call far [dword XMSDriver]

        mov  ah, 02h
        call far [dword XMSDriver]

        RestoreRegs
        retf

;*********************** routines for the UMB's ****************************

;==========================================================================
;===                             UMBalloc                               ===
;==========================================================================
;===  unsigned int UMBalloc(void);                                      ===
;===                                                                    ===
;=== Allocates the largest UMB that can be allocated and returns        ===
;=== it's segment or 0 if error.                                        ===
;===                                                                    ===
;=== Remark: UMB's work with 16 byte blocks (doesn't work).             ===
;==========================================================================

        global _UMBalloc
_UMBalloc:
        SaveRegs
        AssignDS

        mov  ah, 10h
        mov  dx, 0FFFFh
        call far [dword XMSDriver]        ;; Get largest free UMB size.

        push dx
        mov  ah, 10h
        call far [dword XMSDriver]        ;; Allocate largest UMB.
        pop  bx
        
        cmp  ax, 1
        jne  .EndOfProc

        mov  ax, dx
        mov  [UMBsize], bx

.EndOfProc:
        RestoreRegs
        retf

;==========================================================================
;===                             GetUMBSize                             ===
;==========================================================================
;===  unsigned int GetUMBsize(void);                                    ===
;===                                                                    ===
;===  Returns the size of the most recent successfully allocated UMB.   ===
;==========================================================================

        global _GetUMBSize
_GetUMBSize:

        push ds
        AssignDS
        
        mov  ax, [UMBsize]

        pop  ds
        retf

;==========================================================================
;===                              UMBfree                               ===
;==========================================================================
;=== int UMBfree (unsigned int segment);                                ===
;===                                                                    ===
;=== Releases an UMB.                                                   ===
;==========================================================================

        global _UMBfree
_UMBfree:
        mov  bx, sp

        SaveRegs
        AssignDS
        
        mov  ah, 10h
        mov  dx, [ss:bx+04h]

        call far [dword XMSDriver]

        RestoreRegs
	retf

%endif