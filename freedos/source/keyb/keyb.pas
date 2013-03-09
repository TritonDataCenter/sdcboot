{===========================================================================
||  FreeDOS Keyb  2.0                                                     ||
||  (FINAL - 27th AUGUST 2006)                                            ||
|| ---------------------------------------------------------------------- ||
||  Open Source keyboard driver for DOS                                   ||
||  License:       GNU-GPL 2.0  (see copying.txt)                         ||
||  Platform:      PC XT/AT with keyboard interface at int9h or int15h    ||
||  Compiler:      Borland/Turbo Pascal 7.0                               ||
|| ---------------------------------------------------------------------- ||
|| (C) Aitor    SANTAMARIA MERINO                                         ||
|| (aitor.sm@gmail.com)                                                   ||
|| ---------------------------------------------------------------------- ||
|| Acknowledgements: many thanks to:                                      ||
||   - Matthias PAUL       (also contributed cache flush code)            ||
||   - Eric     AUER       (also contributed beep code)                   ||
||   - Axel     FRINKE                                                    ||
||   - Dietmar  HOEHMANN                                                  ||
||   - Henrique PERON                                                     ||
||   - T.       KABE                                                      ||
===========================================================================}


{$A-,B-,D-,E-,F-,G-,I-,L-,N-,O-,R-,S-,V-,Y-}
{$M $2048,0,0}

PROGRAM Keyb;

USES DOS;


CONST
        ConstDriverNameZ = 'KEYBDATA';            {maximum: 8 chars}
                                           { if <8, then ends in #0}
        { Version constants }
        VerS          = '2.01 ';
        Version       = $0201 ;

        { Flags in BIOS variables }
        E1Prefixed    = 1;   {flags in KStatusByte1}
        E0Prefixed    = 2;

        { Flags in KEYB }
        F_Safemode    = $01;
        F_BeepDisabled= $02;
        F_XTMachine   = $04;
        F_Japanese    = $08;
        F_UseScroll   = $10;
        F_UseNum      = $20;
        F_UseCaps     = $40;
        F_PauseLoop   = $80;

        { APMProc function }
        APM_FlushCache = 0;
        APM_WarmReboot = 1;
        APM_ColdReboot = 2;
        APM_PowerOff   = 3;
        APM_Suspend    = 4;

        { Offsets in BIOS segment 0h: }
        KStatusFlags1  = $417;
        KStatusFlags2  = $418;
        AltInput       = $419;
        BufferTail     = $41A;
        BufferHead     = $41C;
        BufferStart    = $480;
        BufferEnd      = $482;
        KStatusByte1   = $496;                    { PC/AT BIOS only }
        KStatusByte2   = $497;                    { PC/AT BIOS only }
        

{$I KEYBMSG.NLS}   { message string constants }


TYPE

        pbyte          = ^byte;
        pword          = ^word;
        SimpleProc     = procedure;     { parameter-less callable function }
        PtrRec         = record         { pointer record }
                            Ofs,Seg : Word;
                         End;

        WArray  = array[0..32766] of word;
        BArray  = array[0..65534] of byte;
   
        pWArray = ^WArray;
        pBArray = ^BArray;



CONST
        {======= KEYB GLOBAL DATA SEGMENT (later accessed through CS) ====}

       { Old interrupt vectors }
       OldInt9h       : Pointer = NIL;         { Chain for int9h. }
       OldInt2Fh      : Pointer = NIL;         { Chain for multiplexer interrupt. }
       OldInt16h      : Pointer = NIL;         { Chain for Int 16h. }
       OldInt15h      : Pointer = NIL;         { Chain for Int 15h. }

       {Layout pointers}
       CurrentLayout  : PBArray = NIL;         { Pointer to the KeybCB }

       {Other global variables}
       PIsActive      : Pointer = NIL; { Pointer to a byte, 0:inactive, <>0: active }
       Flags          : Byte = 0;      { FLAGS of Keyb (see above) }
       UserKeys       : Word = 0;      { User Shifts (hi) and user locks (lo) }
       CombStat       : Word = 0;      { Status of char combination. 0=No combination signs in work. }
                                       { Else pointer to combination table. }
                                       { will probably be integrated into COMBI procedure }
       DecimalChar    : char = #0;     { DecChar given by parameter }
       VIsActive      : byte = $FF;    { Active if int9h is not hooked }

       { Installable functions vectors }
       BIOSStoreKey: SimpleProc   = NIL;    { procedure EXCLUSIVE to StoreKey }
       Beep        : SimpleProc   = NIL;
       APMProcedure: SimpleProc   = NIL;    { APM (power) procedure       }
       DoCommands  : SimpleProc   = NIL;    { perform commands 100..199   }
       ExtCommands : SimpleProc   = NIL;    { extended commands }
       CombineProc : SimpleProc   = NIL;    { combi pending, Combine }
       ExecCombi   : SimpleProc   = NIL;    { execute a COMBI sequence }
       FlushBuffer : SimpleProc   = NIL;    { flushes secondary buffer }
       EmptyBuffer : SimpleProc   = NIL;    { clears the secondary buffer }

       { End of the KEYB Global data segment }
       EoDS        : Byte = 0;


{------------------------------------------------------------------------
---- KEYB 2 CORE ROUTINES (non-discardable)                         -----
------------------------------------------------------------------------}


{*** SPACE for the KEYB Global Data Segment, accessible through CS ***}
Procedure GlobalDS;
Begin
   ASM
   DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
   End;
End;


{************************************************************************
** FailCall:   fails a call to a KEYB function                         **
** ------------------------------------------------------------------- **
** IN:   -                                                             **
** OUT:  CF set (meaning error)                                        **
** Note: This is to be used with non-installed functions               **
*************************************************************************}
procedure FailCall; far; assembler;
asm
      STC
      RET
end;


{************************************************************************
** StoreKey: the unified common entry point to store a key in buffer   **
** ------------------------------------------------------------------- **
** IN:   AH: scancode                                                  **
**       AL: character code                                            **
*************************************************************************}
Procedure StoreKey; assembler;
label  Combi, Store, Ende;
asm

      MOV       DL, [ES:KStatusFlags2]                     { pause? }
      TEST      DL, 8
      JZ        Combi
      
      AND       DL, $F7
      MOV       [ES:KStatusFlags2], DL
      JMP       Ende

Combi:
      MOV       DL, [CS:offset Combstat]                  { combi pending? }
      OR        DL,DL
      JZ        Store

      CALL      DWORD PTR [CS: Offset CombineProc]
      JMP       Ende

Store:
      CALL      DWORD PTR [CS:offset BIOSStoreKey]

ende:

End;


{************************************************************************
** KEYBexecute: executes a KEYB command (except for 0 and 160)         **
** ------------------------------------------------------------------- **
** IN:   AL: command code (excluded 0,160)                             **
**       AH: scancode (when applies)                                   **
** OUT:  registers possibly destroyed                                  **
*************************************************************************}
procedure KEYBExecute; assembler;
label other, ende, command;
asm
      CMP       AL,80                   { 1..79:       XString }
      JAE       Other

      MOV       BH,[CS:offset Flags]    { not allowed in safe mode }
      TEST      BH,F_SafeMode
      JNZ       Ende

      MOV       AH,$FF
      CALL      StoreKey
      JMP       Ende

other:
      CMP       AL,199                  { 200..255:     COMBIs }
      JNA       Command
      CALL      DWORD PTR [CS: Offset ExecCombi]
      JMP       ende

Command:                                {  80..199:     Other commands }
      CALL      DWORD PTR [CS: Offset DoCommands]

ende:
end;


{************************************************************************
** Int2Fh: handler for the DOS-MuX Keyb services                       **
** ------------------------------------------------------------------- **
** INT:  services interrupt 2Fh (AX=AD80h)                             **
*************************************************************************}
procedure Int2Fh; assembler;
label
      StartCode, ComplexCode, ComplexEnd, ChainOld, ExitError, ExitOk,
      No80, No82, No83, No84, No85, No8A, No8B, No8C,
      Skp1,Skp2, kmux81, changecps, changecpexit, loop1;
asm
      {=== Entry point: discard limits ===}

      CMP       AH, $AD           { MUX code=ADh }
      JNE       ChainOld

      CMP       AL, $80           { commands 80h..85h, 8Ah..8Dh }
      JB        ChainOld

      CMP       AL, $8D
      JNA       StartCode

ChainOld:
      JMP       CS:[OldInt2Fh]
      
      {=== Trivial commands (all except 81h, 8Dh) ===}

StartCode:
      {--- 80h: Installation check ---}

      CMP       AL, $80
      JNE       No80

      MOV       AX, $FFFF
      MOV       DX, Version
      LES       DI, CS:[CurrentLayout]
      IRET
      
No80:

      {--- 82h: Set enable/disable ---}

      CMP       AL, $82
      JNE       No82

      OR        BL,BL
      JZ        Skp1
      CMP       BL,$FF
      JNE       ExitError
      
Skp1:
      PUSH      BX
      PUSH      DI
      
      MOV       DI, CS:[Offset PIsActive]
      MOV       [CS:DI], BL

      POP       DI
      POP       BX

      JMP       ExitOk

No82:

      {--- 83h: Get enable/disable ---}

      CMP       AL, $83
      JNE       No83

      PUSH      DI
      
      MOV       DI, CS:[Offset PIsActive]
      MOV       BL, [CS:DI]

      POP       DI
      IRET

No83:

      {--- 84h: Set current submapping ---}

      CMP       AL, $84
      JNE       No84

      PUSH      DS
      PUSH      SI

      LDS       SI, [CS:Offset CurrentLayout]
      CMP       BL, [SI]
      JA        Skp2

      MOV       [SI+3],BL

Skp2:
      POP       SI
      POP       DS

      IRET


No84:
      {--- 85h: Get current submapping ---}

      CMP       AL, $85
      JNE       No85

      PUSH      DS
      PUSH      SI

      LDS       SI, [CS:Offset CurrentLayout]
      MOV       AL, [SI+3]

      POP       SI
      POP       DS

      IRET

No85:

      {--- 8Ah: Load/Unload KeybCB ---}

      CMP       AL, $8A
      JNE       No8A

      {*** NOT IMPLEMENTED ***}
      IRET

No8A:

      {--- 8Bh: Set User Keys ---}

      CMP       AL, $8B
      JNE       No8B

      MOV       BX,[CS:Offset UserKeys]
      IRET

No8B:

      {--- 8Ch: Get User Keys ---}

      CMP       AL, $8C
      JNE       No8C

      MOV       [CS:Offset UserKeys], BX
      PUSH      CS                              { Backdoor: obtain CS }
      POP       ES
      IRET

No8C:

      {--- Last chance for ComplexCode (81,8D) ---}

      CMP       AL, $81
      JE        ComplexCode

      CMP       AL, $8D
      JNE       ChainOld

ComplexCode:

      PUSH      AX                            { save some registers }
      PUSH      CX
      PUSH      BX
      PUSH      DX

      PUSH      DS
      PUSH      ES
      
      PUSH      SI
      PUSH      DI
                                    { set our segments:             }
                                    {   CS= Keyb and global data    }
      XOR       CX,CX               {   ES= 0 (BIOS data)           }
      MOV       ES,CX               {   DS:SI=> currentlayout       }
      LDS       SI,[CS:Offset CurrentLayout]

      CMP       AL, $8D
      JNE       KMux81

      XOR       AH, AH
      MOV       AL, DL
      CALL      KEYBexecute         { we are safe in here }

      CLC
      JMP       ComplexEnd
      
KMux81:
      MOV       CL, [SI]
      DEC       CL               { CL = number of PARTICULAR submappings }

      PUSH      SI
      ADD       SI, 28           { DS:SI-> first particular }
      
loop1:
      MOV       DX, [SI]
      CMP       DX, BX           { if (cp==0) or (cp=bx) break }
      JE        changecps
      OR        DX,DX
      JZ        changecps
      ADD       SI,8
      LOOP      loop1

      POP       SI
      STC
      JMP       ComplexEnd

changecps:
      POP       SI                         { new CP = numCPs-cl }
      NEG       CL
      ADD       CL, [SI]
      MOV       [SI+3], CL
      
      LDS       SI, [CS:Offset PIsActive]       { enable driver }
      MOV       CL, $FF
      MOV       [SI], CL

      CLC

ComplexEnd:

      POP       DI
      POP       SI

      POP       ES
      POP       DS

      POP       DX
      POP       BX
      POP       CX
      POP       AX

      JC        ExitError

ExitOk:					{ exit preserving flags and clear carry }
      PUSH      BP
      MOV       BP, SP
	  AND BYTE  [BP+6],$FE
      POP       BP
      IRET

ExitError:				  { exit preserving flags and set carry }
      PUSH      BP
      MOV       BP, SP
      OR BYTE   [BP+6],1
      POP       BP
      IRET
end;



procedure DefaultTable; assembler;
asm
    DB    59             {F1:   59   !100}
    DB    0
    DB    1
    DB    100

    DB    60             {F2:   60   !101}
    DB    0
    DB    1
    DB    101

    DB    83             {DEL:  83  !0   !161}
    DB    1              {NumLock is preprocessed with the columns}
    DB    3
    DB    0
    DB    161

    DB    0              {STOP}
end;


procedure TableLookUp; assembler;
{   IN:  AL: scancode
         CL: plane (0..7)
         DS:DI-> NewTable                                       }
{   OUT: CF=0 if found
             AX: scancode/ascii pair
             BL: 0 if char, <>0 if command
         CF=1 if not found
             AL: scancode
             CL: plane
         DI trashed                                             }
label

      CompareSC, ScancodeFound, NotFound, Ende, ContFound, FoundEnd,
      NotReplace, XBit, jmp1, jmp2, jmp3;
asm

        { loop to find it }
CompareSC:
      XOR       BX, BX                  { BL <- column number }
      MOV       BL, [DS:DI+1]
      and       BL, 7
      INC       BL
        
      CMP       AL, [DS:DI]
      JE        ScancodeFound
      JB        NotFound

      TEST      BYTE [DS:DI+1], $80     { if 'S', columnNumber x2 }
      JZ        jmp1
      SHL       BL,1
jmp1:

      ADD       BL, 3                   { table overhead=3 }
      ADD       DI, BX
      MOV       BL, [DS:DI]
      OR        BL,BL
      JZ        NotFound
      JMP       CompareSC               { next }


ScancodeFound:

      MOV       BH, [DS:DI+1]          { attributes }

      { step 1: ignore scancode? }
      TEST      BH, 16                  { ignore? }
      JZ        ContFound

      MOV       SI,[CS:Offset pIsActive]{ inactive? in that case }
      TEST      BYTE [CS:SI], $FF       { simply NOT FOUND }
      JZ        NotFound

      MOV       AL,160                  { set command !160: NOP }
      MOV       BL,1
      JMP       FoundEnd                { we are done! }

      { step 2: check length }
ContFound:
      PUSH      BX
      MOV       BL,BH
      AND       BL, 7
      CMP       BL, CL
      POP       BX
      JB        NotFound
      PUSH      CX                      { SAVE the column, in case not found }
      
      { step 3: check NumLock, CapsLock }
      CMP       CL, 1                   { it must be columns 0 or 1 }
      JA        XBit

      MOV       BL, BH                  { flags AND lock keys }
      AND       BL, $60
      AND       BL,[ES:KStatusFlags1]
      JZ        XBit

      XOR       CL, 1
      
      { step 4: Xbit }
XBit:
      MOV       BL, [DS:DI+2]
      ROR       BL, CL
      AND       BL, 1

      { step 5: column data }
      ADD       DI,3
      TEST      BYTE [DS:DI-2],$80
      JNZ       jmp2

      ADD       DI, CX                  { byte granularity: set AL }
      MOV       AH, AL
      MOV       AL, [DS:DI]
      JMP       jmp3

jmp2:
      SHL       CX, 1                   { word granularity: set AX }
      ADD       DI, CX
      MOV       AX, [DS:DI]

jmp3:
      POP       CX

      { step 7: if !0 => not found }
      OR        AL,AL
      JNZ       FoundEnd

      OR        BL,BL
      JZ        FoundEnd

      MOV       AL,AH

      { final: return }
NotFound:
      STC
      JMP       ende

FoundEnd:
      CLC

Ende:
end;



{************************************************************************
** Int15h: handler for int15h                                          **
** ------------------------------------------------------------------- **
** INT:  services interrupt 15h (AH=4Fh)                               **
*************************************************************************}
procedure Int15h; assembler;
label
  { entry and exit points }
  IsFunc4F, KeyboardHandler, ende,
  { plane computation block }
  skp0_1, PreTableA, PreTableB, PreTableC, Loop1, PreTableD, PreTableE,
  { table lookup block (and not found) }
  TableBlock, SecondTable, ThirdTable, skp1_0, skp1_1, skp1_2a, skp1_2b, NotFound,
  ChainScanCode,ChainScancodeEnd,
  { scancode found block }
  Found, X, KeybIsEnabled, FinishScancode;

asm

{================= INITIAL CONTROL BLOCK ================================}

      PUSHF                 { preserve flags! }

      CMP       AH, $4F     { is it function 4Fh? }
      JE        IsFunc4F    { check functions }

      POPF
      JMP       DWORD PTR CS:[OldInt15h]

IsFunc4F:
      STC
      CALL      DWORD PTR CS:[OldInt15h] { the old one first! }
      JC        KeyboardHandler          { if Carry clear, }

      PUSH      BP		{ return, preserve flags, clear carry }
      MOV       BP, SP
      AND BYTE   [BP+6],$FE
      POP       BP
      IRET

{================= KEYB's KEYBOARD HANDLER ==============================}

KeyboardHandler:

      PUSH      AX                            { save some registers }
      PUSH      CX
      PUSH      BX
      PUSH      DX

      PUSH      DS
      PUSH      ES
      
      PUSH      SI
      PUSH      DI
                                    { set our segments:             }
                                    {   CS= Keyb and global data    }
      XOR       CX,CX               {   ES= 0 (BIOS data)           }
      MOV       ES,CX               {   DS:SI=> currentlayout       }
      LDS       SI,[CS:Offset CurrentLayout]

{====================== PLANE COMPUTE BLOCK ========================}
{  IN:      AL: scancode                                            }

      {***** Compute PLANE: first attempt: normal or shift  *****}
      { so far we only need BL and DX }

      {--- Making of BL ---}

      MOV       BL, [CS:Offset Flags]    { Scroll/Num/Caps compute planes? }
      AND       BL, (F_UseScroll+F_UseNum+F_UseCaps)
      OR        BL, $0F                      { we get BL= selectable flags }
      AND       BL, ES:[KStatusFlags1]

      {--- Make DX ---}
      MOV       DX, [CS:Offset UserKeys]

      { BX: standard modifier keys }
      { DX: user modifier keys }

      {--- First discard if UserKeys or E0-prefixed }

      OR        DX,DX
      JNZ       PreTableA

      TEST      BYTE [CS: offset Flags], F_XTMachine
	  JNZ		skp0_1

      TEST      BYTE [ES:KStatusByte1], E0Prefixed
      JNZ       PreTableA

      {--- Second: planes 0 or 1? (implicit ones) }
      
skp0_1:
      TEST      BL,$7F          { either shift, ctrl, alt, scroll? }
      JZ        TableBlock      { NO -> finished, NoKey }

      INC       CL              { plane number = 1 }

      TEST      BL,$7C          { ctrl, alt, scroll? }
      JZ        TableBlock      { NO -> finished, SHIFT }
      
      {***** Compute PLANE: second attempt: loop on planes  *****}

PreTableA:

      MOV       CL, 2

      MOV       BL, [SI+1]      { more planes? }
      OR        BL,BL
      JZ        TableBlock
                                { otherwise, CL is for free use }

      {--- Making of BL ---}
      MOV       BL, [ES:KStatusFlags1]
      AND       BL, $7F        { turn INS off }

      {--- Making of BH ---}

      {--- Flags 0000 1100 ---}
      MOV       CL, [ES:KStatusFlags2]
      AND       CL, $03
      XOR       CL, $03
      SHL		CL, 1
      SHL		CL, 1
      
      MOV       BH, [ES:KStatusFlags1]
      AND       BH, $0C
      AND       BH, CL

      {--- Flags 0000 0011 ---}
      MOV       CL, [ES:KStatusFlags2]
      AND		CL, $03
      OR        BH, CL

      {--- Flag  0001 0000 (E0prefixed)   ---}
      TEST      BYTE [CS: offset Flags], F_XTMachine
	  JNZ		PreTableB

      TEST      BYTE [ES:KStatusByte1], E0Prefixed
      JZ        PreTableB
      OR        BH, $10

PreTableB:
      {--- Flag  0100 0000 (EitherShift)  ---}
      TEST      BL, $03
      JZ        PreTableC
      OR        BH, $40

      {--- Loop on planes ---}

PreTableC:
      PUSH      SI              { must exit through PreTableCEnd }
      PUSH      AX

      MOV       CL, [SI+1]      { CL=NumPlanes }

      XOR       AH,AH           { skip submappings }
      MOV       AL,[SI]
      SHL       AL,1            { each submapping is 1 qword }
      SHL       AL,1
      SHL       AL,1
      ADD       SI,20           {magic number of KL offset}
      ADD       SI,AX

Loop1:
      MOV       AX,[SI]           { first word: mandatory standard flags }
      AND       AX,BX
      CMP       AX,[SI]
      JNE       PreTableD

      MOV       AX,[SI+2]        { second word: forbidden standard flags }
      AND       AX,BX
      JNZ       PreTableD

      MOV       AX,[SI+4]             { third word: mandatory user flags }
      AND       AX,DX
      CMP       AX,[SI+4]
      JNE       PreTableD

      MOV       AX,[SI+6]            { fourth word: forbidden user flags }
      AND       AX,DX
      JZ        PreTableE

PreTableD:
      ADD       SI,8
      LOOP      Loop1            { NEXT }

PreTableE:
      POP       AX
      POP       SI

      NEG       CL                          {final: CX = NumColumns-CX+2}
      ADD       CL,[SI+1]
      ADD       CL,2

{====================== TABLE LOOKUP BLOCK =========================}
{   IN:  AL: scancode
         CL: plane (0..7)                                           }

TableBlock:

      {--- 1.- First Table (codepage dependant) ---}

      MOV       DI,SI

      XOR       BX,BX
      MOV       BL,[DI+3]               { BX = CurrentSubmapping }
      SHL       BX,1
      SHL       BX,1
      SHL       BX,1
      ADD       BX,22
      MOV       BX,[BX+DI]

      OR        BX,BX
      JZ        SecondTable
      ADD       DI, BX

      CALL      TableLookUp          { SAFE: AL,CL preserved if NOT found  }
                                     {       and AX, BL set to appropriate }
                                     {       values if found               }
      JNC       Found

      {--- 2.- Second table (global) ---}

      MOV       DI,SI
SecondTable:

      MOV       BX, [DI+22]

      OR        BX,BX
      JZ        ThirdTable
      ADD       DI, BX

      CALL      TableLookUp          { SAFE: AL,CL preserved if NOT found  }
                                     {       and AX, BL set to appropriate }
                                     {       values if found               }
      JNC       Found

      {--- 3.- Third Table (inner) ---}

ThirdTable:
      XOR       CL,CL                         { recompute planes }
      MOV       DL,[ES:KStatusFlags1]

      TEST      BYTE [CS: offset Flags], F_XTMachine
      JNZ		skp1_0
      TEST      BYTE [ES:KStatusByte1], E0Prefixed
      JNZ       NotFound            { plane1: Ctrl+Alt, not E0 }

skp1_0:
      MOV       BL,DL

      AND       BL,12
      CMP       BL,12
      JE        skp1_2b

Skp1_1:
      INC       CL                  { plane 2: shift xor numlock, not E0 }
      MOV       BL,DL
      TEST      DL,3
      JZ        Skp1_2a
      OR        DL,1                     { AL.bit1 = EitherShift }

Skp1_2a:
      PUSH      CX
      MOV       CL,5
      SHR       BL,CL
      POP       CX
      AND       BL,1                        { BL.bit1 = NumLock? }
      XOR       DL,BL
      TEST      DL,1
      JZ        NotFound         { ASSUMED: internal table -> 2 planes }

skp1_2b:
      PUSH      DS               { the third table is at CS: space }

         PUSH      CS
         POP       DS
         MOV       DI,Offset DefaultTable

         CALL      TableLookUp       { SAFE: AL,CL preserved if NOT found  }
                                     {       and AX, BL set to appropriate }
                                     {       values if found               }

      POP       DS

      JNC       Found


      {--- 4.- Finally it wasn't found in the tables! ---}

NotFound:

      TEST      AL,$80                         { if breakcode, chain }
      JNZ       ChainScanCode

      CMP       AL,42             { avoid: Shifts, Ctrls, Alts, Caps }
      JE        ChainScanCode
      CMP       AL,54
      JE        ChainScanCode
      CMP       AL,29
      JE        ChainScanCode
      CMP       AL,157
      JE        ChainScanCode
      CMP       AL,56
      JE        ChainScanCode
      CMP       AL,184
      JE        ChainScanCode
      CMP       AL,58
      JE        ChainScanCode

      MOV       DI,[CS: Offset CombStat]       { diacritics pending? }
      OR        DI,DI
      JZ        ChainScanCode
                                                           { drop it }
      CALL      DWORD PTR [CS:offset Beep]               { reject it }

      MOV       AL,[DS:DI-1]
      XOR       AH,AH
      CALL      DWORD PTR [CS: Offset BIOSStoreKey]
                          { SAFE: we don't need to preserve anything }
      XOR       AX,AX
      MOV       [CS: Offset CombStat],AX

ChainScanCode:
      CALL      DWORD PTR [CS: Offset FlushBuffer]

      STC
      JMP       Ende

{====================== RESOLUTION BLOCK =================================}
{ IN:  AL: character / command
       AH: scancode ($FF means string)
       BL: 0 if character, <>0 if command                             }

Found:

      {--- 1.- Keyb disabled? ---}
      MOV       DI,[CS:Offset PIsActive]        {is it disabled?}
      MOV       CL,[CS:DI]
      OR        CL,CL
      JNZ       KeybIsEnabled

      OR        BL,BL                           {is it X?}
      JZ        ChainScancode

      CMP       AL,101                          {command 101..139?}
      JB        ChainScanCode
      CMP       AL,139
      JA        ChainScanCode                   {YES: continue processing!}


      {--- 2.- Determine if Store or Execute ---}

KeybIsEnabled:

      OR        BL,BL              { XOperation: command, XString or COMBI }
      JNZ       X

      CALL      StoreKey
      JMP       FinishScancode

      {--- 3.- Execute Subblock (!0 was already processed) ---}
X:

      CMP       AL,160                         { 160: NOP (absorve) }
      JE        FinishScancode

      CALL      KEYBexecute    { SAFE: AX set, and we don't need to }
                               {       preserve any register        }

      {--- 4.- Scancode was processed ---}
FinishScancode:
      CLC

{====================== END OF ROUTINE: EXIT POINTS ================}

Ende:
      POP       DI
      POP       SI

      POP       ES
      POP       DS

      POP       DX
      POP       BX
      POP       CX
      POP       AX

      PUSH      BP
      MOV       BP, SP
      RCR BYTE  [BP+6],1
      ROL BYTE  [BP+6],1
      POP       BP
      IRET

end;


{************************************************************************
** EfStoreKey0: Stores AX into BIOS KEYB buffer (AT/PS machines, enh K **
** ------------------------------------------------------------------- **
** IN:   AX: word to be stored                                         **
** OUT:  -                                                             **
*************************************************************************}
Procedure EfStoreKey0; far; assembler;
label ende;
asm
      MOV       CX, AX
      MOV       AH, $05
      INT       $16
      CLC
end;



{------------------------------------------------------------------------
---- KEYB 2 DISCARDABLE FUNCTIONS BEGIN HERE                        -----
------------------------------------------------------------------------}


{------------------------------------------------------------------------
---- MODULE 1: COMBI routines and BEEP                              -----
------------------------------------------------------------------------}

{ ENTRY: AX: scancode/char pair }
procedure DoCombi; far; assembler;
label Lop, Skp, NoCombine, Done;
asm
      PUSH      SI

      CMP       AH, $FF            { if string: ignore, leave for later }
      JE        Done

      MOV       DI, CS:[offset CombStat]        { no more combi pending }
      MOV       WORD [CS:offset CombStat],0

      CMP       AL, 8           { if backspace, then finish, do nothing }
      JE        Done

      MOV       SI,DI                { DI/SI->Combstat: number of pairs }
      XOR       CH,CH                             { CX: number of pairs }
      MOV       CL,[DI]
      INC       DI                                    { DI-> first pair }

Lop:  CMP       Al,[DI]                  { loop to find the second char }
      JNZ       Skp

      MOV       AL,[DI+1]                                       { Found }
      XOR       AH,AH                                      { ScanCode=0 }
      CALL      DWORD PTR [CS: Offset BIOSStoreKey]
      JMP       Done

Skp:
      INC       DI                                               { Next }
      INC       DI
      LOOPNZ    Lop

NoCombine:
      CALL      DWORD PTR [CS:offset Beep]

      PUSH      AX                                 { char to be dropped }
      XOR       AH,AH
      MOV       AL,[SI-1]
      CALL      DWORD PTR [CS:Offset BIOSStoreKey]
      POP       AX
      CALL      DWORD PTR [CS:Offset BIOSStoreKey]

Done:

      POP       SI
      CLC
end;


          { ENTRY: AL = COMBI code }
procedure StartCombi; far; assembler;
label skp0, skp1, lop, ende;
asm
      XOR       BX,BX                   { Load current layout offset }
      MOV       BL,[SI+3]
      SHL       BX,1
      SHL       BX,1
      SHL       BX,1
      ADD       BX,24

      mov       BX,[SI+BX]              { does current layout have strings? }
      or        BX,BX
      jnz       skp0

      MOV       BX,[SI+24]              { and the default }
      OR        BX,BX
      JZ        ende

skp0:
      MOV       DI, SI
      ADD       DI, BX

Lop:
      CMP       BYTE PTR [DI],0         { loop on list of COMBIs: eoList? }
      JZ        Ende

      CMP       AL,200                                { Is this the char? }
      JE        Skp1

      INC       DI                                                 { next }
      MOV       CL,[DI]
      XOR       CH,CH
      SHL       CX,1
      ADD       DI,CX
      INC       DI
      DEC       AL
      JMP       Lop

Skp1: INC       DI
      MOV       [CS: offset CombStat],DI

Ende:
      CLC
end;


procedure Speaker; far; assembler;
label a1, a2, a3, delayL, tick, x1,EndSpeaker;
asm
      MOV       BH,[CS:offset Flags]                 { beeping enabled? }
      TEST      BH,F_BeepDisabled
      JNZ       EndSpeaker

      MOV       BX,420   { frequency: Approx 500 MHz (420 = 500/1.194)  }
      MOV       CX,3     { delay:     Approx 164 ms  (1 tick = 54.9 ms) }


(*  BEGIN Beep code (by Eric Auer) *)

        push  ax

        { SOUND (BX) }
        mov al,0b6h
        out 43h,al      { program timer }
        jmp a1
a1:     mov al,bl
        out 42h,al      { low byte  }
        mov al,bh
        jmp a2
a2:     mov al,bh
        out 42h,al      { high byte  }
        in al,61h
        jmp a3
a3:     or al,3
        out 61h,al      { enable speaker  }

        { DELAY (CX) }
        push ds
        mov ax,40h
        mov ds,ax
        sti
delayL: mov ax,[ds:6ch]
tick:   nop
        cmp ax,[ds:6ch]
        jz tick
        loop delayL
        pop ds

        { NOSOUND() }
        in al,61h
        and al,0fch     { disable speaker }
        jmp x1
x1:     out 61h,al

        pop   ax

(*  END Beep code (by Eric Auer) *)
EndSpeaker:

end;


{------------------------------------------------------------------------
---- MODULE 2: Empty                                                -----
------------------------------------------------------------------------}


{------------------------------------------------------------------------
---- MODULE 3: Basic commands                                       -----
------------------------------------------------------------------------}


{************************************************************************
** BasicCommand: executes KEYB commands 80-199 (excluded 160)          **
** ------------------------------------------------------------------- **
** IN:   AL: KEYB command number (see KEYB documentation)              **
**       AH: Scancode (when appliable) or 0 otherwise                  **
** OUT:  Many registers are probably trashed                           **
**       On APMCommands, may never return                              **
** NOTE: for extended commands, it calls the Extended procedure        **
*************************************************************************}
{**** List of basic commands:
100: disable keyb
101: reenable keyb
161: decimal char
162: beep
164: toggle beeping
}
procedure BasicCommand ; far; assembler;
label  no100, no101, updateActive, skp1, no161, no162, no164, endPC, ende;
ASM
      {---- individual quick commands ---}
      CMP       AL, 100
      JNE       no100

      {100: disable KEYB}
      XOR       CL,CL
      JMP       UpdateActive

      {101: reenable KEYB}
no100:
      CMP       AL, 101
      JNE       no101

      MOV       CL,$FF
UpdateActive:
      MOV       DI,[CS: Offset PIsActive]
      MOV       [CS:DI],CL
      JMP       endPC

no101:
      CMP       AL, 161
      JNE       no161

      XOR       AH,AH

      MOV       AL,[CS: offset DecimalChar]           { first try: GENERIC }
      OR        AL,AL
      JNZ       skp1

      MOV       AL,[SI+2]                   { second try: layout dependant }
      OR        AL,AL
      JNZ       skp1

      MOV       AL,'.'                                { third try: default }
         
skp1:
      call      StoreKey
      jmp       endPC

no161:
      CMP       AL, 162
      JNE       no162

      CALL      DWORD PTR [CS: Offset Beep]
      jmp       endPC

no162:
      CMP       AL, 164
      JNE       no164

      MOV       AL,[CS:offset Flags]
      XOR       AL,F_BeepDisabled
      MOV       [CS:offset Flags],AL


no164:
      CALL      DWORD PTR [CS: Offset ExtCommands]
      JMP       Ende

endPC:
      clc
ende:
end;






{------------------------------------------------------------------------
---- MODULE 4: Extended commands                                    -----
------------------------------------------------------------------------}


{************************************************************************
** ExtCommand: executes KEYB extended commands                         **
** ------------------------------------------------------------------- **
** IN:   AL: KEYB command number (see KEYB documentation)              **
**       AH: Scancode (when appliable) or 0 otherwise                  **
** OUT:  Many registers are probably trashed                           **
**       On APMCommands, may never return                              **
*************************************************************************}
{**** List of commands already implemented ****
89: simulate INS
120..139: change submapping
140: int5h
141: int19h
142: int1bh
143: SysReq
150..154: APMcommand (n-150)
163: set pause bit
168: reset character-by-code buffer
169: release alternative character from character-by-code buffer
170-179: alternative character introduction
180-187: user defined locks OFF
188-195: user defined locks ON }
procedure ExtCommand ; far; assembler;
label  no100, no101, no120, no140, no141, no142, no143,
       no161, no163, no89, noAPM, noAltNum, noSafeMode,
       no168, no169, delAltNum, userLON, error, success, endPC;
ASM

      {--- change submappings (120-139) ---}
      CMP       AL, 120
      JB        No120
      CMP       AL,139
      JA        No120

      SUB       AL,119
      CMP       AL,[SI]
      JA        error

      MOV       [SI+3],AL
      MOV       DL,$FF
      MOV       DI,[CS:offset PIsActive]
      MOV       [CS:DI],DL

      JMP       success


      {---- individual quick commands ---}
No120:
      {---- In SafeMode, commands 140-159 are banned --}
      MOV       BH,[CS:offset Flags]    { not allowed in safe mode }
      TEST      BH,F_SafeMode
      JZ        NoSafeMode

      CMP       AL,140
      JB        NoSafeMode
      CMP       AL,159
      JBE       success

NoSafeMode:
      CMP       AL, 140
      JNE       No140

      {140: int 5H}
      INT       $5
      JMP       success

No140:
      CMP       AL, 141
      JNE       NO141

      {141: int 19H}
      INT       $19
      JMP       success

No141:
      CMP       AL, 142
      JNE       NO142

      {142: int 1BH}

                {empty primary buffer}
      MOV       AL, [ES:BufferHead]
      MOV       [ES:BufferTail], AL
                {empty secondary buffer}
      CALL      DWORD PTR [CS: Offset EmptyBuffer]
      
      INT       $1B
      JMP       success


No142:

      CMP       AL,143
      JNE       No143

      {143: sysreq}
      MOV       AL,AH           { 0: pressed, 1: released }
      XOR       AL,AL
      MOV       AH,$85
      INT       $15
      JMP       endPC

No143:
      CMP       AL, 163
      JNE       No163

      {163: SET PAUSE}
      MOV       AL, [ES:KStatusFlags2]
      OR        AL, 8
      MOV       [ES:KStatusFlags2], AL
      JMP       success

No163:
      CMP       AL, 89
      JNE       No89

      {164: simulate INS}
      MOV       AL, [ES:KStatusFlags1]
      XOR       AL, $80
      MOV       [ES:KStatusFlags1], AL
      MOV       AX,$5200
      CALL      StoreKey
      JMP       success

      {----  APM commands (150-154) ----}
No89:
      CMP       AL, 150
      JB        NoAPM
      CMP       AL,154
      JA        NOAPM
      SUB       AL,150
      CALL      DWORD PTR [CS:Offset APMProcedure]
      JMP       EndPC

      {---- alternative character introduction ---}
NoAPM:
      CMP       AL, 170
      JB        NoAltNum
      CMP       AL,179
      JA        NoAltNum

      SUB       AL,170
      MOV       CL,AL
      MOV       DX,10
      XOR       AH,AH
      MOV       AL,[ES:AltInput]
      MUL       DX
      ADD       AL,CL
      MOV       [ES:AltInput],AL
      JMP       success

NoAltNum:
      CMP       AL,168
      JNE       No168

DelAltNum:
      XOR       AL,AL
      MOV       [ES:ALTINPUT],AL
      JMP       success

No168:
      CMP       AL,169
      JNE       No169

      XOR       AH,AH
      MOV       AL,[ES:AltInput]
      CALL      StoreKey
      JMP       DelAltNum
         
      {---- user defined keys ----}
No169:
      CMP       AL,180
      JB        ERROR
      CMP       AL,195
      JA        ERROR
      CMP       AL,187
      JA        USERLON

      MOV       CL,AL
      SUB       CL,180
      MOV       AL,1
      SHL       AL,CL
      TEST      [CS: OFFSET UserKeys],AL  { SWITCH OFF BIT }
      JZ        success
      XOR       [CS: OFFSET UserKeys],AL
      JMP       success
         
UserLON:
      MOV       CL,AL
      SUB       CL,188
      MOV       AL,1
      SHL       AL,CL
      TEST      [CS: OFFSET UserKeys],AL  { SWITCH OFF BIT }
      JNZ       success
      OR        [CS: OFFSET UserKeys],AL

      {---- exit routines ----}
success:
      CLC
      JMP       endPC

error:
      STC

endPC:

end;


{------------------------------------------------------------------------
---- MODULE 5: Default shift/locks (to be moved/patched)            -----
------------------------------------------------------------------------}


{------------------------------------------------------------------------
---- MODULE 6: Specific Japanese API                                -----
------------------------------------------------------------------------}

procedure Japanese; assembler;
label SetState, SetKanaOff, JapaneseError, UpdateAndEnd;
asm
      OR        AL,AL
      JZ        SetState

      { Get state }
      MOV       DX, [CS:Offset UserKeys]
      AND       DX, 1
      SHL       DX, 1
      XOR       AX, AX
      RET

      { Set state }
SetState:
      AND       DL, 7
      OR        DL, DL
      JZ        SetKanaOff
      MOV       AX, [CS: offset UserKeys]

      CMP       DL, 2
      JNE       JapaneseError

      OR        AX, 1
UpdateAndEnd:
      MOV       [CS: offset UserKeys], AX
      RET
      
SetKanaOff:
      AND       AX, $FFFE
      JMP       UpdateAndEnd

JapaneseError:
      MOV       AX, $FFFF

end;


{------------------------------------------------------------------------
---- MODULE 7: XT specific routines                                 -----
------------------------------------------------------------------------}


CONST
        { keyboard commands }
        DisableKeyboard = $AD;
        EnableKeyboard  = $AE;

{************************************************************************
** EnableKeyboardProc: enable keyboard hardware                        **
** ------------------------------------------------------------------- **
** IN:   -                                                             **
** OUT:  AX and CX are trashed                                         **
*************************************************************************}
Procedure EnableKeyboardProc; assembler;
label WaitReady2;
asm
      XOR       CX, CX       { counter to 65535! }

WaitReady2:
     IN         AL, $64      { read status register }
     TEST       AL, 2        { bit 1 set: input buffer full? }
     LOOPNZ     WaitReady2   { loop until timeout or bit 1 clear }

     MOV        AL, EnableKeyboard
     OUT        $64, AL      { send the enable command }

end;



{************************************************************************
** Int9H: Basic int9 management                                        **
** ------------------------------------------------------------------- **
** INT:  services interrupt 9h (keyboard IRQ)                          **
*************************************************************************}
procedure Int9H; assembler;
Label  IsActiveF, Start, WaitReady1, Reenable, Lop1, EndProcessPause,
       ChainToOld, LeaveInt;
ASM

    {******* SECTION 1: COMMON INT9h ************}

        {-- Workaround for APL software --}
     JMP        Start
IsActiveF:
     DW         1             { modified by APL software}

        {-- Save the registers --}
Start:
     PUSH       AX            { scancode }
     PUSH       CX            { counter }
     PUSH       ES

     {-- Compute if we should handle the extended --}
     MOV        AL, CS:[Offset IsActiveF]
     OR         AL,AL
     JZ         ChainToOld

     {-- Disable the interrupts and the keyboard --}
     CLI                  { disable hardware ints }
     XOR        CX,CX
     MOV        ES,CX
     DEC        CX            { counter to 65535! }

WaitReady1:
     IN         AL, $64       { read status register }
     TEST       AL, 2         { bit 1 set: input buffer full? }
     LOOPNZ     WaitReady1    { loop until timeout or bit 1 clear }

     MOV        AL, DisableKeyboard
     OUT        $64, AL       { send the disable command }

     {-- Read and authenticate scancode --}
     IN         AL, $60      { get scancode to AL }
     MOV        AH, $4F      { Authenticate scancode}
     STC

     INT        $15          { Return: CF clear if key needs no further processing }
     JC         ChainToOld { No further processing of pressed keys if CARRY cleared! }

Reenable:
     {-- Reenable Interrupt controller, Interrupts and Keyboard --}
     CALL       EnableKeyboardProc  { reenable keyboard }
     MOV        AL, $20       { report End of Interrupt to interrupt controller }
     OUT        $20, AL
     STI                  { restore interrupts }

     {-- Process PAUSE, if needed (this is EXTENDED handling) --}
     MOV        AL, [CS: offset Flags] {LoopRunning}
     TEST       AL, F_PauseLoop       { is Pause loop running?}
     JNZ        EndProcessPause        { yes: leave interrupt! }

     OR         AL, F_PauseLoop                  { lock this region }
     MOV        CS:[Offset Flags], AL   {LoopRunning}
Lop1:
     MOV        AL,[ES:KStatusFlags2]
     TEST       AL, 8                { in Pause? then continue loop }
     JNZ        Lop1

     MOV        AL, [CS: offset Flags] {LoopRunning}
     XOR        AL, F_PauseLoop
     MOV        CS:[Offset Flags], AL { exit locked region }  {LoopRunning}

EndProcessPause:

     JMP        LeaveInt

ChainToOld:
     CALL       EnableKeyboardProc
     PUSHF
     STI
     CALL       DWORD PTR CS:[Offset OldInt9h]

    {******* SECTION 5: LEAVE INTERRUPT ROUTINE ************}

LeaveInt:
     POP        ES
     POP        CX
     POP        AX
     IRET
end;



{------------------------------------------------------------------------
---- MODULE 8: APM routines                                         -----
------------------------------------------------------------------------}


{************************************************************************
** APMProc: process an APM command                                     **
** ------------------------------------------------------------------- **
** IN:   AL: (0-based) APM command number (see constants above)        **
** OUT:  Many registers are probably trashed                           **
**       On some commands, may never return                            **
*************************************************************************}
procedure APMProc; far; assembler;
label
      FlushNLCACHE, NLCACHE_callf, NLCACHE_farentry, NLCACHE_reentry,
      FlushSMARTDRV, FlushCacheForce, FlushCacheExit,
      EndAPMProc, Skp1, Skp2, Skp3, doReset;

const CallReboot : pointer = ptr ($FFFF,$0000);

asm

        push es
        push ax { save parameter }

(*  BEGIN FlushCache code (by Matthias Paul, Axel C. Frinke) *)
(* == Last edit: 2001-06-18 MPAUL
;
; - Taken from Axel C. Frinkeïs & Matthias Paulïs FreeKEYB sources
;   with some modifications to possibly work as a drop-in replacement
;   in 4DOS.
; - While the implied actions are different for SMARTDRV 4.0+
;   and NWCACHE 1.00+, the result is the same for both of
;   them - the cache will be flushed unconditionally.
; - Works around a problem with DBLSPACE loaded, where DBLSPACE
;   may terminate the current process with an error message, that
;   it would not be compatible with SMARTDRV before 4.10+.
; - Works around a problem, where the cache would not be flushed
;   with NWCACHE 1.00+, if the CX register accidently happened
;   to be 0EDCh.
; - Is enabled to continue to work with future issues of NWCACHE,
;   which might no longer flush the cache just on calling
;   SMARTDRV's install check...
; - Supports NetWare Lite's NLCACHE and Personal NetWare's
;   NWCACHE sideband API to asynchronously flush the cache.
;   This ensures system integrity even when the NetWare Lite or
;   Personal NetWare SERVER is loaded on the machine.
; - Furthermore, under some conditions on pre-386 machines
;   NWCACHE cannot intercept a reboot broadcast by itself, and
;   hence it must be called explicitely before a possible reboot.  *)


FlushNLCACHE:
        mov     ax,0D8C0h       { NLCACHE/NWCACHE install check  }
        mov     cl,ah           { (sanity check: preset CL >= 10h) }
        push    cs              { (preset to ourselves for safety) }
        pop     es              
        mov     di,offset NLCACHE_farentry
        int     2Fh             { (NLCACHE/NWCACHE modify AL,CL,DX,DI,ES)}

        cmp     ax,0D8FFh       { cache installed? (AL = FFh)             }
        jne    FlushSMARTDRV

        cmp     cl,al           { CL=FFh? (workaround for NWCACHE before  }
         je     NLCACHE_callf   { BETA 17 1993-09-28, CL=FFh,00h,01h)     }
        cmp     cl,10h          { (sanity check: CL < 10h on return,      }
         jae    FlushSMARTDRV   { only CL=01h,02h,03h are defined so far) }

NLCACHE_callf:
        xor     bx,bx           { BX=0: asynch. flush request from server }
        push    cs              { push return address on stack            }
        mov     ax,offset NLCACHE_reentry
        push    ax              
        push    es              { push ES:DI -> entry point into far API  }
        push    di              
        clc                     { assume pure cache                       }
NLCACHE_farentry:               { (dummy entry point)                     }
        retf                    { simulate a CALLF into sideband function }

NLCACHE_reentry:                { return from sideband (AX/flags modified)}
         jc     FlushNLCACHE    { if error retry because still dirty cache}
        test    ax,ax           { CF clear and AX=0000h?                  }
         jnz    FlushNLCACHE    { if not, retry until everything is OK    }

(*NLCACHE/NWCACHE is pure now, so it would be safe to exit here.
; However, it doesn't harm to play it extra safe and
; just fall through into the normal SMARTDRV flush sequence...
; Who knows, multiple caches might be loaded at the same time...  *)

FlushSMARTDRV:
        mov     ax,4A10h        { SMARTDRV 4.00+ API                      }
        xor     bx,bx           { install check (BX=0)                    }
        mov     cx,0EBABh       { mimic SMARTDRV 4.10+ (CX=EBABh)         }
                                { to workaround DBLSPACE problem.         }
                                { CX<>0EDCh to avoid NWCACHE's /FLUSH:OFF }
                                { special case! Flush regardless          }
                                { of NWCACHE's configuration setting.     }
        int     2Fh             { (modifies AX,BX,CX,DX,DI,SI,BP,DS;ES?)  }
                                { NWCACHE 1.xx has flushed its buffers    }
                                { now, but we should not rely on this.    }
        cmp     ax,6756h        { NWCACHE 1.00+ magic return?             }
         je     FlushCacheForce {  (extra-safe for future NWCACHE 2.00+)  }
        cmp     ax,0BABEh       { SMARTDRV 4.0+ magic return?             }
         jne    FlushCacheExit  {  nothing we can do                      }
         jcxz   FlushCacheExit  { any dirty cache elements?               }
FlushCacheForce:
        mov     cx,ax           { CX<>0EDCh to avoid NWCACHE special case,}
                                { hence we preset with magic return for   }
                                { possible future broadcast expansion.    }
        mov     ax,4A10h        { SMARTDRV 4.00+ API                      }
        mov     bx,0001h        { force synchronous cache flush           }
        push    cx              
        int     2Fh             { (modifies BP???)                        }
        pop     cx              { (safety only, not necessary)            }
        cmp     cx,6756h        { retry for any cache but NWCACHE         }
        jne    FlushSMARTDRV    { probably obsolete, but safer            }
                                { at the risk of a possible deadlock in   }
                                { case some hyphothetical SMARTDRV        }
                                { clone would not support the CX return   }
FlushCacheExit:

(*  END FlushCache code (by Matthias Paul, Axel C. Frinke)  *)

     POP        AX              { Recover APM_Command }

     TEST       AX,AX           { APM_command=0? then we are done }
     JZ         EndAPMProc

     XOR        CX,CX           { For the rest, set ES=0 }
     MOV        ES,CX

     DEC        AL              { APM_command=1: Warm Reboot }
     JNZ        skp1

     MOV        CX,$1234        { YES: warm reboot }
     JMP        doReset

     STC                        { if we are back, return with errors }
     JMP        EndAPMProc

skp1:
     DEC        AL              { APM_command=2: Cold Reboot }
     JNZ        skp2

doReset:
     MOV        [ES:$472],CX    { YES: cold reboot  (CX=0 here)}
     CALL       callReboot

     STC                        { if we are back, return with errors }
     JMP        EndAPMProc

skp2:
                                { FOR THE REST OF THE COMMANDS: }
                                { Enable Real Mode APM-BIOS Interface }
     PUSH       AX

     MOV        AX, $5301      {Real Mode interface connect}
     XOR        BX, BX
     INT        $15

     MOV        AX, $530F      {Engage power management}
     MOV        BX, 1
     MOV        CX, BX
     INT        $15

     MOV        AX, $5308      {Enable APM for all devices}
     MOV        BX, 1
     MOV        CX, BX
     INT        $15

     MOV        AX, $530E      {force version 1.1}
     XOR        BX, BX
     MOV        CX, $0101
     INT        $15

     POP        AX             { DONE: Continue processing commands }

     DEC        AL              { APM_command=3: Shut system down }
     JNZ        skp3

     MOV        AX, $5307              {First attempt: switch all off}
     MOV        BX, 1
     MOV        CX, 3
     INT        $15

     MOV        AX, $5307              {Second attempt: system bios}
     XOR        BX, BX
     MOV        CX, 3
     INT        $15
     
     STC                        { if we are back, return with errors }
     JMP        EndAPMProc

skp3:
     DEC        AL              { APM_command=4: Suspend system }
     JNZ        skp3

     MOV        AX, $5307
     MOV        BX, 1
     MOV        CX, 2
     INT        $15

     CLC                        { we return OK }

EndAPMProc:
     POP        ES

end;


{------------------------------------------------------------------------
---- MODULE 10: XString management                                  -----
------------------------------------------------------------------------}

{************************************************************************
** EfStoreKey1: Stores AX into BIOS KEYB buffer                        **
** ------------------------------------------------------------------- **
** IN:   AX: word to be stored                                         **
**                                                                     **
** NOTE: this one is to be called whenever Int16h is hooked            **
*************************************************************************}
Procedure EfStoreKey1; far; assembler;
label jmp1,jmp2,ende,BufferFull1,ende2;
asm
      MOV       CX, AX
      MOV       AH, $05
      PUSHF
      CALL CS:OldInt16h
end;


{************************************************************************
** XStrData: variables for XStrings management                         **
*************************************************************************}
procedure XStrData;
Begin
   ASM
     DD   0               { XBuffer: starts on 0 }
     DW   0,0             { XBufferHead and XBufferTail }
     DB   0               { PutXStr (bool): XString in process? }
     DD   0               { XString (ptr):  being processed }
     DB   0               { XStrPos (byte): position in string }
   End;
End;



{************************************************************************
** EfStoreKey2: store a key in the secondary buffer                    **
** ------------------------------------------------------------------- **
** IN:   AX: scancode/char pair to be stored                           **
** OUT:  BX, ES:DI are trashed                                         **
*************************************************************************}
Procedure EfStoreKey2; far; assembler;  { effectively store key in XBuffer. }
label jmp1, BufferFull2, ende;
asm
     MOV        BX,CS:[Offset XStrData+4]   { buffer head to BX }

     PUSH       AX                   { is the buffer full? }
     MOV        AX,CS:[Offset XStrData+6]   { AX-> previous char to Tail }
     OR         AX,AX
     JNE        jmp1
     MOV        AX,64
jmp1:
     DEC        AX
     CMP        BX,AX                { done, compare both }
     POP        AX
     JE         BufferFull2


     LES        DI,CS:[Offset XStrData]
     PUSH       BX                          { Increase to DI the Head offset }
     SHL        BX,1                        { (it is counted in words) }
     ADD        DI,BX
     POP        BX
     MOV        [ES:DI],AX                  { store the value }

     INC        BX                          { increase pointer, maximally is 64 }
     AND        BX,63

     MOV        [CS:Offset XStrdata+4], BX  { store again the buffer head }
     JMP        ende

BufferFull2:
     CALL       DWORD PTR [CS:offset Beep]  { Beep! }

ende:
     CLC
end;


{ Exit: ZF: 1 if not empty, 0 if empty }
procedure IsSecondaryBufferEmpty; assembler;
ASM
     PUSH       BX
     MOV        BX,[CS:offset XStrData+4]   {XBufferHead}
     CMP        BX,[CS:offset XStrData+6]   {XBufferTail}
     POP        BX
end;


procedure EmptySecondaryBuffer; far; assembler;
asm
     PUSH       BX
     MOV        BX,[CS:offset XStrData+4]   {XBufferHead}
     MOV        [CS:offset XStrData+6],BX   {XBufferTail}
     POP        BX

end;


{ Find the String in the current layout }
{ In: DS=CS to use XStrData }
procedure FindStr; assembler;
label Skp0,Lop2,Skp2,EndFind;
ASM
           PUSH       BX
           PUSH       CX
           PUSH       ES
           
           LES        DI,CS:[offset currentLayout]

           {** Get a pointer to the strings **}
           XOR        BH,BH
           MOV        BL,ES:[DI+3]     { current submapping }

           SHL        BX,1             { Particular submapping first }
           SHL        BX,1
           SHL        BX,1
           ADD        BX,26
           MOV        BX, ES:[DI+BX]

           OR         BX,BX                 { Second, try the general }
           JNZ        skp0
           MOV        BX, ES:[DI+26]

           OR         BX,BX                 { Finally, NOT found }
           JZ         EndFind

skp0:
           ADD        DI, BX


           {** Skip AL-1 strings **}


           XOR        CH,CH
           MOV        CL,AL
           DEC        CL
           JZ         skp2

Lop2:
           MOV        AL,ES:[DI]              { Length of string }
           OR         AL,AL                   { sign for no more strings }
           JZ         EndFind
           XOR        AH,AH
           SHL        AX,1                    { each char is a WORD }
           INC        AX
           ADD        DI,AX
           LOOP       Lop2


           {** String found, register it in String data }
           
Skp2:
           CMP        BYTE PTR ES:[DI],0     { if length=0, end }
           JZ         EndFind

           MOV        [offset XStrData+9],DI
           MOV        [offset XStrData+11],ES

           MOV        AL, 1
           MOV        [offset XStrData+13],AL  { Initial position = 1 }
           MOV        [offset XStrData+8],AL   { String in process = TRUE }


EndFind:
           POP        ES
           POP        CX
           POP        BX
END;



procedure FlushSecondaryBuffer; far; assembler;
label  work, skp1, lop1, CheckStringPending, skp2, StoreIt, ContinueLoop,
       NoAction, Ende;
asm

      {** If the secondary buffer is empty, exit **}
      CALL      IsSecondaryBufferEmpty
      JNZ        work

      MOV        AL,[CS: offset XStrData+8]
      OR         AL,AL
      JZ         ende


work:

      {** set ES=0, DS=CS **}
      XOR       BX,BX
      MOV       ES,BX

      PUSH      CS
      POP       DS

      {** Compute free space in primary buffer to CX **}
      MOV       CX,[ES:BufferEnd]
      SUB       CX,[ES:BufferStart]  { Buffer size in Byte. }
      MOV       AX,[ES:BufferHead]
      SUB       AX,[ES:BufferTail]
      JNC       Skp1                 { If BX negative, free space =-BX }
      XOR       CX,CX
Skp1: SUB       CX,AX

      SHR       CX,1                 { 1 char = 2 bytes }
      DEC		CX					 { even if empty, size = end-start-1 }

      OR		CL,CL
      JZ        ende


      {** Loop while room in buffer (CX) which will be in stack **}
Lop1:

           PUSH        CX              { protect counter }
           
CheckStringPending:
           { Are there strings in process? }
           MOV         AL,[offset XStrData+8]
           OR          AL,AL
           JZ          skp2

           { YES: Put a character from the String }
           LES         DI,[offset XStrData+9]   {ES:DI-> String being put}

           XOR         BH,BH       { position: from pos to string offset }
           MOV         BL,[offset XStrData+13]               { X => 2X-1 }
           SHL         BL,1
           DEC         BL

           MOV         AX,[ES:DI+BX]         { recover char and store it }
           CALL        EfStoreKey1

           MOV         AL,[offset XStrData+13] { forward string position }
           INC         AL
           MOV         [offset XStrData+13],AL

           CMP         AL,[ES:DI]              { is the string finished? }
           JBE         ContinueLoop

           XOR         AL,AL                     { YES: disable the flag }
           MOV         [offset XStrData+8],AL
           JMP         ContinueLoop

           { NO: Continue the loop }
skp2:
           { if secondary buffer is empty, exit the loop directly}
           CALL        IsSecondaryBufferEmpty
           JZ          NoAction

           LES         DI,[offset XStrData]     {ES:DI-> Secondary buffer}

           { Get a key from secondary buffer to AX }
           MOV         BX,[offset XStrData+6]
           SHL         BX,1
           MOV         AX,ES:[DI+BX]

           SHR         BX,1             { Forward secondary buffer pointer }
           INC         BX
           AND         BX,63
           MOV         [offset XStrData+6],BX


           { If scancode is $FF, then it is String, else store it }
           CMP         AH,$FF
           JNE         StoreIt

           CALL        FindStr
           JMP         CheckStringPending

StoreIt:
           CALL        EfStoreKey1

      {** End of Loop  **}
ContinueLoop:
      POP       CX              { recover loop pointer }

      LOOP      Lop1
      JMP       ende

NoAction:
      POP       CX


ende:
end;




{************************************************************************
** Int16h: new int 16h handler                                         **
** ------------------------------------------------------------------- **
** INT:  before int16h is served, transfers keys from secondary to     **
**       primary/BIOS buffer                                           **
*************************************************************************}

Procedure Int16h; assembler;
label NoFlushBuffer, NewGetKey, Chain16, NoGetKey;
ASM
      PUSHF
      PUSH      AX
      PUSH      CX
      PUSH      BX
      PUSH      DX
      PUSH      SI
      PUSH      DI
      Push      DS
      Push      ES

      {** Flush secondary buffer **}

NoFlushBuffer:

      {** Our new functions: 0h, 10h, 13h **}
      OR        AH,AH
      JZ        NewGetKey

      CMP       AH,$10
      JNE       NoGetKey

      {** The new GetKey routines **}
NewGetKey:               { loop until key available, flushing secondary }
      PUSH      AX
      CALL      FlushSecondaryBuffer

      POP       AX
      
Chain16:
      POP       ES      { there's a char, go call BIOS function }
      POP       DS
      POP       DI
      POP       SI
      POP       DX
      POP       BX
      POP       CX
      POP       AX
      POPF

      JMP CS:OldInt16h

      {** Test if it is the special Japanese call **}
NoGetKey:
      CMP       AH,$13
      JNE       Chain16

      PUSH      AX
      MOV       AH,[CS:offset Flags]
      TEST      AH,F_Japanese
      POP       AX
      JZ        Chain16

      POP       ES      { there's a char, go call BIOS function }
      POP       DS
      POP       DI
      POP       SI
      POP       DX
      POP       BX
      POP       CX
      POP       AX
      POPF
      
      CALL      Japanese
      IRET

End;

{------------------------------------------------------------------------
---- DATA PADDING SPACE                                             -----
------------------------------------------------------------------------}

Procedure Data;                               {  about 2'75K free memory. }
Begin ASM
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
{ 1Kb }
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DD 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
End; End;

{------------------------------------------------------------------------
---- KEYB 2 INITIALISATION STUB BEGINS HERE                         -----
-------------------------------------------------------------------------}

{$S+,I+,R-}


VAR
        s           : string;                { multipurpose generic string }
        KeybCBSize  : word;                           { size of the KeybCB }
        int9hChain  : boolean;                    { should we chain int9h? }
        checksum    : byte;                    { checksum used in KC files }
        Verbose     : boolean;                           { Verbose output? }
        MCB         : word;                                  { MCB Segment }
        ResCS       : word;                      { CS of the resident copy }
        KCauthor, KCinfo: string;
        DriverNameZ : string;
        NOHI        : boolean;                    { don't use upper memory }
        SafeMode    : boolean;
        BeepDisabled: boolean;



function TestInstallation : Byte;
{ Check whether a copy of FD-Keyb is already installed. }
{ Result:   0 -> No keyboard driver installed. }
{           1 -> Identical version of FD-Keyb installed. }
{                ÈÍ>> CurrentLayout will be set to data region of resident copy. }
{           2 -> Different version of FD-Keyb installed. }
{           3 -> Different keyboard driver installed. }
label  NoKeyb, OtherKeyb, OtherVersion, EndTest;
var    r: byte;
begin

  asm
      mov  ax, $AD80
      xor  cx,cx
      mov  bx,cx
      int  $2F

      cmp  al, $FF      {=== Part 1: any-KEYB? ==}
      jne  NoKeyb

      mov  ax,1

      test bx,$FFFF     {=== Part 2: which KEYB? ==}
      jnz  OtherKeyb
      test cx,$FFFF
      jnz  OtherKeyb

      cmp  dx,Version   {=== Part 3: which version? ==}
      jne  OtherVersion

      push ax
      push es
      mov  ax,$AD8C             { use our backdoor ;-) }
      int  $2F

      mov  ResCS,es
      pop  es
      pop  ax
      
      jmp  endTest      { ax=1 already! }

OtherKeyb:
      inc  ax

OtherVersion:
      inc  ax
      jmp  endTest
      
NoKeyb:
      xor  ax,ax

endTest:
      mov r, al
  end;
  TestInstallation := r
end;



function maxb (a,b: byte): byte;     { determine the maximum of two values }
begin
    if a>b then maxb:=a else maxb:=b
end;



procedure Keep(Laenge: word; ecode: byte);                       {TSR exit }
var  vES: word;
Begin
   vES := PWord (ptr(PrefixSeg,$2C))^;

   asm
      MOV AX, $4900                                     { free the env code }
      MOV ES, vES
      INT $21

      MOV AH, $31                                                { exit TSR }
      MOV AL, ecode
      MOV DX, Laenge
      INT $21
   END;
End;


function KEYBqueryActive: boolean;
var
   regs: registers;
begin
      regs.AX := $AD83;
     intr ($2F, regs);
     KEYBqueryActive := regs.Bl<>0
end;



Procedure ShowInfo;
var
   s,s2: string;
   i,n: byte;
Begin
     WriteLn; WriteLn;

     { 1.- Get the layout name }
     
     s := '';
     i := 12;
     while pBArray(CurrentLayout)^[i]<>0 do begin
         s := s + chr (pBArray(CurrentLayout)^[i]);
         inc (i)
     end;

     if pWArray (CurrentLayout)^[5]<>0 then begin
        str (pWArray (CurrentLayout)^[5], s2);
        s := s+' (ID:'+s2+')'
     end;

     { 2.- Write the data }
     WriteLn (STR4_1, ord(KEYBQueryActive));
     WriteLn (STR4_2, s);
     WriteLn;
     WriteLn (STR4_3);

     i := pBArray(CurrentLayout)^[3];
     WriteLn (STR4_4, pWArray(CurrentLayout)^[10 + 4*i]);
     WriteLn (STR4_4b, i);

     n :=pBArray(CurrentLayout)^[0]-1;
     WriteLn (STR4_5, n);
     Write   (STR4_6 );
     for i:=1 to n do begin
         Write ( pWArray(CurrentLayout)^[10 + 4*i] );
         if i<n then write (', ')
     end;
     writeLn;
     writeLn;

     Halt (0);
End;


procedure ShowFastHelp;
begin
    WriteLn;
    Writeln (STR3_1);
    WriteLn (STR3_2);
    WriteLn ('(c) Aitor Santamar¡a_Merino 2003-2005 - GNU GPL 2.0 (/? help, /V verbose)');
    WriteLn;
    WriteLn (STR3_3);
    WriteLn ('      [/NOHI] [/B] [/S]');
    WriteLn ('KEYB');
    WriteLn ('KEYB  /U');
    WriteLn ('KEYB  /?');
    WriteLn;
    WriteLn (STR3_5);
    WriteLn (STR3_6);
    WriteLn (STR3_7);
    Writeln (STR3_8);
    WriteLn (STR3_9);
    WriteLn (STR3_10);
    WriteLn (STR3_11);
    WriteLn (STR3_12);
    WriteLn (STR3_13);
    WriteLn (STR3_14);
    WriteLn (STR3_15);
    WriteLn (STR3_16);
    WriteLn (STR3_17);
    WriteLn (STR3_18);
    WriteLn (STR3_19);
    WriteLn (STR3_20);
    WriteLn;
    WriteLn (STR3_21);
    halt (0)
end;



procedure Error(B : byte; AddErrorInfo: string);  { aborts KEYB with error }
var e: byte;
Begin
   WriteLn;
   Case B of
      1 : begin Writeln(STR2_1); e:=7 end;
      2 : begin Writeln(STR2_2); e:=7 end;
      3 : begin Writeln(STR2_3); e:=1 end;
      4 : begin Writeln(AddErrorInfo,': ',STR2_4); e:=2 end;
      5 : begin Writeln(STR2_5); e:=8 end;
      6 : begin Writeln(STR2_6); e:=9 end;
      7 : begin Writeln(STR2_7); e:=9 end;
      8 : begin WriteLn(STR2_8); e:=8 end;
      9 : begin WriteLn(STR2_9); e:=9 end;
      10: begin WriteLn(STR2_10); e:=3 end;
      11: begin WriteLn(STR2_11,AddErrorInfo); e:=1 end;
      12: begin WriteLn(STR2_12,AddErrorInfo); e:=6 end;
      13: begin WriteLn(STR2_13); e:=2 end;
      14: begin WriteLn(STR2_14); e:=1 end;
      15: begin WriteLn(STR2_15); e:=9 end;
{      16: begin WriteLn(STR2_16); e:=4 end;
      17: begin WriteLn(STR2_17); e:=5 end; }
      18: begin WriteLn(STR2_18); e:=7 end;
      19: begin WriteLn(STR2_19); e:=2 end;
      20: begin WriteLn(STR2_20); e:=2 end;
      21: Begin WriteLn(STR2_21); e:=1 end;
      22: Begin WriteLN(STR2_22); e:=2 end;
      { these never occur }
   End;
   Halt(e);
End;



procedure MCBDeAlloc ( MCBseg: word );
var r_ax, f: word;
begin
     asm
        mov     ah, $49
        mov     es, MCBSeg
        int     $21
        mov     r_ax, ax
        pushf
        pop     ax
        mov     f, ax
     end;
     if (f and FCarry)>0
        then writeln (STR2_50, r_ax);
end;


{Allocate memory, make it self-parented}
function MCBAlloc ( para: word ) : word;
var allocs,f,m: word;
    umblink,myumblink: byte;
begin
     myumblink := ord (not NOHI);
     asm
        mov     ah, $58          { get alloc strategy }
        xor     al, al
        int     $21
        mov     allocs, ax

        mov     ah, $58          { set alloc strategy }
        mov     al, 1
        xor     bh,bh
        mov     bl, $41
        int     $21

        mov     ah, $58          { get UMB link state }
        mov     al,2
        int     $21
        mov     umblink, al

        mov     ah, $58          { set UMB link state }
        mov     al, 3
        xor     bh,bh
        mov     bl, myumblink
        int     $21

        mov     ah, $48          { allocate memory }
        mov     bx, para
        int     $21
        mov     m, ax
        pushf
        pop     ax
        mov     f,ax

        mov     ah, $58          { set alloc strategy }
        mov     al, 1
        mov     bx,allocs
        int     $21

        mov     ah, $58          { set UMB link state }
        mov     al, 3
        xor     bh,bh
        mov     bl, umblink
        int     $21
     end;

     if (f and FCarry)>0 then begin
        writeln (STR2_51, m);
        halt (9);
     end;

     pWord ( ptr(m-1,1) )^ := m;        { make it self-parented }
     move (DriverNameZ[1], ptr(m-1,8)^, length(DriverNameZ));

     MCBAlloc := m
end;


Procedure Remove;                           { Remove Keyb from memory. }
Var    IntVec      : Array[Byte] of Pointer absolute 0:0;
       MultiHand   : Pointer;
       Int16Hand   : Pointer;
       Int9Hand    : Pointer;
       Int15Hand   : Pointer;
       s           : word;

Begin
   WriteLn;

   if MCB<>0 then MCBDeAlloc (MCB);

{ Check whether removing is possible. }


   MultiHand := ptr (ResCS,ofs(Int2Fh));
   Int16Hand := ptr (ResCS,ofs(Int16h));
   Int9Hand  := ptr (ResCS,ofs(Int9h));
   Int15Hand := ptr (ResCS,ofs(Int15h));

   if not (
         ((IntVec[$09]=Int9Hand)  or (not assigned(OldInt9h))) and
         ((IntVec[$16]=Int16Hand) or (not assigned(OldInt16h))) and
         (IntVec[$2F]=MultiHand) and
         (IntVec[$15]=Int15Hand) )
   Then begin
        if IntVec[$15]<>Int15Hand
             then writeln ('ERROR: vector int15h was overwritten');
        if IntVec[$2F]<>MultiHand
             then writeln ('ERROR: vector int2Fh was overwritten');
        if (IntVec[$16]<>Int16Hand) and assigned(OldInt16h)
             then writeln ('ERROR: vector int16h was overwritten');
        if (IntVec[$09]<>Int9Hand) and assigned(OldInt9h)
             then writeln ('ERROR: vector int9h was overwritten');
        Error(5,'');      { Removing impossible. Interrupt vectors were changed by another program. }
   end;

{ Uninstall it. }
      if assigned (OldInt9h)  then SetIntVec ($9, OldInt9h);
      if assigned (OldInt16h) then SetIntVec ($16, OldInt16h);
      SetIntVec ($15, OldInt15h);
      SetIntVec ($2F, OldInt2Fh);


      s := ResCS-16;           { Program segment. }

      asm
         mov  ah,$49
         mov  es,s
         int $21
      end;

   Writeln(STR1_4);
   Halt (0);
End;


function ProgramPath: string;       { returns the Path in which keyb works }
var
   s: string;
begin
   s := ParamStr(0);
   while (length(s)>0) and (s[length(s)]<>'\') do
      delete (s,length(s),1);
   delete (s,length(s),1);
   ProgramPath := s
End;


{ Reads a single KL file. }
{ Returns:   length of the KeybCB stored there }
{            0 whenever the ID was NOT found there }
{            On error, it will halt }

function ReadKLFile (var f: file; idn: word; ids: string; maxsize: word;
                         computechecksum: boolean ): word;
var
    b         : byte;
    sizeread  : word;
    i         : byte;
    s         : string;
    c         : word;

begin
     { Id }
     BlockRead (f, b, 1, sizeread); {length in bytes}

     if (sizeread<1) or (b=0) then error (13,'');
     if computechecksum then checksum := lo ( checksum + b);

     s := '';
     for i:=1 to b do s := s+' ';
     BlockRead (f, s[1], b, sizeread);
     if sizeread<b then error(13,'');

     if computechecksum then
        for i:=1 to b do checksum := lo (checksum + ord(s[i]));

     maxsize := maxsize - 1 - length(s);

     BlockRead (f, CurrentLayout^[0], maxsize,sizeread);

     s := ','+s+',';
     if ( pos(','+chr(lo(idn))+chr(hi(idn))+ids+',',s)=0 )
        then sizeread := 0
        else begin
             write (':',ids);
             if idn<>0 then write ( '(',idn,')' )
        end;

     if (sizeread>0) and computechecksum then
           for c:=0 to sizeread-1 do checksum := checksum + lo(CurrentLayout^[c]);

     ReadKLFile := sizeread;
end;


{Reads and parses the KC file
 Returns:  0      if the layout/id was not found in file
           length found otherwise
 On error: halts }
function ReadKCFile (var f: file; idn: word; ids: string): word;
var
    b,i       : byte;
    s         : string;
    sizeread  : word;
    fileIsRO  : boolean;
    KLsize    : word;
    l         : word;
    
begin

     BlockRead (f, b, 1, sizeread);                            { flag }
     if (sizeread<1) then error (13,'');
     FileIsRO := (b AND 1)>0;

     if (SafeMode and not FileIsRO ) then error (22,'');

     BlockRead (f, b, 1, sizeread);                          { length }
     if (sizeread<1) then error (13,'');

     s := '';
     for i:=1 to b do s := s+ ' ';                  { set length to s }

     BlockRead (f, s[1], b, sizeread);                  { info string }
     if (sizeread<b) then error (13,'');

     KCAUthor := '';

     while (length(s)>0) and (s[1]<>#255) do begin
        KCAuthor := KCAuthor + s[1];
        delete (s,1,1)
     end;
     delete (s,1,1);
     KCInfo := s;

     { NOW, the data blocks }

     BlockRead (f, l, 2, sizeread);                { len of the block }
     if (sizeread<2) then error (13,'');   { only used to know if <>0 }

     KLsize := 0;

     while (l>0) and (KLsize=0) do begin

        if not fileIsRO then begin
           BlockRead (f, s, 13, sizeread);           { filename string }
           if (sizeread<13) then error (13,'')             { (ignore) }
        end;

        checksum := 0;

        KLsize := ReadKLFile (f, idn, ids, l, TRUE);

        BlockRead (f, b, 1, sizeread);
        if (sizeread<1) then error (13,'');

        if (KLSize>0) and (b<>checksum) then error (20, '');

        BlockRead (f, l, 2, sizeread);              { len of the block }
        if (sizeread<2) then error (13,''); { only used to know if <>0 }

     end;

     { l=0 or KLsize>0 }
     if KLSize>0 then ReadKCFile := KLSize
                 else ReadKCFile := 0
end;

{TRUE if file was read successfully,
 FALSE if there isn't information for that layout/ID on the file }
function ReadDataFile (idn: word; ids: string; name: string): boolean;
var
    f         : file;
    totalsize : word;
    oldfilemode : byte;

begin
     { Try to open }
     assign (f,name);
     oldfilemode := filemode;
     filemode := 0;
     {$I-}
     Reset(f,1);
     {$I+}
     filemode := oldfilemode;
     If IOResult<>0 Then begin
        error (4, name)
     end;

     { File header }
     s := '12345';   { set size to 5 }
     BlockRead (f, s[1], 5, totalsize);
     if (totalsize<5) then error (13,'j');

     Write (STR1_3,name);

     { Contents }
     if       s = 'KLF'+#1+#1
                then KeybCBSize := ReadKLFile (f, idn, ids, 65535, FALSE)
     else if  s = 'KCF'+#0+#1
                then KeybCBSize := ReadKCFile (f, idn, ids)
     else error (13, 'k');
     close (f);

     { Summary }
     if KeybCBSize>6144 then error(15,'');

     ReadDataFile := KeybCBSize > 0;

end;




{obtain the minimum neccessary module from the KL}
function  GetLastModule (f: word): byte;
var
   i,b: byte;
begin
     b  := 0;

     for i:=0 to 10 do
        if ((1 shl i) and f)>0 then b := i+1;

     GetLastModule := b
end;



label NoMoreMod;

Var
     CutHere     : pointer;        { pointer where to cut }

     Installed   : Byte;           { existing KEYB status }
     LastModule  : byte;           { last module to be installed }
     regs        : registers;

     SChar       : char;           { switchar variables }
     switches    : string;

     ConFileName : String;         { name of the configuration file }
     IdStr       : string;         { name of string ID }
     SearchPath  : string;         { path where to look for cfg files }
     CodePage,ID : word;
     ForcedLayout: byte;
     KeyboardSysExitst: boolean;

     b,i         : byte;
     intg        : integer;
     pb          : pByte;
     pp          : ^pointer;

     t           : string;

Begin

   {********** MAIN ENTRY POINT **********}

   DriverNameZ := ConstDriverNameZ;

   {--- 1.- Test self-integrity ---}

   If Ofs(EoDS)>Ofs(FailCall) Then Error(6,'');        {this should never happen}

   {--- 2.- Welcome message ---}

   WriteLn ('FreeDOS KEYB ',VerS,' - (c) Aitor Santamar¡a Merino - GNU GPL 2.0');

   {--- 3.- Initialise some data ---}
   LastModule    := 0;
   CutHere       := NIL;
   int9hChain    := Mem[$F000:$FFFE] >= $FD;    { no chain in AT/PS2 class }
   Verbose       := FALSE;

   ID            := 0;
   ConFileName   := '';
   IDStr         := '';
   CodePage      := 0;
   ResCS         := 0;
   ForcedLayout  := 0;
   NOHI          := FALSE;
   KeyboardSysExitst := FALSE;
   SafeMode      := FALSE;
   
   BIOSStorekey  := EfStoreKey0;
   Beep          := FailCall;
   APMProcedure  := FailCall;
   DoCommands    := FailCall;
   ExtCommands   := FailCall;
   CombineProc   := FailCall;
   ExecCombi     := FailCall;
   FlushBuffer   := FailCall;
   EmptyBuffer   := FailCall;

   CurrentLayout := @BArray(@Data^);       { Pointer to Layout information }
                                           { temporary buffer, later moved }

   { check that DR-DOS KEYB (some versions) is not loaded }
   with regs do begin
        ax := $AD80;
        bx := 0;
        cx := 0
   end;
   intr ($2F,regs);
   if (regs.ah=$FF) AND (regs.bx=0) AND (regs.cx<>0) and (regs.ch<7) then
      error (18,'');

   { continue with parsing and installation }
   Installed     := TestInstallation;      { check if installed }

   If Installed=1 Then  { get data from global data segment }
      Move(Ptr(ResCS , Ofs(OldInt9h))^, OldInt9h,40);

   MCB := pwArray(CurrentLayout)^[2];

   ConFileName   := '';                    { no configuration file yet }

   asm                                     { admissible switches: }
      mov  ax,$3700
      int  $21
      mov  SChar,dl
   end;                                    { DOS system switch AND }
   switches := '/-'+SChar;                 { / AND - }

   {--- 3.- Parse commandline ---}

   if ParamCount>0 then
   for b:=1 to ParamCount do begin
       s := ParamStr (b);
       if pos(s[1],switches)>0 then begin             { we are in a switch }
          if length(s)=2 then                          { one letter switch }
             case upcase(s[2]) of
                                 { IMMEDIATE SWITCHES (no return, program exits) }
                                 'U': If Installed=1 Then Remove Else Error(8,'');
                                 '?': ShowFastHelp;

                                 { OTHER SWITCHES }
                                 'I': if Installed>1 then Installed := 0;
                                 'B': BeepDisabled := TRUE;
                                 'E': int9hChain := FALSE;  { Enhanced: forced }
                                 '9': int9hChain := TRUE;   { Standard: forced }
                                 'S': SafeMode := TRUE;        { Use safe mode }
                                 'V': Verbose := TRUE;
                                 else Error (11,s);
             end
          else case upcase(s[2]) of        { switch with more than 1 letter }
                                 'D': if length(s)>3 then error (11,s)
                                      else begin
                                           DecimalChar := s[3];
                                           LastModule  := maxb (LastModule,3);
                                      end;
                                 'I': begin
                                           s[2] := upcase(s[2]);
                                           s[3] := upcase(s[3]);
                                           if pos('/ID:',s)<>1 then error (11,s);
                                           delete (s,1,4);
                                           val(s,ID,intg);
                                           if intg<>0 then error (11,'/ID:'+s);
                                      end;
                                 'L': begin
                                           s[2] := upcase(s[2]);
                                           if (pos('/L=',s)<>1) then error (11,s);
                                           delete (s,1,3);
                                           val (s,ForcedLayout,intg);
                                           if (intg<>0) or (ForcedLayout=0) then error (11,'/L='+s);
                                      end;
                                 'N': begin
                                           for i:=2 to length(s) do s[i]:=upcase(s[i]);
                                           if s='/NOHI' then NOHI := TRUE
                                                        else error (11,s+'kk');
                                      end;
                                           
                                 else error (11,s)
               end
       end else begin                   { case of a layout }
           if IdStr<>'' then error (11,s);
           while (length(s)>0) and (s[1]<>',') do begin
              IDStr := IDStr + upcase(s[1]);
              delete (s,1,1)
           end;

           if IdStr='' then error(3,s);

           if length(s)>0 then begin
           delete(s,1,1);

           {temporary use of ConFileName}
           while (length(s)>0) and (s[1]<>',') do begin
              ConFileName := ConFileName + s[1];
              delete (s,1,1)
           end;

           if ConFileName='' then CodePage :=0 else begin
              val (ConFileName, CodePage, intg);
              if intg<>0 then error(11,s);
              ConFileName := ''
           end;

           if length(s)>0 then begin
           delete(s,1,1);
           
           while (length(s)>0) and (s[1]<>',') do begin
              ConFileName := ConFileName + s[1];
              delete (s,1,1)
           end;
       end; end; end;
   end;

   if IdStr='' then begin
        If Installed=1 Then ShowInfo
        Else Error(3,'');
   end;


   case Installed of
                    1: CurrentLayout := @BArray(@Data^);
                    2: error(1,'');        { wrong version of FD-KEYB }
                    3: error(2,'');        { other driver was installed }
   end;

   { if we are here, it means we proceed to install }

   if verbose then begin WriteLN (STR1_1); WriteLn end;

   { preconfigure the codepage }

   if codepage=0 then begin     {if not set, check DISPLAY}
      regs.ax := $AD00;
      intr ($2F, regs);

      if (regs.al=$ff) { or (regs.ah=$ff) } then begin
                       { ^^ bypassing a bug in FD-DISPLAY, versions till 0.10 }
         regs.ax := $AD02;
         intr ($2F, regs);
      
        codepage := regs.bx;
        if codepage=$FFFF then codepage:=0;
   end;

   if codepage=0 then begin     { if not set, resort to SYSTEM }
        regs.ax := $6601;
        msdos(regs);
        codepage := regs.bx;
      end
   end;

   { now the data }

   if MCB<>0 then MCBDeAlloc (MCB);
   CurrentLayout := @BArray(@Data^);  {restore currentlayout}

   { preconfigure the filename }

   SearchPath := '.;'+ProgramPath+';'+GetEnv('PATH');
   if length(ConFileName)=0 then ConFileName := 'KEYBOARD.SYS'; {IdStr+'.KL';}

   s := ConFileName;

   If (Pos('\',ConFileName)=0) and
      (Pos(':',ConFileName)=0)
   then ConFileName := FSearch (ConFileName, SearchPath);

   if length(ConFileName)>0 then begin
      KeyboardSysExitst := TRUE;
      if not ReadDataFile (ID, IdStr, ConFileName)
         then ConFileName := ''
   end;

   if length(ConFileName)=0 then begin

        ConFileName := IdStr+'.KL';

        s := ConFileName;

        If (Pos('\',ConFileName)=0) and
           (Pos(':',ConFileName)=0)
        then ConFileName := FSearch (ConFileName, ProgramPath+';'+GetEnv('PATH'));

        if ConFileName = '' then begin
           if KeyboardSysExitst then error (14,'')
                                else error (4, FExpand(s));
        end;

        ConFileName := FExpand (ConFileName);

        if not ReadDataFile (ID, IdStr, ConFileName)
           then error(14,'');
           
   end;


   {********** INSTALL, PREPARE ROUTINES **********}
   { if we are here, we have to install }

   {--- 0.- Some data first ---}

   pWArray(CurrentLayout)^[2] := 0;   { hosted with KEYB, MyMCB=0 }

   pWArray(CurrentLayout)^[5] := ID;   { /ID=0 for the moment }
   for b:=1 to length(idstr) do
       pBArray(CurrentLayout)^[11+b] := ord(idstr[b]);


   {--- 1.- Compute current submapping according to CP ---}

   b := pBArray(CurrentLayout)^[0]-1; { number of particular submappings }
   i := 1;

 if ForcedLayout=0 then
 begin
   if CodePage<>0 then
      while (i<=b) and  (pWArray(CurrentLayout)^[10 + i*4] <> CodePage)
                     and (pWArray(CurrentLayout)^[10 + i*4] <> 0)
      do  inc (i);

   if i>b then begin
      str   (CodePage, s);
      s := s+' (Available: ';
      for i:=1 to b do begin
        str (pWArray(CurrentLayout)^[10 + i*4],t);
        s := s+t;
        if i<b then s:= s+', ';
      end;
      s := s+')';
      error (12,s)
   end;
 end else begin
   if (CodePage=0) or (pWArray(CurrentLayout)^[10 + ForcedLayout*4] = CodePage)
                   or (pWArray(CurrentLayout)^[10 + ForcedLayout*4] = 0)
   then i := ForcedLayout
   else error (21,'');


 
 end;

   write (' [',pWArray(CurrentLayout)^[10+i*4],']');
   
   pBArray(CurrentLayout)^[3] := i;

   {--- 2.- Compute last module ---}
   { Use flags }
   LastModule := maxb (GetLastModule (pwArray(CurrentLayout)^[3] AND $0FFF),LastModule);
   Flags :=  (CurrentLayout^[7] AND $70) OR
            ((CurrentLayout^[6] AND $20) SHR 2);

   If SafeMode then flags := flags OR F_SafeMode;
   If BeepDisabled then flags := flags OR F_BeepDisabled;
   If int9hChain   then flags := flags OR F_XTMachine;

   { UseScroll, usenum, usecaps flags }
   if int9hChain   then LastModule := maxb (LastModule, 7);

   pWArray(CurrentLayout)^[3] := 0;   { pointer to next Layout = 0 }
   pWArray(CurrentLayout)^[4] := 0;

   LastModule := maxb (LastModule,3);
      { NOTE: this is so because of the default table, always tied up }

   if (Flags AND F_Japanese)<>0 then LastModule := maxb (LastModule,10);

   WriteLn ('  (', LastModule,')');

   if verbose then begin
      WriteLn (STR1_7,KCAuthor);
      WriteLn (STR1_8,KCInfo)
   end;

   if (Flags AND F_Japanese)<>0 then writeln (STR1_9);

   {--- 3.- Compute the core functions ---}

   GetIntVec($2F,OldInt2Fh);                     { old 15h and 2Fh vectors }
   GetIntVec($15,OldInt15h);

   PIsActive  := @VIsActive;

   {--- 4.- Compute last module to be discarded ---}

   if LastModule>0 then begin

      {== MODULE 1 ==}
      CombineProc  := DoCombi;
      ExecCombi    := StartCombi;
      Beep         := Speaker;
      CutHere      := @BasicCommand;
      if LastModule<2 then goto NoMoreMod;

      {== MODULE 3 ==}
      DoCommands   := BasicCommand;
      CutHere      := @ExtCommand;
      if LastModule<4 then goto NoMoreMod;

      {== MODULE 4 ==}
      ExtCommands  := ExtCommand;
      CutHere      := @Japanese;
      if LastModule<6 then goto NoMoreMod;

      {== MODULE 6 ==}
      CutHere      := @EnableKeyboardProc;
      if LastModule<7 then goto NoMoreMod;

      {== MODULE 7 ==}
      if int9hChain then begin
         GetIntVec($9 ,OldInt9h);
         PIsActive := Ptr (CSeg, ofs(Int9H)+3)
      end;
      CutHere      := @APMProc;
      if LastModule<8 then goto NoMoreMod;

      {== MODULE 8 ==}
      APMProcedure := APMProc;
      CutHere      := @EfStoreKey1;
      if LastModule<10 then goto NoMoreMod;

      {== MODULE 10 ==}
      pp           := @XStrData;                      { set XBuffer }
      pp^          := Ptr(PrefixSeg,128);
      FlushBuffer  := FlushSecondaryBuffer;
      EmptyBuffer  := EmptySecondaryBuffer;
      CutHere      := @Data;

      GetIntVec    ($16,OldInt16h);
      BIOSStoreKey := EfStoreKey2;

      
   end;
NoMoreMod:

   {--- 6.- Move all the data ---}

   MCB := MCBAlloc ( (KeybCBSize + 15 ) shr 4 );
   pWArray(CurrentLayout)^[2] := MCB;
   CurrentLayout := ptr (MCB,0);

   Move (pbyte(@data)^, CurrentLayout^, KeybCBSize );

   if installed<>0 then begin
      Move(CurrentLayout, Ptr(ResCS , Ofs(CurrentLayout))^, 4);
      halt (0)
   end
   else

   {--- 5.- Make driver active ---}

   pbyte(PIsACtive)^:= $FF;

   Move(Ptr(DSeg,0)^,Ptr(CSeg,0)^,Ofs(EoDS));   { GlobalDS }

   {--- 7.- Set our vectors ---}

   if lastModule>=10 then SetIntVec($16,@Int16h);
   if int9hChain then SetIntVec($9, @Int9H);
   SetIntVec  ($15,@Int15h);
   SetIntVec  ($2F,@Int2Fh);

   if verbose then
   if not int9hChain
        then WriteLn (STR1_5)
        else Writeln (STR1_6);

   {--- 8.- Make it resident ---}

   Keep (ptrRec(CutHere).Seg-PrefixSeg + (ptrrec(CutHere).Ofs+15) Div 16, 0);


   {--- 9.- Data procedures, not to be removed by the linker ---}
   GlobalDS;
   XStrData;
   DefaultTable;
END.




