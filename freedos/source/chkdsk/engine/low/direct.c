/*
   Direct.c - directory manipulation code.
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
   email me at: imre.leber@worldonline.be
*/

#include <assert.h>
#include <time.h>
#include <string.h>

#include "../../misc/bool.h"
#include "../header/rdwrsect.h"
#include "../header/boot.h"
#include "../header/direct.h"
#include "../header/fatconst.h"
#include "../header/fsinfo.h"
#include "../header/bufshift.h"
#include "../header/ftemem.h"
#include "../header/dataarea.h"
#include "../header/fteerr.h"

#define DSY 1980                      /* DOS standard year. */

/*******************************************************************
**                        GetDirectoryStart
********************************************************************
** Returns the start sector of the root directory
**
** Only FAT12/16
********************************************************************/

SECTOR GetDirectoryStart(RDWRHandle handle)
{
    unsigned short reserved;
    unsigned char  fats;
    unsigned long  SectorsPerFat;

    if (handle->dirbegin == 0)
    {    
	reserved = GetReservedSectors(handle);
	if (reserved == 0) RETURN_FTEERR(FALSE);

	fats = GetNumberOfFats(handle);
	if (fats == 0) RETURN_FTEERR(FALSE);

	SectorsPerFat = GetSectorsPerFat(handle);
	if (SectorsPerFat == 0) RETURN_FTEERR(FALSE);

	handle->dirbegin = (SECTOR) reserved + ((SECTOR)fats * SectorsPerFat);
    }
    
    return handle->dirbegin;
}

/*******************************************************************
**                        ReadDirEntry
********************************************************************
** Reads an entry from the root directory
**
** Only FAT12/16
********************************************************************/

BOOL ReadDirEntry(RDWRHandle handle, unsigned short index,
                 struct DirectoryEntry* entry)
{
   struct DirectoryEntry* buf;
   SECTOR dirstart;

   /* We are using a sector as an array of directory entries. */
   buf = (struct DirectoryEntry*)AllocateSector(handle);
   if (buf == NULL) RETURN_FTEERR(FALSE);
       
   dirstart = GetDirectoryStart(handle);
   if (!dirstart) RETURN_FTEERR(FALSE);

   if (ReadSectors(handle, 1, dirstart + (index / ENTRIESPERSECTOR),
                   buf) == -1)
   {
      FreeSectors((SECTOR*)buf);
      RETURN_FTEERR(FALSE);
   }

   memcpy(entry, buf + (index % ENTRIESPERSECTOR),
	  sizeof(struct DirectoryEntry));

   FreeSectors((SECTOR*)buf);
   return TRUE;
}

/*******************************************************************
**                        WriteDirEntry
********************************************************************
** Writes an entry to the root directory
**
** Only FAT12/16
********************************************************************/

BOOL WriteDirEntry(RDWRHandle handle, unsigned short index,
                   struct DirectoryEntry* entry)
{
    int WriteDirectory(RDWRHandle handle, struct DirectoryPosition* pos,
	               struct DirectoryEntry* direct);
			   
    struct DirectoryPosition pos;
	
    if (!GetRootDirPosition(handle, index, &pos))
	RETURN_FTEERR(FALSE);
    
    return WriteDirectory(handle, &pos, entry);
}

/*******************************************************************
**                        GetRootDirPosition
********************************************************************
** Retreives the directory position of a root directory entry.
**
** Only FAT12/16
********************************************************************/

BOOL GetRootDirPosition(RDWRHandle handle, unsigned short index,
                        struct DirectoryPosition* pos)
{
     SECTOR dirstart = GetDirectoryStart(handle);

     if (!dirstart) RETURN_FTEERR(FALSE);

     pos->sector = dirstart + (index / ENTRIESPERSECTOR);
     pos->offset = index % ENTRIESPERSECTOR;

     return TRUE;
}

/*******************************************************************
**                        IsRootDirPosition
********************************************************************
** Returns wheter a certain position is pointing to an entry in
** the root directory.
**
** Notice that we assume that pos point to a valid directory entry
**
** Only FAT12/16
********************************************************************/

BOOL IsRootDirPosition(RDWRHandle handle, struct DirectoryPosition* pos)
{
    struct DirectoryPosition pos1;
    unsigned short NumberOfRootEntries;

    assert(pos->sector && (pos->offset < 16));
    
    NumberOfRootEntries = GetNumberOfRootEntries(handle);
    if (NumberOfRootEntries == 0) return FALSE; /* Strange, but ultimately correct */

    if (!GetRootDirPosition(handle, NumberOfRootEntries-1, &pos1))
       RETURN_FTEERR(-1);

    if (pos->sector < pos1.sector)
	return TRUE;
    
    return ((pos->sector == pos1.sector) && (pos->offset <= pos1.offset));
}

/*******************************************************************
**                        GetFirstCluster
********************************************************************
** Calculates the full start cluster of an entry by looking at both
** parts of the start cluster in the entry structure.
**
** Meant for compatibility with FAT32
********************************************************************/

CLUSTER GetFirstCluster(struct DirectoryEntry* entry)
{
   return (CLUSTER) (entry->firstclustHi << 16) +
                    entry->firstclustLo;
}

/*******************************************************************
**                        SetFirstCluster
********************************************************************
** Sets the full start cluster of an entry by changing both
** parts of the start cluster in the entry structure.
**
** Meant for compatibility with FAT32
********************************************************************/

void SetFirstCluster(CLUSTER cluster, struct DirectoryEntry* entry)
{
   assert(entry); 
    
   entry->firstclustLo = cluster & 0xFFFF;
   entry->firstclustHi = cluster >> 16;
}

/*******************************************************************
**                        EntryLength
********************************************************************
** Returns the number of elementary entries in a, possibly LFN, entry.
********************************************************************/
unsigned long EntryLength(struct DirectoryEntry* entry)
{
   int counter = 1;

   assert(entry); 
    
   while (IsLFNEntry(entry)) 
   {
        entry++; 
        counter++;
   }

   return counter;
}

/*******************************************************************
**                        LocatePreviousDir
********************************************************************
** Returns the start cluster of the parent directory indicated by
** the given first cluster of a directory.
**
** If there was an error this function returns 0xFFFFFFFFL, not 0
** because then there would be problems with directories in the
** root directory.
********************************************************************/

CLUSTER LocatePreviousDir(RDWRHandle handle, CLUSTER firstdircluster)
{
   SECTOR* sectbuf;
   SECTOR sector;
    
   assert(firstdircluster);
           
   sectbuf = AllocateSector(handle);
   if (!sectbuf) RETURN_FTEERR(0xFFFFFFFFL);
   
   sector = ConvertToDataSector(handle, firstdircluster);
   if (!sector)
   {        
      FreeSectors(sectbuf);
      RETURN_FTEERR(0xFFFFFFFFL);
   }
   
   if (!ReadDataSectors(handle, 1, sector, (void*) sectbuf))
   {
      FreeSectors(sectbuf);
      RETURN_FTEERR(0xFFFFFFFFL);
   }   
   
   if (IsPreviousDir(((struct DirectoryEntry*) sectbuf)[1]))
   {
       CLUSTER retVal = GetFirstCluster(&(((struct DirectoryEntry*) sectbuf)[1]));
       FreeSectors(sectbuf);       
       return retVal;
   }
   
   SetFTEerror(FTE_FILESYSTEM_BAD);
   
   FreeSectors(sectbuf);
   RETURN_FTEERR(0xFFFFFFFFL);
}
