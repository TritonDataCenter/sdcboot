/*    
   Descifat.c - check descriptor in fat.

   Copyright (C) 2000, 2002 Imre Leber

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
/*
    8/10/02
    
      Adjusted for use in chkdsk
*/

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\errmsgs\errmsgs.h"

/****************************** Checking *********************************/

/*************************************************************************
**                           CheckDescriptorInFat
**************************************************************************
** Checks the descriptor in the FAT against the one in the boot sector. 
***************************************************************************/
 
RETVAL CheckDescriptorInFat(RDWRHandle handle)
{
    struct  BootSectorStruct boot;
    SECTOR  fatstart;
    char    buffer[BYTESPERSECTOR], *p;

    if (!ReadBootSector(handle, &boot)) return ERROR;

    fatstart = GetFatStart(handle);
    if (!fatstart) return ERROR;

    if (ReadSectors(handle, 1, fatstart, buffer) == -1) return FALSE;

    if (boot.descriptor == buffer[0])
    {
       return SUCCESS;
    }
    else
    {
        ReportFileSysError("Wrong descriptor value in FAT",
                           0,
                           &p,
                           0,
                           FALSE);
    }

    return FAILED;
}

/****************************** Fixing ***********************************/

/*************************************************************************
**                           PlaceDescriptorInFat
**************************************************************************
** Makes sure the descriptor in the FAT is equal to the one in the boot 
** sector. 
***************************************************************************/

RETVAL PlaceDescriptorInFat(RDWRHandle handle)
{
    struct  BootSectorStruct boot;
    SECTOR  fatstart;
    char    buffer[BYTESPERSECTOR], *p;

    if (!ReadBootSector(handle, &boot))
       return ERROR;

    fatstart = GetFatStart(handle);
    if (!fatstart) return ERROR;

    if (ReadSectors(handle, 1, fatstart, buffer) == -1)
       return ERROR;

    if (buffer[0] != boot.descriptor)
    {
        ReportFileSysError("Wrong descriptor value in FAT",
                           0,
                           &p,
                           0,
                           FALSE);

       buffer[0] = boot.descriptor;

       if (WriteSectors(handle, 1, fatstart, buffer, WR_FAT) == -1)
          return ERROR;

       if (!BackupFat(handle))
	  return ERROR;
    }

    return SUCCESS;
}