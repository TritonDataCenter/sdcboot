/*
 * dos\getdisk.c    get disk parameters
 *
 * This file is part of the BETA version of DISKLIB
 * Copyright (C) 1998, Gregg Jennings
 *
 * See README.TXT for information about re-distribution.
 * See DISKLIB.TXT for information about usage.
 *
 * 20-Dec-1998  greggj  fixed byte count for reading Media Descriptor
 * 12-Nov-1998  greggj  added better media type checking in boot_...()
 * 11-Nov-1998  greggj  added function disk_getparamblk()
 *
 */

/*
   Stripped down by Imre Leber,

   visit http://www.diskwarez.com for the full version of this library
*/


#include <string.h>     /* memset, memcpy */

#include "dosio.h"
#include "disklib.h"

static int boot_parameters(int disk, struct DEVICEPARAMS *dp);

/*
 * disk_get_params      get disk (DOS) parameters
 *
 * Note: This function works on NTFS drives.
 *
 */

extern int disk_getparams(int disk, struct DEVICEPARAMS *dp)
{
int i;

    memset(dp,0,sizeof(struct DEVICEPARAMS));

    /* TODO: check for remote */
    /* TODO: check for removable */

    dp->special = 1;

    if ((i = dos_ioctl(DOS_DEV_IOCTL,DOS_MINOR_GET_DEVICE,disk,dp)) != DEVICE_OK)
        if ((i = boot_parameters(disk,dp)) == DISK_OK)
            dp->cylinders = max_track(dp);

    if (i == DEVICE_OK)
        i = DISK_OK;

    return i;
}

/*
 * boot_parameters      get disk (BOOT) parameters
 *
 */

static int boot_parameters(int disk, struct DEVICEPARAMS *dp)
{
int i;
struct BOOT boot;
unsigned short tmp;
unsigned char buf[512];

    memset(buf,0,512);
    i = disk_read(disk,0,buf,1);

    tmp = (unsigned short)*(unsigned char*)(buf+21);    /* extract md */

    if ((tmp < 0xf8 || tmp > 0xff) && tmp != 0xf0) {
	return DOS_ERR;
    }

    /* ramdrives might not have the AA55 signature */
    /*  (well, RAMDRIVE.SYS does not have the AA55 signature) */
    /* and non-DOS disks can have the AA55 signature */

    tmp = *(unsigned short*)(buf+11);       /* extract sector size */

    if ((buf[0]!=0xeb && buf[0]!=0xe9 && buf[0]!=0) ||
       (buf[2]!=0x90 && buf[2]!=0) || tmp != 512) {
	return DOS_ERR;
    }

    if (buf[510] != 0x55 && buf[511] != 0xAA) {
	return DOS_ERR;
    }

    /*
        Note that the INT 25h < 32MB function is used here without
        testing for the drive size. Because boot_parameters() is
        called only when disk_getparams() fails, and that happens
        either on a ramdrive or other installable driver that is
        rather old and/or lame and this will work, or on a drive
        where any DOS disk I/O will fail.

        This has yet to be tested on a Zip/Jazz/other removable
        device.
    */

    if (i == DISK_OK) {
        memset(&boot,0,sizeof(struct BOOT));
        memcpy(&boot,buf,sizeof(struct BOOT));
        memcpy(&dp->sec_size,&boot.sec_size,sizeof(struct BPB));
    }

    return i;
}

