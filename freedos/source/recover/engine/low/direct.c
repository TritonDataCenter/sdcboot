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

    reserved = GetReservedSectors(handle);
    if (reserved == 0) return FALSE;

    fats = GetNumberOfFats(handle);
    if (fats == 0) return FALSE;

    SectorsPerFat = GetSectorsPerFat(handle);
    if (SectorsPerFat == 0) return FALSE;

    return (SECTOR) reserved + ((SECTOR)fats * SectorsPerFat);
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

   if (handle->dirbegin == 0)
   {
      handle->dirbegin = GetDirectoryStart(handle);
      if (!handle->dirbegin) return FALSE;
   }

   /* We are using a sector as an array of directory entries. */
   buf = (struct DirectoryEntry*)AllocateSector(handle);
   if (buf == NULL) return FALSE;

   if (ReadSectors(handle, 1, handle->dirbegin + (index / ENTRIESPERSECTOR),
                   buf) == -1)
   {
      FreeSectors((SECTOR*)buf);
      return FALSE;
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
	return FALSE;
    
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

     if (!dirstart) return FALSE;

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
** Only FAT12/16
********************************************************************/

BOOL IsRootDirPosition(RDWRHandle handle, struct DirectoryPosition* pos)
{
    struct DirectoryPosition pos1;
    unsigned short NumberOfRootEntries;

    NumberOfRootEntries = GetNumberOfRootEntries(handle);
    if (NumberOfRootEntries == 0) return FALSE;

    if (!GetRootDirPosition(handle, NumberOfRootEntries-1, &pos1))
       return -1;

    if ((pos->sector <= pos1.sector) && (pos->offset <= pos1.offset))
       return TRUE;
    else
       return FALSE;
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
   entry->firstclustLo = cluster & 0xFFFF;
   entry->firstclustHi = cluster >> 16;
}

/* Depricated and does not work!

void UnPackTimeDateStamp(struct tm* time, short timestamp, short datestamp)
{
     time->tm_sec  = (timestamp &   0xf) * 2;
     time->tm_min  = timestamp &  0x3f0;
     time->tm_hour = timestamp & 0x7c00;

     time->tm_mday = datestamp &    0xf;
     time->tm_mon  = datestamp &   0xf0;
     time->tm_year = (datestamp & 0xf00+DSY)-1900;
}

void PackTimeDateStamp(struct tm* time, short* timestamp, short* datestamp)
{
     *timestamp  = time->tm_sec;
     *timestamp += time->tm_min  << 5;
     *timestamp += time->tm_hour << 11;

     *datestamp  = time->tm_mday;
     *datestamp += time->tm_mon          <<  5;
     *datestamp += ((time->tm_year)-DSY) << 10;
}
*/

/*******************************************************************
**                        EntryLength
********************************************************************
** Returns the number of elementary entries in a, possibly LFN, entry.
********************************************************************/
unsigned long EntryLength(struct DirectoryEntry* entry)
{
   int counter = 1;

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
********************************************************************/

CLUSTER LocatePreviousDir(RDWRHandle handle, CLUSTER firstdircluster)
{
   SECTOR* sectbuf;
   SECTOR sector;
      
   if (firstdircluster) return FALSE;
           
   sectbuf = AllocateSector(handle);
   if (!sectbuf) return FALSE;
   
   sector = ConvertToDataSector(handle, firstdircluster);
   if (!sector)
   {        
      FreeSectors(sectbuf);
      return FALSE;
   }
   
   if (!ReadDataSectors(handle, 1, sector, (void*) sectbuf))
   {
      FreeSectors(sectbuf);
      return FALSE;
   }   
   
   if (IsPreviousDir(((struct DirectoryEntry*) sectbuf)[1]))
   {
      FreeSectors(sectbuf);
      return GetFirstCluster(&(((struct DirectoryEntry*) sectbuf)[1]));
   }
   
   SetFTEerror(FTE_FILESYSTEM_BAD);
   
   FreeSectors(sectbuf);
   return FALSE;
}

#if 0 /* Not used */

/*
   This function assumes that the two directories are in
   the same sector and that they are fully contained
   whitin this sector.
   
   Also: Not tested
*/

static int SwapEntriesInSector(RDWRHandle handle,
                               struct DirectoryPosition* pos1, 
                               struct DirectoryPosition* pos2)
{   
   int retVal;
   int len1, len2;
   struct DirectoryEntry *entry1, *entry2;
   char* buffer;
          
   /* Check wether both entries are in the same sector. */
   if (pos1->sector != pos2->sector)
      return FALSE;
    
   buffer = (char*)AllocateSector(handle);
   if (!buffer) return FALSE; 
   
   if (ReadSectors(handle, 1, pos1->sector, buffer) == -1)
   {
      FreeSectors(buffer);
      return FALSE;
   }
   
   entry1 = (struct DirectoryEntry*) &buffer[pos1->offset << 5];
   entry2 = (struct DirectoryEntry*) &buffer[pos2->offset << 5];
      
   len1 = EntryLength(entry1); 
   len2 = EntryLength(entry2);
   
   SwapBufferParts((char*)entry1, (char*)entry1+(len1 << 5),
                   (char*)entry2, (char*)entry2+(len2 << 5)) ;
   
   retVal = !(WriteSectors(handle, 1, pos1->sector, buffer, WR_DIRECT) == -1);
   
   FreeSectors(buffer);
   return retVal;
}

/*
   This function assumes that both entries are equal in length.
   And that they are fully contained whitin their sectors.
*/
static int SwapEntriesSameSize(RDWRHandle handle,
                               struct DirectoryPosition* pos1, 
                               struct DirectoryPosition* pos2)
{
   int len;
   char* sector1[BYTESPERSECTOR], *sector2[BYTESPERSECTOR];   

   sector1 = (char*)AllocateSector(handle);
   if (!sector) return FALSE;
   
   sector2 = (char*)AllocateSector(handle);
   if (!sector2)
   {
      FreeSectors(sector1);
      return FALSE;
   }
   
   if (ReadSectors(handle, 1, pos1->sector, sector1) == -1)
   {   
      FreeSectors(sector1);
      FreeSectors(sector2);
      return FALSE;
   }
   if (ReadSectors(handle, 1, pos2->sector, sector2) == -1)
   {
      FreeSectors(sector1);
      FreeSectors(sector2);
      return FALSE;
   }

   SwapBuffer(sector1[pos1->offset << 5], sector2[pos2->offset << 5],
              EntryLength((struct DirectoryEntry*) 
                          &sector1[pos1->offset << 5]));   

   if (WriteSectors(handle, 1, pos1->sector, sector1, WR_DIRECT) == -1)
   {
      FreeSectors(sector1);
      FreeSectors(sector2);
      return FALSE;
   }      
   if (WriteSectors(handle, 1, pos2->sector, sector2, WR_DIRECT) == -1)
   {
      FreeSectors(sector1);
      FreeSectors(sector2);
      return FALSE;
   }
   return TRUE;
}
#endif
