

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  (1) The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  (2) The Software, or any portion of it, may not be compiled for use on any
//  operating system OTHER than FreeDOS without written permission from Rex Conn
//  <rconn@jpsoft.com>
//
//  (3) The Software, or any portion of it, may not be used in any commercial
//  product without written permission from Rex Conn <rconn@jpsoft.com>
//
//  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.


// INISTRUC.H - INI file data structure for 4xxx / TCMD products
//   Copyright 1998-2002  JP Software Inc.  All rights reserved


// INI file constants

#define INI_DEFKEYS 4

#define INI_SIG 0x4DD4

#define INI_STRMAX 1536
#define UINT unsigned char	// temp until I change 4DOS code

#define INI_KEYMAX 64
#define INI_KEYS_SIZE (2 * sizeof(UINT) * INI_KEYMAX)
#define INI_PRODUCT 0x80
#define INI_PRIMARY 0x40
#define INI_SECONDARY 0x20
#define INI_TOTAL_BYTES (sizeof (INIFILE) + INI_STRMAX + INI_KEYS_SIZE)

#define INI_EMPTYSTR ((unsigned int)-1)
#define OPTBITS 384
#define OBSIZE	((OPTBITS + 7) / 8)


// structures and macros for token lists
typedef struct {
	int num_entries;
	int entry_size;
	TCHAR **elist;
} TL_HEADER;

#define TOKEN_LIST(tl_name, tlist) TL_HEADER tl_name = { (sizeof(tlist) / sizeof(tlist[0])), sizeof(tlist[0]), (TCHAR **)&tlist }


// INI file data structure
// This structure is maintained in the following order:
//    Header
//    INI file data for:
//       All products
//       Character mode products
//       GUI products
//       Character/GUI product crossovers
//
// Within each section of INI file data, there is a further subdivision:
//    String pointers
//    Integers
//    Choices (2-valued)
//    Characters / bytes (includes 3+ valued choices)
//
// Within each of those sections, there is a further division:
//    INI file data, alphabetically by directive name
//    Internal data, alphabetically by structure element name
//

typedef struct
{
	/*
	**********************************************************************
	** Header (all products)
	**********************************************************************
	*/

	char       *StrData;		// (internal) pointer to strings data
	unsigned int StrMax;		// (internal) maximum string bytes
	unsigned int StrUsed;		// (internal) actual string bytes

	unsigned int *Keys;			// (internal) pointer to key list
	unsigned int KeyMax;		// (internal) maximum key count
	unsigned int KeyUsed;		// (internal) actual key count
	unsigned int SecFlag;		// (internal) INI file section bit flags

	unsigned int OBCount;		// (internal) OPTION bit flag count
	unsigned char OptBits[OBSIZE];	// (internal) OPTION bit flags

	/*
	**********************************************************************
	** All Products
	**********************************************************************
	*/
		// String pointers

	unsigned int DescriptName;	// alternative name for DESCRIPT.ION
	unsigned int DirColor;		// DirColors
	unsigned int FC;		// Filename completion
	unsigned int FSPath;		// 4StartPath / TCStartPath
	unsigned int HistLogName;	// HistoryLogName
	unsigned int LogName;		// LogName
	unsigned int Printer;		// Printer
	unsigned int TreePath;		// CD / CDD tree index file

   		// Integers
	unsigned int AliasSize;		// Alias
	unsigned int AliasNew;		// Alias (new from OPTION, in 4DOS)
// unsigned int Base;		// Base 0 or Base 1
	unsigned int BeepDur;		// BeepLength
	unsigned int BeepFreq;		// BeepFreq
	unsigned int CDDHeight;		// CDDWinHeight
	unsigned int CDDLeft;		// CDDWinLeft
	unsigned int CDDTop;		// CDDWinTop
	unsigned int CDDWidth;		// CDDWinWidth
	int CursI;					// CursorIns
	int CursO;					// CursorOver
	unsigned int DescriptMax;	// DescriptionMax
	unsigned int DirHistorySize;	// DirHistory
	unsigned int DirHistoryNew;	// DirHistory (new from OPTION, in 4DOS)

	unsigned int EnvSize;		// Environment
	unsigned int EnvNew;		// Environment (new from OPTION, in 4DOS)

// unsigned int ErrorColor;	// ErrorColors
	unsigned int EvalMax;		// Maximum # of EVAL decimal places
	unsigned int EvalMin;		// Minimum # of EVAL decimal places
	unsigned int FunctionSize;	// Function list size
	unsigned int FunctionNew;
	unsigned int FuzzyCD;		// fuzzy CD completion style
	unsigned int HistoryDups;	// History duplicate removal
	unsigned int HistMin;		// HistMin
	unsigned int HistorySize;	// History
	unsigned int HistoryNew;	// History (new from OPTION, in 4DOS)
	unsigned int InputColor;	// color used for command input
	unsigned int ListColor;		// ListColors
	unsigned int ListRowStart;
	unsigned int PWHeight;		// PopupWinHeight
	unsigned int PWLeft;		// PopupWinLeft
	unsigned int PWTop;			// PopupWinTop
	unsigned int PWWidth;		// PopupWinWidth
	unsigned int SelectColor;	// SelectColors
	unsigned int StdColor;		// StdColors
	unsigned int Tabs;			// tabstops (for LIST)

	unsigned int INIDebug;		// (internal) debug bit flags
	int ShellLevel;				// (internal) -1 if /C, 0 if /P, 1 otherwise
	int ShellNum;				// (internal) shell number
	unsigned int Expansion;	// (internal) alias/variable expansion flag

		// Choices
	UINT AppendDir;				// Append backslash to directory on tab
	UINT BatEcho;				// BatchEcho
	UINT CompleteHidden;		// Tab completion match hidden files/dirs
	UINT CopyPrompt;	// CopyPrompt
	UINT DecimalChar;	// decimal character for @EVAL
	UINT Descriptions;	// Descriptions
	UINT EditMode;		// EditMode
	UINT HistoryCopy;	// HistCopy
	UINT HistoryMove;	// HistMove
	UINT HistoryWrap;	// HistWrap
	UINT INIQuery;		// INIQuery
	UINT NoClobber; 	// NoClobber
	UINT PathExt;		// PathExt
	UINT PauseErr;		// PauseOnError
	UINT ThousandsChar;	// decimal character for @EVAL
	UINT TimeFmt;		// AmPm
	UINT UnixPaths;		// UnixPaths
	UINT Upper;			// UpperCase

		// Characters
	TCHAR CmdSep;		// CommandSep
	TCHAR EscChr;		// EscapeChar
	TCHAR ParamChr;		// ParameterChar
	TCHAR BootDrive;	// (internal) boot drive letter (upper case)
	TCHAR SwChr;		// (internal) SwitchChar

		// bytes
	UINT LogOn;		// (internal) command logging flag
	UINT LogErrors;	// (internal) error logging flag
	UINT HistLogOn;	// (internal) history logging flag
// UINT PrintLogOn;	// (internal) printer logging flag
	UINT SingleStep;	// (internal) batch file single step flag


	/*
	**********************************************************************
	** Character Mode Products
	**********************************************************************
	*/

	// ---------------------------------
	// -- All Character Mode Products
	// ---------------------------------
   		// Integers
  	unsigned int CDDColor;		// CDDWinColor
	unsigned int LBBar;		// ListboxBarColors
	unsigned int ListStatusColor;	// ListStatusColors
  	unsigned int PWColor;		// PopupWinColor
	unsigned int SelectStatusColor;	// SelectStatusColors

	// ---------------------------------
	// -- 4DOS
	// ---------------------------------

		// Choices
	unsigned char BrightBG;		// BrightBG
	unsigned char LineIn;		// LineInput

		// String pointers
	unsigned int AEParms;		// AutoExecParms
	unsigned int AEPath;		// AutoExecPath
	unsigned int HOptions;		// HelpOptions
	unsigned int HPath;		// HelpPath
	unsigned int InstallPath;	// InstallPath
	unsigned int RexxPath;		// path to REXX.EXE (if any)	
	unsigned int Swap;		// Swapping

	char _far *AliasLoc;		// (internal) alias list location
	char _far *DirHistLoc;		// (internal) dir history list location
	char _far *FunctionLoc;		// (internal) function list location
	char _far *HistLoc;		// (internal) history list location

   		// Integers
	unsigned int Columns;		// ScreenColumns
	unsigned int EnvFree;		// EnvFree
	unsigned int MaxLoad;		// MaxLoadAddress
	unsigned int StackSize;		// StackSize

	unsigned int EnvSeg;		// (internal) local master env segment
	unsigned int HighSeg;		// (internal) transient's starting segment
	unsigned int MastSeg;		// (internal) global master env segment

		// Choices
	unsigned char DRSets;		// DRSets            *
	unsigned char DVCleanup;	// DVCleanup         *
	unsigned char FineSwap;		// FineSwap          *
	unsigned char FullINT2E;	// FullINT2E         *
	unsigned char Inherit;		// Inherit
	unsigned char Mouse;		// Mouse present
	unsigned char MsgServer;	// MessageServer     *
	unsigned char NoWin95GUI;	// Win95 GUI disabled by /D
	unsigned char NWNames;		// NetwareNames      *
	unsigned char Reduce;		// Reduce            *
	unsigned char ReserveTPA;	// ReserveTPA        *
	unsigned char SDFlush;		// SDFlush
	unsigned char Reopen;		// SwapReopen        *
	unsigned char UMBAlias;		// UMBAlias
	unsigned char UMBDirHistory;	// UMBDirHistory
	unsigned char UMBEnv;		// UMBEnvironment    *
	unsigned char UMBFunction;	// UMBFunction
	unsigned char UMBHistory;	// UMBHistory
	unsigned char UMBLd;		// UMBLoad           *
	unsigned char USwap;		// UniqueSwapName    *

		// Characters / bytes
	unsigned char OutputBIOS;	// OutputBIOS
	unsigned char DVMode;		// (internal) DESQview flag   *
	unsigned char MSDOS7;		// (internal) MSDOS 7+ flag
	unsigned char Win95Desk;	// (internal) windows 95/98 desktop flag
	unsigned char SwapMeth;		// (internal) swapping method *
	unsigned char PrevShellNum;	// (internal) previous shell number *


	/*
	**********************************************************************
	** Special
	**********************************************************************
	*/


   		// String pointers

	unsigned int NextININame;		// NextINIFile
	unsigned int PrimaryININame;	// (internal) primary INI file name

	UINT DupBugs;		// duplicate CMD.EXE bugs?
	UINT LocalAliases;	// LocalAliases
	UINT LocalFunctions;	// LocalFunctions
	UINT LocalDirHistory;	// LocalDirHistory
	UINT LocalHistory;	// LocalHistory

	UINT ANSI;		// ANSI escapes enabled/disabled

	// ---------------------------------
	// -- 4DOS
	// ---------------------------------

		// Choices
	unsigned char ChangeTitle;	// ChangeTitle
	unsigned char CopyEA;		// Copy EA's
	unsigned char CritFail;		// CritFail
	unsigned char DiskReset;	// DiskReset
	unsigned char Win95LFN;		// Win95LFN
	unsigned char Win95SFN;		// Win95SFNSearch

		// Characters / bytes
	unsigned char WinMode;		// (internal) windows mode flag

   		// Integers
	unsigned int Rows;		// ScreenRows


	/*
	**********************************************************************
	** Trailer (all products)
	**********************************************************************
	*/
	unsigned int INIBuild;		// (internal) current internal build number
	unsigned int INISig;		// (internal) INI file signature
} INIFILE;


#undef UINT

