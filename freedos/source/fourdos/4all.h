

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


// 4ALL.H - Include file for 4dos
//   Copyright (c) 1988 - 2002  Rex C. Conn   All rights reserved

#include <limits.h>		// get int & long sizes


typedef unsigned char TCHAR;
typedef unsigned char * LPTSTR;
typedef unsigned char _far * LPBYTE;

#define wcmemset memset

#define STDIN 0			// standard input, output, error, aux, & prn
#define STDOUT 1
#define STDERR 2
#define STDAUX 3
#define STDPRN 4

#define HISTMIN 256		// minimum and maximum history list size
#define HISTMAX 8192
#define HISTORY_SIZE 1024

#define DIRSTACKSIZE 512	// directory stack size (for PUSHD/POPD)

#define BADQUOTES ((LPTSTR)-1)	// flag for unmatched quotes in line

#define MKFNAME_NOERROR 1
#define MKFNAME_NOCASE  4

#define DESCRIPTION_READ 1	// flags for process_descriptions()
#define DESCRIPTION_WRITE 2
#define DESCRIPTION_COPY 3	// (read | write)
#define DESCRIPTION_CREATE 4
#define DESCRIPTION_EDIT 8
#define DESCRIPTION_REMOVE 0x10
#define DESCRIPTION_PROCESS 0x20

#define	EXETYPE_ERROR		0	// Error, or not an EXE file
#define	EXETYPE_DOS		1	// Just an plain old MS-DOS
#define EXETYPE_DOS_PIF		2	// DOS app with a PIF file
#define	EXETYPE_WIN16		3	// Windows 3.x
#define	EXETYPE_WIN386		4	// Windows 3.x VxD
#define	EXETYPE_OS2		5	// OS/2 2.x
#define	EXETYPE_WIN32GUI	6	// Windows NT or WIN32s
#define	EXETYPE_WIN32CUI	7	// Windows NT character mode
#define EXETYPE_POSIX		8	// Windows NT Posix

#define FIND_NO_ERRORS 0x10000L			// don't display any errors
#define FIND_ONLY_DIRS 0x20000L			// only return subdirectories
#define FIND_BYATTS 0x40000L			// search by attributes
#define FIND_RANGE 0x80000L				// check ranges
#define FIND_EXCLUDE 0x100000L			// exclude lists
#define FIND_CLOSE 0x200000L			// close the directory search handle after finding the file
#define FIND_NO_CASE 0x400000L			// no case conversion
#define FIND_NO_DOTNAMES 0x800000L		// no "." or ".."
#define FIND_EXACTLY 0x1000000L			// no wildcard matches
#define FIND_NO_FILELISTS 0x2000000L	// no '@' files!
#define FIND_SFN 0x4000000L				// return the SFN instead of the LFN

#define BREAK_STACK_OVERFLOW 0x100
#define BREAK_CANCEL_PROCESSING 0x400
#define BREAK_ON_ERROR 0x800

// Debug flags for Debug = setting in INI file
#define INI_DB_SHOWTAIL 0x01
#define INI_DB_ERRLABEL 0x02
#define INI_DB_DIRMEM 0x04
#define INI_DB_DIRMEMTOT 0x08
#define INI_DB_FAT32FREE 0x10

// valid response code when branding code is all done
#define BRAND_RESPONSE 38693

// gfPRFlags bits
#define PR_BETAVER 1
#define PR_PUBBETA 2
#define PR_NOORDMSG 4

// disable macro definitions of toupper() and tolower()
#ifdef toupper
#undef toupper
#undef tolower
#endif

#include <setjmp.h>


typedef struct
{
	// These variables need to be saved when doing an INT 2Eh or
	//   REXX "back door"
	int bn;			// current batch nesting level
	int fCall;		// 0 for batch overwrite, 1 for nesting, 2 for NT "call :label args"
	int fException;	// ^C or stack overflow detected flag
	jmp_buf env;		// setjmp save state
	union {
	    long lFlags;
	    struct {
		unsigned char _else;	// waiting for ELSE/ENDIFF
		unsigned _iff : 4;	// parsing an IFF/THEN
		unsigned _endiff : 4;	// ENDIFF nesting level
		unsigned _do : 4;	// parsing a DO/WHILE
		unsigned _do_end : 4;	// ENDDO nesting level
		unsigned _do_leave : 4;
	    } flag;
	} f;
	int fVerbose;		// command line ECHO ON flag

	int fLfnFor;
} CRITICAL_VARS;


typedef struct
{
	union {
		unsigned long ulDTStart;
		struct {
			unsigned short usTStart;
			unsigned short usDStart;
		} DTS;
	} DTRS;
	union {
		unsigned long ulDTEnd;
		struct {
			unsigned short usTEnd;
			unsigned short usDEnd;
		} DTE;
	} DTRE;
	unsigned short usDateType;	// 0=last write, 1=last access; 2=created
	unsigned short usTimeStart;
	unsigned short usTimeEnd;
	unsigned short usTimeType;	// 0=last write, 1=last access; 2=created

	unsigned long ulSizeMin;
	unsigned long ulSizeMax;

	LPTSTR pszExclude;
	LPTSTR pszDescription;
} RANGES;


// directory structure
typedef struct
{
	unsigned int uAttribute;		// file attribute
	unsigned int seconds;
	unsigned int minutes;
	unsigned int hours;
	unsigned int days;
	unsigned int months;
	unsigned int years;

	unsigned long ulFileSize;

	TCHAR szFATName[13];
	TCHAR szAmPm;				// am/pm for filetime

	unsigned short uMarked;	// SELECT file marker
	short nColor;

	unsigned int uCompRatio;	// compression ratio for DBLSPACE

	unsigned long lCompressedFileSize;
	unsigned char _huge *lpszFileID;	// optional file description
	unsigned char _huge *lpszLFN;
	unsigned char _huge *lpszLFNBase;
	char dummy[10];			// don't need padding for 32-bit
} DIR_ENTRY;


// CD / CDD flags
#define CD_CHANGEDRIVE 	1
#define CD_SAVELASTDIR 	2
#define CD_NOFUZZY	4
#define CD_NOERROR	8


// COPY_FLAGS
#define APPEND_FLAG 1
#define DEVICE_SOURCE 2
#define PIPE_SOURCE 4
#define DEVICE_TARGET 8
#define PIPE_TARGET 0x10
#define ASCII_SOURCE 0x20
#define BINARY_SOURCE 0x40
#define ASCII_TARGET 0x80
#define BINARY_TARGET 0x100
#define COPY_CREATE 0x200
#define MOVING_FILES 0x400
#define VERIFY_COPY 0x1000
#define COPY_PROGRESS 0x2000L
#define FTP_ASCII 0x4000		// transfer using FTP ASCII
#define NO_ERRORS 0x8000L
#define COPY_RESTARTABLE 0x10000L


// DIR flags

#define FAT 0
#define LFN 1

#define DIRFLAGS_UPPER_CASE 1
#define DIRFLAGS_LOWER_CASE 2
#define DIRFLAGS_RECURSE 4
#define DIRFLAGS_JUSTIFY 8
#define DIRFLAGS_SUMMARY_ONLY 0x10
#define DIRFLAGS_VSORT 0x20
#define DIRFLAGS_NO_HEADER 0x40
#define DIRFLAGS_NO_FOOTER 0x80
#define DIRFLAGS_NAMES_ONLY 0x100
#define DIRFLAGS_LFN 0x200
#define DIRFLAGS_LFN_TO_FAT 0x400
#define DIRFLAGS_FULLNAME 0x800
#define DIRFLAGS_NO_COLOR 0x1000
#define DIRFLAGS_RECURSING_NOW 0x2000
#define DIRFLAGS_NO_DOTS 0x4000
#define DIRFLAGS_NT_ALT_NAME 0x8000
#define DIRFLAGS_WIDE 0x10000L
#define DIRFLAGS_COMPRESSION 0x20000L
#define DIRFLAGS_HOST_COMPRESSION 0x40000L
#define DIRFLAGS_PERCENT_COMPRESSION 0x80000L
#define DIRFLAGS_SHOW_ATTS 0x100000L
#define DIRFLAGS_TRUNCATE_DESCRIPTION 0x200000L
#define DIRFLAGS_TREE 0x400000L
#define DIRFLAGS_ALLOCATED 0x800000L
#define DIRFLAGS_LOCAL_SUMMARY 0x1000000L
#define DIRFLAGS_GLOBAL_SUMMARY 0x2000000L
#define DIRFLAGS_OWNER 0x4000000L
#define DIRFLAGS_ENUM_STREAMS 0x8000000L

typedef struct
{
	TCHAR szColorName[30];
	short sColor;
} CDIR;


// array used for internal commands (indirect function calls)
typedef struct
{
	LPTSTR szCommand;			// command name
	int (_near * pFunc)( LPTSTR );	// pointer to function
	unsigned int fParse;		// command line parse control flag
} BUILTIN;


#define CMD_EXPAND_VARS 1			// expand variables
#define CMD_EXPAND_REDIR 2			// perform redirection
#define CMD_STRIP_QUOTES 4			// remove single back quotes
#define CMD_ADD_NULLS 8				// add terminators to each arg (batch)
#define CMD_GROUPS 0x10				// check for command grouping
#define CMD_CLOSE_BATCH 0x20		// close batch file before executing
#define CMD_ONLY_BATCH 0x40			// command only allowed in batch files
#define CMD_DISABLED 0x80			// command enabled (0) or disabled (1)
#define CMD_RESET_DISKS 0x100		// reset disks after command
#define CMD_SET_ERRORLEVEL 0x200	// set ERRORLEVEL upon return (OS2 only)
#define CMD_BACKSLASH_OK 0x400		// kludge for trailing '\'s
#define CMD_DETACH_LINE 0x800		// pass entire line to DETACH command
#define CMD_UCASE_CMD 0x1000		// ensure command is in upper case
#define CMD_EXTERNAL 0x2000			// this is an external command
#define CMD_HELP 0x4000				// if != 0, check for /? anywhere on line
#define CMD_PERIOD_OK 0x8000		// accept trailing '.' as part of command (i.e., "dir.", "cd..")

#define EXPAND_NO_ALIASES 0x01
#define EXPAND_NO_NESTED_ALIASES 0x02
#define EXPAND_NO_VARIABLES 0x04
#define EXPAND_NO_NESTED_VARIABLES 0x08
#define EXPAND_NO_COMPOUNDS 0x10
#define EXPAND_NO_REDIR 0x20
#define EXPAND_NO_QUOTES 0x40
#define EXPAND_NO_ESCAPES 0x80
#define EXPAND_NO_USER_FUNCTIONS 0x100


// structure used by COLOR to set screen colors via ANSI escape sequences
typedef struct
{
	LPTSTR szShade;
	unsigned int uANSI;
} ANSI_COLORS;


// structure used by COLORDIR to set screen colors based on attributes
typedef struct
{
	LPTSTR pszType;
	unsigned int uAttribute;
} COLORD;


// start time for three timers
typedef struct
{
	int fTimer;
	int uTHours;
	int uTMinutes;
	int uTSeconds;
	int uTHundreds;
	long lDay;
} TIMERS;


#define MAXBATCH 16			// maximum batch file nesting depth
#define SETLOCAL_NEST 8

#define ABORT_LINE 0x0FFF		// strange value to abort multiple cmds
#define BATCH_RETURN 0x7ABC		// strange value to abort batch nesting
#define BATCH_RETURN_RETURN 0x7ABD	// strange value to abort GOSUBs
#define IN_MEMORY_FILE 0x7FFF		// flag for .BTM file


// We define structures ("frames") for batch files; allowing us to nest batch
//   files without the overhead of calling a copy of the command processor
typedef struct
{
	LPTSTR pszBatchName;		// fully qualified filename
	LPTSTR *Argv;				// pointer to argument list
	int bfd;					// file handle for batch file
	int Argv_Offset;			// offset into Argv list

	long lOffset;				// current file pointer position

	unsigned int uBatchLine;		// current file line (0-based)
	int nGosubOffset;				// current gosub nesting level
	LPTSTR pszGosubArgs;		// arg list passed to GOSUB
	unsigned int uGosubStack;		// stack of GOSUBs w/args
	unsigned int uEcho;		// current batch echo state (0 = OFF)
	LPTSTR pszOnBreak;				// command to execute on ^C
	LPTSTR pszOnError;				// command to execute on error
	LPTSTR pszOnErrorMsg;			// command to execute on error message
	unsigned int uOnErrorState;		// prior state of error popups	
	LPTSTR pszTitle;			// window/icon title
	LPBYTE lpszBTMBuffer;		// pointer to buffer for .BTM files
	int nFlags;				// see below
	int nReturn;				// return value
	int nGosubReturn;			// return from GOSUB

	int nSetLocalOffset;			// nesting level in SETLOCAL
	LPTSTR lpszLocalDir[SETLOCAL_NEST];			// saved disk and directory

	unsigned char _far *lpszLocalEnv[SETLOCAL_NEST];		// saved environment for SETLOCAL

	unsigned int uLocalEnvSize[SETLOCAL_NEST];

	unsigned char _far *lpszLocalAlias[SETLOCAL_NEST];	// saved alias list for SETLOCAL

	unsigned int uLocalAliasSize[SETLOCAL_NEST];
	TCHAR cLocalParameter[SETLOCAL_NEST];
	TCHAR cLocalEscape[SETLOCAL_NEST];
	TCHAR cLocalSeparator[SETLOCAL_NEST];
	TCHAR cLocalDecimal[SETLOCAL_NEST];
	TCHAR cLocalThousands[SETLOCAL_NEST];
} BATCHFRAME;

#define BATCH_REXX 1
#define BATCH_COMPRESSED 2
#define BATCH_ENCRYPTED 4
#define BATCH_UNICODE 8


// FFIND options
#define FFIND_QUIET 1
#define FFIND_HIDDEN 2
#define FFIND_LINE_NUMBERS 4
#define FFIND_SUBDIRS 8
#define FFIND_ALL 0x10
#define FFIND_TEXT 0x20
#define FFIND_SHOWALL 0x40
#define FFIND_HEX_DISPLAY 0x80
#define FFIND_HEX_SEARCH 0x100
#define FFIND_CHECK_CASE 0x200
#define FFIND_TOPSEARCH 0x400
#define FFIND_ENDSEARCH 0x800
#define FFIND_NOT 0x1000
#define FFIND_NOERROR 0x2000
#define FFIND_DIALOG 0x4000
#define FFIND_REVERSE_SEARCH 0x8000
#define FFIND_STDIN 0x10000L
#define FFIND_NOWILDCARDS 0x20000L
#define FFIND_ONLY_FIRST 0x40000L
#define FFIND_QUERY_CONTINUE 0x80000L
#define FFIND_SUMMARY 0x100000L


// LIST file info structure
typedef struct {
	char  szName[260];

	int   hHandle;

	int   fEoL;		// line end character (CR or LF)
	union {
		unsigned short ufTime;
		struct {
			unsigned seconds : 5;
			unsigned minutes : 6;
			unsigned hours : 5;
		} file_time;
	} ft;
	union {
		unsigned short ufDate;
		struct {
			unsigned days : 5;
			unsigned months : 4;
			unsigned years : 7;
		} file_date;
	} fd;

	long  lSize;

	long  lCurrentLine;

	long lViewPtr;				// pointer to top line

	long lFileOffset;			// offset of block

	LPBYTE lpBufferEnd;			// end of file buffer
	LPBYTE lpEOF;				// pointer to EOF in buffer
	LPBYTE lpCurrent;			// current char in get buffer
	LPBYTE lpBufferStart;		// beginning of file buffer

	unsigned int uTotalSize;	// size of entire buffer
	unsigned int uBufferSize;	// size of each buffer block
	int nListHorizOffset;		// horizontal scroll offset
	int nSearchLen;
	int fDisplaySearch;
} LISTFILESTRUCT;


#define LIST_BY_ATTRIBUTES 1	// /a:-rhsda
#define LIST_HIBIT 2			// strip high bit
#define LIST_NOWILDCARDS 4
#define LIST_REVERSE 8
#define LIST_STDIN 0x10			// get input from STDIN
#define LIST_WRAP 0x20			// wrap lines at right margin
#define LIST_HEX 0x40			// display in hex
#define LIST_SEARCH 0x80

// Include INI file data structures
#include "inistruc.h"


// popup text window control block
typedef struct
{
	int nTop;			// upper left (inside) corner of window
	int nLeft;
	int nBottom;
	int nRight;
	int nThumb;			// position of thumbwheel
	int nAttrib;		// attribute to be used in window
	int nInverse;		// inverse of "attrib"
	int nCursorRow;		// current cursor offset in window
	int nCursorColumn;
	int nOldRow;		// cursor position when window was
	int nOldColumn;		//   opened (used for screen restore)
	int nHOffset;		// horizontal scroll offset
	int fShadow;		// if != 0, draw shadow
	int fPopupFlags;	// flags for window

	char _far *lpScreenSave;	// pointer to screen save buffer
} POPWINDOW, *POPWINDOWPTR;


#define EDIT_COMMAND 1		// constants for egets()
#define EDIT_DATA 2
#define EDIT_DIALOG 4
#define EDIT_ECHO 8			// and getkey()
#define EDIT_NO_ECHO 0x10
#define EDIT_BIOS_KEY 0x20
#define EDIT_ECHO_CRLF 0x40
#define EDIT_NO_CRLF 0x80
#define EDIT_SCROLL_CONSOLE 0x100
#define EDIT_UC_SHIFT 0x200
#define EDIT_PASSWORD 0x400
#define EDIT_NO_INPUTCOLOR 0x800
#define EDIT_DIGITS 0x1000
#define EDIT_SWAP_SCROLL 0x2000
#define EDIT_MOUSE_BUTTON 0x4000
#define EDIT_NO_PIPES 0x8000
#define EDIT_UNICODE_FILE 0x10000L


// bits for cvtkey, used in key mapping directives
#define EXTENDED_KEY 0x0100
#define MAP_GEN 0x0200
#define MAP_EDIT 0x0400
#define MAP_HWIN 0x0800
#define MAP_LIST 0x1000
#define NORMAL_KEY 0x8000


#define FIND_FIRST 0x4E
#define FIND_NEXT 0x4F


// 4xxx-specific error messages
#define OFFSET_4DOS_MSG			0x4000

#define ERROR_4DOS_BAD_SYNTAX		0+OFFSET_4DOS_MSG
#define ERROR_4DOS_UNKNOWN_COMMAND	1+OFFSET_4DOS_MSG
#define ERROR_4DOS_COMMAND_TOO_LONG	2+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_CLOSE_QUOTE	3+OFFSET_4DOS_MSG
#define ERROR_4DOS_CANT_OPEN		4+OFFSET_4DOS_MSG
#define ERROR_4DOS_CANT_CREATE		5+OFFSET_4DOS_MSG
#define ERROR_4DOS_CANT_DELETE		6+OFFSET_4DOS_MSG
#define ERROR_4DOS_READ_ERROR		7+OFFSET_4DOS_MSG
#define ERROR_4DOS_WRITE_ERROR		8+OFFSET_4DOS_MSG
#define ERROR_4DOS_DUP_COPY		9+OFFSET_4DOS_MSG
#define ERROR_4DOS_DISK_FULL		10+OFFSET_4DOS_MSG
#define ERROR_4DOS_CONTENTS_LOST	11+OFFSET_4DOS_MSG
#define ERROR_4DOS_INFINITE_COPY	12+OFFSET_4DOS_MSG
#define ERROR_4DOS_INFINITE_MOVE	12+OFFSET_4DOS_MSG
#define ERROR_4DOS_NOT_ALIAS		13+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_ALIASES		14+OFFSET_4DOS_MSG
#define ERROR_4DOS_ALIAS_LOOP		15+OFFSET_4DOS_MSG
#define ERROR_4DOS_VARIABLE_LOOP	16+OFFSET_4DOS_MSG
#define ERROR_4DOS_UNKNOWN_CMD_LOOP	17+OFFSET_4DOS_MSG
#define ERROR_4DOS_EE_LOOP		18+OFFSET_4DOS_MSG
#define ERROR_4DOS_INVALID_DATE		19+OFFSET_4DOS_MSG
#define ERROR_4DOS_INVALID_TIME		20+OFFSET_4DOS_MSG
#define ERROR_4DOS_DIR_STACK_EMPTY	21+OFFSET_4DOS_MSG
#define ERROR_4DOS_CANT_GET_DIR		22+OFFSET_4DOS_MSG
#define ERROR_4DOS_LABEL_NOT_FOUND 	23+OFFSET_4DOS_MSG
#define ERROR_4DOS_OUT_OF_ENVIRONMENT 	24+OFFSET_4DOS_MSG
#define ERROR_4DOS_OUT_OF_ALIAS		25+OFFSET_4DOS_MSG
#define ERROR_4DOS_NOT_IN_ENVIRONMENT 	26+OFFSET_4DOS_MSG
#define ERROR_4DOS_STACK_OVERFLOW	27+OFFSET_4DOS_MSG
#define ERROR_4DOS_FTP_FTP		27+OFFSET_4DOS_MSG
#define ERROR_4DOS_ONLY_BATCH		28+OFFSET_4DOS_MSG
#define ERROR_4DOS_MISSING_BATCH 	29+OFFSET_4DOS_MSG
#define ERROR_4DOS_EXCEEDED_NEST 	30+OFFSET_4DOS_MSG
#define ERROR_4DOS_INVALID_BATCH	31+OFFSET_4DOS_MSG
#define ERROR_4DOS_ENCRYPTED_BATCH	32+OFFSET_4DOS_MSG
#define ERROR_4DOS_MISSING_ENDTEXT 	33+OFFSET_4DOS_MSG
#define ERROR_4DOS_BAD_RETURN 		34+OFFSET_4DOS_MSG
#define ERROR_4DOS_ENV_SAVED		35+OFFSET_4DOS_MSG
#define ERROR_4DOS_ENV_NOT_SAVED	36+OFFSET_4DOS_MSG
#define ERROR_4DOS_EXCEEDED_SETLOCAL_NEST	37+OFFSET_4DOS_MSG
#define ERROR_4DOS_UNBALANCED_PARENS	38+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_EXPRESSION	39+OFFSET_4DOS_MSG
#define ERROR_4DOS_OVERFLOW		40+OFFSET_4DOS_MSG
#define ERROR_4DOS_UNDERFLOW		41+OFFSET_4DOS_MSG
#define ERROR_4DOS_DIVIDE_BY_ZERO	42+OFFSET_4DOS_MSG
#define ERROR_4DOS_SIGNIFICANCE		43+OFFSET_4DOS_MSG
#define ERROR_4DOS_FILE_EMPTY		44+OFFSET_4DOS_MSG
#define ERROR_4DOS_NOT_A_DIRECTORY      45+OFFSET_4DOS_MSG
#define ERROR_4DOS_CLIPBOARD_INUSE	46+OFFSET_4DOS_MSG
#define ERROR_4DOS_CLIPBOARD_NOT_TEXT	47+OFFSET_4DOS_MSG
#define ERROR_4DOS_ALREADYEXCLUDED	48+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_FUNCTIONS		49+OFFSET_4DOS_MSG
#define ERROR_4DOS_NOT_FUNCTION		50+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_KEYSTACK		51+OFFSET_4DOS_MSG
#define ERROR_4DOS_INVALID_DOS_VER	52+OFFSET_4DOS_MSG
#define ERROR_4DOS_NOT_SWAPPING_MODE	53+OFFSET_4DOS_MSG
#define ERROR_4DOS_NO_UMBS		54+OFFSET_4DOS_MSG
#define ERROR_4DOS_REGION_SYNTAX	55+OFFSET_4DOS_MSG


// error return codes
#define USAGE_ERR 1
#define ERROR_EXIT 2
// #define CTRLC 3
#define ERROR_NOT_IN_LIST 0xFFFE
#define ERROR_LIST_EMPTY 0xFFFF


#define EOS '\0'		// standard end of string
#define FALSE 0
#define TRUE 1

#define SOH 1
#define CTRLC 3
#define EOT 4
#define ACK 6
#define BELL 7
#define BACKSPACE 8
#define BS 8
#define TAB 9
#define LF 10
#define LINEFEED 10
#define FORMFEED 12
#define FF 12
#define CR 13
#define XON 17
#define DC2 18
#define XOFF 19
#define DC4 20
#define NAK 21
#define SYN 22
#define ETB 23
#define CRC 'C'
#define CAN 24
#define EM 25
#define EoF 26
#define ESC 27
#define ESCAPE 27
#define SPACE 32
#define CTL_BS 127

#define FBIT 0x100

#define SHIFT_TAB	15+FBIT
#define ALT_TAB		165+FBIT
#define CTL_TAB		148+FBIT

#define F1		59+FBIT		// function keys
#define F2		60+FBIT
#define F3		61+FBIT
#define F4		62+FBIT
#define F5		63+FBIT
#define F6		64+FBIT
#define F7		65+FBIT
#define F8		66+FBIT
#define F9		67+FBIT
#define F10		68+FBIT
#define F11     0x85+FBIT
#define F12     0x86+FBIT
#define SHFT_F1		84+FBIT
#define SHFT_F2		85+FBIT
#define SHFT_F3		86+FBIT
#define SHFT_F4		87+FBIT
#define SHFT_F5		88+FBIT
#define SHFT_F6		89+FBIT
#define SHFT_F7		90+FBIT
#define SHFT_F8		91+FBIT
#define SHFT_F9		92+FBIT
#define SHFT_F10	93+FBIT
#define SHFT_F11	0x87+FBIT
#define SHFT_F12	0x88+FBIT
#define CTL_F1		94+FBIT
#define CTL_F2		95+FBIT
#define CTL_F3		96+FBIT
#define CTL_F4		97+FBIT
#define CTL_F5		98+FBIT
#define CTL_F6		99+FBIT
#define CTL_F7		100+FBIT
#define CTL_F8		101+FBIT
#define CTL_F9		102+FBIT
#define CTL_F10		103+FBIT
#define CTL_F11		0x89+FBIT
#define CTL_F12		0x8A+FBIT
#define ALT_F1		104+FBIT
#define ALT_F2		105+FBIT
#define ALT_F3		106+FBIT
#define ALT_F4		107+FBIT
#define ALT_F5		108+FBIT
#define ALT_F6		109+FBIT
#define ALT_F7		110+FBIT
#define ALT_F8		111+FBIT
#define ALT_F9		112+FBIT
#define ALT_F10		113+FBIT
#define ALT_F11		0x8B+FBIT
#define ALT_F12		0x8C+FBIT
#define HOME		71+FBIT
#define CUR_UP		72+FBIT
#define PgUp		73+FBIT
#define CUR_LEFT	75+FBIT
#define CUR_RIGHT	77+FBIT
#define END		79+FBIT
#define CUR_DOWN	80+FBIT
#define PgDn		81+FBIT
#define INS		82+FBIT
#define DEL		83+FBIT
#define CTL_LEFT	115+FBIT
#define CTL_RIGHT	116+FBIT
#define CTL_END		117+FBIT
#define CTL_PgDn	118+FBIT
#define CTL_HOME	119+FBIT
#define CTL_PgUp	132+FBIT
#define CTL_UP		141+FBIT
#define CTL_DOWN	145+FBIT

#define SHIFT_LEFT      200+FBIT	// dummy values for line editing
#define SHIFT_RIGHT     201+FBIT
#define SHIFT_HOME      202+FBIT
#define SHIFT_END       203+FBIT
#define CTL_SHIFT_LEFT  204+FBIT
#define CTL_SHIFT_RIGHT 205+FBIT

#define LEFT_MOUSE_BUTTON 250+FBIT
#define RIGHT_MOUSE_BUTTON 251+FBIT
#define MIDDLE_MOUSE_BUTTON 252+FBIT

#include "version.h"

#include "4dos.h"


#include "globals.h"		// global variables
#include "proto.h"		// function prototypes

#ifdef ENGLISH
#include "message.h"		// English message definitions for 4xxx
#endif
#ifdef GERMAN
#include "message.h_d"		// German message definitions for 4xxx
#endif
