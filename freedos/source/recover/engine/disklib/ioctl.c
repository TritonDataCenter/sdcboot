/*
 * dos\ioctl.c      perform DOS IOCTL calls
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

/*
 * dos_ioctl        perform DOS IOCTL call (INT 21h/440Dh)
 *
 * Note: Only SUBFUNCTION 0x08 category codes are supported.
 *       Functions that have output data are not supported.
 *
 */

extern int dos_ioctl(DOS_SUBFUNCTION func, DOS_MINOR_CODE code, int dev,
                     void *data)
{
union REGS regs;
struct SREGS sregs;

    regs.x.ax = 0x4400 + func;
    regs.x.bx = dev + 1;

    if (code != DOS_MINOR_NONE)
        regs.x.cx = 0x0800 + code;

    if (data)
    {
        regs.x.dx = FP_OFF(data);
        sregs.ds = FP_SEG(data);
    }

    intdosx(&regs,&regs,&sregs);

    if (regs.x.cflag) {
	return DOS_ERR;
    }

    /* handle special cases */

    if (func == DOS_DEV_REMOVE)
        return regs.x.ax;

    if (func == DOS_DRV_REMOTE)
        return regs.x.dx;

    return DEVICE_OK;
}