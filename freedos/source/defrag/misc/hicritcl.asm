;    
;   hicritcl.asm - the effective critical handler for this application.
;   Copyright (C) 2000 Imre Leber
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
;   If you have any questions, comments, suggestions, or fixes please
;   email me at:  imre.leber@worldonline.be
;

%assign FAIL 3              ;; fail on critical error.

extern _SetCriticalHandler

segment CRITICAL_DATA 

        CriterrOccured DB 0 ;; has there been a critical error. 

        status  DB 0        ;; status byte.                     
        cause   DB 0        ;; cause of error.                  

segment CRITICAL_TEXT 

;***********************************************************************
;***                     criticalhandler                             ***
;***********************************************************************
;*** Our critical error handler.                                     ***
;***                                                                 ***
;*** Remembers status and cause of critical error.                   ***
;***********************************************************************

criticalhandler:
        mov  bx, sp
        mov  ax, [ss:bx+04h]           ;; DS already set

        mov  [status], ah              ;; Save status byte.
        mov  [cause],  al              ;; Save cause of error.
        mov  [CriterrOccured], byte 1  ;; Remember that a critical 
                                       ;; error has occured.
        mov  ax, FAIL
        retf                            

;***********************************************************************
;***                     CriticalHandlerOn                           ***
;***********************************************************************
;*** void CriticalHandlerOn(void);                                   ***
;***                                                                 ***
;*** Installs our critical error handler.                            ***
;***********************************************************************

        global _CriticalHandlerOn
_CriticalHandlerOn:
        push ds
        
        push cs
        mov  ax, criticalhandler       ;; Point the real critical handler
        push ax                        ;; to our handler.
        call far _SetCriticalHandler
        pop  ax
        pop  ax
        
        mov  ax, CRITICAL_DATA
        mov  ds, ax
        mov  [CriterrOccured], byte 0  ;; Make sure that there is no pending
                                       ;; critical error.
        pop  ds
        retf

;***********************************************************************
;***                     CriticalErrorOccured                        ***
;***********************************************************************
;*** int CriticalErrorOccured(void);                                 ***
;***                                                                 ***
;*** Returns wether there has been a critical error.                 ***
;***                                                                 ***
;*** Remark: can only be called once.                                ***
;***********************************************************************
        
        global _CriticalErrorOccured
_CriticalErrorOccured:
        push ds

        mov  ax, CRITICAL_DATA
        mov  ds, ax 
        
        xor  ax, ax
        mov  al, [CriterrOccured]      ;; See if there hasn't been a critical
                                       ;; error.
        mov  [CriterrOccured], byte 0
        pop  ds
        retf

;***********************************************************************
;***                     GetCriticalCause                            ***
;***********************************************************************
;*** int GetCriticalCause(void);                                     ***
;***                                                                 ***
;*** Returns the cause of the critical error.                        ***
;***********************************************************************

        global _GetCriticalCause
_GetCriticalCause:
        push ds
        
        mov  ax, CRITICAL_DATA
        mov  ds, ax
        
        xor  ax, ax                    ;; Return critical error cause.
        mov  al, [cause]
        
        pop  ds
        retf

;***********************************************************************
;***                     GetCriticalStatus                           ***
;***********************************************************************
;*** int GetCriticalStatus(void);                                    ***
;***                                                                 ***
;*** Returns the status byte for the critical error.                 ***
;***********************************************************************

        global _GetCriticalStatus
_GetCriticalStatus:
        push ds
        
        mov  ax, CRITICAL_DATA
        mov  ds, ax
        
        xor  ax, ax
        mov  al, [status]              ;; Return status byte.
        
        pop  ds
        retf
