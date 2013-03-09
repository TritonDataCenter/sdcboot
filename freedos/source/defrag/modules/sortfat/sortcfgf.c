/*    
   Sortcfgf.c - takes care of installing and handling sort configurations.

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

#include "fte.h"
#include "sortcfgf.h"

static struct CompareConfiguration cmpconfig;
static struct ResourceConfiguration* resconfig;

void SetCompareFunction(int (*cmpfunc)(struct DirectoryEntry* e1,
		                       struct DirectoryEntry* e2))
{
    cmpconfig.cmpfunc = cmpfunc;
}

void SetFilterFunction(int (*filterfunc)(int x))
{
    cmpconfig.filterfunc = filterfunc;    
}

void SetResourceConfiguration(struct ResourceConfiguration* config)
{
    resconfig = config;
}

int CompareEntriesToSort(struct DirectoryEntry* e1,
			 struct DirectoryEntry* e2)
{
    return cmpconfig.cmpfunc(e1, e2);
}

int FilterSortResult(int x)
{
    return cmpconfig.filterfunc(x);
}                                

int GetEntryToSort(void* entries, unsigned pos, struct DirectoryEntry* result)
{
    return resconfig->getentry(entries, pos, result);
}

int SwapEntryForSort(void* entries, unsigned pos1, unsigned pos2)
{
    return resconfig->swapentry(entries, pos1, pos2);
}

int GetSlotToSort(void* entries, int entrynr, int slot,
                  struct DirectoryEntry* entry)
{
    return resconfig->getslot(entries, entrynr, slot, entry);
}
