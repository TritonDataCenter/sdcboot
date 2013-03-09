#ifdef __DJGPP__	/* only for DOS */
/*
   Lowfat32.c - Low level FAT32 api calls (DJGPP version).

   Copyright (C) 2000, 2004 Imre Leber

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/

#include <dos.h>
#include <string.h>
#include <dpmi.h>
#include "common.h"
#include "lowfat32.h"

#define CARRY 1

struct DISKIO
{
    __u32  diStartSector;     /* Sector number to start */
    __u16  diSectors;         /* Number of sectors      */
    __u32  diBuffer;          /* pointer to buffer      */
} __attribute__((packed));

struct xdfree
{
    __u16  size;
    __u16  version;
    __u32  SectorsPerCluster;
    __u32  BytesPerSector;
    __u32  AvailableClusters;
    __u32  TotalClusters;
    __u32  AvailablePhysicalSectors;
    __u32  TotalPhysicalSectors;
    __u32  AvailUnits;
    __u32  TotalUnits;
    __u8   Reserved[8];
} __attribute__((packed));

/******************************************************************
**                     ExtendedAbsReadWrite
*******************************************************************
** Call to extended absolute read/write function in C
**
** This calls needs the FAT32 api. Because this may run on
** non-FAT32 enabled kernels you should first check whether
** the FAT32 api is available. Drive number is 1 based (1 = A:).
**
** Also works on FAT12/16 for FAT32 kernels. Area is odd for writes.
*******************************************************************/

static int iosel = 0;
static int bufsel = 0;
static int ioseg = 0;
static int bufseg = 0;

int ExtendedAbsReadWrite(int drive, int nsects, unsigned long lsect,
			 void* buffer, unsigned area, unsigned BytesPerSector)
{
    __dpmi_regs regs;
    struct DISKIO io;

    /* Allocate dos memory (only once) */
    if (!bufsel)
        if ((bufseg = __dpmi_allocate_dos_memory(((nsects * BytesPerSector) + 15) >> 4 , &bufsel)) == -1)
            return -1;

    if (!iosel)
        if ((ioseg = __dpmi_allocate_dos_memory((sizeof(struct DISKIO)+15)>>4, &iosel)) == -1)
        {
            __dpmi_free_dos_memory(bufsel);
	    return -1;
        }

    /* Copy buffer to conventional memory if WRITE */
    if (area & 1)
        dosmemput(buffer, nsects * BytesPerSector, bufseg << 4);

    /* Copy diskio struct to conventional memory */
    io.diStartSector = lsect;
    io.diSectors     = nsects;
    io.diBuffer      = bufseg << 16;	/* was * 16 ??? */

    dosmemput(&io, sizeof(struct DISKIO), ioseg << 4);

    regs.x.ax = 0x7305;
    regs.x.cx = 0xffff;
    regs.x.si = (unsigned short)area;
    regs.h.dl = (unsigned char) drive;
    regs.x.bx = 0;
    regs.x.ds = ioseg;

    __dpmi_int(0x21,&regs);

    /* Copy conventional memory to buffer if READ */
    if ((area & 1) == 0)
        dosmemget(bufseg << 4, nsects * BytesPerSector, buffer);

    /* __dpmi_free_dos_memory(bufsel); */
    /* __dpmi_free_dos_memory(iosel); */

    return (regs.x.flags & CARRY) ? -1 : (nsects * BytesPerSector);
}

/******************************************************************
**                     GetFAT32SectorSize
*******************************************************************
** Returns the size of a sector on the drive in question, and the
** number of clusters and sectors . Can use both the FAT32 and FAT1x api.
*******************************************************************/

unsigned GetFAT32SectorSize(int drivenum, __u32 * clusters, __u32 * sectors)
{

    __dpmi_regs regs;
    int hasFAT32 = 1;
    unsigned retVal = 0;

    regs.x.ax = 0x7300; /* get FAT32 flag */
    regs.x.dx = drivenum+1;
    regs.x.cx = 1; /* dirty buffers flag */
    __dpmi_int(0x21,&regs);
    if (regs.x.ax == 0x7300)
        hasFAT32 = 0; /* unimplemented: AL 0, no carry */

    struct diskfree_t df;

    if (_dos_getdiskfree(drivenum+1, &df)==0) /* nonzero on error! */
    {
	*clusters = df.total_clusters;
	*sectors = (__u32)(df.total_clusters) * df.sectors_per_cluster;
	retVal = df.bytes_per_sector;
	if (retVal == 0xffff)	/* "invalid drive" (e.g. non-FAT) */
            retVal = 0;
    } /* else leave retVal 0, unable to get info */

    if (hasFAT32==0)
        return retVal; /* FAT16 kernel, cannot check FAT32 style */

    /* if FAT32 kernel, ask FAT32 API for the real cluster / sector count */
    int drvseg, drvsel;
    int freeseg, freesel;
    char drive[4];
    struct xdfree free;

    *clusters = 0;
    *sectors = 0;

    drvseg = __dpmi_allocate_dos_memory(1 ,&drvsel);
    if (drvseg == -1)
        return 0;

    freeseg = __dpmi_allocate_dos_memory(((sizeof(struct xdfree))+15)>>4, &freesel);
    if (freeseg == -1)
    {
        __dpmi_free_dos_memory(drvsel);
	return 0;
    }

    strcpy(drive, "?:\\");
    drive[0] = (char) drivenum + 'A';
    dosmemput(drive, 4, drvseg << 4);

    free.version = 0;
    dosmemput(&free, sizeof(struct xdfree), freeseg << 4);

    regs.x.ax = 0x7303;
    regs.x.ds = drvseg;
    regs.x.dx = 0;
    regs.x.es = freeseg;
    regs.x.di = 0;
    regs.x.cx = sizeof(struct xdfree);

    __dpmi_int(0x21,&regs);

    if (regs.x.flags & 1)	/* if carry set */
    {
	retVal = 0;		/* not a suitable drive */
    }
    else /* note: if AL was 0, we probably have a non FAT32 kernel */
    {
        dosmemget(freeseg << 4, sizeof(struct xdfree), &free);
        *clusters = (__u32) free.TotalUnits; /* or TotalClusters ? */
        *sectors = (__u32) free.TotalPhysicalSectors;
        retVal = (__u16) free.BytesPerSector;
    }

    __dpmi_free_dos_memory(drvsel);
    __dpmi_free_dos_memory(freesel);

    return retVal;
}
#endif
