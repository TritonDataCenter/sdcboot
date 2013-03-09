/*    
   Logcache.c - sector cache for extended memory (conventional part).

   Copyright (C) 2002 Imre Leber

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General presePublic License as published by
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

#include <limits.h>
#include <stdlib.h>

#include "fte.h"                                     



#include "ems.h"
#include "phscache.h"

#define SECTORSPERBLOCK 31

struct LogicalBlockInfo
{
   char*    address;
   unsigned age;
   unsigned long PhysicalBlock;
};

static struct LogicalBlockInfo LogicalBlocks[4+4];

static unsigned LogicalTime = 0;

/* Returns wether there was enough conventionel memory to contain the logical
   blocks for XMS. */
unsigned long InitialiseLogicalCache(void)
{
    int i, j;
    unsigned int EMSBase;
    unsigned long NumberOfPhysicalBlocks;
    
    /* Allocate the memory to contain the pages for XMS (or just a
       conventional cache). Initialise them. */
    for (i = 0; i < 4; i++)
    {
        LogicalBlocks[i].address = (char*) FTEAlloc(CACHEBLOCKSIZE); 
        
        if (!LogicalBlocks[i].address)
        {
           for (j = 0; j < i; j++)
               FTEFree(LogicalBlocks[j].address);
               
           return 0;
        }
               
        LogicalBlocks[i].age     = 0;
        LogicalBlocks[i].PhysicalBlock = i;
    }
    
    /* Now initialise the physical extended memory cache */
    NumberOfPhysicalBlocks = InitialisePhysicalCache();
    if (NumberOfPhysicalBlocks < 4) NumberOfPhysicalBlocks = 4;

    /* Initialise the EMS part, according to wether EMS is available */
    if (IsEMSCached())
    {
       EMSBase = EMSbaseaddress();
       
       for (; i < 8; i++)
       {
           LogicalBlocks[i].address = (char far *)MK_FP(EMSBase, i * CACHEBLOCKSIZE);
           LogicalBlocks[i].age     = 0;
           LogicalBlocks[i].PhysicalBlock = i;             
       }
    }
    
    return NumberOfPhysicalBlocks;
}

void CloseLogicalCache(void)
{
   int i;
   
   for (i = 0; i < 4; i++)
   {
       FTEFree(LogicalBlocks[i].address);
   }
        
   ClosePhysicalCache();
}

char* GetLogicalBlockAddress(int logicalblock)
{
    return LogicalBlocks[logicalblock].address;
}

unsigned long GetMappedPhysicalBlock(int logicalblock)
{
    return LogicalBlocks[logicalblock].PhysicalBlock;
}

static int GetOldestLogicalBlock(int memtype)
{
    int min = UINT_MAX, index, i;
    int from=0, to=0;
    
    if (memtype == XMS)
    {    
       from = 0;
       to   = 4;
    }
    else if (IsEMSCached())
    {
        from = 4;
        to   = 8;
    }
                        
    for (i = from; i < to; i++)
    {
        if (LogicalBlocks[i].age < min)
        {
           min = LogicalBlocks[i].age;
           index = i;
        } 
    }
 
    return index;
}

static void RedistributeAges(void)
{
    int i, from, to;
   
    from = 0;
    to   = (IsEMSCached()) ? 8 : 4;   
    
    for (i = from; i < to; i++)
        LogicalBlocks[i].age /= 2;

    LogicalTime = UINT_MAX / 2;     
}

char* EnsureBlockMapped(unsigned long physicalblock)
{
    int i, oldblock;
    int from, to;
    
    /* First see wether the block is not already cached. */    
    from = 0;
    to   = (IsEMSCached()) ? 8 : 4;   

    for (i = from; i < to; i++)
    {
        if (LogicalBlocks[i].PhysicalBlock == physicalblock)
        {
           LogicalBlocks[i].age = LogicalTime++;         
           if (LogicalBlocks[i].age == UINT_MAX)
              RedistributeAges();
           return LogicalBlocks[i].address;
        }
    }
    
    /* Not in conventional memory, swap the oldest logical block, with
       the wanted physical block. */ 
    switch (GetPhysicalMemType(physicalblock))
    {
        case XMS:
             oldblock = GetOldestLogicalBlock(XMS);
             if (RemapXMSBlock(oldblock, physicalblock))
             {
                LogicalBlocks[oldblock].age     = LogicalTime++;
                if (LogicalBlocks[oldblock].age == UINT_MAX)
                   RedistributeAges();
                   
                LogicalBlocks[oldblock].PhysicalBlock = physicalblock; 
                
                return LogicalBlocks[oldblock].address;
             }
             break;
                
        case EMS:
             oldblock = GetOldestLogicalBlock(EMS);  
             if (RemapEMSBlock(oldblock-4, physicalblock))
             {
                LogicalBlocks[oldblock].age     = LogicalTime++;
                if (LogicalBlocks[oldblock].age == UINT_MAX)
                   RedistributeAges();     
                        
                LogicalBlocks[oldblock].PhysicalBlock = physicalblock; 

                return LogicalBlocks[oldblock].address;                             
             }      
             break;   
    }
     
    return NULL;     
}