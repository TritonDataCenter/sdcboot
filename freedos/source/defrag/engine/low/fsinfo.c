/*
   FSinfo.c - FAT32 FSinfo manipulation code.
   Copyright (C) 2002 Imre Leber

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
#include <stdlib.h>

#include "../../misc/bool.h"
#include "../header/rdwrsect.h"
#include "../header/fat.h"
#include "../header/fatconst.h"
#include "../header/boot.h"
#include "../header/direct.h"
#include "../header/ftemem.h"
#include "../header/fsinfo.h"
#include "../header/fteerr.h"

/***********************************************************
**                      ReadFSInfo
************************************************************
** Reads the FSInfo structure from the volume.
**
** Only FAT32
************************************************************/

BOOL ReadFSInfo(RDWRHandle handle, struct FSInfoStruct* info)
{
    unsigned short FSinfoStart;
    
    assert(info);
    
    FSinfoStart = GetFSInfoSector(handle);
    if (!FSinfoStart)  RETURN_FTEERR(FALSE);
    
    return ReadSectors(handle, 1, FSinfoStart, info) != -1;
}

/***********************************************************
**                      WriteFSInfo
************************************************************
** Writes the FSInfo structure to the volume.
**
** Only FAT32
************************************************************/

BOOL WriteFSInfo(RDWRHandle handle, struct FSInfoStruct* info)
{
    unsigned short FSinfoStart;
    
    assert(info);
    
    FSinfoStart = GetFSInfoSector(handle);
    if (!FSinfoStart)  RETURN_FTEERR(FALSE);
    
    return WriteSectors(handle, 1, FSinfoStart, info, WR_UNKNOWN) != -1;
}

/***********************************************************
**                      CheckFSInfo
************************************************************
** Checks the signatures of the FSInfo structure.
**
** Only FAT32
************************************************************/

BOOL CheckFSInfo(RDWRHandle handle, BOOL* isfaulty)
{
    struct FSInfoStruct* info;
    
    assert(isfaulty);
    
    info = AllocateFSInfo();
    if (!info)  RETURN_FTEERR(FALSE);

    if (ReadFSInfo(handle, info))
    {
       *isfaulty = (info->LeadSignature   != FSINFO_LEAD_SIGNATURE)  ||
                   (info->StructSignature != FSINFO_STRUC_SIGNATURE) ||
                   (info->TailSignature   != FSINFO_TRAIL_SIGNATURE);
                   
       FreeFSInfo(info);            
       return TRUE;
    }
    else
    {
       FreeFSInfo(info);
        RETURN_FTEERR(FALSE);
    }
}
/***********************************************************
**                 GetFreeClusterSearchStart
************************************************************
** Returns the cluster from which to start searching for
** a free cluster.
**
** This function also returns valid data for FAT12 and FAT16
************************************************************/

BOOL GetFreeClusterSearchStart(RDWRHandle handle, CLUSTER* startfrom) 
{
    int labelsize;
    struct FSInfoStruct* info;   
	
    assert(startfrom);
    
    labelsize = GetFatLabelSize(handle);
    switch (labelsize)
    {
       case FAT12:
       case FAT16:
            *startfrom = 2;   
            return TRUE;   
               
       case FAT32:
	    {
		BOOL retVal = FALSE;
		info = AllocateFSInfo();
		if (!info)  RETURN_FTEERR(FALSE);
                           
		if (ReadFSInfo(handle, info))
		{
		   *startfrom = info->FreeSearchStart;
		   if ((*startfrom == FSINFO_DONTKNOW) ||
		       (*startfrom == 0)) // DOSFSCK peculiarity
		   {
		      *startfrom = 2;
		   }

		   retVal = TRUE;
		} 
	    
		FreeFSInfo(info);
		return retVal;	    
	    }
       default:
             RETURN_FTEERR(FALSE);
    }
}

/***********************************************************
**                   GetNumberOfFreeClusters
************************************************************
** Returns the number of free clusters.
**
** Only FAT32
************************************************************/

BOOL GetNumberOfFreeClusters(RDWRHandle handle, CLUSTER* freeclustercount)
{
    BOOL retVal = FALSE;
    struct FSInfoStruct* info;

    assert(freeclustercount);
    
    info = AllocateFSInfo();
    if (!info) RETURN_FTEERR(FALSE);
            
    if (ReadFSInfo(handle, info))
    {
       *freeclustercount = info->FreeClusterCount;   
       retVal = TRUE;
    }
                
    FreeFSInfo(info);      
    return retVal;    
}

/***********************************************************
**              WriteFreeClusterStart
************************************************************
** Writes the start of the free clusters to the FSInfo structure.
**
** Only FAT32
***********************************************************/

void WriteFreeClusterStart(struct FSInfoStruct* info, CLUSTER startfrom)
{
    assert(info);
    
    info->FreeSearchStart = startfrom;  
}

/***********************************************************
**              WriteFreeClusterCount
************************************************************
** Writes the number of free clusters to the FSInfo structure.
**
** Only FAT32
***********************************************************/

void WriteFreeClusterCount(struct FSInfoStruct* info, CLUSTER count)
{
    assert(info);
    
    info->FreeClusterCount = count;   
}
