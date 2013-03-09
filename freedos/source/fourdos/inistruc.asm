

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


	;
	; Macros and structures for parsing and storing 4DOS / 4OS2 INI
	; file data
	;
	;
	; Macros
	;
				;;define an item in INI file
DefItem	macro	Text, ParsType, Default, Valid, DataName
	DefStr	Text		;;incorporate name string
	INIItem	<IP_&ParsType, Default, Valid, DataName>
				;;incorporate fixed data
ItemNum	=	ItemNum + 1	;;bump item number
	endm			;;end DefItem
	;
NPK	macro	Name, StdVal, SecVal
	DefStr	Name		;;key name
	NPKVals	<StdVal, SecVal>	;;store values
	endm			;;end NPK
	;
DefStr	macro	Text		;;define an internal string
	local	StrEnd		;;used to calculate length
	db	StrEnd - $ - 1	;;set length, compensate for length
				;;  byte itself
	db	Text		;;now include text
StrEnd	=	$		;;set end location
	endm			;;end DefStr
	;
	;
NOVAL	equ	0		;value for vo validation list
EMPTYSTR	equ	-1		;offset value for empty string
;	ifdef	_RT
;INISIG	equ	4DD5h		;runtime INI file signature value
;	else
INISIG	equ	4DD4h		;INI file signature value
;	endif
OPTBITS	equ	384		;number of option bit flags available
OBSIZE	equ	((OPTBITS + 7) / 8) ;number of bytes
	;
	; Bits in general flags for INIParse
	;
IP_SKLF	equ	80h		;query on each line
IP_NVAL	equ	40h		;use new value from query
IP_FATAL	equ	20h		;fatal error
	;
	; Bits in section flags for INIParse (MUST be in same order as
	; section names in list in INIParse!)
	;
SB_4DOS	equ	80h		;[4DOS] section
SB_PRIM	equ	40h		;[Primary] section
SB_SEC	equ	20h		;[Secondary] section
	;
	;
	; INI file parsing types.  Note that IP_KEY and greater types
	; represent items whose data goes into the key table or string
	; areas; this numeric ordering of types is used by the parsing
	; code and should not be disturbed or that code may break.
	;
IP_BYTE	equ	0		;byte numeric item (byte)
IP_CHAR	equ	1		;character item (byte)
IP_INT	equ	2		;signed integer item (word)
IP_UINT	equ	3		;unsigned integer item (word)
IP_CHOIC	equ	4		;byte choice item (byte)
IP_COLOR	equ	5		;color item (word)
IP_KEY	equ	6		;keystroke item (key table)
IP_STR	equ	7		;string item (string)
IP_PATH	equ	8		;path item (string)
IP_PROC	equ	9		;procedure item
IP_INCL	equ	10		;include file
	;
	; Keystroke types
	;
EXTKEY	equ	1		;extended key bit
MAP_GEN	equ	2		;general keys mapping context
MAP_EDIT	equ	4		;command line editing keys mapping
				;  context
MAP_HWIN	equ	8		;history window keys mapping context
MAP_LIST	equ	10h		;LIST keys mapping context
NORM_KEY	equ	80h		;"normal key" flag
	;
	;
	;
	; ******************************************************************
	; ******************************************************************
	; ** Structure for data needed to parse INI file
	; ******************************************************************
	; ******************************************************************
INIItem	struc			;structure for INI file item data
II_PType	db	?		;item parsing type
II_Def	dw	?		;default (pointer for strings)
II_Valid	dw	?		;validation pointer
II_Data	dw	?		;data pointer
INIItem	ends
	;
	;
	;
	; ******************************************************************
	; ******************************************************************
	; ** INI File Data Structure
	; ******************************************************************
	; ******************************************************************
INIData	struc			;start structure
	;
	; INI file data
	;
	; ** MUST ** be kept exactly in sync with INIFILE structure
	; in INISTRUC.H
	;
	; For ordering rules see 4ALL.H
	;
	;
	; ******************************************************************
	; ** Header (all products)
	; ******************************************************************
	ife	_FLAT
I_StrDat	dw	?		;pointer to string data
I_StrMax	dw	?		;maximum string bytes
I_StrUse	dw	?		;actual string bytes
I_Keys		dw	?		;pointer to key list
I_KeyMax	dw	?		;maximum key count
I_KeyUse	dw	?		;actual key count
I_SecFlg	dw	0		;bit flags for sections found
	else
I_StrDat	dd	?		;pointer to string data
I_StrMax	dd	?		;maximum string bytes
I_StrUse	dd	?		;actual string bytes
I_Keys	dd	?		;pointer to key list
I_KeyMax	dd	?		;maximum key count
I_KeyUse	dd	?		;actual key count
I_SecFlg	dd	0		;bit flags for sections found
	endif	;_FLAT
	;
	if	(_WIN eq 0) and (_PM eq 0) and (_NT eq 0)  ;4DOS and 4OS2 only
I_OBCnt	dw	OBSIZE		;option bit flag byte count
I_OBits	db	OBSIZE dup (?)	;option bit flags
	endif	;4DOS only
	;
	;
	; ******************************************************************
	; ** All Products
	; ******************************************************************
	;
	;   String pointers
	;
	ife	_FLAT
I_DesNam	dw	?		;DescriptName
I_DirCol	dw	?		;DirColors
I_FC		dw	?		;FileCompletion
I_4SPath	dw	?		;4StartPath
I_HLogNam	dw	?		;HistoryLogName
I_LogNam	dw	?		;LogName
I_Printer	dw	?		;Printer
I_TPath	dw	?		;TreePath
	else
I_DesNam	dd	?		;DescriptName
I_DirCol	dd	?		;DirColors
I_FC		dd	?		;FileCompletion
I_4SPath	dd	?		;4StartPath
I_HLogNam	dd	?		;HistoryLogName
I_LogNam	dd	?		;LogName
I_Printer	dd	?		;Printer
I_TPath		dd	?		;TreePath
	endif 	;_FLAT
	;
	;   Integers
	;
	ife	_FLAT
I_Alias		dw	0		;Alias
I_AlsNew	dw	0		;Alias (new from OPTION)
; I_Base	dw	0		;Base 0 or 1
I_BpDur		dw	?		;BeepLength
I_BpFreq	dw	?		;BeepFreq
I_CDHght	dw	?		;CDDWinHeight
I_CDLeft	dw	?		;CDDWinLeft
I_CDTop		dw	?		;CDDWinTop
I_CDWdth	dw	?		;CDDWinWidth
I_CursI		dw	?		;CursorIns
I_CursO		dw	?		;CursorOver
I_DesMax	dw	?		;DescriptionMax
I_DHist		dw	0		;DirHistory
I_DHNew		dw	0		;DirHistory (new from OPTION)
I_Env		dw	ENVDEF		;Environment
I_EnvNew	dw	0		;Environment (new from OPTION)
; I_ErrCol	dw	?		;ErrorColors
I_EMax		dw	?		;EvalMax
I_EMin		dw	?		;EvalMin
I_Func		dw	0		;FunctionSize
I_FuncNew	dw	0		;FunctionSize
I_FuzCD		dw	?		;FuzzyCD
I_HDups		dw	?		;HistoryDups
I_HMin		dw	?		;HistMin
I_Hist		dw	0		;History
I_HstNew	dw	0		;History (new from OPTION)
I_InCol		dw	?		;InputColor
I_LstCol	dw	?		;ListColors
I_LstRow	dw	?		;ListRowStart
I_PWHght	dw	?		;PopupWinHeight
I_PWLeft	dw	?		;PopupWinLeft
I_PWTop		dw	?		;PopupWinTop
I_PWWdth	dw	?		;PopupWinWidth
I_SelCol	dw	?		;SelectColors
I_StdCol	dw	?		;StdColors
I_Tabs		dw	8		;TabStops
I_Debug		dw	0		;debug bit flags (internal)
I_ShLev		dw	1		;0 if /P on cmd line,1 if not (intern)
I_Shell		dw	0		;shell number (internal)
I_ExpAV		dw	0		;expansion (internal)
	else
I_Alias		dd	0		;Alias
I_AlsNew	dd	0		;Alias (new from OPTION)
; I_Base	dd	0		;Base 0 or 1
I_BpDur		dd	?		;BeepLength
I_BpFreq	dd	?		;BeepFreq
I_CDHght	dd	?		;CDDWinHeight
I_CDLeft	dd	?		;CDDWinLeft
I_CDTop		dd	?		;CDDWinTop
I_CDWdth	dd	?		;CDDWinWidth
I_CursI		dd	?		;CursorIns
I_CursO		dd	?		;CursorOver
I_DesMax	dd	?		;DescriptionMax
I_DHist		dd	?		;DirHistory
I_DHNew		dd	0		;DirHistory (new from OPTION)
I_Env		dd	ENVDEF		;Environment
; I_ErrCol	dd	?		;ErrorColors
I_HDups		dd	?		;HistoryDups
I_HMin		dd	?		;HistMin
I_Hist		dd	0		;History
I_HstNew	dd	0		;History (new from OPTION)
I_InCol		dd	?		;InputColor
I_LstCol	dd	?		;ListColors
I_LstRow	dd	?		;ListColors
I_PWHght	dd	?		;PopupWinHeight
I_PWLeft	dd	?		;PopupWinLeft
I_PWTop		dd	?		;PopupWinTop
I_PWWdth	dd	?		;PopupWinWidth
I_SelCol	dd	?		;SelectColors
I_StdCol	dd	?		;StdColors
I_Tabs		dd	8		;TabStops
I_Debug		dd	0		;debug bit flags (internal)
I_ShLev		dd	1		;0 if /P on cmd line,1 if not (intern)
I_Shell		dd	0		;shell number (internal)
I_ExpAV		dd	0		;expansion (internal)
	endif	;_FLAT
	;
	;   Choices
	;
I_ApDir		db	?		;AppendDir
I_BEcho		db	?		;BatchEcho
I_CHidden	db	?		;CompleteHidden
I_CPrompt	db	?		;CopyPrompt
I_DecChr	db	?		;DecimalChar
I_Descr		db	?		;Descriptions
I_EMode		db	?		;EditMode
I_HCopy		db	?		;HistoryCopy
I_HMove		db	?		;HistoryMove
I_HWrap		db	?		;HistoryWrap
I_Query		db	?		;INIQuery
I_NoClob	db	?		;NoClobber
I_PathExt	db	?		;PathExt
I_PsErr		db	?		;PauseOnError
I_ThChr		db	?		;ThousandsChar
I_TmFmt		db	2		;AmPm
I_UnixPaths	db	?		;UnixPaths
I_Upper		db	?		;UpperCase
	;
	;   Characters / bytes
	;
I_CmdSep	db	?		;CommandSep
I_EscChr	db	?		;EscapeChar
I_ParChr	db	?		;ParameterChar
I_BootDr	db	0		;boot drive letter (int.,upper case)
I_SwChr		db	?		;switch character (internal)

I_LogOn		db	0		;command logging flag (internal)
I_LogErr	db	0		;error logging flag (internal)
I_HLogOn	db	0		;history logging flag (internal)
; I_PLogOn	db	0		;printer logging flag (internal)
I_Step		db	0		;single step (internal)
	;
	;
	; ******************************************************************
	; ** Character Mode Products
	; ******************************************************************
	;
	if (_WIN eq 0) and (_PM eq 0)
	; ---------------------------------
	; -- All Character Mode Products
	; ---------------------------------
	;
	;    Integers
	;
	ife	_FLAT
I_CDCol	dw	?		;CDDWinColor
I_LBBar	dw	?		;ListboxBarColors
I_LstSCol	dw	?		;ListStatusColors
I_PWCol	dw	?		;PopupWinColor
I_SelSCol	dw	?		;SelectStatusColors
	else
I_CDCol	dd	?		;CDDWinColor
I_LBBar	dd	?		;ListboxBarColors
I_LstSCol	dd	?		;ListStatusColors
I_PWCol	dd	?		;PopupWinColor
I_SelSCol	dd	?		;SelectStatusColors
	endif	;_FLAT
	;
	; ---------------------------------
	; -- 4DOS and 4OS2
	; ---------------------------------
	if (_DOS ne 0) or (_OS2 ne 0)
	;
	;   Choices
	;
I_BriBG	db	0		;BrightBG
I_LineIn	db	?		;LineInput
	endif	;4DOS and 4OS2
	;
	; ---------------------------------
	; -- 4DOS Only
	; ---------------------------------
	if _DOS ne 0
	;
	;   String pointers
	;
I_AEParm	dw	?		;AutoExecParms
I_AEPath	dw	?		;AutoExecPath
I_HOpt	dw	?		;HelpOptions
I_HPath	dw	?		;HelpPath (obsolete)
I_IPath	dw	?		;InstallPath
I_RxPath	dw	?		;RexxPath
I_Swap	dw	?		;Swapping
I_ALoc	dd	0		;(internal) alias list location
I_DLoc	dd	0		;(internal) dir history list location
I_FLoc	dd	0		;(internal) function list location
I_HLoc	dd	0		;(internal) history list location
	;
	;   Integers
	;
I_Columns	dw	?		;ScreenColumns
I_EnvFr	dw	?		;EnvFree
I_MaxLd	dw	0		;maximum load address (K)
I_Stack	dw	?		;StackSize
I_ESeg	dw	0		;local master env seg (internal)
I_HiSeg	dw	0		;transient portion's strting seg (int)
I_MSeg	dw	0		;global master env seg (internal)
	;
	;   Choices
	;
I_DRSets	db	?		;DRSets
I_DVCln	db	?		;DVCleanup
I_FSwap	db	?		;FineSwap
I_Int2E	db	?		;FullINT2E
I_Inhrit	db	?	;Inherit
I_Mouse db	?		;Mouse
I_CCMsg	db	?		;MessageServer
I_NoGUI	db	?		;GUI load disabled by /D
I_NName	db	0		;NetwareNames
I_Reduce	db	?		;Reduce
I_TPARes	db	?		;ReserveTPA
I_SDFlsh	db	?		;SDFlush
I_Reopen	db	?		;SwapReopen
I_UMBAls	db	?		;UMBAlias
I_UMBDir	db	?		;UMBDirHistory
I_UMBEnv	db	?		;UMBEnvironment
I_UMBFunc	db	?		;UMBFunction
I_UMBHst	db	?		;UMBHistory
I_UMBLd	db	?		;UMBLoad
I_USwap	db	?		;UniqueSwapName
	;
	;   Characters / bytes
	;
I_BIOS	db	0		;if != 0, use BIOS (10h) for direct i/o
I_DVMode	db	0		;DESQView mode (internal)
I_MSDOS7	db	0		;if != 0 we are in MSDOS 7+ (internal)
I_W95Dsk	db	0		;Windows 95/98 desktop flag (internal)
I_SwMeth	db	0		;swapping method (internal)
I_PShell	db	0		;previous shell number (internal)
	endif	;4DOS Only
	endif	;All Character Mode Products
	;
	;
	; ******************************************************************
	; ** GUI Products
	; ******************************************************************
	;
	if (_WIN ne 0) or (_PM ne 0)
	; ---------------------------------
	; -- All GUI Products
	; ---------------------------------
	;
	;   String pointers
	;
	ife	_FLAT
I_Editor	dw	0		;EditorName
	else
I_Editor	dd	0		;EditorName
	endif	;_FLAT
	;
	;   Integers
	;
	ife	_FLAT
I_ScrBuf	dw	0		;ScrBufSize
I_VScrCol	dw	0		;VirtScrCols
I_VScrRow	dw	0		;VirtScrRows
	else
I_ScrBuf	dd	0		;ScrBufSize
I_VScroll	dd	0		;VScroll
I_VScrCol	dd	0		;VirtScrCols
I_VScrRow	dd	0		;VirtScrRows
	endif	;_FLAT
	;
	;   Choices
	;
I_IBeam	db	?		;IBeam
; I_HScrl	db	?		;HScrollBar
I_StatBOn	db	?		;StatusBarOn
I_ScrlKey	db	?		;SwapScrollKeys
I_ToolBOn	db	?		;ToolBarOn
; I_VScrlB	db	?		;VScrollBar
	;
	; ---------------------------------
	; -- Take Command 16 and 32
	; ---------------------------------
	if _WIN ne 0
	;
	;   Integers
	;
	ife	_FLAT
I_StatBT	dw	0		;StatusBarText
I_ToolBT	dw	0		;ToolBarText
	else
I_StatBT	dd	0		;StatusBarText
I_ToolBT	dd	0		;ToolBarText
	endif	;_FLAT
	;
	;   Choices
	;
I_CUA	db	0		;CUA
I_ExtSel	db	?		;ExtendedSel
	endif	;Take Command 16 and 32
	;
	; ---------------------------------
	; -- Take Command 16 Only
	; ---------------------------------
	if _WIN eq 16
	;
	;   Choices
	;
I_PrmptSh	db	0		;PromptShellExit
I_TaskLst	db	0		;TCMDTaskList
	endif	;Take Command 16 Only
	;
	; ---------------------------------
	; -- Take Command 32 Only
	; ---------------------------------
	if _WIN eq 32
	;
	;   Integers
	;
I_ConCols	dd	0		;ConsoleColumns
I_ConRows	dd	0		;ConsoleRows
	;
	;   Choices
	;
I_Caveman	db	0		;Caveman
I_HideCon	db	0		;HideConsole
	endif	;Take Command 32 Only
	endif	;All GUI Products
	;
	;
	; ******************************************************************
	; ** Special
	; ******************************************************************
	;
	; ---------------------------------
	; -- All Products Except 4DOS
	; ---------------------------------
	if (_DOS eq 0) or (_WIN ne 0)
	;
	;   Integers
	;
	ife	_FLAT
I_WHght	dw	0		;WindowHeight
I_WState	dw	0		;WindowState
I_WWidth	dw	0		;WindowWidth
I_WX	dw	0		;WindowX
I_WY	dw	0		;WindowY
	else
I_WHght	dd	0		;WindowHeight
I_WState	dd	0		;WindowState 
I_WWidth	dd	0		;WindowWidth 
I_WX	dd	0		;WindowX  
I_WY	dd	0		;WindowY   
	endif	;_FLAT
	;
	;   Choices
	;
I_ExecWt	db	0		;ExecWait
	endif	;All Products Except 4DOS
	;
	; ---------------------------------------
	; -- All Products Except Take Command 16
	; ---------------------------------------
	if _WIN ne 16
	;
	;   String pointers
	;
	ife	_FLAT
I_NxtINI	dw	EMPTYSTR	;NextINIFile
I_PrmINI	dw	EMPTYSTR	;PrimaryININame (internal)
	else
I_NxtINI	dd	EMPTYSTR	;NextINIFile
I_PrmINI	dd	EMPTYSTR	;PrimaryININame (internal)
	endif
	;
	;   Choices
	;
I_DupBugs	db	?		;DupBugs
I_LocalA	db	?		;LocalAliases
I_LocalF	db	?		;LocalFunctions
I_LocalD	db	?		;LocalDirHistory
I_LocalH	db	?		;LocalHistory
	endif	;All Products Except Take Command 16
	;
	; ---------------------------------
	; -- All Products Except 4NT
	; ---------------------------------
	if (_NT eq 0) or (_WIN ne 0)
	;
	;   Choices
	;
I_ANSI	db	?		;ANSI
	endif	;All Products Except 4NT
	;
	; ---------------------------------
	; -- 4DOS and Take Command 16
	; ---------------------------------
	if _DOS ne 0
	;
	;   Choices
	;
I_ChgTtl	db	?		;ChangeTitle
I_CopyEA	db	?		;CopyEA
I_CFail	        db	?		;CritFail
I_DskRst	db	?		;DiskReset
I_W95LFN	db	?		;Win95LFN
I_W95SFN	db	?		;Win95SFNSearch
	;
	;   Characters / bytes
	;
I_WMode	db	0		;Windows mode flag (internal)
	;
	endif	;4DOS and Take Command 16
	;
	; ---------------------------------
	; -- 4OS2 and Take Command OS/2
	; ---------------------------------
	if _OS2 ne 0
	;
	;   String pointers
	;
I_HelpBk	dd	EMPTYSTR		;HelpBook
I_SwapPth	dd	EMPTYSTR		;SwapPath
	endif	 ;4OS2 and Take Command OS/2
	;
	; --------------------------------------------------------
	; -- 4DOS, 4OS2, Take Command/16, and Take Command OS/2
	; --------------------------------------------------------
	if (_DOS ne 0) or (_OS2 ne 0)
	;
	;    Integers
	;
	ife	_FLAT
I_Rows	dw	?		;ScreenRows
	else
I_Rows	dd	?		;ScreenRows
	endif	;_FLAT
	endif	;4DOS, 4OS2, Take Command/16, Take Command OS/2

	; ---------------------------------------------------
	; -- 4NT, Take Command/16, and Take Command/32
	; ---------------------------------------------------
	if (_WIN ne 0) or (_NT ne 0)
	;
	;   Choices
	;
I_Assoc	db	?		;LoadAssociations
	endif	;4NT, Take Command/16, and Take Command/32

	; ---------------------------------------------------
	; -- 4NT and Take Command/32
	; ---------------------------------------------------
	if _NT ne 0
	;
	;   Choices
	;
I_PipeCP	db	?		;DOSPipeWithCP
	endif	;4NT and Take Command/32
	;
	;
	; ******************************************************************
	; ** Trailer (all products)
	; ******************************************************************
	;
	ife	_FLAT
I_Build	dw	INTVER		;current build
I_Sig	dw	0		;signature
	else
I_Build	dd	0		;current build
I_Sig	dd	0		;signature
	endif	;_FLAT
	;
INIData	ends			;end IniData
	;
	;
	;
	; ******************************************************************
	; ******************************************************************
	; ** Macro and structure for non-printing keys table
	; ******************************************************************
	; ******************************************************************
NPKVals	struc			;structure for key values
NPStd	dw	?		;standard keystroke value
NPSecond	dw	?		;Ctrl or Shift value
NPKVals	ends			;end NPKVals

