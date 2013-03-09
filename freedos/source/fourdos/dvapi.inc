
;  DESQview 2.26 API interfaces

;***************************************************************
;
;  Function numbers (AX values) for the @CALL interface
;
;***************************************************************

DVC_PAUSE       EQU     1000H
DVC_PRINTC      EQU     1003H
DVC_GETBIT      EQU     1013H
DVC_FREEBIT     EQU     1014H
DVC_SETBIT      EQU     1015H
DVC_ISOBJ       EQU     1016H
DVC_LOCATE      EQU     1018H
DVC_SOUND       EQU     1019H
DVC_OSTACK      EQU     101AH
DVC_BEGINC      EQU     101BH
DVC_ENDC        EQU     101CH
DVC_STOP        EQU     101DH
DVC_START       EQU     101EH
DVC_DISPEROR    EQU     101FH
DVC_PGMINT      EQU     1021H
DVC_POSWIN      EQU     1023H
DVC_GETBUF      EQU     1024H
DVC_USTACK      EQU     1025H
DVC_POSTTASK    EQU     102BH
DVC_NEWPROC     EQU     102CH
DVC_KMOUSE      EQU     102DH

DVC_COMMONMEM   EQU     1104H                   ; (2.0)
DVC_CONVENMEM   EQU     1105H                   ; (2.0)
DVC_EXPANDMEM   EQU     1106H                   ; (2.0)
DVC_APPNUM      EQU     1107H
DVC_DBGPOKE     EQU     110AH
DVC_APILEVEL    EQU     110BH
DVC_GETMEM      EQU     110CH
DVC_PUTMEM      EQU     110DH
DVC_FINDMAIL    EQU     110EH
DVC_PUSHKEY     EQU     1110H
DVC_JUSTIFY     EQU     1111H
DVC_CSTYLE      EQU     1112H

DVC_GETCRIT     EQU     1113H                   ; NEW (2.2)
DVC_OBJTYPE     EQU     1114H                   ; NEW (2.2)
DVC_SETERROR    EQU     1115H                   ; NEW (2.2)
DVC_GETERROR    EQU     1116H                   ; NEW (2.2)
DVC_ASSERTMAP   EQU     1117H                   ; NEW (2.2)
DVC_GETCOMMON   EQU     1119H                   ; NEW (2.23)
DVC_PUTCOMMON   EQU     111AH                   ; NEW (2.23)
DVC_ENTERC      EQU     111CH                   ; NEW (2.23)
DVC_PUTKEY      EQU     111DH                   ; NEW (2.23)
DVC_SCRNINFO    EQU     111EH                   ; NEW (2.23)
DVC_DOSUSER     EQU     111FH                   ; NEW (2.23)
DVC_DISPATCHINT EQU     1120H                   ; NEW (2.26)
DVC_ASSERTVIR   EQU     1121H                   ; NEW (2.26)
DVC_PROCESSMEM  EQU     1122H                   ; NEW (2.26)

DVC_DVPRESENT   EQU     0FFFFH
DVC_SHADOW      EQU     0FFFEH
DVC_UPDATE      EQU     0FFFDH

;***************************************************************
;
;  Message numbers (BH values) for the @SEND interface
;
;***************************************************************

DVM_HANDLE      EQU     00H
DVM_NEW         EQU     01H
DVM_FREE        EQU     02H
DVM_ADDR        EQU     03H
DVM_DIR         EQU     03H
DVM_CONNECT     EQU     03H                     ; NEW (2.26)
DVM_READ        EQU     04H
DVM_APPLY       EQU     04H
DVM_WRITE       EQU     05H

DVM_SETPRI      EQU     06H                     ; NEW (2.2)
DVM_GETPRI      EQU     07H                     ; NEW (2.2)

DVM_SIZEOF      EQU     08H
DVM_LEN         EQU     09H
DVM_ADDTO       EQU     0AH
DVM_SUBFROM     EQU     0BH
DVM_OPEN        EQU     0CH
DVM_CLOSE       EQU     0DH
DVM_ERASE       EQU     0EH
DVM_STATUS      EQU     0FH
DVM_EOF         EQU     10H
DVM_AT          EQU     11H
DVM_SETSCALE    EQU     11H
DVM_SETNAME     EQU     11H
DVM_READN       EQU     12H
DVM_GETSCALE    EQU     12H
DVM_REDRAW      EQU     13H
DVM_SETESC      EQU     14H
DVM_LOCK        EQU     14H

DVM_SETFLAGS    EQU     15H                     ; NEW (2.2)
DVM_GETFLAGS    EQU     16H                     ; NEW (2.2)

;***************************************************************
;
;  Alias numbers (BL values) for the @SEND interface
;
;***************************************************************

DVA_TOS         EQU     00H
DVA_ME          EQU     01H
DVA_MAILTOS     EQU     02H
DVA_MAILME      EQU     03H
DVA_KEYTOS      EQU     04H
DVA_KEYME       EQU     05H
DVA_OBJQTOS     EQU     06H
DVA_OBJQME      EQU     07H
DVA_WINDOW      EQU     08H
DVA_MAILBOX     EQU     09H
DVA_KEYBOARD    EQU     0AH
DVA_TIMER       EQU     0BH
DVA_OBJECTQ     EQU     0CH                     ; NEW (2.20)
DVA_OWNERTOS    EQU     0CH                     ; NEW (2.26)
DVA_OWNERME     EQU     0DH                     ; NEW (2.26)
DVA_POINTER     EQU     0FH
DVA_PANEL       EQU     10H


;***************************************************************
;
;  @SEND interface macro - bombs AH and BX
;
;***************************************************************

@SEND           macro   message,object
                ifdef DVA_&object
                  MOV   BX,DVM_&message*256+DVA_&object
                  MOV   AH,12H
                  INT   15H
                else
                  @PUSH &object
                  @SEND &message,TOS
                endif
                endm

;***************************************************************
;
;  @CALL interface macro - bombs AX
;
;***************************************************************

@CALL           macro   func
                local   L1
                ifndef DVC_&func
                  MOV   AX,&func
                  INT   15H
                else
                if (DVC_&func eq DVC_APILEVEL)
                  CMP   BX,200H         ; is 2.00 sufficient ?
                  JB    L1              ; jump if so
                  MOV   AX,DVC_APILEVEL ; issue the call
                  INT   15H
                  CMP   AX,2            ; early version 2.00 ?
                  JNE   L1              ; jump if not
                  XCHG  BH,BL           ; reverse bytes
                  MOV   AX,DVC_APILEVEL ; reissue call
                  INT   15H
                  XCHG  BH,BL           ; correct byte order
L1:
                else
                if (DVC_&func eq DVC_DVPRESENT)
                  PUSH  BX              ; save registers
                  PUSH  CX
                  PUSH  DX
                  MOV   AX,2B01H        ; DOS Set Date function
                  XOR   BX,BX           ; in case outside DESQview
                  MOV   CX,'DE'         ; invalid date value
                  MOV   DX,'SQ'
                  INT   21H
                  MOV   AX,BX           ; version # to AX
                  CMP   AX,2            ; early DV 2.00 ?
                  JNE   L1              ; jump if not
                  XCHG  AH,AL           ; swap bytes if so
L1:               POP   DX              ; restore registers
                  POP   CX
                  POP   BX
                else
                if (DVC_&func eq DVC_SHADOW)
                  MOV   AH,0FEH
                  INT   10H
                else
                if (DVC_&func eq DVC_UPDATE)
                  MOV   AH,0FFH
                  INT   10H
                else
                  MOV   AX,DVC_&func
                  INT   15H
                endif
                endif
                endif
                endif
                endif
                endm


;***************************************************************
;
;  @PUSH and supporting macros - pushes 32-bit values on the stack
;
;***************************************************************

@PUSH_ESDI      macro
                PUSH    ES
                PUSH    DI
                endm

@PUSH_DSSI      macro
                PUSH    DS
                PUSH    SI
                endm

@PUSH_BXAX      macro
                PUSH    BX
                PUSH    AX
                endm

@PUSH_DXCX      macro
                PUSH    DX
                PUSH    CX
                endm

@PUSH_ESSI      macro
                PUSH    ES
                PUSH    SI
                endm

@PUSH_DSDI      macro
                PUSH    DS
                PUSH    DI
                endm

@PUSH           macro   parm
                ifdef @PUSH_&parm
                  @PUSH_&parm
                else
                  PUSH  WORD PTR &parm+2
                  PUSH  WORD PTR &parm
                endif
                endm


;***************************************************************
;
;  @POP and supporting macros - pops 32-bit values from the stack
;
;***************************************************************

@POP_ESDI       macro
                POP     DI
                POP     ES
                endm

@POP_DSSI       macro
                POP     SI
                POP     DS
                endm

@POP_BXAX       macro
                POP     AX
                POP     BX
                endm

@POP_DXCX       macro
                POP     CX
                POP     DX
                endm

@POP_ESSI       macro
                POP     SI
                POP     ES
                endm

@POP_DSDI       macro
                POP     DI
                POP     DS
                endm

@POP            macro   parm
                ifdef @POP_&parm
                  @POP_&parm
                else
                  POP   WORD PTR &parm
                  POP   WORD PTR &parm+2
                endif
                endm


;***************************************************************
;
;  @MOV and supporting macros - moves 32-bit values to/from memory
;
;***************************************************************

@DV_LOAD        macro   seg,off,arg
                MOV     &seg,WORD PTR &arg+2
                MOV     &off,WORD PTR &arg
                endm

@DV_STORE       macro   seg,off,arg
                MOV     WORD PTR &arg+2,&seg
                MOV     WORD PTR &arg,&off
                endm

@MOV_ESDI       macro   mac,arg
                &mac    ES,DI,&arg
                endm

@MOV_DSSI       macro   mac,arg
                &mac    DS,SI,&arg
                endm

@MOV_BXAX       macro   mac,arg
                &mac    BX,AX,&arg
                endm

@MOV_DXCX       macro   mac,arg
                &mac    DX,CX,&arg
                endm

@MOV_ESSI       macro   mac,arg
                &mac    ES,SI,&arg
                endm

@MOV_DSDI       macro   mac,arg
                &mac    DS,DI,&arg
                endm

@MOV            macro   dest,src
                ifdef @MOV_&dest
                  @MOV_&dest    @DV_LOAD,&src
                else
                  @MOV_&src     @DV_STORE,&dest
                endif
                endm


;***************************************************************
;
;  @CMP macro - compares BX:AX to DWORD in memory
;
;***************************************************************

@CMP            macro   parm
                local   L1
                CMP     AX,WORD PTR &parm
                JNE     L1
                CMP     BX,WORD PTR &parm+2
L1:
                endm




