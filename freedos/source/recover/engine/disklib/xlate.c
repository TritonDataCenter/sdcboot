/*
 * xlate.c      logical drive translations (FAT12 and FAT16)
 *
 * This file is part of the BETA version of DISKLIB
 * Copyright (C) 1998, Gregg Jennings
 *
 * See README.TXT for information about re-distribution.
 * See DISKLIB.TXT for information about usage.
 *
 * 09-Dec-1998  greggj  physical_p(); long casts in logical_sector()
 * 27-Nov-1998  greggj  `hidden' was int in logical_sector(); include disklib.h
 * 26-Nov-1998  greggj  disk_size32()
 * 12-Nov-1998  greggj  physical()
 *
 */

/*
   Stripped down by Imre Leber,

   visit http://www.diskwarez.com for the full version of this library
*/


#include "dosio.h"
#include "disklib.h"

/*
 *  max_track       calculate the maximum track number from DEVICEPARAMS
 *
 */

int max_track(struct DEVICEPARAMS *dp)
{
long sectors;

    sectors = (dp->num_sectors) ? dp->num_sectors : dp->total_sectors;
    sectors += dp->hidden_sectors;
    sectors /= dp->num_heads;
    sectors /= dp->secs_track;
    return (int)sectors - 1;
}