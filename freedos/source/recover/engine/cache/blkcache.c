/*    
   Blkcache.c - sector cache for extended memory (individual blocks).

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
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "fte.h"

#include "logcache.h"
#include "wrtback.h"

struct SectorTag 
{
  SECTOR   sector;
  unsigned devid;
  unsigned age;
  BOOL     invalidated;  
  BOOL     dirty;
  unsigned area;
  
  char sectordata[BYTESPERSECTOR];
};

#define TAGSPERBLOCK 31
#define AMOFTAGSOFFSET 16382
#define NOTAGFOUND TAGSPERBLOCK * 2

static unsigned long NumberOfPhysicalBlocks;
static char* InitialisedField;

static char* DirtyField;

static unsigned LogicalTime = 0;

static void IndicateDirtyBlock(unsigned long block);
static int MakeBlockClean(unsigned long block);
static void GetNextDeviceID(unsigned long block, 
                            unsigned oldevid, unsigned* newdevid,
                            BOOL* found);

static unsigned long HashBlockNumber(SECTOR sector)
{
   return (sector / TAGSPERBLOCK) % NumberOfPhysicalBlocks;
}

static BOOL IsBlockInitialised(unsigned long block)
{
   return GetBitfieldBit(InitialisedField, block);
}

static void InitialiseBlock(unsigned long block)
{
   SetBitfieldBit(InitialisedField, block);
}

static void WriteBlockCount(char* block, char fillcount)
{
   block[AMOFTAGSOFFSET] = fillcount; 
}

static char ReadBlockCount(char* block)
{
   return block[AMOFTAGSOFFSET];
}

static void RedistributeAges(char* block)
{
   char i, tagsused;
   struct SectorTag* tag;
      
   tagsused = ReadBlockCount(block);

   tag = (struct SectorTag*) block;
   
   for (i = 0; i < tagsused; i++)
       tag[i].age /= 2;  
       
   LogicalTime = UINT_MAX / 2;
}

static char GetOldestTag(char* block)
{
   char i, tagsused, index;
   struct SectorTag* tag;
   unsigned min = UINT_MAX;
      
   tagsused = ReadBlockCount(block);
   
   tag = (struct SectorTag*) block;
   
   for (i = 0; i < tagsused; i++)
   {
       if ((!tag[i].dirty) && (tag[i].age < min))
       {
          min = tag[i].age;
          index = i; 
       }
   }
   
   if (min < UINT_MAX)
      return index;
   else
      return NOTAGFOUND;     
}

BOOL InitialisePerBlockCaching(void)
{
   NumberOfPhysicalBlocks = InitialiseLogicalCache();
   if (NumberOfPhysicalBlocks == 0)
      return FALSE;
      
   InitialisedField = CreateBitField(NumberOfPhysicalBlocks);
   if (!InitialisedField)
   {
      CloseLogicalCache();     
      return FALSE;
   }
   
   DirtyField = CreateBitField(NumberOfPhysicalBlocks);
   if (!DirtyField)
   {
      DestroyBitfield(InitialisedField);
      CloseLogicalCache();
      return FALSE;
   }

   return TRUE;
}

void ClosePerBlockCaching(void)
{
   CloseLogicalCache();
   DestroyBitfield(InitialisedField);
   DestroyBitfield(DirtyField);
}

int CacheBlockSector(unsigned devid, SECTOR sector, char* buffer,
                     BOOL dirty, unsigned area)
{
   char  tagsused = 0, i, oldest;
   char* logblock;
   struct SectorTag* tag;
   unsigned long block = HashBlockNumber(sector);

   logblock = EnsureBlockMapped(block);
   if (!logblock) return TRUE;

   if (IsBlockInitialised(block))
   {
      tagsused = ReadBlockCount(logblock);
   }
   else
   {
      InitialiseBlock(block);
      WriteBlockCount(logblock, 0);
   }
   
   if (dirty) IndicateDirtyBlock(block);
   
   /* See wether the sector is already in the cache. */
   tag = (struct SectorTag*) logblock; 
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].sector == sector) && (tag[i].devid == devid) &&
           (!tag[i].invalidated))
       {
          /* Update the cache */       
          tag[i].age = LogicalTime++;
          if (tag[i].age == UINT_MAX)
             RedistributeAges(logblock);
          tag[i].dirty = dirty;   
          tag[i].area = area;
          memcpy(tag[i].sectordata, buffer, BYTESPERSECTOR);
          return TRUE;
       }
   }
   
   /* If it is not, see wheter there are tags that were previously
      invalidated. */
   for (i = 0; i < tagsused; i++)
   {
       if (tag[i].invalidated)
       {
          tag[i].age = LogicalTime++;
          if (tag[i].age == UINT_MAX) RedistributeAges(logblock);
          tag[i].devid = devid;
          tag[i].sector = sector;
          tag[i].invalidated = FALSE;
          tag[i].dirty = dirty;
          tag[i].area = area;
          
          memcpy(tag[i].sectordata, buffer, BYTESPERSECTOR);
          return TRUE;          
       }
   }      
    
   /* If none were invalidated, then see wether the cache is already full
      and add a new tag if possible. */        
   if (tagsused < TAGSPERBLOCK)
   {
      WriteBlockCount(logblock, tagsused+1);
      tag[tagsused].age = LogicalTime++;
      if (tag[tagsused].age == UINT_MAX) RedistributeAges(logblock);
      tag[tagsused].devid = devid;
      tag[tagsused].sector = sector;
      tag[tagsused].invalidated = FALSE;
      tag[i].dirty = dirty;
      tag[i].area = area;

      memcpy(tag[tagsused].sectordata, buffer, BYTESPERSECTOR);
      return TRUE;        
   }  

   /* If this block is full, search the oldest tag and replace that one
      with the new data. */
   oldest = GetOldestTag(logblock);
   if (oldest == NOTAGFOUND)
   {
      if (!MakeBlockClean(block))
         return FALSE;

      oldest = GetOldestTag(logblock); /* Notice that all dirty bits are 
                                          cleared. */
   }
     
   tag[oldest].age = LogicalTime;
   if (tag[oldest].age == UINT_MAX) RedistributeAges(logblock);
   tag[oldest].devid = devid;
   tag[oldest].sector = sector;
   tag[oldest].invalidated = FALSE;
   tag[i].dirty = dirty;
   tag[i].area = area;
          
   memcpy(tag[oldest].sectordata, buffer, BYTESPERSECTOR);   
   
   return TRUE;
}

int RetreiveBlockSector(unsigned devid, SECTOR sector, char* buffer)
{
   struct SectorTag* tag;
   char* logblock;
   char  tagsused = 0, i;
   unsigned long block = HashBlockNumber(sector);

   if (!IsBlockInitialised(block))
      return FALSE;

   logblock = EnsureBlockMapped(block);
   if (!logblock) return FALSE;

   tagsused = ReadBlockCount(logblock);

   tag = (struct SectorTag*) logblock;
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].sector == sector) && (tag[i].devid == devid) &&
           (!tag[i].invalidated))
       {     
          tag[i].age = LogicalTime++;
          if (tag[i].age == UINT_MAX) 
             RedistributeAges(logblock);
                  
          memcpy(buffer, tag[i].sectordata, BYTESPERSECTOR);
          return TRUE;
       }
   }
   
   return FALSE;
}

void UncacheBlockSector(unsigned devid, SECTOR sector)
{
   struct SectorTag* tag;
   char* logblock;
   char  tagsused = 0, i;
   unsigned long block = HashBlockNumber(sector);

   if (!IsBlockInitialised(block)) return;

   logblock = EnsureBlockMapped(block);
   if (!logblock)
   {
      CacheIsCorrupt();
      return;
   }

   tagsused = ReadBlockCount(logblock);
   tag = (struct SectorTag*) logblock;
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].sector == sector) && (tag[i].devid == devid) &&
           (!tag[i].invalidated))
       {     
          tag[i].invalidated = TRUE;
       }
   }
}

void InvalidateBlockCache()
{
   unsigned long i;
   
   for (i = 0; i < NumberOfPhysicalBlocks; i++)
   {
       ClearBitfieldBit(InitialisedField, i);
   }          
}

static void IndicateDirtyBlock(unsigned long block)
{
   SetBitfieldBit(DirtyField, block);  
}

static void IndicateBlockCleaned(unsigned long block)
{
   ClearBitfieldBit(DirtyField, block);  
}

static unsigned long GetNextDirtyBlock(unsigned long previous)
{
   unsigned long i;

   for (i = previous; i < NumberOfPhysicalBlocks; i++)
   {
       if (GetBitfieldBit(DirtyField, i))
          return i;
   }
   
   return ULONG_MAX;
}

int GetFirstDirtyTag(struct SectorTag* tag, unsigned devid)
{
   int result = NOTAGFOUND;     
   char tagsused, i;
   SECTOR min = ULONG_MAX;
   
   tagsused = ReadBlockCount((char*) tag);
   
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].devid == devid) && (tag[i].sector < min) &&
           (tag[i].dirty))
       {
          result = i;
          min = tag[i].sector;
       }
   }
 
   return result;
}

int GetNextDirtyTag(struct SectorTag* tag, unsigned devid, 
                    SECTOR prevsector)
{
   int result = NOTAGFOUND, i;     
   char tagsused;
   SECTOR min = ULONG_MAX;
   
   tagsused = ReadBlockCount((char*) tag);

   for (i = 0; i < (int) tagsused; i++)
   {         
       if ((tag[i].devid == devid)       && 
           (tag[i].sector < min)         && 
           (tag[i].dirty)                &&
           (tag[i].sector > prevsector))
       {          
          result = i;
          min = tag[i].sector;
       }
   }

   return result;
}

static void GetNextDeviceID(unsigned long block,
                            unsigned oldevid, unsigned* newdevid, 
                            BOOL* found)
{
   struct SectorTag* tag; 
   char tagsused, i;
   unsigned result = UINT_MAX;
   
   *found = FALSE; 
   
   tag = (struct SectorTag*) EnsureBlockMapped(block);   
   
   tagsused = ReadBlockCount((char*) tag);
   
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].devid >= oldevid) && (tag[i].devid < result))
       {
          *newdevid = tag[i].devid;
          result = *newdevid;
          *found = TRUE;
       }
   } 
}

static int WriteBackBlock(unsigned devid, unsigned long block)
{  
   int tagindex;
   char* WriteBuf;
   struct SectorTag* tag;
   SECTOR firstsector=0, prevsector=0;
   unsigned prevarea = 0xFFFF;

   /* Checked on validity in the for loop */
   WriteBuf = (char*) FTEAlloc(TAGSPERBLOCK * BYTESPERSECTOR);
        
   tag = (struct SectorTag*) EnsureBlockMapped(block);
   if (!tag)
   {
      if (WriteBuf) FTEFree(WriteBuf);
      CacheIsCorrupt();
      return FALSE;
   }   

   tagindex = GetFirstDirtyTag(tag, devid);
   while (tagindex != NOTAGFOUND)
   {              
      if (!WriteBuf)
      {           
         if (!WriteBackSectors(devid, 1, tag[tagindex].sector, 
                               tag[tagindex].sectordata, tag[tagindex].area)) 
         {
            if (WriteBuf) FTEFree(WriteBuf);
            CacheIsCorrupt();
            return FALSE;
         }
         prevsector = tag[tagindex].sector;
      }
      else
      {
         if (firstsector &&
             ((prevsector != tag[tagindex].sector-1) || 
              (prevarea != tag[tagindex].area)))    
         {   
            if (!WriteBackSectors(devid, (int)(prevsector - firstsector + 1),
                                  firstsector, WriteBuf, prevarea)) 
            {
                if (WriteBuf) FTEFree(WriteBuf);
                CacheIsCorrupt();
                return FALSE;
            }
            
            firstsector = tag[tagindex].sector;
            prevarea = tag[tagindex].area;
         }
         else
         {   
            if (!firstsector)
            {                  
               firstsector = tag[tagindex].sector;
               prevarea = tag[tagindex].area;
            }
         }
         
         prevsector = tag[tagindex].sector;        
         memcpy(WriteBuf + 
                    ((int)(prevsector - firstsector)) * BYTESPERSECTOR,
                tag[tagindex].sectordata, BYTESPERSECTOR);         
      }

      tag[tagindex].dirty = FALSE;     
      tagindex = GetNextDirtyTag(tag, devid, prevsector);  
   }
   
   if (WriteBuf && firstsector)
   {  
      if (!WriteBackSectors(devid, (int)(prevsector - firstsector + 1),
                            firstsector, WriteBuf, prevarea)) 
      {
         if (WriteBuf) FTEFree(WriteBuf);
         CacheIsCorrupt();
         return FALSE;
      }      
   }
   
   IndicateBlockCleaned(block);
   if (WriteBuf) FTEFree(WriteBuf);
   return TRUE;
}

static int MakeBlockClean(unsigned long block)
{
   BOOL found;
   unsigned devid=0;
  
   GetNextDeviceID(block, 0, &devid, &found);
   while (found)
   {      
       if (!WriteBackBlock(devid, block))
          return FALSE;
           
       GetNextDeviceID(block, devid+1, &devid, &found);  
   }
   
   return TRUE;
}

int WriteBackCache(void)
{
   unsigned long block;

   block = GetNextDirtyBlock(0);
   
   while (block != ULONG_MAX)
   {
         if (!MakeBlockClean(block))
            return FALSE;
         
         block = GetNextDirtyBlock(block+1);
   }
   
   return TRUE;
}
