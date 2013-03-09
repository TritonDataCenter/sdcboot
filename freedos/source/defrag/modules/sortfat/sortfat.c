/*
   sortfat.c - directory sort module entry

   Copyright (C) 2004 Imre Leber

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
#include <ctype.h>
#include <stdio.h>

#include "fte.h"
#include "sortfatf.h"
#include "misc.h"
#include "sortcfgf.h"
#include "expected.h"
#include "..\dtstruct\vlhandle.h"

struct CriteriumFunction
{
    int (*func)(struct DirectoryEntry* e1, struct DirectoryEntry* e2);
};

struct OrderFunction
{
    int (*func)(int x);
};

static struct CriteriumFunction CriteriumFunctions[] =
{
    CompareNames,
    CompareExtension,
    CompareDateTime,    
    CompareSize
};

static struct OrderFunction OrderFunctions[] =
{
    AscendingFilter,
    DescendingFilter
};

/*
** For constants to use as parameters look in ..\..\modlgate\defrpars.h
*/

int SortFAT(int criterium, int order)
{
   int retVal;
   RDWRHandle handle;

   /* Get handle to volume */ 
   handle = GetCurrentVolumeHandle();
   if (!handle) RETURN_FTEERR(FALSE);    

   /* Mention what comes next */
   LargeMessage("Sorting directory entries . . .");
   SmallMessage(" Sorting directory entries . . .");

   /* Notice that we assume right input from the interface */
   SetCompareFunction(CriteriumFunctions[criterium-1].func);
   SetFilterFunction(OrderFunctions[order].func);
      
   retVal = SortDirectoryTree(handle);

   if (retVal) LogMessage("Directories successfully sorted.\n");
	   
   return retVal;
}
