/*    
   Blkcache.c - sector cache for extended memory (individual blocks).

   Copyright (C) 2002, 2004 Imre Leber

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
  BOOL     dirty;
  unsigned area;
  
  char sectordata[BYTESPERSECTOR];
};

#ifndef NDEBUG

void DBGLogSector(SECTOR sector);
void DBGLogDelim(char* delim);

unsigned long DBG_HITS   = 0;
unsigned long DBG_WRITES = 0;
unsigned long DBG_TOTAL  = 0;

unsigned long DBG_A  = 0;
unsigned long DBG_B  = 0;
unsigned long DBG_C  = 0;
unsigned long DBG_D  = 0;

void DumpCache(void);

#endif

#define TAGSPERBLOCK 31
#define AMOFTAGSOFFSET 16382
#define NOTAGFOUND TAGSPERBLOCK * 2

static unsigned NumberOfPhysicalBlocks;
static char* InitialisedField;

static char* DirtyField;

static unsigned LogicalTime = 0;

/*static void IndicateDirtyBlock(unsigned block); */
static int MakeBlockClean(unsigned block);
static void GetNextDeviceID(unsigned block, 
                            unsigned oldevid, unsigned* newdevid,
			    BOOL* found);

// Inline some functionality

#define HashBlockNumber(sector) \
        (unsigned)(((sector) / TAGSPERBLOCK) % NumberOfPhysicalBlocks)
  
#define IsBlockInitialised(block) \
        GetBitfieldBit(InitialisedField, (block))
	
#define InitialiseBlock(block) \
	SetBitfieldBit(InitialisedField, (block))
	
#define WriteBlockCount(block, fillcount) \
	(block)[AMOFTAGSOFFSET] = (fillcount)
	
#define ReadBlockCount(block) \
	(block)[AMOFTAGSOFFSET]
       
#define IndicateDirtyBlock(block) \
	SetBitfieldBit(DirtyField, (block))

#define IndicateBlockCleaned(block) \
	ClearBitfieldBit(DirtyField, (block));  

static void RedistributeAges(char* block)
{
   char i, tagsused;
   struct SectorTag* tag;
   
   assert(block);
      
   tagsused = ReadBlockCount(block);

   tag = (struct SectorTag*) block;
   
   for (i = 0; i < tagsused; i++)
       tag[i].age >>= 3;  
       
   LogicalTime = UINT_MAX / 8;
}

static char GetOldestTag(char* block)
{
   char i, index;
   struct SectorTag* tag;
   unsigned min = UINT_MAX;
   
   assert(block);
      
   tag = (struct SectorTag*) block;
   
   for (i = 0; i < TAGSPERBLOCK; i++)
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
       RETURN_FTEERROR(FALSE);
      
   InitialisedField = CreateBitField(NumberOfPhysicalBlocks);
   if (!InitialisedField)
   {
      CloseLogicalCache();     
      RETURN_FTEERROR(FALSE); 
   }
   
   DirtyField = CreateBitField(NumberOfPhysicalBlocks);
   if (!DirtyField)
   {
      DestroyBitfield(InitialisedField);
      CloseLogicalCache();
      RETURN_FTEERROR(FALSE); 
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
   unsigned block = HashBlockNumber(sector);
   
   assert(buffer);
   
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
   
   if (dirty) 
   {
#ifndef NDEBUG       
       DBG_TOTAL++; 
#endif       
       
      IndicateDirtyBlock(block);
   }

   /* See wether the sector is already in the cache. */
   tag = (struct SectorTag*) logblock; 
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].sector == sector) && (tag[i].devid == devid))
       {
#ifndef NDEBUG	   
	  if (dirty) DBG_A++;	
#endif	   
          /* Update the cache */       
          tag[i].age = LogicalTime++;
          if (tag[i].age == UINT_MAX)
             RedistributeAges(logblock);
	  
#ifndef NDEBUG	  
          if (dirty && tag[i].dirty)
             DBG_HITS++;
#endif	  
	  
          tag[i].dirty = dirty;   
          tag[i].area = area;
          memcpy(tag[i].sectordata, buffer, BYTESPERSECTOR);
          return TRUE;
       }
   }
   
   /* Then see wether the cache is already full and add a new tag if possible. */        
   if (tagsused < TAGSPERBLOCK)
   {
#ifndef NDEBUG       
      if (dirty) DBG_C++;       
#endif
       
      WriteBlockCount(logblock, tagsused+1);
      tag[tagsused].age = LogicalTime++;
      if (LogicalTime == UINT_MAX) RedistributeAges(logblock);
      tag[tagsused].devid = devid;
      tag[tagsused].sector = sector;
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
         RETURN_FTEERROR(FALSE); 

      oldest = GetOldestTag(logblock); /* Notice that all dirty bits are */
      IndicateDirtyBlock(block);       /* cleared. */      
   }
   
#ifndef NDEBUG
   if (dirty) DBG_D++;   
#endif
   
   tag[oldest].age = LogicalTime;
   if (tag[oldest].age == UINT_MAX) RedistributeAges(logblock);
   tag[oldest].devid = devid;
   tag[oldest].sector = sector;
   tag[oldest].dirty = dirty;
   tag[oldest].area = area;
          
   memcpy(tag[oldest].sectordata, buffer, BYTESPERSECTOR);   
   
   return TRUE;
}

int RetreiveBlockSector(unsigned devid, SECTOR sector, char* buffer)
{
   struct SectorTag* tag;
   char* logblock;
   char  tagsused = 0, i;
   unsigned block = HashBlockNumber(sector);

   assert(buffer);
   
   if (!IsBlockInitialised(block))
       return FALSE;

   logblock = EnsureBlockMapped(block);
   if (!logblock) RETURN_FTEERROR(FALSE); 

   tagsused = ReadBlockCount(logblock);

   tag = (struct SectorTag*) logblock;
   for (i = 0; i < tagsused; i++)
   {
       if ((tag[i].sector == sector) && (tag[i].devid == devid))
       {     
          tag[i].age = LogicalTime++;
          if (tag[i].age == UINT_MAX) 
             RedistributeAges(logblock);
                  
          memcpy(buffer, tag[i].sectordata, BYTESPERSECTOR);
          return TRUE;
       }
   }
   
   return FALSE; // sector not found
}

void UncacheBlockSector(unsigned devid, SECTOR sector)
{
   struct SectorTag* tag;
   char* logblock;
   char  tagsused = 0, i;
   int   j;
   unsigned block = HashBlockNumber(sector);
assert(0);
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
       if ((tag[i].sector == sector) && (tag[i].devid == devid))
       {     
	   /* Move all the data to the front */
	   if (i < tagsused - 1)
	   {
	      for (j = i+1; j < tagsused; j++)
	      {
		  memcpy(&tag[j], &tag[j-1], sizeof(*tag));  		  
              }
	   }
	   
	   WriteBlockCount(logblock, tagsused+1);	   
	   break;
       }
   }
}

void InvalidateBlockCache()
{
   unsigned i;
   
   for (i = 0; i < NumberOfPhysicalBlocks; i++)
   {
       ClearBitfieldBit(InitialisedField, i);
   }          
}

static unsigned GetNextDirtyBlock(unsigned previous)
{
   unsigned i;

   for (i = previous; i < NumberOfPhysicalBlocks; i++)
   {
       if (GetBitfieldBit(DirtyField, i))
          return i;
   }
   
   return UINT_MAX;
}

int GetFirstDirtyTag(struct SectorTag* tag, unsigned devid)
{
   int result = NOTAGFOUND;     
   char tagsused, i;
   SECTOR min = ULONG_MAX;
    
   assert(tag);
   
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

static void GetNextDeviceID(unsigned block,
                            unsigned oldevid, unsigned* newdevid, 
                            BOOL* found)
{
   struct SectorTag* tag; 
   char tagsused, i;
   unsigned result = UINT_MAX;
   
   assert(found && newdevid);
   
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

static int WriteBackBlock(unsigned devid, unsigned block)
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
      RETURN_FTEERROR(FALSE); 
   }   

   tagindex = GetFirstDirtyTag(tag, devid);
   while (tagindex != NOTAGFOUND)
   {              
      if (!WriteBuf)
      {          
#ifndef NDEBUG
         DBG_WRITES++; 
#endif	  
	  
         if (!WriteBackSectors(devid, 1, tag[tagindex].sector, 
                               tag[tagindex].sectordata, tag[tagindex].area)) 
         {
	     if (WriteBuf) FTEFree(WriteBuf);
             CacheIsCorrupt();
             RETURN_FTEERROR(FALSE); 
         }
      }
      else
      {
         if (firstsector &&
             ((prevsector != tag[tagindex].sector-1) || 
              (prevarea != tag[tagindex].area)))    
         { 
#ifndef NDEBUG	     
	    DBG_WRITES+=prevsector - firstsector + 1;	     
#endif
	     
            if (!WriteBackSectors(devid, (int)(prevsector - firstsector + 1),
                                  firstsector, WriteBuf, prevarea)) 
            {
                if (WriteBuf) FTEFree(WriteBuf);
                CacheIsCorrupt();
                RETURN_FTEERROR(FALSE); 
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
      tagindex = GetFirstDirtyTag(tag, devid);  
   }
   
   if (WriteBuf && firstsector)
   {  
#ifndef NDEBUG       
      DBG_WRITES+=prevsector - firstsector + 1;	       
#endif
       
      if (!WriteBackSectors(devid, (int)(prevsector - firstsector + 1),
                            firstsector, WriteBuf, prevarea)) 
      {
         if (WriteBuf) FTEFree(WriteBuf);
	     assert(FALSE);
         CacheIsCorrupt();
         RETURN_FTEERROR(FALSE); 
      }      
   }
   
   IndicateBlockCleaned(block);
   if (WriteBuf) FTEFree(WriteBuf);
   return TRUE;
}

static int MakeBlockClean(unsigned block)
{
   BOOL found;
   unsigned devid=0;
  
   GetNextDeviceID(block, 0, &devid, &found);
   while (found)
   {      
       if (!WriteBackBlock(devid, block))
          RETURN_FTEERROR(FALSE); 
           
       GetNextDeviceID(block, devid+1, &devid, &found);  
   }
   
   return TRUE;
}

int WriteBackCache(void)
{
   unsigned block;

   block = GetNextDirtyBlock(0);
       
   while (block != UINT_MAX)
   {
         if (!MakeBlockClean(block))
	 {
	     RETURN_FTEERROR(FALSE); 
	   
	 }
         
         block = GetNextDirtyBlock(block+1);
   }
   
   assert(DBG_A + DBG_B + DBG_C + DBG_D == DBG_TOTAL);
   assert(DBG_HITS + DBG_WRITES == DBG_TOTAL);

#ifndef NDEBUG
   
   DBG_HITS   = 0;
   DBG_WRITES = 0;
   DBG_TOTAL  = 0;

   DBG_A  = 0;
   DBG_B  = 0;
   DBG_C  = 0;
   DBG_D  = 0;

#endif
   
   return TRUE;
}

#ifndef NDEBUG

void DBGLogSector(SECTOR sector)
{
   FILE* fptr = fopen("c:\\defrag.tst", "a+t");
    
   if (fptr)
   {
      fprintf(fptr, "%lu\n", sector); 
      
      fclose(fptr);
   }    
}


void DBGLogDelim(char* delim)
{
   FILE* fptr = fopen("c:\\defrag.tst", "a+t");
    
   if (fptr)
   {
      fprintf(fptr, "%s\n", delim); 
      
      fclose(fptr);
   }
}

void DumpCache(void)
{
    unsigned i;
    char tagsused, j;
     
    char* logblock;
    struct SectorTag* tag;   
	
    printf("Number of physical blocks: %lu\n", NumberOfPhysicalBlocks);
    
    for (i=0; i<NumberOfPhysicalBlocks; i++)
    {
	if (IsBlockInitialised(i))
	{
	    printf("[-- Block: %lu --]\n", i);
	    
	    logblock = EnsureBlockMapped(i);
            if (!logblock) {printf("Problem\n"); return;}
		
	    tagsused = ReadBlockCount(logblock);
	    
	    tag = (struct SectorTag*) logblock;
	    
	    for (j = 0; j< tagsused; j++)
	    {		
	        if (tag[j].dirty)
		{		
		   printf("tag:     %d\n",   (int) j);
		   printf("sector   %lu\n",  tag[j].sector);
		   printf("devid:   %u\n",   tag[j].devid);
		   printf("age:     %u\n",   tag[j].age);
		   //printf("invalid: %d\n",   tag[j].invalidated);
		   printf("dirty:   %d\n",   tag[j].dirty);
		   printf("area:    %u\n",   tag[j].area);
		   printf("\n");
		}
            }
	}	
    }    
}

#endif
