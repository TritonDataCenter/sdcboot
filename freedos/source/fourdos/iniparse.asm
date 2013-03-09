

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


          title     INIPARSE - Parse the 4DOS INI file
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991 - 1994  JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  4/17/91

          These routines read and process the 4DOS INI file.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                       ;  externally
          include   4dlparms.asm        ;4DOS parameters
          include   inistruc.asm        ;INI file data structures and macros
          include   keyvalue.asm        ;keystroke values
          include   model.inc           ;Spontaneous Assembly memory models
          include   fileio.inc          ;Spontaneous Assembly constants
          include   util.inc            ;Spontaneous Assembly util macros
          ;
          ;
          ; Parameters
          ;
NESTMAX	equ	3		;max include nesting level
LINEMAX   equ       512                 ;max length of a line = size of
                                        ;  internal buffer
NVMAX     equ       64                  ;max length of new value
NVBUFLEN  equ       NVMAX + 3           ;length of new value buffer
ERR_LEN	equ       -1                  ;line too long error
ERR_NEST  equ       -2                  ;nested too deep
O_NOINH   equ       80h                 ;no inherit bit for OPENEX_H
          ;
          ;
          ; Data segment
          ;
          assume    cs:nothing, ds:nothing, es:nothing, ss:nothing
          ;
          ; If under OS/2, define data segment and DGROUP; otherwise put
          ; everything in code segment
          ;
          .defcode
          ;
          ;
          ; Local data
          ;
IncLevel  db        ?                   ;include nesting level
PLToken   dw        ?                   ;address of next token in INILine
          ;
          db        4                   ;length
EOLBrk    db        0, CR, LF, EOF      ;end of line character table
          ;
NameBrk   db        " ", TAB, 0, " ;=", TAB, 0  ;name start/end delimiters
Val1Brk   db        " =", TAB, 0, " ;", TAB, 0  ;value 1st token delimiters
ValNBrk   db        " ", TAB, 0, " ;", TAB, 0   ;value next token delimiters
ColorBrk  db        " -", TAB, 0, " -;", TAB, 0 ;color next token delimiters
KeyBrk    equ       ColorBrk                    ;keystroke same as color
          ;
          ;
          ; Tables for parsing the file -- macros to define table items
          ; are in INISTRUC.ASM
          ;
          ;
          ; Define items which are allowed in the INI file
          ;
ItemNum   =         0                   ;start at item 0
          ;
INIDefs   label     byte                ;INI file definitions
          db        ItemCnt             ;save item count
          db        size INIItem        ;number of bytes after name
          ;
          ; All items except key mapping
          ;
          DefItem   "4StartPath", PATH, 0, NOVAL, I_4SPath
          DefItem   "Alias", UINT, ALSDEF, O_AlsRng, I_Alias
          DefItem   "AmPm", CHOIC, 2, O_YNAuto, I_TmFmt
          DefItem   "ANSI", CHOIC, 0, O_YNAuto, I_ANSI
          DefItem   "AppendToDir", CHOIC, 0, O_YesNo, I_ApDir
          DefItem   "AutoExecPath", PATH, 0, NOVAL, I_AEPath
          DefItem   "AutoExecParms", STR, 0, NOVAL, I_AEParm
;          DefItem   "Base", UINT, 0, O_BaseRng, I_Base
          DefItem   "BatchEcho", CHOIC, 1, O_YesNo, I_BEcho
          DefItem   "BeepFreq", UINT, 440, O_BFRng, I_BpFreq
          DefItem   "BeepLength", UINT, 2, O_BLRng, I_BpDur
          DefItem   "BrightBG", CHOIC, 2, O_YesNo, I_BriBG
          DefItem   "CDDWinColors", COLOR, 0, NOVAL, I_CDCol
          DefItem   "CDDWinHeight", UINT, 16, O_RCRng, I_CDHght
          DefItem   "CDDWinLeft", UINT, 3, O_RCRng, I_CDLeft
          DefItem   "CDDWinTop", UINT, 3, O_RCRng, I_CDTop
          DefItem   "CDDWinWidth", UINT, 72, O_RCRng, I_CDWdth
          DefItem   "ChangeTitle", CHOIC, 1,  O_YesNo, I_ChgTtl
          DefItem   "ClearKeyMap", PROC, 0, P_ClrKey, NOVAL
          DefItem   "ColorDir", STR, 0, NOVAL, I_DirCol
          DefItem   "CommandSep", CHAR, '^', NOVAL, I_CmdSep
          DefItem   "CompleteHidden", CHOIC, 0, O_YesNo, I_CHidden
          DefItem   "CopyEA", CHOIC, 1, O_YesNo, I_CopyEA
          DefItem   "CopyPrompt", CHOIC, 0, O_YesNo, I_CPrompt
          DefItem   "CritFail", CHOIC, 0, O_YesNo, I_CFail
          DefItem   "CursorIns", INT, 100, O_CurRng, I_CursI
          DefItem   "CursorOver", INT, 10, O_CurRng, I_CursO
          DefItem   "Debug", UINT, 0, NOVAL, I_Debug
          DefItem   "DecimalChar", CHOIC, 0, O_DTChar, I_DecChr
          DefItem   "DescriptionMax", UINT, 512, O_DesRng, I_DesMax
          DefItem   "DescriptionName", STR, 0, NOVAL, I_DesNam
          DefItem   "Descriptions", CHOIC, 1, O_YesNo, I_Descr
          DefItem   "DirHistory", UINT, DHISTDEF, O_DHRng, I_DHist
          DefItem   "DiskReset", CHOIC, 0, O_YesNo, I_DskRst
          DefItem   "DRSets", CHOIC, 1, O_YesNo, I_DRSets
          DefItem   "DVCleanup",CHOIC, 1, O_YesNo, I_DVCln
          DefItem   "EditMode", CHOIC, 0, O_EMList, I_EMode
          DefItem   "EnvFree", UINT, ENVMFREE, O_FrRng, I_EnvFr
          DefItem   "Environment", UINT, ENVDEF, O_EnvRng, I_Env
;          DefItem   "ErrorColors", COLOR, 0, NOVAL, I_ErrCol
          DefItem   "EscapeChar", CHAR, '', NOVAL, I_EscChr
          DefItem   "EvalMax", UINT, 10, O_EvalRng, I_EMax
          DefItem   "EvalMin", UINT, 0, O_EvalRng, I_EMin
          DefItem   "FileCompletion", STR, 0, NOVAL, I_FC
          DefItem   "FineSwap", CHOIC, 0, O_YesNo, I_FSwap
          DefItem   "FullINT2E", CHOIC, 1, O_YesNo, I_Int2E
          DefItem   "Function", UINT, ALSDEF, O_AlsRng, I_Func
          DefItem   "FuzzyCD", UINT, 0, O_FuzzyRng, I_FuzCD
          DefItem   "HelpOptions", STR, 0, NOVAL, I_HOpt
          DefItem   "HelpPath", PATH, 0, 4000h, I_HPath
          DefItem   "HistMin", UINT, 0, O_HMRng, I_HMin
          DefItem   "History", UINT, HISTDEF, O_HstRng, I_Hist
          DefItem   "HistCopy", CHOIC, 0, O_YesNo, I_HCopy
          DefItem   "HistDups", CHOIC, 0, O_HDups, I_HDups
          DefItem   "HistMove", CHOIC, 0, O_YesNo, I_HMove
          DefItem   "HistLogName", PATH, 0, <4000h + PATHBLEN>, I_HLogNam
          DefItem   "HistLogOn", CHOIC, 0, O_YesNo, I_HLogOn
          DefItem   "HistWrap", CHOIC, 1, O_YesNo, I_HWrap
          DefItem   "Include", INCL, 0, 0, NOVAL
          DefItem   "Inherit", CHOIC, 1, O_YesNo, I_Inhrit
          DefItem   "INIQuery", CHOIC, 0, O_YesNo, I_Query
          DefItem   "InputColors", COLOR, 0, NOVAL, I_InCol
          DefItem   "InstallPath", PATH, 0, NOVAL, I_IPath
          DefItem   "LineInput", CHOIC, 0, O_YesNo, I_LineIn
          DefItem   "ListboxBarColors", COLOR, 0, NOVAL, I_LBBar
          DefItem   "ListColors", COLOR, 0, NOVAL, I_LstCol
          DefItem   "ListRowStart", UINT, 1, NOVAL, I_LstRow
          DefItem   "ListStatBarColors", COLOR, 0, NOVAL, I_LstSCol
          DefItem   "LocalAliases", CHOIC, 1, O_YesNo, I_LocalA
          DefItem   "LocalDirHistory", CHOIC, 1, O_YesNo, I_LocalD
          DefItem   "LocalFunctions", CHOIC, 1, O_YesNo, I_LocalF
          DefItem   "LocalHistory", CHOIC, 1, O_YesNo, I_LocalH
          DefItem   "LogName", PATH, 0, <4000h + PATHBLEN>, I_LogNam
          DefItem   "LogOn", CHOIC, 0, O_YesNo, I_LogOn
          DefItem   "LogErrors", CHOIC, 0, O_YesNo, I_LogErr
          DefItem   "MaxLoadAddress", UINT, 0, O_MLRng, I_MaxLd
          DefItem   "MessageServer", CHOIC, 1, O_YesNo, I_CCMsg
          DefItem   "Mouse", CHOIC, 0, O_YNAuto, I_Mouse
          DefItem   "NetwareNames", CHOIC, 0, O_YesNo, I_NName
          DefItem   "NextINIFile", STR, 0, NOVAL, I_NxtINI
          DefItem   "NoClobber", CHOIC, 0, O_YesNo, I_NoClob
          DefItem   "OutputBIOS", CHOIC, 0, O_YesNo, I_BIOS
          DefItem   "ParameterChar", CHAR, '&', NOVAL, I_ParChr
          DefItem   "PathExt", CHOIC, 0, O_YesNo, I_PathExt
          DefItem   "PauseOnError", CHOIC, 1, O_YesNo, I_PsErr
          DefItem   "PopupWinColors", COLOR, 0, NOVAL, I_PWCol
          DefItem   "PopupWinHeight", UINT, 12, O_RCRng, I_PWHght
          DefItem   "PopupWinLeft", UINT, 40, O_RCRng, I_PWLeft
          DefItem   "PopupWinTop", UINT, 1, O_RCRng, I_PWTop
          DefItem   "PopupWinWidth", UINT, 36, O_RCRng, I_PWWdth
          DefItem   "Printer", STR, 0, NOVAL, I_Printer
          DefItem   "Reduce", CHOIC, 1, O_YesNo, I_Reduce
          DefItem   "ReserveTPA", CHOIC, 1, O_YesNo, I_TPARes
          DefItem   "RexxPath", PATH, 0, 4000h, I_RxPath
          DefItem   "ScreenColumns", UINT, 0, O_RCRng, I_Columns
          DefItem   "ScreenRows", UINT, 0, O_RCRng, I_Rows
          DefItem   "SelectColors", COLOR, 0, NOVAL, I_SelCol
          DefItem   "SelectStatBarColors", COLOR, 0, NOVAL, I_SelSCol
          DefItem   "StackSize", UINT, STACKDEF, O_StkRng, I_Stack
          DefItem   "StdColors", COLOR, 0, NOVAL, I_StdCol
          DefItem   "Swapping", STR, 0, NOVAL, I_Swap
          DefItem   "SDFlush", CHOIC, 0, O_YesNo, I_SDFlsh
          DefItem   "SwapReopen", CHOIC, 0, O_YesNo, I_Reopen
          DefItem   "TabStops", UINT, 8, O_TabRng, I_Tabs
          DefItem   "ThousandsChar", CHOIC, 0, O_DTChar, I_ThChr
          DefItem   "TreePath", PATH, 0, 4000h, I_TPath
          DefItem   "UMBAlias", CHOIC, 0, O_YNReg, I_UMBAls
          DefItem   "UMBDirHistory", CHOIC, 0, O_YNReg, I_UMBDir
          DefItem   "UMBEnvironment", CHOIC, 0, O_YNReg, I_UMBEnv
          DefItem   "UMBFunction", CHOIC, 0, O_YNReg, I_UMBFunc
          DefItem   "UMBHistory", CHOIC, 0, O_YNReg, I_UMBHst
          DefItem   "UMBLoad", CHOIC, 0, O_YNReg, I_UMBLd
          DefItem   "UniqueSwapName", CHOIC, 0, O_YesNo, I_USwap
          DefItem   "UnixPaths", CHOIC, 0, O_YesNo, I_UnixPaths
          DefItem   "UpperCase", CHOIC, 0, O_YesNo, I_Upper
          DefItem   "Win95LFN", CHOIC, 2, O_YesNo, I_W95LFN
          DefItem   "Win95SFNSearch", CHOIC, 2, O_YesNo, I_W95SFN

          ;
          ; Dummy item to hold name of primary INI file, set up here so
          ; INIStr will move it around as needed, but with no name so user
          ; can't enter it.
          ;
          DefItem   0, STR, 0, NOVAL, I_PrmINI
          ;
          ; Key mapping items
          ;
          DefItem   "AddFile", KEY, <K_F10 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "AliasExpand", KEY, <K_CtlF + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Backspace", KEY, <K_Bksp + (MAP_GEN shl 8)>, NOVAL
          DefItem   "BeginLine", KEY, <K_Home + (MAP_GEN shl 8)>, NOVAL
          DefItem   "CommandEscape", KEY, <K_255 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Del", KEY, <K_Del + (MAP_GEN shl 8)>, NOVAL
          DefItem   "DelHistory", KEY, <K_CtlD + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "DelToBeginning", KEY, <K_CtlHm + (MAP_GEN shl 8)>, NOVAL
          DefItem   "DelToEnd", KEY, <K_CtlEnd + (MAP_GEN shl 8)>, NOVAL
          DefItem   "DelWordLeft", KEY, <K_CtlL + (MAP_GEN shl 8)>, NOVAL
          DefItem   "DelWordRight", KEY, <K_CtlR + (MAP_GEN shl 8)>, NOVAL
          DefItem   "DirWinOpen", KEY, <K_CtlPgU + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Down", KEY, <K_Down + (MAP_GEN shl 8)>, NOVAL
          DefItem   "EndHistory", KEY, <K_CtlE + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "EndLine", KEY, <K_End + (MAP_GEN shl 8)>, NOVAL
          DefItem   "EraseLine", KEY, <K_ESC + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "ExecLine", KEY, <K_Enter + (MAP_GEN shl 8)>, NOVAL
          DefItem   "Help", KEY, <K_F1 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "HelpWord", KEY, <K_CtlF1 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "PopupWinBegin", KEY, <K_CtlPgU + (MAP_HWIN shl 8)>, NOVAL
          DefItem   "PopupWinDel", KEY, <K_CtlD + (MAP_HWIN shl 8)>, NOVAL
          DefItem   "PopupWinEdit", KEY, <K_CtlEnt + (MAP_HWIN shl 8)>, NOVAL
          DefItem   "PopupWinEnd", KEY, <K_CtlPgD + (MAP_HWIN shl 8)>, NOVAL
          DefItem   "PopupWinExec", KEY, <K_Enter + (MAP_HWIN shl 8)>, NOVAL
          DefItem   "HistWinOpen", KEY, <K_PgUp + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Ins", KEY, <K_Ins + (MAP_GEN shl 8)>, NOVAL
          DefItem   "Left", KEY, <K_Left + (MAP_GEN shl 8)>, NOVAL
          DefItem   "LFNToggle", KEY, <K_CtlA + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "LineToEnd", KEY, <K_CtlEnt + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "ListExit", KEY, <K_Esc + (MAP_LIST shl 8)>, NOVAL
          ifdef     ENGLISH
          DefItem   "ListContinue", KEY, <K_C + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListFind", KEY, <K_F + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListFindReverse", KEY, <K_CtlF + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListHex", KEY, <K_X + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListHighBit", KEY, <K_H + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListInfo", KEY, <K_I + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListNext", KEY, <K_N + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListPrevious", KEY, <K_CtlN + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListPrint", KEY, <K_P + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListWrap", KEY, <K_W + (MAP_LIST shl 8)>, NOVAL
          endif
          ifdef     GERMAN
          DefItem   "ListContinue", KEY, <K_C + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListFind", KEY, <K_S + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListHex", KEY, <K_X + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListHighBit", KEY, <K_H + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListInfo", KEY, <K_I + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListNext", KEY, <K_W + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListPrint", KEY, <K_D + (MAP_LIST shl 8)>, NOVAL
          DefItem   "ListWrap", KEY, <K_U + (MAP_LIST shl 8)>, NOVAL
          endif
          DefItem   "NextFile", KEY, <K_F9 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "NextHistory", KEY, <K_Down + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "NormalEditKey", KEY, <(NORM_KEY + MAP_EDIT) shl 8>, NOVAL
          DefItem   "NormalPopupKey", KEY, <(NORM_KEY + MAP_HWIN) shl 8>, NOVAL
          DefItem   "NormalKey", KEY, <(NORM_KEY + MAP_GEN) shl 8>, NOVAL
          DefItem   "NormalListKey", KEY, <(NORM_KEY + MAP_LIST) shl 8>, NOVAL
          DefItem   "NormalPopupKey", KEY, <(NORM_KEY + MAP_HWIN) shl 8>, NOVAL
          DefItem   "PopFile", KEY, <K_F7 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "PrevFile", KEY, <K_F8 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "PrevHistory", KEY, <K_Up + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "RepeatFile", KEY, <K_F12 + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Right", KEY, <K_Right + (MAP_GEN shl 8)>, NOVAL
          DefItem   "SaveHistory", KEY, <K_CtlK + (MAP_EDIT shl 8)>, NOVAL
          DefItem   "Up", KEY, <K_Up + (MAP_GEN shl 8)>, NOVAL
          DefItem   "WordLeft", KEY, <K_CtlLft + (MAP_GEN shl 8)>, NOVAL
          DefItem   "WordRight", KEY, <K_CtlRt + (MAP_GEN shl 8)>, NOVAL
          ;
ItemCnt   =         ItemNum             ;set item count as final item number
          ;
          ;
          ; Validation choice lists and ranges for above items
          ;
          ;
EMList    db        4, 0                ;edit mode choices, no skip
          DefStr    "Overstrike"
          DefStr    "Insert"
          DefStr    "InitOverstrike"
          DefStr    "InitInsert"
          ;
HDups     db        3, 0
          DefStr    "Off"
          DefStr    "First"
          DefStr    "Last"
          ;
YesNo     db        2, 0                ;yes or no choices, no skip
          DefStr    "No"
          DefStr    "Yes"
          ;
YNAuto    db        3, 0                ;yes or no or auto choices, no skip
          DefStr    "Auto"
          DefStr    "Yes"
          DefStr    "No"
          ;
YNReg     db        10, 0               ;yes, no, or region number choices
          DefStr    "No"
          DefStr    "Yes"
          DefStr    "1"
          DefStr    "2"
          DefStr    "3"
          DefStr    "4"
          DefStr    "5"
          DefStr    "6"
          DefStr    "7"
          DefStr    "8"
	;
DTChar	db        3, 0                ; Decimal / thousands choices
          DefStr    "Auto"
          DefStr    "."
          DefStr    ","
          ;
AlsRange  dw        ALSMIN, ALSMAX	;alias range (bytes)
BaseRange dw        0, 1		; Base 0 or Base 1
BFRange   dw        0, 20000            ;beep frequency range (Hz)
BLRange   dw        0, 54		;beep length range (ticks)
CurRange  dw        -1, 100             ;cursor shape range (percent)
DesRange  dw        20, 512             ;description max range
DHRange   dw        DHISTMIN, DHISTMAX	;directory history range (bytes)
EnvRange  dw        ENVMIN, ENVMAX	;environment range (bytes)
EvalRang  dw        0, 10               ;@EVAL decimal precision range
FreeRang  dw        ENVMFREE, ENVMAX	;free space range (bytes)
FuzzyRang dw        0, 3                ;FuzzyCD range
HDRange   dw        0, 2
HMRange   dw        0, 256              ;history minimum save range (chars)
HstRange  dw        HISTMIN, HISTMAX	;command history range (bytes)
MLRange   dw        256, 768            ;maximum load address range (K)
RCRange   dw        0, 2048             ;row / column count range
StkRange  dw        STACKDEF, 16384     ;stack size range (bytes)
TabRange  dw        1, 32		; tabstop range
          ;
          ; Range and choice list offsets for use in INIDefs
          ;
O_EMList  equ       offset @DATASEG:EMList
O_HDups   equ       offset @DATASEG:HDups
O_YesNo   equ       offset @DATASEG:YesNo
O_YNAuto  equ       offset @DATASEG:YNAuto
O_YNReg   equ       offset @DATASEG:YNReg
O_DTChar  equ       offset @DATASEG:DTChar
          ;
O_AlsRng  equ       offset @DATASEG:AlsRange
O_BaseRng equ       offset @DATASEG:BaseRange
O_BFRng   equ       offset @DATASEG:BFRange
O_BLRng   equ       offset @DATASEG:BLRange
O_CurRng  equ       offset @DATASEG:CurRange
O_DesRng  equ       offset @DATASEG:DesRange
O_DHRng   equ       offset @DATASEG:DHRange
O_EnvRng  equ       offset @DATASEG:EnvRange
O_EvalRng equ       offset @DATASEG:EvalRang
O_FuzzyRng equ      offset @DATASEG:FuzzyRang
O_FrRng   equ       offset @DATASEG:FreeRang
O_HDRange equ       offset @DATASEG:HDRange
O_HMRng   equ       offset @DATASEG:HMRange
O_HstRng  equ       offset @DATASEG:HstRange
O_MLRng   equ       offset @DATASEG:MLRange
O_RCRng   equ       offset @DATASEG:RCRange
O_StkRng  equ       offset @DATASEG:StkRange
O_TabRng  equ       offset @DATASEG:TabRange
          ;
          ;
          ; Internal strings and choice lists
          ;
          ;         Section names (MUST be in same order as section bit
          ;         flags in INISTRUC.ASM)
          ;
SecNames  db        3, 0                ;section names, no skip
          DefStr    "4DOS"
          DefStr    "Primary"
          DefStr    "Secondary"
          ;
          ;         Color parsing
          ;
BriBli    db        2, 1                ;bright / blink, skip 1 byte
          DefStr    "Bright"
          db        08h                 ;value for bright
          DefStr    "Blink"
          db        80h                 ;value for blink
          ;
ColList   db        8, 0                ;colors, no skip
          DefStr    "Black"
          DefStr    "Blue"
          DefStr    "Green"
          DefStr    "Cyan"
          DefStr    "Red"
          DefStr    "Magenta"
          DefStr    "Yellow"
          DefStr    "White"
          ;
On        db        "On", 0             ;for color parsing
Border    db        "Border", 0         ;for color parsing
Bright    db        "Bright", 0         ;for color parsing
          ;
          ;
          ; Parsing errors
          ;
E_SECNAM  db        "Invalid section name", 0
E_BADNAM  db        "Invalid item name", 0
E_AMBNAM  db        "Ambiguous item name", 0
E_BADNUM  db        "Invalid numeric or character value", 0
E_BADCHC  db        "Invalid choice value", 0
E_BADKEY  db        "Invalid key substitution", 0
E_KEYFUL  db        "Keystroke substitution table full", 0
E_BADCOL  db        "Invalid color", 0
E_BADPTH  db        "Invalid path or file name", 0
E_STROVR  db        "String area overflow", 0
E_LENGTH	db        "Line too long", 0
E_NEST	db        "Include files nested too deep", 0
E_INCL	db        "Include file not found", 0
E_PREFIX  db        "Error on line "
ErrLNum   db        "     ", 0
E_OF      db        " of ", 0
E_SUFFIX  db        ":  ", CR, LF, "  ", 0
FatalMsg	db        "Fatal error, some directives may not have been processed", CR, LF, 0
PauseMsg  db        "Press a key to continue ...", BELL, 0
QueryMsg  db        "  (Y/N/Q/R/E) ? ", 0
NVMsg     db        "Enter new value:  ", 0
QCList    db        "YNQRE"
QCLLen    equ       $ - QCList
          ;
          ;
          ; Externals
          ;
          ;         Spontaneous Assembly (tm) externals
          ;
          .extrnx    E_CODE:word        ;DOS error code
          ;
          ;
          ; If we are in OS/2 end data segment, and start code segment.
          ; If not, we just continue with code segment.
          ;
          assume    cs:@curseg,ds:@DATASEG  ;set up CS assume
          ;
          ;
          ; Externals
          ;
          extrn     FNCheck:near
          ;
          ;         Externals for routines in ASM_TEXT segment
          ;
          extrn     NextTokP:dword, TokListP:dword, KeyParsP:dword
          ;
          ;         Spontaneous Assembly (tm) externals
          ;
          .extrn    OPENEX_H:auto, CLOSE_H:auto, READ_H:auto
          .extrn    PUT_NEWLINE:auto, PUT_STR:auto
          .extrn    PUT_CHR:auto, GET_CHR:auto
          .extrn    GET_STR:auto
          .extrn    DEC_TO_BYTE:auto, DEC_TO_WORD:auto, DEC_TO_WORDS:auto
          .extrn    WORD_TO_DEC:auto
          .extrn    IS_ALPHA:auto, IS_DIGIT:auto, TO_UPPER:auto
          .extrn    MEM_CHRN:auto, MEM_CPY:auto
          .extrn    MEM_CSPN:auto, MEM_SET:auto, MEM_CMPI:auto
          .extrn    MEM_MOV:auto
          .extrn    STR_CSPN:auto, STR_LEN:auto, STR_NCMPI:auto
          .extrn    STR_PBRKN:auto
          ;
          ;
          ;
          ; INICLEAR - Clear the INI file data structure
          ;
          ; On entry:
          ;    Arguments on stack, pascal calling convention:
          ;         INIData *Init       address of INIData structure for
          ;                             results
          ;         
          ; On exit:
          ;         Init filled in with defaults
          ;         AX, BX, CX, DX, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     INIClear,argframe   ;define entry point
          ;
          argW      CInit               ;pointer to INIData structure
          ;
          pushm     si,di               ;save registers
          ;
          cld                           ;go forward
          mov       di,CInit            ;point to data structure
          mov       si,offset @DATASEG:INIDefs  ;point to definitions table
          mov       cl,[si]             ;get count of items to clear
          xor       ch,ch               ;clear high byte
          add       si,2                ;point to first item
          ;
ICLoop:   lodsb                         ;get name length, skip to text
          xor       ah,ah               ;clear high byte
          add       si,ax               ;skip name text
          mov       bl,[si].II_PType    ;get parse type
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;shift for word offset
          mov       dx,cs:ClrTabl[bx]   ;get address of clear routine
          mov       ax,[si].II_Def      ;get default value or pointer
          mov       bx,[si].II_Data     ;get data offset
          jmp       dx                  ;go to clear routine
          ;
ClrTabl   label     word                ;jump table for clearing all data
          dw        ClrByte             ;0=BYTE, byte numeric item
          dw        ClrByte             ;1=CHAR, character item
          dw        ClrInt              ;2=INT, signed integer item
          dw        ClrInt              ;3=UINT, unsigned integer item
          dw        ClrByte             ;4=CHOICE, byte choice item
          dw        ClrInt              ;5=COLOR, color item (integer)
          dw        ClrNext             ;6=KEY, integer keystroke item, no
                                        ;  clear
          dw        ClrStr              ;7=STRING, string item
          dw        ClrStr              ;8=PATH, path item
          dw        ClrNext             ;9=PROC, procedure item, no clear
          dw        ClrNext             ;10=INCL, include item, no clear
          ;
          ; Clear a byte item
          ;
ClrByte:  mov       [di][bx],al         ;store byte
          jmp       short ClrNext       ;and move on to next item
          ;
          ; Clear a string item (drops through to integer clear)
          ;
ClrStr:   mov       ax,EMPTYSTR         ;get empty string value
          ;
          ; Clear an integer item (signed or unsigned)
          ;
ClrInt:   mov       [di][bx],ax         ;store integer
          ;
ClrNext:  add       si,(size INIItem)   ;move to next item
          loop      ICLoop              ;and loop until done
          ;
          popm      di,si               ;restore registers
          exit                          ;all done
          ;
          ;
          ;
          ; INIPARSE - Main line parser
          ;
          ; On entry:
          ;    Arguments on stack, pascal calling convention:
          ;         char *fname         address of ASCIIZ filename string
          ;         INIData *Init       address of INIData structure for
          ;                             results, must be initialized with
          ;                             string data and key data locations
          ;         int Flags           high byte:  parsing control flags
          ;                             low byte:  bit flags for sections to
          ;                               be ignored
          ;         char *NewSecFlags   bit flags for sections found
          ;         
          ; On exit:
          ;         Carry flag clear if no error, set if error
          ;         AX = 0 if no error
          ;              DOS error code if I/O error occurred
          ;              -1 if INI file line too long
          ;         BX = Bit flags for sections found if no error
          ;              Line number in error if error occurred
          ;         Init filled in
          ;         CX, DX, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     INIParse            ;define entry point
          ;
          argW      IFlags              ;flags
          argW      Init                ;pointer to INIData structure
          argW      FName               ;pointer to file name
          ;
          varend                        ;end of variables
          ;
          ; Call the internal file processor to read and parse the file;
          ; may generate recursion if include files are used
          ;
          pushm     si,di               ;save registers
          ;
          cld                           ;go forward
          mov       IncLevel,0          ;clear include level
          mov       si,FName            ;point to file name
          mov       di,Init             ;point to data structure
          mov       ax,IFlags           ;get flags
          call      INIFProc            ;process the file
          ;
          popm      di,si               ;restore registers
          exit                          ;all done
          ;
          ;
          ; INIFProc - Process an INI file
          ;
          ; On entry:
          ;         AL = section ignore flags
          ;         DS:SI = address of the INI file name
          ;         DS:DI = address of the INIData structure
          ;         
          ; On exit:
          ;         INIData structure filled in
          ;         AX = 0 if no error
          ;              DOS error code if I/O error occurred
          ;              -1 if internal error
          ;         BX = Bit flags for sections found if no error
          ;              Line number in error if error occurred
          ;         BX, CX, DX, SI, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     INIFProc,,,local    ;define entry point
          ;
	varW	LineNum		;current line number in current file
          varW      FNAddr              ;file name address for error messages
          varW      INIHdl              ;INI file handle
          varW      SIFlags             ;section ignore flags
          varW      SecFlags            ;current section flags
          varB      SecFound            ;sections found flags
          varB      IniFlags            ;general INI flags
          var       NVBuf,NVBUFLEN      ;new value buffer
          var       InBuf,LINEMAX       ;input buffer for INI file
          varend
          ;
          ; In the code below BX holds the INI file handle, SI holds
          ; the address of the start of the current line, and DI holds
          ; the address of the INI file data structure
          ;
          ; Open the file and initialize for parsing
          ;
	mov       ah,1                ;set flag to ignore non-4DOS sections
          mov       SIFlags,ax          ;save flags
          xor       ax,ax               ;get zero
          mov       bptr IniFlags,al    ;clear general flags
          mov       wptr SecFlags,ax    ;clear current section flags
          mov       bptr SecFound,al    ;clear sections found flags
          mov       LineNum,ax          ;start at line 0
          mov       FNAddr,si           ;save file name address
	;
          mov       al,O_RDONLY+O_DENYWR+O_NOINH ;read, no write, no inherit
          call      OPENEX_H            ;let SA open the file for us
           jc       SAError             ;if any problem get out
          mov       si,BufEnd           ;go past end of buffer to force read
          mov       INIHdl,bx           ;save handle
          ;
          ; Read a line
          ;
NextLine: mov       bx,INIHdl           ;get back handle
	lea	dx,InBuf		;point to buffer
          call      GetLine             ;get next line
           jb       SAError             ;if real error go get it
           ja       IPDone              ;if end of file get out
          or        ax,ax               ;check internal error code
           jz       LineGo		;continue if OK
	mov	dx,offset @DATASEG:E_LENGTH  ;might be line length error
	cmp	ax,ERR_LEN	;line too long?
	 je	Fatal		;if so display error
	jmp	ErrExit		;otherwise get out
	;
LineGo:	inc       wptr LineNum        ;bump line count
          mov       PLToken,si          ;set tokenizer start address
          bclr      <bptr IniFlags>,IP_NVAL  ;clear new value flag
          pushm     si,di               ;save line and data pointers
          mov       di,offset @DATASEG:NameBrk   ;point to name break tables
          call      NewToken            ;get name and length
          popm      di,si               ;restore pointers
           ja       IPQLine             ;if not empty go on
          ;
          ; Skip this line and go on to the next one
          ;
IPSkipLn: call      STR_LEN             ;find end of line
          add       si,cx               ;point to end + 1
          jmp       NextLine            ;and loop for more
          ;
          ; Query for the line if query requested
          ;
IPQLine:  cmp       [di].I_Query,0      ;query on each line?
           je       IPLProc             ;if not go process the line
          pushm     si,di               ;save line and data pointers
          loadseg   es,ds               ;set segment for scasb below
          cld                           ;go forward
          call      PUT_STR             ;display the line
          mov       si,offset @DATASEG:QueryMsg  ;point to query message
          call      PUT_STR             ;display it
          ;
IPNewChr: call      GET_CHR             ;read a character
           jc       IPBadChr            ;if no good complain
          mov       ah,al               ;save original character
          cmp       al,'a'              ;is it lower case?
           jb       IPChrCmp            ;if not go on
          cmp       al,'z'              ;check high end
           ja       IPBadChr            ;if too big it's no good
          sub       al,32               ;make it upper case
          ;
IPChrCmp: mov       di,offset QCList    ;point to allowed characters
          mov       bx,di               ;copy address
          mov       cx,QCLLen           ;get length
          repne     scasb               ;see if we have the character
           jne      IPBadChr            ;if not, complain
          mov       al,ah               ;copy back original character
          call      PUT_CHR             ;echo it
          call      PUT_NEWLINE         ;do cr/lf
          dec       di                  ;back up to character found
          sub       di,bx               ;find offset of character
          mov       bx,di               ;put it back in BX
          shl       bx,1                ;make it word offset
          popm      di,si               ;restore line and data pointers
          jmp       wptr IPQTab[bx]     ;branch based on character
          ;
IPBadChr: mov       al,BELL             ;get beep character
          call      PUT_CHR             ;complain
          jmp       IPNewChr            ;and loop
          ;
IPQTab    label     word                ;jump table
          dw        IPLProc             ;Y = yes, go ahead and process it
          dw        IPSkipLn            ;N = no, go to next line
          dw        IPDone              ;Q = quit
          dw        IPQRest             ;R = rest
          dw        IPQEdit             ;E = edit
          ;
IPQRest:  mov       [di].I_Query,0      ;kill future queries
          jmp       IPLProc             ;go process line
          ;
IPQEdit:  push      si                  ;save line address
          mov       si,offset @DATASEG:NVMsg  ;point to new value message
          call      PUT_STR             ;display it
          lea       si,NVBuf            ;get buffer address
          mov       cl,NVMAX            ;get buffer length
          call      GET_STR             ;read the string
          bset      <bptr IniFlags>,IP_NVAL  ;force use of new value
          mov       bx,si               ;copy buffer address for IniLine
          pop       si                  ;restore original line address
          ;
          ; Process the line
          ;
IPLProc:  mov       al,IniFlags         ;get general flags 
          mov       dx,SIFlags          ;get section ignore flags
          and       dx,SecFlags         ;isolate current section bit
          call      INILine             ;parse the line
           jb       LineEx              ;if error complain
           je       NextLine            ;loop if not section name or error
          mov       SecFlags,dx         ;save it
          or        SecFound,dl         ;save cumulative list
          jmp       NextLine            ;and loop
          ;
          ; Line exception (error or include)
          ;
LineEx:   inc	ax		;was it set to FFFF?
	 jnz	LineErr		;if not, we got an error
	inc	IncLevel		;bump nesting level
	cmp	IncLevel,NESTMAX	;check it
	 jb	InclGo		;if OK, go on
	dec	IncLevel		;back to previous level
	mov	dx,offset @DATASEG:E_NEST  ;get nesting error
       	jmp	LineErr		;go display it
	;
InclGo:	pushm	si,di		;save current pointers
	mov	si,bx		;copy include file name address
	mov	ax,SIFlags	;get current section flags
	call	INIFProc		;call ourselves to process the file
	popm	di,si		;restore current pointers
	dec	IncLevel		;back to previous level
	or	ax,ax		;check for error
	 jbe	NextLine		;continue if OK or internal error
	mov       dx,offset @DATASEG:E_INCL  ;include file open error
	jmp	LineErr		;handle error
	;
	; Fatal error
	;
Fatal:	bset	<bptr IniFlags>,IP_FATAL  ;set fatal error flag
	;
	; Line error
	;
LineErr:	push      si                  ;save parsing address
          mov       si,offset @DATASEG:ErrLNum   ;get address for line number
          mov       cx,5                ;length of line number area
          mov       al,' '              ;character to fill
          call      MEM_SET             ;clear the line number area
          mov       ax,LineNum          ;get line number
          call      WORD_TO_DEC         ;convert word to ASCII
          mov       si,offset @DATASEG:E_PREFIX  ;point to error pfx message
          call      PUT_STR             ;display it
          mov       si,offset @DATASEG:E_OF  ;point to " of "
          call      PUT_STR             ;display it
          mov       si,FNAddr           ;point to file name
          call      PUT_STR             ;display it
          mov       si,offset @DATASEG:E_SUFFIX  ;point to error sfx message
          call      PUT_STR             ;display it
          mov       si,dx               ;get specific error message address
          call      PUT_STR             ;display that
          call      PUT_NEWLINE         ;do cr/lf
	test	bptr IniFlags,IP_FATAL  ;fatal error?
	 jz	ChkPause		;if not go on to pause
          mov       si,offset @DATASEG:FatalMsg  ;point to fatal message
          call      PUT_STR             ;display it
	;
ChkPause:	cmp       [di].I_PsErr,0      ;pause on error set?
           je       ErrDone             ;if not go on
          mov       si,offset @DATASEG:PauseMsg  ;point to pause message
          call      PUT_STR             ;display it
          call      GET_CHR             ;read a character and ignore it
          call      PUT_NEWLINE         ;do cr/lf
          ;
ErrDone:  pop       si                  ;restore parsing address
	test	bptr IniFlags,IP_FATAL  ;fatal error?
	 jz	NextLine            ;if not go on to next line
	mov	ax,-1		;get error code
	jmp	IPExit		;and exit
          ;
          ;
SAError:  mov       ax,E_CODE           ;SA error, get code
          ;
ErrExit:  mov       bx,LineNum          ;get error line
          jmp       short IPExit        ;and get out
          ;
IPDone:   call      CLOSE_H             ;close the input file
          ;
OKExit:   xor       ax,ax               ;OK, clear error number
          mov       bl,SecFound         ;get sections found
          mov       bptr [di].I_SecFlg,bl  ;save sections found flags
          xor       bh,bh               ;clear high byte
          ;
IPExit:   exit                          ;that's all folks
          ;
          ;
          ; GetLine - Read next line
          ;
          ; On entry:
	;	DX = line buffer base address
          ;         BX = file handle
          ;         SI = end of old line + 1
          ;         
          ; On exit:
          ;         Flags set to:
          ;           JE      normal exit (may be an error in AX)
          ;           JA      file was at EOF
          ;           JB      I/O error occurred
          ;         AX = error code, 0 if none
          ;         If no error, SI = address of new line within buffer,
          ;           line null-terminated
          ;         CX, DX, ES destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; NOTE:  This routine must remain set to noframe as it uses
          ; local variables defined by INIFProc.
          ;
          entry     GetLine,,,local	;define entry point
	;
	varW	BufAdr		;line buffer address
	varW	BufEnd		;line buffer end
	varend
          ;
	mov	BufAdr,dx		;save buffer address
	add	dx,LINEMAX	;point to end + 1
	mov	BufEnd,dx		;save buffer end address
          ;
          push      di                  ;save DI
          loadseg   es,ds               ;set ES to local data
          ;
          mov       cx,BufEnd           ;get end of buffer address
          sub       cx,si               ;get remaining characters
           ja       ScanEOL             ;if some left go look for EOL
          ;
ReadAll:  mov       si,BufAdr           ;reset to start of buffer
          mov       cx,LINEMAX          ;get count to read
          mov       al,EOF              ;get EOF
          call      MEM_SET             ;fill buffer with EOFs
          call      READ_H              ;read the whole buffer if we can
           je       GetProc             ;if OK go on
          jmp       GetExit             ;if error or EOF return
          ;
GetProc:  test      bptr IniFlags,IP_SKLF  ;are we skipping an LF?
           jz       ScanEOL             ;if not go on
          bclr      <bptr IniFlags>,IP_SKLF  ;clear skip flag
          cmp       bptr [si],LF        ;is it an LF?
           jne      ScanEOL             ;if not go on
          inc       si                  ;if so skip it
          ;
ScanEOL:  xor       al,al               ;get null
          call      MEM_CHRN            ;skip any nulls
           jc       ReadAll             ;if rest of buffer is null read more
          cmp       bptr [si],EOF       ;start of line is EOF?
          je        GetEOF              ;if so return EOF
          ;
          mov       dx,cx               ;save starting count
          mov       di,offset @DATASEG:EOLBrk    ;get address of break table
          call      MEM_CSPN            ;search for EOL
          cmp       cx,dx               ;did we run out of room?
           jne      GotEOL              ;if not we found EOL, go return line
          ;
          ; End of line not found -- if we haven't scanned a whole buffer,
          ; move the partial line down and read more, then scan that
          ;
          mov       di,BufAdr           ;get beginning as move destination
          cmp       si,di               ;did we start scan at beginning?
           je       LineLong            ;if so line is too long
          mov       cx,dx               ;get length to move
          call      MEM_CPY             ;move partial line to start of buffer
          push      di                  ;save buffer start
          mov       si,di               ;copy start
          add       si,dx               ;point to new read location
          mov       cx,LINEMAX          ;get buffer length
          push      cx                  ;save length
          sub       cx,dx               ;adjust for what's already there
          mov       al,EOF              ;get EOF
          call      MEM_SET             ;fill buffer with EOFs
          mov       bptr [si],0         ;set first byte of new read to null
                                        ;  so that if read fails at an EOF
                                        ;  the line will be terminated
          call      READ_H              ;read into the buffer
          pop       cx                  ;restore length
          pop       si                  ;restore buffer start
           jne      GetExit             ;if error or EOF return (if EOF line
                                        ;  is not terminated by CR/LF, but
                                        ;  since buffer was filled with EOFs
                                        ;  it will be terminated anyway)
          mov       dx,cx               ;save starting count
          mov       di,offset @DATASEG:EOLBrk    ;get address of break table
          call      MEM_CSPN            ;search for EOL
          cmp       cx,dx               ;did we run out of room?
           je       GetExit             ;if so no EOL, exit with flags=zero,
                                        ;  just as if we got EOF on read
          ;
GotEOL:   mov       di,si               ;copy start address
          add       di,cx               ;point to EOL
          xor       al,al               ;get null
          xchg      [di],al             ;null-terminate line, get last char
          cmp       al,CR               ;CR at end?
           jne      GetOK               ;if not we're done
          inc       di                  ;move to next byte
          cmp       di,BufEnd           ;was CR at exact end of buffer?
           jb       GetLF               ;if not go check LF
          bset      <bptr IniFlags>,IP_SKLF  ;set skip LF flag
          jmp       short GetOK         ;and go on
          ;
GetLF:    cmp       bptr [di],LF        ;followed by LF?
           jne      GetOK               ;if not go on
          xor       al,al               ;get zero
          stosb                         ;null out LF
          ;
GetOK:    xor       ax,ax               ;show no error
          jmp       short GetExit       ;and exit
          ;
GetEOF:   mov       al,1                ;get value > 0
          or        al,al               ;set flags
          jmp       short GetExit       ;exit with EOF
          ;
LineLong: mov       ax,ERR_LEN	;line too long, get error
          xor       dl,dl               ;set flags for normal exit
          ;
GetExit:  pop       di                  ;restore DI
          exit                          ;and return
          ;
          ;
          ; INILine - Parse a line and save results in INIData structure
          ;
          ; On entry:
          ;         AL = General INI file processing flags
          ;         BX = address of new value buffer
          ;         DX = 0 to allow body lines, non-zero to allow [section]
          ;              lines only
          ;         DS:SI = pointer to start of null-terminated line
          ;         DS:DI = address of the INIData structure
          ;         
          ; On exit:
          ;         Flags set to:
          ;           JA      section name
          ;           JE      normal line
          ;           JB      bad line, or include
          ;         AX = 0FFFFh if include found (JB)
	;	BX = address of include file name if include found (JB)
          ;         DX = error message address if error (JB)
          ;              section value if JA
          ;         SI = address of end of line + 1
          ;         AX, BX, CX, DX, ES destroyed except as noted above
          ;         Other registers and interrupt state unchanged
          ;
          entry     INILine             ;define entry point
          ;
          varW      DataPtr             ;saved INI file data pointer
          varW      IParsPtr            ;saved item parse info pointer
          varW      NVAddr              ;address of new value buffer
          varW      SecOnly             ;non-zero for [section] only
          varB      ILFlags             ;general INI file flags
          var       PathBuf,PATHBLEN    ;buffer for any path
          varend                        ;end of local variables
          ;
          .push     di,es               ;save DI
          loadseg   es,ds               ;set ES to local data
          mov       DataPtr,di          ;save INI file data pointer
          ;
          ; Find the first token on the line
          ;
          mov       ILFlags,al          ;save general flags
          mov       SecOnly,dx          ;save section name only flag
          mov       NVAddr,bx           ;save new value buffer address
          mov       PLToken,si          ;set tokenizer start address
          call      STR_LEN             ;find end of line
          add       cx,si               ;point to end + 1
          push      cx                  ;save to pop off at end
          mov       di,offset @DATASEG:NameBrk   ;point to name break tables
          call      NewToken            ;get name and length
           jbe      ParsOK		;if empty or all comment, skip it
          cmp       bptr [si],'['       ;appear to be a section name?
           jne      NotSect             ;if not go on
          ;
          ; This is a section name -- see if it's legal
          ;
          mov       bx,cx               ;copy token length
          cmp       bptr -1[bx][si],']' ;token terminated properly?
           jne      SectErr             ;no, complain
          inc       si                  ;skip to section name
          sub       cx,2                ;adjust length for brackets
           jle       SectErr            ;if nothing left complain
          mov       di,offset @DATASEG:SecNames  ;point to section names list
          call      FindTok             ;valid name?
           je       SectOK              ;if so go on
          ;
          ; Somebody else's section name -- return forced-ignore flag
          ;
          mov       dx,100h             ;get return value to force ignore
          jmp       SectRet             ;and get out
          ;
          ; Our section name -- get section flag
          ;
SectOK:   mov       cl,al               ;get section value
          mov       dl,80h              ;get a 1 bit
          ror       dl,cl               ;make section number into bit flag
          xor       dh,dh               ;clear high byte
          ;
SectRet:  mov       ax,1                ;get a value > 0
          or        ax,ax               ;set flags for JA return
          jmp       ParsPop             ;and exit
          ;
SectErr:  mov       dx,offset @DATASEG:E_SECNAM  ;section termination error
          jmp       ParsErr		;go holler
          ;
          ; Not a section name -- see if all we want are section names
          ;
NotSect:  cmp       wptr SecOnly,0      ;section names only?
           jne      ParsOk		;if so ignore the line
          ;
          ; The line should be processed -- see if we can recognize it
          ;
	mov       di,offset @DATASEG:INIDefs   ;point to definition table
          call      FindTok             ;find the token if we can
           jb       BadName             ;if not found complain
           ja       NamAmbig            ;if non-unique complain
          mov       IParsPtr,di         ;save address of item parse info
          test      bptr ILFlags,IP_NVAL  ;do we have a new value?
           jz       ScanVal             ;if not, scan the line
          mov       si,NVAddr           ;get new value buffer address
          mov       PLToken,si          ;adjust address for tokenizer
          ;
ScanVal:  push      di                  ;save parse info pointer
          mov       di,offset @DATASEG:Val1Brk   ;point to value break tables
          call      NewToken            ;skip to value
          pop       di                  ;restore parse info pointer
          mov       bl,[di].II_PType    ;get parse type
           ja       GotValue            ;go on if something after name
          cmp       bl,IP_PROC          ;no value, is it a procedure?
           jne      ParsOK		;if not just skip the line
          ;
DoProc:   jmp       wptr [di].II_Valid  ;it's a procedure, jump to code
          ;
GotValue: cmp       bl,IP_PROC          ;is it a procedure?
           je       DoProc              ;if so go back and handle that
          cmp       bl,IP_KEY           ;is it a keystroke or string item?
           jae      DoParse             ;if so don't adjust data address
          mov       ax,[di].II_Data     ;get offset
          add       DataPtr,ax          ;add to data pointer
          ;
DoParse:  xor       bh,bh               ;clear high byte of item type
          shl       bx,1                ;shift for byte offset
          mov       ax,cs:ParsTabl[bx]  ;get address of handler for this type
          mov       di,[di].II_Valid    ;get possible range
          jmp       ax                  ;and go to handler
          ;
BadName:  mov       dx,offset @DATASEG:E_BADNAM  ;get error number
          jmp       ParsErr		;go holler
          ;
NamAmbig: mov       dx,offset @DATASEG:E_AMBNAM  ;get error number
	jmp       ParsErr             ;go holler
          ;
          ;
ParsTabl  label     word                ;jump table for different parse types
          dw        ParsByte            ;0=BYTE, byte numeric item
          dw        ParsChar            ;1=CHAR, character item
          dw        ParsInt             ;2=INT, signed integer item
          dw        ParsUInt            ;3=UINT, unsigned integer item
          dw        ParsChc             ;4=CHOICE, byte choice item
          dw        ParsCol             ;5=COLOR, color item
          dw        ParsKey             ;6=KEY, integer keystroke item
          dw        ParsStr             ;7=STRING, string item
          dw        ParsPath            ;8=PATH, path item
          dw	ParsOK		;9=PROC not used, handled via
                                        ;  code, not table
	dw	ParsIncl		;10=INCLUDE, include another file
          ;
          ;
          ; Code for procedures (started directly from any IP_PROC item)
          ;
          ;         ClearKeyMap:  Clear the keystroke map
          ;
P_ClrKey: mov       di,DataPtr          ;point to data structure
          mov       [di].I_KeyUse,0     ;clear key map item count
          jmp       ParsOK		;and get out
          ;
          ;
          ; Parse a byte numeric item
          ;
ParsByte: call      DEC_TO_BYTE         ;get byte value
           ja       NumError            ;if error complain
           jb       ParsOK		;if no number leave it alone
          jmp       short TestByte      ;go test value
          ;
          ;
          ; Parse a character item -- if more than one character, see if
          ; the user entered a keystroke name instead
          ;
ParsChar: cmp       cx,1                ;how many characters are there?
           je       GetChar             ;if only one go get it
          call      dword ptr KeyParsP  ;call the keystroke parser
           jbe      NumError            ;if no good holler
          or        ah,ah               ;is high byte set?
           jnz      NumError            ;if so it's wrong
          jmp       short TestByte      ;otherwise go test it
          ;
GetChar:  lodsb                         ;get first byte of arg
          ;
          ;
TestByte: cmp       di,NOVAL            ;check range
           je       SBJump              ;if no range specified go on
          mov       dx,di               ;copy validation range
          cmp       al,dl               ;check low end
           jb       NumError            ;if no good complain
          cmp       al,dh               ;check high end
           ja       NumError            ;if no good complain
          ;
SBJump:   jmp       StorByte            ;go store it
          ;
          ;
          ; Parse an integer numeric item
          ;
ParsInt:  call      DEC_TO_WORDS        ;get signed word value
           jne      NumError            ;if error complain
          cmp       di,NOVAL            ;check range
           je       SWJump              ;if no range specified go on
          cmp       ax,[di]             ;check low end
           jl       NumError            ;if no good complain
          cmp       ax,2[di]            ;check high end
           jg       NumError            ;if too big complain
          jmp       short SWJump        ;otherwise go store it

ParsUInt: call      DEC_TO_WORD         ;get unsigned word value
           jne      NumError            ;if error complain
          cmp       di,NOVAL            ;check range
           je       SWJump              ;if no range specified go on
          cmp       ax,[di]             ;check low end
           jb       NumError            ;if no good complain
          cmp       ax,2[di]            ;check high end
           ja       NumError            ;if too big complain
          ;
SWJump:   jmp       StorWord            ;word OK, go store it
          ;
NumError: mov       dx,offset @DATASEG:E_BADNUM  ;bad number
          jmp       ParsErr		;go to error handler
          ;
          ;
          ; Parse a choice item (assumes validation pointer is there)
          ;
ParsChc:  call      FindTok             ;see if this token is in the list
                                        ;  the validation pointer points to
           je       SBJump              ;if it's OK go store it
          mov       dx,offset @DATASEG:E_BADCHC  ;no match, holler
	jmp       ParsErr             ;go holler
          ;
          ;
          ; Parse a keystroke item
          ;
ParsKey:  call      dword ptr KeyParsP  ;call the keystroke parser
           jb       KeyError            ;holler if error
           je       ParsOK		;if empty or all comment, skip it
          ;
          ; Store the keystroke -- first check if it overrides a mapping
          ; that's already in the table
          ;
          loadseg   es,ds               ;set scan / destination segment
          mov       di,DataPtr          ;get INI file data structure address
          mov       cx,[di].I_KeyUse    ;get item count
          mov       bx,cx               ;copy item count
          shl       bx,1                ;make it a byte count
          mov       di,[di].I_Keys      ;point to key table
          mov       si,IParsPtr         ;get parse info pointer
          mov       dh,bptr [si].II_Def+1  ;get high byte of default (has
                                           ;  mapping context bits)
          bclr      dh,<(EXTKEY + NORM_KEY)>  ;clear extended & normal bits
          ;
SKScan:    jcxz     SKNew               ;if no more to look at it's new
          repne     scasw               ;look for this character
           jne      SKNew               ;if not there it's new
          mov       dl,-1[bx][di]       ;get byte with context bits (-2 for
                                        ;  offset due to scasw + 1 to get
                                        ;  to high byte = -1 total offset)
          bclr      dl,<(EXTKEY + NORM_KEY)>  ;clear extended & normal bits
          cmp       dl,dh               ;is this mapping in the same context?
           jne      SKScan              ;if not keep scanning
          mov       -2[di],ax           ;same key and context, override old
                                        ;  mapping with new
          mov       ax,[si].II_Def      ;get original key with mapping
          mov       -2[bx][di],ax       ;store in second half of table
          jmp       ParsOK		;and we're all done
          ;
          ; This is a new keystroke.  Open the table up to hold it.
          ;
SKNew:    mov       bx,si               ;copy parse info address
          mov       di,DataPtr          ;get INI file data structure address
          mov       dx,[di].I_KeyUse    ;get item count
          cmp       dx,[di].I_KeyMax    ;check if out of room
           jae       KeysFull           ;holler if out of room
          inc       [di].I_KeyUse       ;bump item count
          mov       si,[di].I_Keys      ;point to key table
          shln      dx,1                ;make item count a byte count
          add       si,dx               ;skip to second half
          mov       di,si               ;copy address of second half
          add       di,2                ;destination is 2 bytes up
          mov       cx,dx               ;get byte count
           jcxz     AddKey              ;if table is empty just go add key
          call      MEM_MOV             ;slide existing data up
          ;
AddKey:   mov       [si],ax             ;store new keystroke at end of first
                                        ;  half of table
          add       di,dx               ;point to new end of second half
          mov       ax,[bx].II_Def      ;get original key with mapping
                                        ;  context bits
          mov       [di],ax             ;store that in second half of table
          jmp       ParsOK		;and we're all done
          ;
KeysFull: mov       dx,offset @DATASEG:E_KEYFUL  ;get error 
          jmp       ParsErr		;and go complain
          ;
          ; Keystroke spec was no good
          ;
KeyError: mov       dx,offset @DATASEG:E_BADKEY  ;get error 
          jmp       ParsErr		;and go complain
          ;
          ;
          ; Parse a color item
          ;
ParsCol:  mov       dx,0FF00h           ;default to no border, black on black
          ;
BBLoop:   mov       PLToken,si          ;reset token pointer
          mov       di,offset @DATASEG:ColorBrk  ;get color break table
          call      NewToken            ;break out token w/ color delims
          mov       di,offset @DATASEG:BriBli    ;point to bright / blink
          call      FindTok             ;see if token is in the list
           jne      GetFG               ;if not go on to foreground
          ;
BBSet:    or        dl,[di]             ;set bright / blink bit
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           ja       BBLoop              ;if more there go on
          jmp       short ColDone       ;if not get out
          ;
GetFG:    call      ColorNam            ;scan for a color name
           jne      ColorErr            ;if not complain
          or        dl,al               ;save foreground
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           jbe      ColDone             ;if no more get out
          mov       di,offset @DATASEG:On        ;point to "on"
          call      STR_NCMPI           ;see if we have it
           jne      ColorErr            ;if not complain
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           jbe      ColDone             ;if no more get out
          mov       di,offset @DATASEG:Bright     ;point to "bright"
          call      STR_NCMPI           ;see if we have it
           jne      GetBG               ;if not go on
          or        dl,80h              ;set bright background bit (same as
                                        ;  blink)
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           jbe      ColDone             ;if no more get out
          ;
GetBG:    call      ColorNam            ;see if we have a valid color name
           jne      ColorErr            ;if not complain
          shln      al,4,cl             ;shift into background position
          or        dl,al               ;save background
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           jbe      ColDone             ;if no more get out
          mov       di,offset @DATASEG:Border    ;point to "border"
          call      STR_NCMPI           ;see if we have it
           jne      ColorErr            ;if not complain
          mov       di,offset @DATASEG:ColorBrk   ;point to delimiter table
          call      NewToken            ;skip to next token
           jbe      ColDone             ;if no more get out
          mov       di,offset @DATASEG:ColList   ;point to color list
          call      FindTok             ;see if fg color is in the list
           jne      ColorErr            ;if not complain
          mov       dh,al               ;save border
          ;
ColDone:  mov       ax,dx               ;copy result
          jmp       short StorWord      ;and go store it
          ;
ColorErr: mov       dx,offset @DATASEG:E_BADCOL  ;bad color, holler
          jmp       short ParsErr       ;go to error handler
          ;
          ;
          ; Parse a string item
          ;
ParsStr:  call      STR_LEN             ;find length without null
          jmp       short StorStr       ;just go store the string
          ;
          ;
          ; Parse a path item
          ;
ParsPath: mov       dx,di               ;save any minimum
          lea       di,PathBuf          ;get address of path buffer
          call      MEM_CPY             ;copy the path
          mov       si,di               ;copy buffer address
          add       si,cx               ;skip to end + 1
          mov       bptr [si],0         ;add null at end
          ;
          test      dh,40h              ;validity check disabled?
           jnz      PathOK              ;if so go on
          mov       si,di               ;copy buffer address again
          push      dx                  ;save minimum
          call      FNCheck             ;normalize and check path
          pop       dx                  ;restore minimum
           jnc      PathOK              ;go on if OK
          test      dh,80h              ;are we looking at path part only?
           jz       PathErr             ;if not it's an error
          mov       bx,cx               ;copy length
          cmp       bptr -2[bx][di],'\'  ;was it a directory spec?
           je       PathErr             ;if so it wasn't found, complain
          cmp       ax,18               ;we only care about path -- was it a
                                        ;  "no files" error from find first?
           jne      PathErr             ;if not, it's a real error
          ;
          ;
PathOK:   mov       si,di               ;get back buffer address again
          and       dx,0FFh             ;isolate minimum in DX
           jz       StorStr             ;if no minimum go on
          cmp       cx,dx               ;check minimum length
           jge      StorStr             ;if greater go on
          xchg      cx,dx               ;copy minimum
          push      cx                  ;save it
          sub       cx,dx               ;get amount to fill
          add       di,dx               ;get address to start fill
          xor       al,al               ;get zero
          rep       stosb               ;fill the rest with nulls
          pop       cx                  ;get back real count
          jmp       short StorStr       ;go store string
          ;
PathErr:  mov       dx,offset @DATASEG:E_BADPTH  ;bad path, holler
          jmp       short ParsErr       ;go to error handler
	;
	;
	; Handle an include file
	;
ParsIncl:	mov	bx,si		;copy filename address
	mov	ax,0FFFFh		;get return value
	stc			;set JB return
	jmp	ParsPop		;and exit
          ;
          ;
          ; Store a byte item
          ;
StorByte: mov       di,DataPtr          ;get item address
          mov       [di],al             ;store byte
          jmp       short ParsOK        ;all done, get out
          ;
          ; Store a word item
          ;
StorWord: mov       di,DataPtr          ;get item address
          mov       [di],ax             ;store word
          jmp       short ParsOK        ;all done, get out
          ;
          ; Store a string item
          ;
StorStr:  mov       bx,IParsPtr         ;get parse info address
          mov       bx,[bx].II_Data     ;get item data offset in INI data
                                        ;  structure
          mov       di,DataPtr          ;get INI file data structure address
          call      INIStr              ;store string, return its offset
           jnc      ParsOK              ;all done, get out
          mov       dx,offset @DATASEG:E_STROVR  ;string area overflow
          ;
          ; Error, return error code
          ;
ParsErr:  xor       ax,ax               ;clear flags
          stc                           ;set JB return
          jmp       ParsPop		;and exit
          ;
          ; All OK, get pointer to next line and return (flags cannot be
          ; changed after ParsPop)
          ;
ParsOK:   xor       ax,ax               ;show no error
          ;
ParsPop:  pop       si                  ;get back end address
	.pop      di,es               ;restore DI
          exit                          ;and return
          ;
          ;
          ; NewToken- Get new token
          ;
          ; On entry:
          ;         Registers (except SI) set up for NextTok call
          ;         PLToken = address of end of old token + 1
          ;         
          ; On exit:
          ;         All flags / registers set from NextTok call
          ;
          entry     NewToken,noframe,,local  ;define entry point
          ;
          mov       si,PLToken          ;get old token end + 1
          call      dword ptr NextTokP  ;get next token
          mov       PLToken,ax          ;save for next time
          ;
          exit                          ;and return
          ;
          ;
          ; FindTok - Find a token in a list
          ;
          ; On entry:
          ;         Registers set up for TokList call
          ;         
          ; On exit:
          ;         All flags / registers set from TokList call
          ;
          entry     FindTok,noframe,,local  ;define entry point
          ;
          call      dword ptr TokListP  ;scan token list
          ;
          exit                          ;and return
          ;
          ;
          ; ColorNam - Parse a color name
          ;
          ; On entry:
          ;         DS:SI = Address of color name / number string
          ;         CX = length of token to check
          ;         
          ; On exit:
          ;         AL = color number
          ;         Flags set to:
          ;           JNE     not a valid color
          ;           JE      valid color, number in AL
          ;           JB      token is not in the list
          ;         
          ;         DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     ColorNam,noframe,,local  ;define entry point
          ;
          push      si                  ;save token pointer
          call      DEC_TO_BYTE         ;check for numeric color value?
          pop       si                  ;restore token pointer
           je       CNDone              ;if numeric we're done, result in AL
          mov       di,offset @DATASEG:ColList   ;point to color list
          call      FindTok             ;see if fg color is in the list
          ;
CNDone:   exit                          ;return
          ;
          ;
          ; INIStr - Store a string in the INI file string area, removing
          ;          any previous string for the same data item
          ;
          ; On entry:
          ;         BX = offset of string address word in INI file data
          ;           structure
          ;         CX = string length (without terminator)
          ;         DS:SI = address of string to be stored
          ;         DS:DI = address of INI file data structure
          ;         
          ; On exit:
          ;         Carry clear if string fits, set if not
          ;         Address of string within string area stored at
          ;           DS:[BX][DI] if carry clear
          ;         AX, CX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     INIStr,varframe     ;define near entry point in DOS
          ;
          varW      ItemPtr             ;item data pointer
          varW      OldAddr             ;old string relative address
          varW      AdjCnt              ;string area move byte count
          varend
          ;
          .push     dx,di,es            ;save registers
          ;
          ; Get the amount of increase or decrease in the string space used
          ;
          loadseg   es,ds               ;set destination segment
          mov       ItemPtr,bx          ;save item data pointer
          pushm     cx,si               ;save new string length and address
          inc       cx                  ;add to include terminator
          mov       ax,cx               ;copy new string length
          mov       si,[di][bx]         ;get old string relative address
          mov       OldAddr,si          ;save it for later
          cmp       si,EMPTYSTR         ;old string there?
           je       ISChkLen            ;if not go on
          add       si,[di].I_StrDat    ;make it the absolute string address
          call      STR_LEN             ;get old string length
          inc       cx                  ;add to include terminator
          sub       ax,cx               ;get new - old = change in length
          ;
          ; See if the new string will fit.  If so, and there's no old
          ; string, just go store the new one
          ;
ISChkLen: mov       dx,[di].I_StrUse    ;get current string area count
          add       ax,dx               ;add to length for this one
          cmp       ax,[di].I_StrMax    ;room for this one?
           ja       ISOver              ;if not complain
          cmp       si,EMPTYSTR         ;old string there?
           je       ISPutNew            ;if not go store new
          ;
          ; There's an old string to remove.  Figure out whether it's the
          ; last one in the pile
          ;
          push      ax                  ;save new count for all strings (this
                                        ;  is the count after the new string
                                        ;  is added)
          mov       AdjCnt,cx           ;save amount of string space removed
                                        ; for use in adjustments
          mov       ax,si               ;copy address of old string
          add       ax,cx               ;point to end of old string + 1
          mov       cx,dx               ;copy count of all strings
          sub       dx,AdjCnt           ;get new count of all strings
          add       cx,[di].I_StrDat    ;get end of all strings + 1
          sub       cx,ax               ;difference between old end of all
                                        ;  strings and end of removed string
                                        ;  is bytes to move
           jbe      ISPutRdy            ;if nothing to move the old string
                                        ;  was last, just go store it
          ;
          ; OK we really have to remove the old string from the middle of
          ; the pile.  First slide the strings above it down.
          ;
          push      dx                  ;save value for count of all strings
                                        ; after removal
          push      di                  ;save structure address
          mov       di,si               ;set destination for move
          mov       si,ax               ;set source
          cld                           ;be sure we go the right way
          rep       movsb               ;slide the strings down
          pop       di                  ;restore structure address
          ;
          ; Now go through the entire INI file data structure, find all the
          ; other strings that are above the one we removed, and reduce the
          ; pointer to each of them accordingly.
          ;
          mov       dx,OldAddr          ;get old string address
          mov       si,offset @DATASEG:INIDefs  ;point to definitions table
          mov       cl,[si]             ;get count of items to clear
          xor       ch,ch               ;clear high byte
          add       si,2                ;point to first item
          ;
ISLoop:   lodsb                         ;get name length, skip to text
          xor       ah,ah               ;clear high byte
          add       si,ax               ;skip name text
          mov       bl,[si].II_PType    ;get parse type
          cmp       bl,IP_STR           ;string item?
           je       ISChkAdj            ;if so go check adjustment
          cmp       bl,IP_PATH          ;path item?
           jne      ISNext              ;if not go on
          ;
ISChkAdj: mov       bx,[si].II_Data     ;get pointer to data offset
          cmp       wptr [di][bx],EMPTYSTR   ;is the string empty?
           je       ISNext              ;if so don't adjust it
          cmp       [di][bx],dx         ;check address of string against
                                        ;  address of removed string
           jbe      ISNext              ;if this string at or below the one
                                        ;  we removed then don't adjust
          mov       ax,AdjCnt           ;get move distance
          sub       [di][bx],ax         ;adjust pointer by amount of move
          ;
ISNext:   add       si,(size INIItem)   ;move to next item
          loop      ISLoop              ;and loop until done
          ;
          ; The strings have all been moved and the pointers adjusted.
          ; Fix up the registers and to the new string on the end.
          ;
          pop       dx                  ;get back current end of all strings
                                        ;  + 1
          ;
ISPutRdy: pop       ax                  ;get back new end after the new
                                        ;  string is added
          ;
          ; Now add the new string on at the end of the string area
          ; When we get here AX = the new total string count; DX = the
          ; current total string count = the place to put the new string
          ;
ISPutNew: popm      si,cx               ;get back address and length of new
                                        ;  string
          mov       bx,ItemPtr          ;get new string item pointer
          mov       [di].I_StrUse,ax    ;save new count
           jcxz     ISRemove            ;if new one is empty remove old
          mov       [di][bx],dx         ;save new string address
          add       dx,[di].I_StrDat    ;get new string data address
          mov       di,dx               ;copy as move destination
          rep       movsb               ;copy the string into place
          stocb     0                   ;store terminating null
          jmp       short ISOK          ;all done
          ;
ISRemove: mov       wptr [di][bx],EMPTYSTR  ;show old string isn't there
          ;
ISOK:     clc                           ;show all OK
          jmp       short ISExit        ;and get out
          ;
ISOver:   add       sp,4                ;throw away address and length of new
                                        ;  string
          stc                           ;show no room
          ;
ISExit:   .pop     dx,di,es            ;restore registers
          exit                          ;and return
          ;
          ;
@curseg   ends                          ;close segment
          ;
          end

