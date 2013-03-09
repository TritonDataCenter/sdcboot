/*
   Filessum.c - file summary gattering.
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

#include <string.h>
#include <limits.h>

#include "fte.h"
#include "..\struct\FstTrMap.h"

#include "filessum.h"

static BOOL DirSizeGetter(RDWRHandle handle, struct DirectoryPosition* pos,
                          void** structure);

static BOOL FilesSummaryGetter(RDWRHandle handle,
			       struct DirectoryPosition* position,
			       struct DirectoryEntry* entry,
                               void** structure);

static char* fatmap;                               
                               
BOOL GetFilesSummary(RDWRHandle handle, struct FilesSummary* info)
{
    memset(info, 0, sizeof(struct FilesSummary));
    return FastWalkDirectoryTree(handle, FilesSummaryGetter, (void**)&info);
}

/* Add the found length to the total size of directories. */
/* Notice that FAT32 supports 2Tb of data and this does not fit
   into 1 unsigned long. */
   
/* helper to add ULONGs into ULONGLONGs */

static void AddUlongLong(unsigned long *sum, unsigned long toAdd)
{
     if (toAdd > 0xffffffffl - sum[0]) /* carry */
        sum[1]++;
     sum[0] += toAdd;
}

static unsigned long CalculateClustersSize(unsigned long size,
                                           unsigned long bytespercluster)
{
     return ((size / bytespercluster) + ((size % bytespercluster) > 0)) *
            bytespercluster;
}

static BOOL FilesSummaryGetter(RDWRHandle handle,
			       struct DirectoryPosition* position,
			       struct DirectoryEntry* entry,
                               void** structure)
{
    CLUSTER firstcluster;
    struct FilesSummary** info = (struct FilesSummary**) structure;
    unsigned long dirsize=0, *pdirsize = &dirsize, bytespercluster, labelsinfat;

    position = position;

    bytespercluster = GetSectorsPerCluster(handle) * BYTESPERSECTOR;
    if (!bytespercluster) return FAIL;

    if (entry->attribute & FA_LABEL)
       return TRUE;

    if (IsLFNEntry(entry))
       return TRUE;
    if (IsDeletedLabel(*entry))
       return TRUE;

    if (entry->attribute & FA_DIREC)
    {
       if (IsCurrentDir(*entry))  return TRUE;
       if (IsPreviousDir(*entry)) return TRUE;

       (*info)->DirectoryCount++;

       labelsinfat = GetLabelsInFat(handle);
       if (!labelsinfat) return FAIL;
       
       fatmap = CreateBitField(labelsinfat);
       if (!fatmap) return FAIL;
      
       firstcluster = GetFirstCluster(entry);
       if (!TraverseSubdir(handle, firstcluster, DirSizeGetter,
                           (void**) &pdirsize, FALSE))
       {
          return FAIL;
       }

       DestroyBitfield(fatmap);
       
       AddUlongLong(&(*info)->SizeOfDirectories[0],dirsize);

       return TRUE;     /* Not taking directories into account when
                           gathering information on files. */
    }

    (*info)->TotalFileCount++;

    AddUlongLong(&(*info)->TotalSizeofFiles[0],entry->filesize);
    AddUlongLong(&(*info)->SizeOfAllFiles[0],
                 CalculateClustersSize(entry->filesize, bytespercluster));

    if (entry->attribute & FA_SYSTEM)
    {
       (*info)->SystemFileCount++;

       AddUlongLong(&(*info)->SizeOfSystemFiles[0],
                    CalculateClustersSize(entry->filesize, bytespercluster));
    }

    if (entry->attribute & FA_HIDDEN)
    {
       (*info)->HiddenFileCount++;

       AddUlongLong(&(*info)->SizeOfHiddenFiles[0],
                    CalculateClustersSize(entry->filesize, bytespercluster));

    }

    return TRUE;
}

static BOOL DirSizeGetter(RDWRHandle handle, struct DirectoryPosition* pos,
                          void** structure)
{
   BOOL InRoot;
   unsigned long **dirsize = (unsigned long**) structure;
   unsigned char sectorspercluster;
   CLUSTER label;

   pos = pos, handle = handle;
    
   /* Check for loops in the directory structure */ 
   InRoot = IsRootDirPosition(handle, pos);
   if (InRoot == -1) return FAIL;

   if (!InRoot && (pos->offset == 0))
   {
      sectorspercluster = GetSectorsPerCluster(handle);
      if (!sectorspercluster) return FAIL;

      if ((pos->sector % sectorspercluster) == 0) 
      {
         label = DataSectorToCluster(handle, pos->sector);
         if (!label) return FAIL;
         
         if (GetBitfieldBit(fatmap, label))
         {
            return FALSE;
         }
         SetBitfieldBit(fatmap, label);
      }
   }
    
   **dirsize += sizeof(struct DirectoryEntry);

   return TRUE;
}
