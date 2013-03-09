

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


          title     KEYDOS - Parse a keystroke name for 4DOS
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  4/17/91
                               revised as separate routine 8/29/91

          This routine is used throughout 4DOS to convert an ASCII
          keystroke name to a binary value.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg
          include   4dlparms.asm        ;4DOS parameters
          include   inistruc.asm        ;INI file data structures and macros
          include   keyvalue.asm        ;keystroke values
          include   model.inc           ;Spontaneous Assembly memory models
          include   util.inc            ;Spontaneous Assembly util macros
          ;
          ;
          ; Data segment
          ;
          assume    cs:nothing, ds:nothing, es:nothing, ss:nothing
          ;
          .dataseg
          ;
          ;
          ; Local data
          ;
KSName    db        11 dup (?)          ;keystroke name, copied from caller
          ;
          ;
KeyBrk    db        " -", TAB, 0, " -;", TAB, 0 ;keystroke token delimiters
          ;
          ;         Keystroke parsing tables
          ;
KPrefix   db        3, 0                ;keystroke prefixes, no skip
          DefStr    "Alt"
          DefStr    "Ctrl"
          DefStr    "Shift"
          ;
FuncAdd   label     byte                ;amount to add to function key value
                                        ;  for various key prefixes
          db        K_F1 and 0FFh       ;0 = no prefix, F1 scan code
          db        K_AltF1 and 0FFh    ;1 = Alt, Alt-F1 scan code
          db        K_CtlF1 and 0FFh    ;2 = Ctrl, Ctrl-F1 scan code
          db        K_ShfF1 and 0FFh    ;3 = Shift, Shift-F1 scan code
                                        ;Second part of table is for F11 -
                                        ;  F12; note 10 is subtracted to
                                        ;  compensate for increased base
                                        ;  value for these keys
          db        (K_F11 and 0FFh) - 10      ;0 = no prefix, F11 scan code
          db        (K_AltF11 and 0FFh) - 10   ;1 = Alt, Alt-F11 scan code
          db        (K_CtlF11 and 0FFh) - 10   ;2 = Ctrl, Ctrl-F11 scan code
          db        (K_ShfF11 and 0FFh) - 10   ;3 = Shift, Shift-F11 scan code
          ;
NPKeys    db        14                  ;non-printing keys (except F1-F10)
                                        ;  first value is normal keystroke,
                                        ;  second is Ctrl keystroke (except
                                        ;  Tab where second is Shift-Tab)
          db        size NPKVals        ;bytes to skip after each name
          NPK       "Esc", K_Esc, 0
          NPK       "Bksp", K_Bksp, K_CtlBS
          NPK       "Tab", K_Tab, K_ShfTab
          NPK       "Enter", K_Enter, K_CtlEnt
          NPK       "Up", K_Up, 0
          NPK       "Down", K_Down, 0
          NPK       "Left", K_Left, K_CtlLft
          NPK       "Right", K_Right, K_CtlRt
          NPK       "PgUp", K_PgUp, K_CtlPgU
          NPK       "PgDn", K_PgDn, K_CtlPgD
          NPK       "Home", K_Home, K_CtlHm
          NPK       "End", K_End, K_CtlEnd
          NPK       "Ins", K_Ins, K_CtlIns
          NPK       "Del", K_Del, K_CtlDel
          ;
AlphScan  label     byte                ;scan codes for alpha keys
          db        K_AltA and 0FFh
          db        K_AltB and 0FFh
          db        K_AltC and 0FFh
          db        K_AltD and 0FFh
          db        K_AltE and 0FFh
          db        K_AltF and 0FFh
          db        K_AltG and 0FFh
          db        K_AltH and 0FFh
          db        K_AltI and 0FFh
          db        K_AltJ and 0FFh
          db        K_AltK and 0FFh
          db        K_AltL and 0FFh
          db        K_AltM and 0FFh
          db        K_AltN and 0FFh
          db        K_AltO and 0FFh
          db        K_AltP and 0FFh
          db        K_AltQ and 0FFh
          db        K_AltR and 0FFh
          db        K_AltS and 0FFh
          db        K_AltT and 0FFh
          db        K_AltU and 0FFh
          db        K_AltV and 0FFh
          db        K_AltW and 0FFh
          db        K_AltX and 0FFh
          db        K_AltY and 0FFh
          db        K_AltZ and 0FFh
          ;
DigScan   label     byte                ;scan codes for digit keys
          db        K_Alt0 and 0FFh
          db        K_Alt1 and 0FFh
          db        K_Alt2 and 0FFh
          db        K_Alt3 and 0FFh
          db        K_Alt4 and 0FFh
          db        K_Alt5 and 0FFh
          db        K_Alt6 and 0FFh
          db        K_Alt7 and 0FFh
          db        K_Alt8 and 0FFh
          db        K_Alt9 and 0FFh
          ;
          ;
          ; Start code segment
          ;
@curseg   ends                          ;end data segment
          .defcode
          ;
          assume    cs:@curseg,ds:@DATASEG  ;set up CS assume
          ;
          ;
          ; Externals
          ;
          .extrn    NextTok:far
          .extrn    TokList:far
          ;
          .extrn    DEC_TO_BYTE:auto
          .extrn    IS_ALPHA:auto
          .extrn    IS_DIGIT:auto
          .extrn    MEM_MOV:auto
          .extrn    TO_UPPER:auto
          ;
          ;
          ; __KEYPARSE - Parse a keystroke name
          ;
          ; On entry:
          ;    CX = length of the keystroke name string; only the first 10
          ;           characters will be used
          ;    DS:SI = address of an ASCIIZ string containing a keystroke
          ;            name
          ;         
          ; On exit:
          ;           JA      if valid keystroke found
          ;           JE      if spec was empty
          ;           JB      if name was present but invalid
          ;         AX = keystroke binary value if JA
          ;         BX, CX, DX, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     __KeyParse,noframe,far  ;define entry point
          ;
          pushm     ds,si,di            ;save registers
          ;
          ; Copy the name to local storage
          ;
          cmp       cx,10               ;name too long?
           jbe      DoCopy              ;if not go on
          mov       cx,10               ;only copy the first 10 bytes
          ;
DoCopy:   loadseg   es,DGROUP,ax        ;set DGROUP segment
          mov       di,offset @DATASEG:KSName  ;get offset
          cld                           ;go forward
          rep       movsb               ;copy the name string
          stocb     0                   ;be sure it's null-terminated
          loadseg   ds,es               ;DGROUP to DS
          mov       si,offset @DATASEG:KSName  ;get offset of name
          ;
          ; Parse the keystroke name (eg F3 or AltF1)
          ;
          mov       di,offset @DATASEG:KeyBrk    ;get keystroke break table
          call      NextTok             ;break out token w/ keystroke delims
           ja       GotKey              ;if something there, parse it
          xor       ax,ax               ;empty or comment -- set up JE exit
          jmp       PKExit              ;and exit
          ;
GotKey:   xor       dx,dx               ;start with zero for the key value
          mov       ax,[si]             ;get first two characters
          cmp       al,'@'              ;at sign for numeric non-print code?
           jne      CkNumDig            ;if not go look for digit
          jmp       KeyNumNP            ;if so go handle non-print numeric
          ;
CkNumDig: call      IS_DIGIT            ;digit for numeric ASCII?
           jne      CkPrefix            ;if not check prefix
          jmp       KeyNum              ;if so go convert
          ;
          ; Look for Alt / Ctrl / Shift
          ;
CkPrefix: mov       di,offset @DATASEG:KPrefix   ;point to possible prefixes
          cmp       cl,1                ;check token length
           ja       TestPref            ;if > 1 go test prefix
          cmp       ah,'-'              ;dash separator?
           je       ScanPref            ;if so must be a prefix
          jmp       KeyAsc              ;if not must be a straight ASCII key
          ;
ScanPref: call      TokList             ;OK, better be a prefix then
           je       GotPref             ;if we got a prefix go handle it
          jmp       short KErrJump      ;if not there's an error
          ;
TestPref: call      TokList             ;test for a prefix
           jne      KeyNP               ;if not, must be non-printing char
          ;
          ; We have Alt / Ctrl / Shift, see what's after it
          ;
GotPref:  mov       dh,al               ;copy prefix value
          inc       dh                  ;bump to show prefix set
          mov       al,ah               ;copy number of characters matched
          xor       ah,ah               ;clear high byte
          add       si,ax               ;skip those characters in input
          mov       di,offset @DATASEG:KeyBrk    ;get keystroke break table
          call      NextTok             ;break out token w/ keystroke delims
           jbe      KErrJump            ;if empty or all comment, holler
          cmp       cl,1                ;check length
           je       KeyAsc              ;if = 1 must be ASCII
          ;
          ; Not standard ASCII, and either no prefix or prefix processed --
          ; must be a non-printing key.  Check for F1 - F12 first.
          ;
KeyNP:    lodsb                         ;get next character
          call      TO_UPPER            ;make character upper case
          cmp       al,'F'              ;does it look like a function key?
           jne      KLookNP             ;if not go scan table
          call      DEC_TO_BYTE         ;convert numeric
           jne      KErrJump            ;if invalid complain
          dec       al                  ;make it 0-based
          mov       bx,offset @DATASEG:FuncAdd   ;point to F key offsets
          cmp       al,11               ;check against max
           ja       KErrJump            ;if invalid complain
          cmp       al,9                ;check against max
           jbe      KFLow               ;if F1 - F10 go on
          add       bx,4                ;skip F1 - F10 part of table
          ;
KFLow:    xchg      al,dh               ;switch with prefix
          xlatb                         ;get amount to add for this prefix
          add       al,dh               ;add in key number (0 = F1 etc.)
          mov       dl,al               ;save result
          jmp       SetNPKey            ;go save non-printing key value
          ;
          ;
          ; Not F1 - F12, look at the non-printing key list
          ;
KLookNP:  dec       si                  ;back up since it wasn't 'F'
          mov       di,offset @DATASEG:NPKeys    ;point to non-print key list
          call      TokList             ;see if key is in the list
           jc       KErrJump            ;if not something's wrong
          mov       ch,dh               ;copy prefix
          mov       dx,[di].NPStd       ;get standard keystroke value
          or        ch,ch               ;check prefix
          ;
          if        _OS2
          jne       ispfx
          jmp       KeyOK
ispfx:
          else
           je       KeyOK               ;if none go store character
          endif
          ;
          cmp       dx,K_Tab            ;is it a tab?
           je        ChkTab             ;if so allow shift, not Ctrl
          cmp       dx,K_Bksp           ;is it a backspace?
           je        ChkBksp            ;if so allow Alt or Ctrl
          cmp       ch,2                ;Ctrl prefix?
           jne       KErrJump            ;if not holler
          mov       dx,[di].NPSecond    ;get second value
          or        dx,dx               ;check for Ctrl illegal
           jnz      KeyOK               ;if value is there go store it
          ;
KerrJump: jmp       short KeyError      ;otherwise illegal Ctrl value, holler
                                        ;  also used as intermediate jump
          ;
ChkTab:   cmp       ch,3                ;Shift prefix?
           je       GetSecnd            ;if so go process it
          cmp       ch,2                ;Ctrl prefix?
           jne      KErrJump            ;if not holler
          mov       dx,K_CtlTab         ;get Ctrl-Tab value
          jmp       short KeyOK         ;go store it
          ;
GetSecnd: mov       dx,[di].NPSecond    ;get new value
          jmp       short KeyOK         ;go store it
          ;
ChkBksp:  cmp       ch,3                ;is it Shift-Bksp?
           je       KErrJump            ;if so holler
          cmp       ch,2                ;Ctrl prefix?
           je       GetSecnd            ;if so we're all set
          mov       dx,K_AltBS          ;get Alt-Bksp value
          jmp       short KeyOK         ;go store it
          ;
          ;
          ; Standard ASCII key, with or without prefix
          ;
KeyAsc:   lodsb                         ;get the key value
          call      TO_UPPER            ;shift to upper case
          mov       dl,al               ;copy to storage register
          xor       ah,ah               ;clear high byte
          mov       di,ax               ;copy for possible use as table index
          or        dh,dh               ;any prefix?
           je       KeyOK               ;no, go ahead and store it
          call      IS_ALPHA            ;is it alpha?
           jne      CkAscDig            ;no, go look for digit
          ;
          ; Alphabetic key with prefix, calculate Ctrl value or look up
          ; Alt value (error if Shift)
          ;
          dec       dh                  ;check for Alt = 1
           je       AlphAlt             ;if Alt, go do lookup
          dec       dh                  ;check for Ctrl = 2
           jne      KErrJump            ;if not, holler (Shift not allowed)
          xor       dh,dh               ;clear high byte
          sub       dl,40h              ;adjust for ctrl character
          jmp       short KeyOK         ;and go store it
          ;
AlphAlt:  sub       di,'A'              ;adjust to 0-based
          mov       dl,AlphScan[di]     ;get alpha scan code
          jmp       short SetNPKey      ;go set up as non-printing key
          ;
          ; Digit key with prefix, look up Alt value, any other prefix
          ; is an error
          ;
CkAscDig: call      IS_DIGIT            ;not alpha, is it a digit?
           jne      KeyError            ;no, it's non-alphanumeric and there
                                        ;  is a prefix ==> error
          dec       dh                  ;check for Alt = 1
           jne      KeyError            ;Ctrl and Shift are illegal, holler
          sub       di,'0'              ;adjust to 0-based
          mov       dl,DigScan[di]      ;get scan code
          ;
SetNPKey: mov       dh,1                ;set 100h for non-printing key
          jmp       short KeyOK         ;and go store it
          ;
          ; Key specified numerically, just convert the number
          ;
KeyNumNP: inc       si                  ;bump SI to skip '@'
          mov       dh,1                ;set high byte for non-printing
          ;
KeyNum:   call      DEC_TO_BYTE         ;get byte value
           jne      KeyError            ;if error complain
          or        al,al               ;check value
          je        KeyError            ;if zero complain
          mov       dl,al               ;copy to DL
          ;
          ;
KeyOK:    mov       ax,dx               ;copy keystroke to AX
          or        ax,ax               ;set up JA exit (valid key will
                                        ;  always be > 0)
          jmp       short PKExit        ;and get out
          ;
          ; Keystroke spec was no good
          ;
KeyError: stc                           ;set JB exit
          ;
PKExit:   popm      di,si,ds            ;restore registers
          exit                          ;all done
          ;
          ;
@curseg   ends                          ;close segment
          ;
          end


