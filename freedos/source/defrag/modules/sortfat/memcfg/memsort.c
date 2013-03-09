/*    
   Memsort.c - routines to sort entries in memory.

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

#include "fte.h"
#include "sortfatf.h"
#include "sortcfgf.h"

static int getentry(void* entries, unsigned pos, 
                       struct DirectoryEntry* result);
static int swapentry(void* entries, unsigned pos1, unsigned pos2);
static int getslot(void* entries, int entrynr, int slot,
                   struct DirectoryEntry* entry);

static struct ResourceConfiguration MemConfig =
{
   getentry,
   swapentry, 
   getslot
};

void MemorySortEntries(struct DirectoryEntry* entries,
                       int amofentries)
{
     int i=0, realentries = 0;

     struct DirectoryEntry* temp = entries;

     SetResourceConfiguration(&MemConfig);

     if (amofentries == 0) return; 
     
     if (IsCurrentDir(entries[0]))
     {
        if (amofentries == 1) return; 
        entries++;
        i++;     
     }
     
     if (IsPreviousDir(entries[0]))
     {
        entries++;
        i++; 
     }

     for (; i < amofentries; i++)
	 if (IsLFNEntry(&temp[i]) == 0)
            realentries++;

     if (realentries)
	SelectionSortEntries(entries, realentries);
}

/*
   This function assumes that all entries are stored in memory.
*/

static struct DirectoryEntry* PtrGetEntry(struct DirectoryEntry* pEntries,
                                          unsigned pos)
{
    unsigned i;    
     
    for (i = 0; i < pos; i++)
    {
	while (IsLFNEntry(pEntries))
              pEntries++;  
        pEntries++;
    }

    return pEntries;
}                                       
                                       
static int getentry(void* entries, unsigned pos, 
                    struct DirectoryEntry* result)                                       
{
    memcpy(result, 
           PtrGetEntry((struct DirectoryEntry*) entries, pos),
           sizeof(struct DirectoryEntry));
             
    return TRUE;
}

static int getslot(void* entries, int entrynr, int slot,
                   struct DirectoryEntry* entry)
{
    int i;    
    struct DirectoryEntry* pEntry;
    
    pEntry = PtrGetEntry((struct DirectoryEntry*) entries, entrynr);
    for (i = 0; i < slot; i++) pEntry++;
    
    memcpy(entry, pEntry, sizeof(struct DirectoryEntry));
             
    return TRUE;
}

static int swapentry(void* entries, unsigned pos1, unsigned pos2)
{
     char *entry1, *entry2;   
        
     entry1 = (char*)PtrGetEntry((struct DirectoryEntry*)entries, pos1);
     entry2 = (char*)PtrGetEntry((struct DirectoryEntry*)entries, pos2);
     
     SwapBufferParts(entry1, entry1+DIRLEN2BYTES((int)EntryLength((struct DirectoryEntry*)entry1)),
                     entry2, entry2+DIRLEN2BYTES((int)EntryLength((struct DirectoryEntry*)entry2)));  
                     
     return TRUE;
}


