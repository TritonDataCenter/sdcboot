/*    
   Dircount.c - count the number of entries in a directory.
   
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

struct PipeStruct
{
     long count;
     char attribute;
};

/**************************************************************
**                         counter
***************************************************************
** Counts the number of entries in the directory.
**************************************************************/

static BOOL counter(RDWRHandle handle, struct DirectoryPosition* pos,
                    void** buffer)
{
    struct DirectoryEntry* entry;

    struct PipeStruct* p = *((struct PipeStruct**) buffer);
    
    entry = AllocateDirectoryEntry();
    if (!entry) return FAIL;

    if (!GetDirectory(handle, pos, entry))
    {
       FreeDirectoryEntry(entry);
       return FAIL;
    }

    if (entry->attribute == LFN_ATTRIBUTES)
    {
       if (IsLFNEntry(entry))
          p->count++;
    }
    else if ((entry->attribute & p->attribute) ||
	     (p->attribute == -1))
    {
        p->count++;
    }

    FreeDirectoryEntry(entry);
    return TRUE;
}

/**************************************************************
**                CountEntriesInRootDirectory
***************************************************************
** Counts the number of entries in the root directory.
**
** Only FAT12/16
**************************************************************/

static long CountEntriesInRootDirectory(RDWRHandle handle, char attribute)
{
    long count = 0, i;
    struct DirectoryEntry* entry;

    long rootcount = GetNumberOfRootEntries(handle);
    if (!rootcount) return FAIL;

    entry = AllocateDirectoryEntry();
    if (!entry) return FAIL;

    for (i = 0; i < rootcount; i++)
    {
	if (!ReadDirEntry(handle, i, entry))
	{
	   FreeDirectoryEntry(entry);
	   return FAIL;
        }
	if (IsLastLabel(*entry))
	{
	   FreeDirectoryEntry(entry);
	   return count;
	}
        
        if (entry->attribute == LFN_ATTRIBUTES)
        {
           if (IsLFNEntry(entry))
              count++;
        }
        else if ((entry->attribute & attribute) ||
	         (attribute == -1))
        {
	   count++;
        }
    }
    FreeDirectoryEntry(entry);
    return rootcount;
}

/**************************************************************
**                       low_dircount
***************************************************************
** Counts the number of entries in a directory that has the given
** attribute.
**
** If the number of clusters equals 0, then we assume that we
** have to return the number of entries in the root directory.
**
** If attribute == -1 (0xFF), then all the files in the directory
** are counted.
**
** If attribute == LFN_ATTRIBUTES, then only the LFN entries are counted.
**
** Returns FAIL on error.
***************************************************************/

long low_dircount(RDWRHandle handle, CLUSTER cluster, char attribute)
{
    int fatlabelsize;
    struct PipeStruct pipe, *ppipe = &pipe;

    if (cluster == 0)
    {
       fatlabelsize = GetFatLabelSize(handle);
       switch (fatlabelsize)
       {
	  case FAT12:
	  case FAT16:
	       return CountEntriesInRootDirectory(handle, attribute);
	  case FAT32:
	       cluster = GetFAT32RootCluster(handle);
	       if (cluster)
		  break;
	       else
		  return FAIL;
	  default:
	       return FAIL;
       }
    }

    pipe.count     = 0;
    pipe.attribute = attribute;

    if (!TraverseSubdir(handle, cluster, counter, (void**) &ppipe, TRUE))
       return FAIL;

    return pipe.count;
}
