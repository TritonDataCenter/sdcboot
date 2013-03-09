/*
    SWSUBST: Alternate CDS manipulator for MS-DOS
    Copyright (C) 1995-97,2002 Steffen Kaiser

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* $Id: cds.h 3.2 2002/11/28 06:20:28 ska Exp ska $

*/

#include <portable.h>

#if defined(__POWERC) || (defined(__TURBOC__) && !defined(__BORLANDC__))
 #define FAR far
#else
 #define FAR _far
#endif

#ifndef MK_FP
    #define MK_FP(seg,off) ((void FAR *)(((long)(seg) << 16)|(unsigned)(off)))
#endif
#ifndef MK_SEG
	#define MK_SEG(fp) ((unsigned)(((long)(fp) >> 16) & 0xffff))
#endif
#ifndef MK_OFF
	#define MK_OFF(fp) ((unsigned)((long)(fp) & 0xffff))
#endif


#define NETWORK     (1 << 15)
#define PHYSICAL    (1 << 14)
#define JOIN        (1 << 13)
#define SUBST       (1 << 12)
#define HIDDEN		(1 << 7)

//typedef unsigned char byte;
//typedef unsigned word;
//typedef unsigned long dword;

extern byte lastdrv;
extern byte FAR *nrJoined;
#define IFS void 

/* one byte alignment */

#if defined(_MSC_VER) || defined(_QC) || defined(__WATCOM__)
    #pragma pack(1)
#elif defined(__ZTC__)
    #pragma ZTC align 1
#elif defined(__TURBOC__) && (__TURBOC__ > 0x202)
    #pragma option -a-
#endif

typedef struct drv {	
	struct drv FAR *nxt;
	unsigned DAttr;
	unsigned OffStrat;
	unsigned OffInter;
	union {
		struct {
			unsigned char drives;
			char dummy[7];
		} block;
		char chr[8];
	} drvr;

	/* for CD-ROM §nly */
	word dummy;
	byte drv;
	byte units;
	char version[6];
} DRV;

extern DRV FAR *firstDRV;

typedef struct dpb {
    byte drive;
    byte unit;
    unsigned bytes_per_sect;
    byte sectors_per_cluster;       // plus 1
    byte shift;                     // for sectors per cluster
    unsigned boot_sectors;          
    byte copies_fat;
    unsigned max_root_dir;
    unsigned first_data_sector;
    unsigned highest_cluster;
    union {
        struct {
            unsigned char sectors_per_fat;
            unsigned first_dir_sector;
            DRV FAR *device_driver;
            byte media_descriptor;
            byte access_flag;
            struct dpb FAR *next;
            unsigned long reserved;
            } dos3;
        struct {
            unsigned sectors_per_fat;       // word, not byte!
            unsigned first_dir_sector;
            DRV FAR *device_driver;
            byte media_descriptor;
            byte access_flag;
            struct dpb FAR *next;
            unsigned long reserved;
            } dos4;
        } vers;
    } DPB;

extern DPB FAR *firstDPB;

#pragma pack(1)

typedef struct {
    byte current_path[67];  // current path
    word flags;             // NETWORK, PHYSICAL, JOIN, SUBST, HIDDEN
    DPB  FAR *dpb;          // pointer to Drive Parameter Block
    union {
        struct {
            word start_cluster; // root: 0000; never accessed: FFFFh
            dword unknown;
            } LOCAL;        // if (! (cds[drive].flags & NETWORK))
        struct {
            dword redirifs_record_ptr;
            word parameter;
            } NET;          // if (cds[drive].flags & NETWORK)
        } u;
    word backslash_offset;  // offset in current_path of '\\'
    // DOS4 fields for IFS
    byte dummy1;
    IFS FAR *ifsdrv;
    word dummy2;
    } CDS;

#pragma pack(1)

typedef struct {
    byte type;          /* 'M'=in chain; 'Z'=at end */
    word owner;         /* PSP of the owner */
    word size;          /* in 16-byte paragraphs */
    byte unused[3];
    byte name[8];
    } MCB;
extern MCB FAR* firstMCB;
extern MCB FAR* firstMCBinUMB;

/* standard alignment */

#if defined (_MSC_VER) || defined(_QC) || defined(__WATCOMC__)
 #pragma pack()
#elif defined (__ZTC__)
 #pragma ZTC align
#elif defined(__TURBOC__) && (__TURBOC__ > 0x202)
 #pragma option -a.
#endif


#ifdef __cplusplus
extern "C" {
#endif
CDS FAR *cds(unsigned drive);
#ifdef __cplusplus
};
#endif
