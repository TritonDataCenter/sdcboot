/*    
   Dsksort.c - this is the configuration for sorting directories directly
               on disk.

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

#include <string.h>
#include <limits.h>

#include "fte.h"
#include "sortcfgf.h"
#include "sortfatf.h"

#define ENTRY 1
#define SLOT  2

struct Pipe 
{
    /* Input */
    int slotorentry; /* Indicates wether to return an entry or a slot
                        in an entry.                                  */
    unsigned entrynr;
    unsigned slot;
    
    /* Processing */
    unsigned ecounter;
    unsigned scounter;
    
    /* Output */    
    struct DirectoryEntry* entry;
};

struct DiskEntryGetterStruct
{
    RDWRHandle handle;
    CLUSTER cluster;
};

static int entrygetter (RDWRHandle handle,
         struct DirectoryPosition* pos,
         void** buffer);
                        
static int getentry(RDWRHandle handle,
                    CLUSTER cluster, 
                    int slotorentry,
                    unsigned entry,
                    unsigned slot, 
                    struct DirectoryEntry* result);   

static int GetEntryFromDisk(void* entries, unsigned entry,  
                     struct DirectoryEntry* result);                     
                   
static int GetSlotFromDisk(void* entries, int entry, int slot,  
                    struct DirectoryEntry* result);
                    
static int SwapEntriesOnDisk(void* entries, unsigned pos1, unsigned pos2);                        

static struct ResourceConfiguration DiskConfig =
{
   GetEntryFromDisk,
   SwapEntriesOnDisk,
   GetSlotFromDisk
};

BOOL DiskSortEntries(RDWRHandle handle, CLUSTER cluster)
{
   int  i=0;
   long totalcount, lfncount, realcount;
   struct DiskEntryGetterStruct parameters;
   struct DirectoryPosition pos;
   struct DirectoryEntry entry;

   parameters.handle = handle;
   parameters.cluster = cluster;
   
   SetResourceConfiguration(&DiskConfig);  

   totalcount = low_dircount(handle, cluster, 0xffff);
   if (totalcount == -1)
      RETURN_FTEERR(FALSE);
      
   lfncount   = low_dircount(handle, cluster, LFN_ATTRIBUTES);
   if (lfncount == -1)
      RETURN_FTEERR(FALSE);
      
   realcount = totalcount - lfncount;
   
   for (i = 0; i < 2; i++)
   {
       if (!GetNthDirectoryPosition(handle, cluster, i, &pos))
       {
          RETURN_FTEERR(FALSE);
       }
   
       if (!GetDirectory(handle, &pos, &entry))
       {
          RETURN_FTEERR(FALSE);
       }
   
       switch (i)
       {
          case 0:
               if (IsCurrentDir(entry))
               {
                  realcount--;
               }
               break;
          case 1:
               if (IsPreviousDir(entry))
               {
                  realcount--;
               }
       }
   } 
            
   if (realcount && realcount <= INT_MAX)
      SelectionSortEntries(&parameters, (int)realcount);

   return TRUE;
}
              
static int getentry(RDWRHandle handle,
                    CLUSTER cluster, 
                    int slotorentry,
                    unsigned entry,
                    unsigned slot, 
                    struct DirectoryEntry* result)
{
   
    struct Pipe pipe, *ppipe = &pipe;
        
    pipe.slotorentry = slotorentry;
    pipe.entrynr = entry;
    pipe.slot    = slot;
    
    pipe.ecounter = 0;
    pipe.scounter = 0;
    
    pipe.entry = result;
         
    return TraverseSubdir(handle, cluster, entrygetter, (void**) &ppipe, TRUE);
}

static int GetEntryFromDisk(void* entries, unsigned entry,  
                     struct DirectoryEntry* result)
{ 
    struct DiskEntryGetterStruct* parameters;

    parameters = (struct DiskEntryGetterStruct*) entries;
    
    return getentry(parameters->handle, parameters->cluster,
                    ENTRY, entry, 0, result);
}

static int GetSlotFromDisk(void* entries, int entry, int slot,  
                    struct DirectoryEntry* result)
{ 
    struct DiskEntryGetterStruct* parameters;

    parameters = (struct DiskEntryGetterStruct*) entries;
    
    return getentry(parameters->handle, parameters->cluster,
                    SLOT, entry, slot, result);
}

static int entrygetter (RDWRHandle handle,
         struct DirectoryPosition* pos,
         void** buffer)
{
     struct Pipe** pipe = (struct Pipe**) buffer;
     struct DirectoryEntry entry;
     
     if (!GetDirectory(handle, pos, &entry))
     {        
        RETURN_FTEERR(FAIL);
     }
     
     if (IsCurrentDir(entry) || IsPreviousDir(entry))
     {
	return TRUE; 
     }
     
     if ((*pipe)->entrynr == (*pipe)->ecounter)
     {
        if ((*pipe)->slotorentry == ENTRY)
        {
           memcpy((*pipe)->entry, &entry, sizeof(struct DirectoryEntry));
           RETURN_FTEERR(FALSE);
        }
        else
        {
           if ((*pipe)->slot == (*pipe)->scounter)
           {
              memcpy((*pipe)->entry, &entry, sizeof(struct DirectoryEntry));
              RETURN_FTEERR(FALSE);
           }
           (*pipe)->scounter++;
           return TRUE;
        }  
     }
     
     if ((entry.attribute & FA_LABEL) == 0)
        (*pipe)->ecounter++;     
     
     return TRUE;   
}

static int SwapEntriesOnDisk(void* entries, unsigned pos1, unsigned pos2)
{
    struct DiskEntryGetterStruct* parameters;

    parameters = (struct DiskEntryGetterStruct*) entries;

    return SwapLFNEntries(parameters->handle,
                          parameters->cluster,
                          pos1, pos2);    
}


