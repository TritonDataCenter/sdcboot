/*    
   Lowfat32.c - Low level FAT32 api calls.
   
   Copyright (C) 2000 Imre Leber

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

#include <assert.h>
#include <dos.h>
#include <string.h>

struct DISKIO
{
    unsigned long diStartSector;     /* Sector number to start.    */
    unsigned diSectors;              /* Number of sectors.         */
    unsigned long diBuffer;          /* Segment address of buffer. */
};

struct xdfree
{
    unsigned      size;
    unsigned      version;
    unsigned long SectorsPerCluster;
    unsigned long BytesPerSector;
    unsigned long AvailableClusters;
    unsigned long TotalClusters;
    unsigned long AvailablePhysicalSectors;
    unsigned long TotalPhysicalSectors;
    unsigned long AvailUnits;
    unsigned long TotalUnits;
    char          Reserved[8];
};

/******************************************************************
**                     ExtendedAbsReadWrite
*******************************************************************
** Call to extended absolute read/write function in C
**
** This calls needs the FAT32 api, because this may run on a
** non-FAT32 enabled kernel you should first check wether
** the FAT32 api is available.
**
** Also works on FAT12/16.
*******************************************************************/

int ExtendedAbsReadWrite(int drive, int nsects, unsigned long lsect,
			 void* buffer, unsigned area)
{
    union  REGS inregs, outregs;
    struct SREGS sregs;
    struct DISKIO io;
	
    void far* fpBuffer = (void far*) buffer;
    struct DISKIO far* fpio = (struct DISKIO far*) &io;

    assert(nsects && buffer);

    io.diStartSector = lsect;
    io.diSectors     = nsects;
    io.diBuffer      = (unsigned long) fpBuffer;

    inregs.x.ax = 0x7305;
    inregs.x.cx = 0xFFFF;
    inregs.x.si = area;
    inregs.h.dl = (unsigned char) drive;
    inregs.x.bx = FP_OFF(fpio);
    sregs.ds    = FP_SEG(fpio);

    intdosx(&inregs, &outregs, &sregs);

    return (outregs.x.cflag) ? -1 : 0;
}

/******************************************************************
**                     GetFAT32SectorSize
*******************************************************************
** Returns the size of a FAT32 sector.
**
** This function can be used to see wether the FAT32 api is available.
*******************************************************************/

unsigned GetFAT32SectorSize(int drivenum)
{
    union REGS inregs, outregs;
    struct SREGS sregs;

    char drive[4], far* fpDrive = (char far*) &drive;
    struct xdfree free, far* fpFree = (struct xdfree far*) &free;

    inregs.x.ax = 0x7303;

    memcpy(drive, "?:\\", 4);
    drive[0] = (char) (drivenum+'A');

    sregs.ds    = FP_SEG(fpDrive);
    inregs.x.dx = FP_OFF(fpDrive);

    free.version = 0;
    sregs.es     = FP_SEG(fpFree);
    inregs.x.di  = FP_OFF(fpFree);
    inregs.x.cx  = sizeof(struct xdfree);

    intdosx(&inregs, &outregs, &sregs);

    if (outregs.x.cflag)
       return 0;
    else if (outregs.h.al == 0)     /* Old DOS */
       return 0;
    else
       return (unsigned) free.BytesPerSector;
}
