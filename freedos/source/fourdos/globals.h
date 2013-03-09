

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


// Global variables for 4xxx / TCMD family
//   Copyright 1992 - 2003 by Rex C. Conn

#ifdef DEFINE_GLOBALS

// 4START is executed at startup; 4EXIT at exit
TCHAR AUTOSTART[] = _TEXT("4START");
TCHAR AUTOEXIT[] = _TEXT("4EXIT");


// "gszCmdline" is the input buffer for command line & batch file entry
// also used as temporary stack by server when a TSR terminates in an
//   unusual way, and by some of the internal commands
TCHAR gszCmdline[CMDBUFSIZ+16];
TCHAR *pszCmdLineOpts;
TCHAR gszOsVersion[6];		// holds DOS / OS2 version as string
TCHAR gszMyVersion[6];		// major and minor 4xxx version string
TCHAR *gpNthptr;			// pointer to nth arg in line
TCHAR *gpInternalName;		// name of internal command being executed
TCHAR *gpBatchName;		// name of batch file to be executed
TCHAR gszSortSequence[16];	// directory sort order flags (DIR & SELECT)

TCHAR gszFindDesc[128];		// File description to search for

TCHAR gszCRLF[] = _TEXT("\r\n");

TCHAR gchBlock = _TEXT('Û');	// chars for DOS/V / non-ASCII support
TCHAR gchRightArrow = 0x10;	// -> arrow for SELECT and DIR


TCHAR gchDimBlock = _TEXT('°');
TCHAR gchVerticalBar = _TEXT('³');
TCHAR gchUpArrow = (TCHAR)24;
TCHAR gchDownArrow = (TCHAR)25;

TCHAR _far *glpEnvironment;		// pointer to environment (paragraph aligned)
TCHAR _far *glpMasterEnvironment;	// pointer to master environment
TCHAR _far *glpAliasList;		// pointer to alias list (paragraph aligned)
TCHAR _far *glpHistoryList;		// pointer to base of history list
TCHAR _far *glpHptr = 0L;		// pointer into history list
TCHAR _far *glpDirHistory;		// pointer to directory history list
TCHAR _far *glpFunctionList = 0L;	// pointer to user functions list

CRITICAL_VARS cv;		// vars that must be saved when doing an INT 2E

BATCHFRAME bframe[MAXBATCH];	// batch file control structure

COUNTRYINFO gaCountryInfo;	// international formatting info

TIMERS gaTimers[3];

BOOL fRuntime = 0;

int  gnInclusiveMode;		// flags for /A:[-]xxxx directory searches
int  gnExclusiveMode;
int  gnOrMode;
int  gnEditMode;		// current edit state (0=overstrike,1=insert,
						//   2=keep overstrike, 3=keep insert)

UINT gaIniKeys[(2 * INI_KEYMAX)] = {
	CTL_BS,             // ^Bksp -> ^R
	TAB,                // TAB -> F9
	SHIFT_TAB,          // Shift-Tab -> F8
	CTL_TAB,			// ^Tab -> F7

	18 + MAP_GEN,		// ^Bksp -> ^R, general keys
	F9 + MAP_EDIT,		// TAB -> F9, editing keys
	F8 + MAP_EDIT,		// Shift-Tab -> F8, editing keys
	F7 + MAP_EDIT,		// ^Tab -> F7, editing keys

};


int gnOsVersion;		// combined major & minor version
int gnTransient = 0;	// transient processor (/C) flag
int gnCurrentDisk;		// current disk drive (A=1, B=2, etc.)
int gnErrorLevel = 0;	// return code for external commands
int gnInternalErrorLevel = 0;	// return code for internal commands
int gnSysError = 0;		// return code for last DOS/Windows error

unsigned int gnPageLength;	// number of rows per page
unsigned long glDirFlags;	// DIR flags (VSORT, RECURSE, JUSTIFY, etc.)
int gnDirTimeField;		// creation, last access, or last write sort

int gfCTTY = 0;			// if != 0, then CTTY from device is active
int gnHighErrorLevel = 0;	// high word from INT 21 4Dh
int gnOSFlags = 0;		// MS-DOS flags for in ROM & in HMA
char gchMajor;			// "true" major and minor OS versions
char gchMinor;
int fWin95 = 0;			// flag for Win95
int fWin95LFN = 0;
int fWin95SFN = 0;

TCHAR AUTOEXEC[MAXFILENAME];
extern int end;  		// end of data segment
INIFILE *gpIniptr = (INIFILE *)&end;	// pointer to inifile


#else		// DEFINE_GLOBALS

// external definitions

extern TCHAR AUTOSTART[];
extern TCHAR AUTOEXIT[];
extern TCHAR gszCmdline[];
extern TCHAR *pszCmdLineOpts;
extern TCHAR gszOsVersion[];
extern TCHAR *gpNthptr;
extern TCHAR *gpInternalName;
extern TCHAR *gpBatchName;
extern TCHAR gszSortSequence[];
extern TCHAR gszFindDesc[];

extern TCHAR gszCRLF[];
extern TCHAR gszMyVersion[];
extern TCHAR gchBlock;		// chars for DOS/V support
extern TCHAR gchRightArrow;

extern TCHAR gchDimBlock;
extern TCHAR gchVerticalBar;
extern TCHAR gchUpArrow;
extern TCHAR gchDownArrow;

extern TCHAR _far *glpEnvironment;
extern TCHAR _far *glpMasterEnvironment;
extern TCHAR _far *glpAliasList;
extern TCHAR _far *glpHistoryList;
extern TCHAR _far *glpHptr;
extern TCHAR _far *glpDirHistory;
extern TCHAR _far *glpFunctionList;

extern CRITICAL_VARS cv;

extern BATCHFRAME bframe[];

extern COUNTRYINFO gaCountryInfo;

extern BUILTIN commands[];

extern TIMERS gaTimers[];

extern int gnInclusiveMode;
extern int gnExclusiveMode;
extern int gnOrMode;
extern int gnEditMode;


extern LPTSTR lpszShellName;
extern INIFILE gaInifile;
extern INIFILE *gpIniptr;
extern TCHAR gaIniStrings[];
extern UINT gaIniKeys[];

extern int gnOsVersion;
extern int gnTransient;
extern int gnCurrentDisk;
extern int gnErrorLevel;
extern int gnInternalErrorLevel;
extern int gnSysError;
extern BOOL fRuntime;

extern int gnDirTimeField;
extern unsigned int gnPageLength;
extern unsigned long glDirFlags;


extern TCHAR gchSysBootDrive;
extern TCHAR gszSessionTitle[];
extern TCHAR *gpRexxCmdline;


extern int gfCTTY;
extern int gnOSFlags;
extern int gnHighErrorLevel;
extern TCHAR AUTOEXEC[];
extern char gchMajor;		// "true" major and minor OS versions
extern char gchMinor;
extern int fWin95;
extern int fWin95LFN;
extern int fWin95SFN;


#endif		// DEFINE GLOBALS
