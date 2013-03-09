/*
 * dos\read.c       FAT12/16/32 disk read functions
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


#include <dos.h>        /* REGS, SREGS, etc. */

#include "dosio.h"
#include "disklib.h"

enum { DISK_READ = 0, DISK_WRITE };

/*
 * disk_read        absolute disk read (INT 25h)
 *
 * Note: This function works only on FAT12/16 drives less than
 *       32MB in size on all Microsoft Platforms.
 */


extern int disk_read(int disk, long sector, void *buffer, int nsecs)
{
union REGS regs;
struct SREGS sregs;

    regs.x.ax = disk;
    regs.x.dx = (unsigned int)sector;      /* truncate is okay */
    regs.x.cx = nsecs;
    regs.x.bx = FP_OFF(buffer);
    sregs.ds = FP_SEG(buffer);

    int86x(0x25,&regs,&regs,&sregs);

    if (regs.x.cflag) {
   return DOS_ERR;                 /*  for later retrieval */
    }

    return DISK_OK;
}

/*
 * disk_read_ext    absolute disk read (INT 25h), > 32MB
 *
 * Note: This function works only on FAT12/16 drives greater
 *       than 32MB in size on all Microsoft Platforms.
 *
 */

extern int disk_read_ext(int disk, long sector, void *buffer, int nsecs)
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

    int86x(0x25,&regs,&regs,&sregs);

    if (regs.x.cflag) {
   return DOS_ERR;                 /*  for later retrieval */
    }

    return DISK_OK;
}

