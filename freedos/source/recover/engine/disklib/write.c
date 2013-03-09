/*
 * dos\write.c      FAT12/16/32 disk write
 *
 * This file is part of the BETA version of DISKLIB
 * Copyright (C) 1998, Gregg Jennings
 *
 * See README.TXT for information about re-distribution.
 * See DISKLIB.TXT for information about usage.
 *
 */

/*
   Stripped down by Imre Leber,

   visit http://www.diskwarez.com for the full version of this library
*/


#include <dos.h>

#include "dosio.h"
#include "disklib.h"

enum { DISK_READ = 0, DISK_WRITE };

/*
 * disk_write_ext   absolute disk write (INT 26h), > 32MB
 *
 * Note: For hard disks, Windows 95 will trap this function and ask
 *       to ignore the write or abort the program. If ignored the
 *       write fails with a Write Protected error. But if the buffer
 *       data is the same as the sector data this function will
 *       appear to be successful.
 */

extern int disk_write_ext(int disk, long sector, void *buffer, int nsecs)
{
union REGS regs;
struct SREGS sregs;
struct DCB Dcb;
struct DCB *dcb = &Dcb;

    /*
        Microsoft bombs if the address of operator is used in the
        FP_OFF() or FP_SEG() macros so a pointer is kludged here so
        there is no need for prepocessor conditionals for MS.
    */

    regs.x.ax = disk;
    dcb->sector = sector;
    dcb->number = (unsigned short)nsecs;
    dcb->buffer = (unsigned char far*)buffer;
    regs.x.cx = 0xffff;
    regs.x.bx = FP_OFF(dcb);
    sregs.ds = FP_SEG(dcb);

    int86x(0x26,&regs,&regs,&sregs);

    if (regs.x.cflag) {
	return DOS_ERR;                 /*  for later retrieval */
    }

    return DISK_OK;
}