/*    
   Descchk.c - descriptor check for floppies.

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

#include "fte.h"
#include "checkfat.h"

struct KnownValues {
   unsigned char  SectorsPerCluster; /* sectors per cluster.              */
   unsigned short ReservedSectors;   /* number of reserved sectors.       */
   unsigned short NumberOfFiles;     /* number of files or directories in */
                                     /* the root directory.               */
   unsigned short NumberOfSectors;   /* number of sectors in the volume.  */
   unsigned short SectorsPerFat;     /* number of sectors per fat.        */
   unsigned short SectorsPerTrack;   /* sectors per track.                */
   unsigned short Heads;             /* number of read/write heads.       */
};

/*                                      SPC RS NOF   NOS  SPF SPT H               */
static struct KnownValues ValuesFEh  = {1,  1,  64,  320, 1,   8, 1};
static struct KnownValues ValuesFFh  = {2,  1, 112,  640, 1,   8, 2};
static struct KnownValues ValuesFCh  = {1,  1,  64,  360, 2,   9, 1};
static struct KnownValues ValuesFDh  = {2,  1, 112,  720, 2,   9, 2};
static struct KnownValues ValuesF9ah = {1,  1, 224, 2400, 7,  15, 2};
static struct KnownValues ValuesF9bh = {2,  1, 112, 1440, 3,   9, 2};
static struct KnownValues ValuesF0h  = {1,  1, 224, 2880, 9,  18, 2};

static int CheckKnownFormats(struct BootSectorStruct* boot,
                             struct KnownValues* values);

int DescriptorCheck(RDWRHandle handle)
{
    struct BootSectorStruct boot;

    if (!ReadBootSector(handle, &boot)) RETURN_FTEERR(FALSE);;

    switch (boot.descriptor)
    {
       case 0xFE:
            return CheckKnownFormats(&boot, &ValuesFEh);

       case 0xFF:
            return CheckKnownFormats(&boot, &ValuesFFh);

       case 0xFC:
            return CheckKnownFormats(&boot, &ValuesFCh);

       case 0xFD:
            return CheckKnownFormats(&boot, &ValuesFDh);

       case 0xF9:
            if (!CheckKnownFormats(&boot, &ValuesF9ah))
               return CheckKnownFormats(&boot, &ValuesF9bh);
            else
               return TRUE;

      case 0xF0:
           return CheckKnownFormats(&boot, &ValuesF0h);
    }

    return TRUE;
}

static int CheckKnownFormats(struct BootSectorStruct* boot,
                             struct KnownValues* values)
{
    if ((boot->BytesPerSector    != BYTESPERSECTOR)            ||
        (boot->SectorsPerCluster != values->SectorsPerCluster) ||
        (boot->ReservedSectors   != values->ReservedSectors)   ||
        (boot->NumberOfFiles     != values->NumberOfFiles)     ||
        (boot->NumberOfSectors   != values->NumberOfSectors)   ||
        (boot->SectorsPerFat     != values->SectorsPerFat)     ||
        (boot->SectorsPerTrack   != values->SectorsPerTrack)   ||
        (boot->Heads             != values->Heads))
    {
       RETURN_FTEERR(FALSE);
    }
    else
       return TRUE;
}
