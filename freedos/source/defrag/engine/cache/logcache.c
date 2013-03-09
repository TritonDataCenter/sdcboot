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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "fte.h"                                     
#include "ems.h"
#include "phscache.h"
#include "logcache.h"

#define SECTORSPERBLOCK 31

static int NextBlockPointer = 0;

struct LogicalBlockInfo LogicalBlocks[4+4];

#ifndef NDEBUG
unsigned NumberOfPhysicalBlocks;
#endif

/* Returns wether there was enough conventionel memory to contain the logical
   blocks for XMS. */
unsigned InitialiseLogicalCache(void)
{
    int i, j;
#ifdef USE_EMS
    unsigned int EMSBase;
#endif

#ifdef NDEBUG    
    unsigned NumberOfPhysicalBlocks;
#endif
    
    /* Allocate the memory to contain the pages for XMS (or just a
       conventional cache). Initialise them. */
    for (i = 0; i < 4; i++)
    {
        LogicalBlocks[i].address = (char*) FTEAlloc(CACHEBLOCKSIZE); 
        
        if (!LogicalBlocks[i].address)
        {
           for (j = 0; j < i; j++)
               FTEFree(LogicalBlocks[j].address);
               
           RETURN_FTEERROR(0); 
        }
               
        LogicalBlocks[i].age     = 0;
        LogicalBlocks[i].PhysicalBlock = i;
    }
    
    /* Now initialise the physical extended memory cache */
    NumberOfPhysicalBlocks = InitialisePhysicalCache();
    if (NumberOfPhysicalBlocks < 4) NumberOfPhysicalBlocks = 4;

    /* Initialise the EMS part, according to wether EMS is available */
#ifdef USE_EMS    
    if (IsEMSCached())
    {
       EMSBase = EMSbaseaddress();
       assert(EMSBase);
       
       for (; i < 8; i++)
       {
           LogicalBlocks[i].address = (char far *)MK_FP(EMSBase, i * CACHEBLOCKSIZE);
           LogicalBlocks[i].age     = 0;
           LogicalBlocks[i].PhysicalBlock = i;             
       }
    }
#endif
    
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

char* EnsureBlockMapped(unsigned physicalblock)
{
    assert(physicalblock < NumberOfPhysicalBlocks);
    
    /* First see wether the block is not already cached. */ 
#ifdef USE_EMS    
    {
	int i, from, to;
	from = 0;
	to   = (IsEMSCached()) ? 8 : 4;   

	for (i = from; i < to; i++)
	{
	    if (LogicalBlocks[i].PhysicalBlock == physicalblock)
	    {
                return LogicalBlocks[i].address;
	    }
	}
    }

#else
    
    if (LogicalBlocks[0].PhysicalBlock == physicalblock) return LogicalBlocks[0].address;
    if (LogicalBlocks[1].PhysicalBlock == physicalblock) return LogicalBlocks[1].address;
    if (LogicalBlocks[2].PhysicalBlock == physicalblock) return LogicalBlocks[2].address;	
    if (LogicalBlocks[3].PhysicalBlock == physicalblock) return LogicalBlocks[3].address;   
    
#endif
       
    /* Not in conventional memory, swap the oldest logical block, with
       the wanted physical block. */ 
#ifdef USE_EMS    
    switch (GetPhysicalMemType(physicalblock))
    {
        case XMS:
#endif	    
	     if (RemapXMSBlock(NextBlockPointer, physicalblock))
             { 
                char* retVal;
		 
		LogicalBlocks[NextBlockPointer].PhysicalBlock = physicalblock; 
		retVal = LogicalBlocks[NextBlockPointer].address;
		 
   		NextBlockPointer++;
		 
#ifndef USE_EMS	
		if (NextBlockPointer == 4)  NextBlockPointer = 0;
#else	    
		NextBlockPointer %= (IsEMSCached()) ? 8 : 4;   
#endif	     

                return retVal;
             }
	     	     
#ifdef USE_EMS
             break;
                

	case EMS:
             oldblock = GetOldestLogicalBlock(EMS); 
	     assert((oldblock >= 4) && (oldblock < 8));
             	
             if (RemapEMSBlock(oldblock-4, physicalblock))
             {
                LogicalBlocks[oldblock].PhysicalBlock = physicalblock; 

                return LogicalBlocks[oldblock].address;                             
             }      
             break;   

	default:
	     assert(FALSE);	 
    }
#endif	          
    assert(FALSE);    
    return NULL;     
}
