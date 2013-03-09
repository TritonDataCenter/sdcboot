

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


// 4DOS.H - Include file for 4DOS
//   Copyright (c) 1988 - 2002  Rex C. Conn   All rights reserved


typedef long LONG_PTR;
typedef unsigned long ULONG_PTR;
typedef int INT_PTR;
typedef unsigned int UINT_PTR;

// get some DOS versions of OS/2 defs
#define SELECTOROF(p)	(((unsigned short *)&(p))[1])
#define OFFSETOF(p)	(((unsigned short *)&(p))[0])

// make a far pointer from the segment and offset
#define MAKEP(seg,off) ((void _far *)(((unsigned long)(seg) << 16) | (unsigned int)(off)))

#undef  PASCAL
#define PASCAL	_pascal
#define FAR	_far
#define VOID	void

#define CHAR	char
#define SHORT	short
#define LONG	long
#define INT	int
#define LPBYTE  char _far *

typedef const char * LPCTSTR;
#define _TEXT(p)	p

#define VeryLongFileName(a,b)

#define _tprintf printf
#define _stprintf sprintf
#define _stscanf sscanf

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef unsigned int  UINT;
typedef unsigned char BYTE;
typedef int    BOOL;
typedef BYTE   FAR *PCH;
typedef BYTE   FAR *PSZ;
typedef BYTE   FAR *PBYTE;
typedef BYTE   near *NPBYTE;
typedef CHAR   FAR *PCHAR;
typedef SHORT  FAR *PSHORT;
typedef LONG   FAR *PLONG;
typedef INT    FAR *PINT;
typedef UCHAR  FAR *PUCHAR;
typedef USHORT FAR *PUSHORT;
typedef ULONG  FAR *PULONG;
typedef UINT   FAR *PUINT;
typedef VOID   FAR *PVOID;

#define ARGMAX		255	// maximum number of arguments in line
#define MAXARGSIZ	260	// maximum size of a single argument
#define MAXFILENAME	260	// increased for Win95
#define MAXPATH		256	// increased for Win95
#define MAXREDIR	10
#define MAXLINESIZ	512	// maximum size of input line
#define CMDBUFSIZ	512	// size of command input buffer

#define TABSIZE		gpIniptr->Tabs

#define MIN_STACK	1024

#define DOS_IN_ROM    0x0800	// DOS is in ROM (DH)
#define DOS_IN_HMA    0x1000	// DOS in HMA (DH)
#define DOS_IS_OS2    0x4000	// OS/2 VDM
#define DOS_IS_DR     0x8000	// DR-DOS

#define malloc _nmalloc
#define free _nfree
#define strdup _strdup
#define stricmp _stricmp
#define strlwr _strlwr
#define strnicmp _strnicmp
#define strupr _strupr

#define SysBeep(a,b) DosBeep(a,b)
#define QueryFileMode(a,b) _dos_getfileattr(a,b)
#define SetFileMode(a,b) _dos_setfileattr(a,b)

#define FileRead(a,b,c,d) _dos_read(a,b,c,d)
#define FileWrite(a,b,c,d) _dos_write(a,b,c,d)
#define wwrite(a,b,c) _write(a,b,c)

#define DosFindClose(a) FindClose(a)

// Disk info (free space, total space, and cluster size)
typedef struct
{
	unsigned long BytesFree;
	unsigned long BytesTotal;
	long ClusterSize;
	TCHAR szBytesFree[16];
	TCHAR szBytesTotal[16];
} QDISKINFO;

/* FindFirst/FindNext structure from <winbase.h>: */

#define WINAPI	_far _pascal

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, FAR *LPFILETIME;


typedef struct _WIN32_FIND_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR  cFileName[260];
    CHAR  szAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, FAR *LPWIN32_FIND_DATA;


typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, FAR *LPSYSTEMTIME;


/* Flags returned by GetVolumeInformationEx: */

#define FS_CASE_SENSITIVE	 0x0001
#define FS_CASE_IS_PRESERVED	 0x0002
#define FS_UNICODE_ON_DISK	 0x0004
#define FS_LFN_APIS		 0x4000
#define FS_VOLUME_COMPRESSED	 0x8000

/* Possible error return from handle functions: */

#define INVALID_HANDLE_VALUE 0xFFFF


// find first / find next structure
typedef struct
{
	char reserved[21];
	char attrib;
	union {
		unsigned short wr_time;
		struct {
			unsigned seconds : 5;
			unsigned minutes : 6;
			unsigned hours : 5;
		} file_time;
	} ft;
	union {
		unsigned short wr_date;
		struct {
			unsigned days : 5;
			unsigned months : 4;
			unsigned years : 7;
		} file_date;
	} fd;
	unsigned long ulSize;
	CHAR szFileName[ MAXPATH ];
	CHAR szAlternateFileName[ 14 ];
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	int hdir;
	USHORT fLFN;
	RANGES aRanges;		// date / time / size ranges
	long lFileOffset;	// file offset for @filename
    char *pszFileList;
} FILESEARCH;


// redirection flags for STDIN, STDOUT, STDERR, and active pipes
typedef struct
{
	short naStd[MAXREDIR];
	char faClip[MAXREDIR];
	char fPipeOpen;
	char fIsPiped;
	char *pszOutPipe;		// ASCII names of output & input pipes
	char *pszInPipe;
} REDIR_IO;


// Date and time structure
typedef struct _DATETIME
{
	UCHAR   hours;			// current hour
	UCHAR   minutes;		// current minute
	UCHAR   seconds;		// current second
	UCHAR   hundredths;		// current hundredths of a second
	UCHAR   day;			// current day
	UCHAR   month;			// current month
	USHORT  year;			// current year
	UCHAR   weekday;		// current day of week
} DATETIME;


// Country information structure (international versions)
typedef struct
{
	unsigned char cID;
	SHORT nLength;
	SHORT nCountryID;
	SHORT nCodePageID;
	SHORT fsDateFmt;
	unsigned char szCurrency[5];
	unsigned char szThousandsSeparator[2];
	unsigned char szDecimal[2];
	unsigned char szDateSeparator[2];
	unsigned char szTimeSeparator[2];
	unsigned char fsCurrencyFmt;
	unsigned char cDecimalPlace;
	unsigned char fsTimeFmt;
	LONG case_map_func;			// far pointer to function
	unsigned char szDataSeparator[2];
	SHORT abReserved2[5];
} COUNTRYINFO;


#define SWAP_EMS 0
#define SWAP_XMS 1
#define SWAP_DISK 2
#define SWAP_NONE 3


// Structure used by UMBReg for making some UMB space unavailable
typedef struct {
	unsigned int RegBegin;
	unsigned int RegEnd;
	unsigned int MinFree;
	unsigned int FreeList;
} UMBREGINFO;

#define MAXUMBREGIONS 8

typedef struct {
	unsigned int ExtFree_Size;
	unsigned int ExtFree_Level;
	unsigned long ExtFree_SectorsPerCluster;
	unsigned long ExtFree_BytesPerSector;
	unsigned long ExtFree_AvailableClusters;
	unsigned long ExtFree_TotalClusters;
	unsigned long ExtFree_AvailablePhysSectors;
	unsigned long ExtFree_TotalPhysSectors;
	unsigned long ExtFree_AvailableAllocationUnits;
	unsigned long ExtFree_TotalAllocationUnits;
	unsigned long ExtFree_Reserved1;
	unsigned long ExtFree_Reserved2;
} FAT32;

// define the error return values (OS/2 defines in BSEERR.H)
#define ERROR_INVALID_FUNCTION          1
#define ERROR_FILE_NOT_FOUND            2
#define ERROR_PATH_NOT_FOUND            3
#define ERROR_TOO_MANY_OPEN_FILES       4
#define ERROR_ACCESS_DENIED             5
#define ERROR_INVALID_HANDLE            6
#define ERROR_ARENA_TRASHED             7
#define ERROR_NOT_ENOUGH_MEMORY         8
#define ERROR_INVALID_BLOCK             9
#define ERROR_BAD_ENVIRONMENT           10
#define ERROR_BAD_FORMAT                11
#define ERROR_INVALID_ACCESS            12
#define ERROR_INVALID_DATA              13

#define ERROR_INVALID_DRIVE             15
#define ERROR_CURRENT_DIRECTORY         16
#define ERROR_NOT_SAME_DEVICE           17
#define ERROR_NO_MORE_FILES             18
#define ERROR_WRITE_PROTECT             19
#define ERROR_BAD_UNIT                  20
#define ERROR_NOT_READY                 21
#define ERROR_BAD_COMMAND               22
#define ERROR_CRC                       23
#define ERROR_BAD_LENGTH                24
#define ERROR_SEEK                      25
#define ERROR_NOT_4DOS_DISK             26
#define ERROR_SECTOR_NOT_FOUND          27
#define ERROR_OUT_OF_PAPER              28
#define ERROR_WRITE_FAULT               29
#define ERROR_READ_FAULT                30
#define ERROR_GEN_FAILURE               31
#define ERROR_SHARING_VIOLATION         32
#define ERROR_LOCK_VIOLATION            33
#define ERROR_WRONG_DISK                34
#define ERROR_FCB_UNAVAILABLE           35
#define ERROR_SHARING_BUFFER_EXCEEDED   36
#define ERROR_NOT_SUPPORTED             50

#define ERROR_FILE_EXISTS               80
#define ERROR_DUP_FCB                   81
#define ERROR_CANNOT_MAKE               82
#define ERROR_FAIL_I24                  83
#define ERROR_OUT_OF_STRUCTURES         84
#define ERROR_ALREADY_ASSIGNED          85
#define ERROR_INVALID_PASSWORD          86
#define ERROR_INVALID_PARAMETER         87
#define ERROR_NET_WRITE_FAULT           88
#define ERROR_NO_PROC_SLOTS             89

