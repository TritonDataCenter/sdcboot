

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


//	DBLSPACE.H - Definitions for the DoubleSpace Compressed Volume File
//		Portions based on original CVF.H supplied by Microsoft
//		This version Copyright 1993, JP Software Inc., All Rights Reserved

// DBLSPACE INT 2F API call values

#define DSAPISig 0x4A11
#define DS_GetVersion 0
#define DS_GetDriveMapping 1
#define DS_SwapDrives 2
#define DS_ActivateDrive 5
#define DS_DeactivateDrive 6
#define DS_DriveSpace 7
#define DS_GetFragSpace 8
#define DS_GetExtra 9

#define DSRetSig 0x444D
#define DSMapped 0x80
#define DSHost 0x7F

// DBLSPACE INT 2F API error codes
#define DS_ERROR_BAD_FUNCTION 0x100
#define DS_ERROR_BAD_DRIVE 0x101
#define DS_ERROR_NOT_COMP 0x102
#define DS_ERROR_ALREADY_SWAPPED 0x103
#define DS_ERROR_NOT_SWAPPED 0x104


#pragma pack(1)

#define szDS_STAMP1 "\xf8" "DR"	// First CVF stamp
#define cbDS_STAMP 4					// Length of stamp (includes NULL)

#define csecRESERVED1 1			// sectors in reserved region 1
#define csecRESERVED2 31		// sectors in reserved region 2
#define csecRESERVED4 2			// sectors in reserved region 4
#define cbPER_BITFAT_PAGE 2048	// BitFAT page size

// MDBPB data structure
typedef struct {

	BYTE	jmpBOOT[3];			// Jump to bootstrap routine
	char	achOEMName[8];		// OEM Name ("MSDBL6.0")

    // DOS BPB fields
	UINT	cbPerSec;			// bytes per sector (always 512)
	BYTE	csecPerClu;			// sectors per cluster (always 16)
	UINT	csecReserved;		// reserved sectors.
	BYTE	cFATs;				// FATs (always 1)
	UINT	cRootDirEntries;	// root directory entries (always 512)
	UINT	csecTotalWORD;		// total sectors (see csecTotalDWORD if 0)
	BYTE	bMedia;				// Media byte (always 0xF8 == hard disk)
	UINT	csecFAT;				// sectors occupied by the FAT
	UINT	csecPerTrack;		// sectors per track (not valid for DBLSPACE)
	UINT	cHeads;				// heads (not valid for DBLSPACE)
	ULONG	csecHidden;			// hidden sectors
	ULONG	csecTotalDWORD;	// total sectors

   // DoubleSpace extensions
	UINT	secMDFATStart;		// Logical sector of start of MDFAT
	BYTE	nLog2cbPerSec;		// Log base 2 of cbPerSec
	UINT	csecMDReserved;	// Number of sectors before DOS BOOT sector
	UINT	secRootDirStart;	//	Logical sector of start of root directory
	UINT	secHeapStart;		// Logical sector of start of sector heap
	UINT	cluFirstData;		// Number of MDFAT entries (clusters) occupied
									// by DOS boot sector, reserved area, and root dir
	BYTE	cpageBitFAT;		// Count of 2K pages in the BitFAT
	UINT	RESERVED1;
	BYTE	nLog2csecPerClu;	// Log base 2 of csecPerClu
	UINT	RESERVED2;
	ULONG	RESERVED3;
	ULONG	RESERVED4;
	BYTE	f12BitFAT;			// 1 => 12-bit FAT, 0 => 16-bit FAT
	UINT	cmbCVFMax;			// Maximum CVF size, in megabytes (1024*1024)
} MDBPB;

// structure for region info inside compressed drive data
typedef struct {
	unsigned long StartSector;
	unsigned int CurrentSector;
	unsigned int BufferOffset;
} CDBUFINFO;

typedef struct {
   MDBPB BPBData;				// BPB data as above
   UCHAR cCDLetter;			// current compressed drive letter, 0 if none
   UCHAR cDriveType;       // drive type:  1=DBLSPACE, 2=SuperStor
   UCHAR cHostDrive;       // host drive number (0=A etc.)
   UINT nCDFH;					// file handle for CVF
//   UINT UnCompSecPerClu;   // uncompressed sectors per cluster
   UINT UnCompSecPerClu;   // uncompressed sectors per cluster
   UINT UnCompBytesPerClu; // uncompressed bytes per cluster
   BYTE nLog2cMDFATPerSec;	// log base 2 of MDFAT entries per sector
   BYTE nLog2cDirPerSec;	// log base 2 of directory entries per sector
   UINT cbPerSecMask;		// mask for bytes per sector - 1
   UINT cMDFATPerSecMask;	// mask for MDFAT entries per sector - 1
   UINT cDirPerSecMask;	   // mask for directory entries per sector - 1
   UINT nBufSeg;				// buffer segment
	UINT nDirSec;				// current relative directory sector
	CDBUFINFO DirBuf;			// directory buffer info
	CDBUFINFO DOSFATBuf;		// DOS FAT buffer info
	CDBUFINFO FileMDFATBuf;	// file MDFAT buffer info
	CDBUFINFO DirMDFATBuf;	// directory MDFAT buffer info
} COMPDRIVEINFO;

// MDFAT entry structure
typedef struct { /* mfe */
    unsigned long secStart  : 21; // Starting sector in sector heap
    unsigned int  fRESERVED :  1; // Reserved for future use
    unsigned int  csecCoded :  4; // Length of compressed data (in sectors)
    unsigned int  csecPlain :  4; // Length of original data (in sectors)
    unsigned int  fUncoded  :  1; // TRUE => coded data is NOT CODED
    unsigned int  fUsed 	 :  1; // TRUE => MDFAT entry is allocated
} MDFATENTRY;

// NOTE: C6 does not want to treat MDFATENTRY as a 4 byte structre, so
//       we hard-code the length here, and use explicit masks and shifts
//       in code that manipulates MDFATENTRYs.

#define cbMDFATENTRY   4

