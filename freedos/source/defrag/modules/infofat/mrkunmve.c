/*    
   Mrkunmve.c - mark unmovable clusters.

   Copyright (C) 2003 Imre Leber

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
#include <stdlib.h>

#include "fte.h"
#include "expected.h"
#include "infofat.h"
#include "..\dtstruct\clmovmap.h"
#include "..\..\modlgate\custerr.h"

int MarkUnmovables32(RDWRHandle handle);

static BOOL FixedFileMarker(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            void** structure);

static BOOL FixedClusterMarker(RDWRHandle handle,
                               CLUSTER label,
                               SECTOR  sector,
                               void**  structure);

/************************************************************
***                        MarkUnmovables
*************************************************************
*** Goes through all the entries on the volume and marks
*** all the clusters that can not be moved as unmovable.
*************************************************************/                      
                      
int MarkUnmovables(RDWRHandle handle)
{
   CLUSTER i;
   unsigned long labelsinfat;
   BOOL isMovable;
   int fatlabelsize;

   fatlabelsize = GetFatLabelSize(handle);

   if (fatlabelsize == 0) 
   {
       SetCustomError(GET_FATTYPE_FAILED);
       RETURN_FTEERR(FALSE);
   }

   if (fatlabelsize == FAT32)
   {
     /* On FAT32 this is the only place where we need the fixed cluster map.
        Therefore we have a special function and do not try to allocate 
        the memory to contain complete FAT data */      

       return MarkUnmovables32(handle);      
   }

   labelsinfat = GetLabelsInFat(handle);
   if (!labelsinfat)
   {
       SetCustomError(WRONG_LABELSINFAT);
       RETURN_FTEERR(FALSE);
   }

   for (i = 2; i < labelsinfat; i++)
   {
        if (!IsClusterMovable(handle, i, &isMovable))
        {        
            RETURN_FTEERR(FALSE);
        }

      if (!isMovable)
         DrawOnDriveMap(i, UNMOVABLESYMBOL);
   }

   return TRUE;
}

/************************************************************
***                        MarkUnmovables32
*************************************************************
*** Goes through all the entries on the FAT32 volume and marks
*** all the clusters that can not be moved as unmovable.
*************************************************************/                      

/*static*/ int MarkUnmovables32(RDWRHandle handle)
{
    CLUSTER rootcluster;

    ResetFileCount();

    if (!WalkDirectoryTree(handle, FixedFileMarker, (void**) NULL))
    {
       SetCustomError(GET_FIXEDFILE_FAILED); 
       RETURN_FTEERR(FALSE);
    }

    /* Mark the root directory as non movable */
    rootcluster = GetFAT32RootCluster(handle);
    if (!rootcluster)
    {
       SetCustomError(GET_FAT32_ROOTCLUSTER_FAILED);
       RETURN_FTEERR(FALSE);
    }

    if (!FileTraverseFat(handle, rootcluster, FixedClusterMarker, (void**) NULL))
    {
        SetCustomError(GET_FAT32_ROOTDIR_FAILED);  
        RETURN_FTEERR(FAIL);
    }

    return TRUE;
}

static BOOL FixedFileMarker(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            void** structure)
{
    CLUSTER firstcluster;
    struct DirectoryEntry entry;

    if (structure);
    
    if (!GetDirectory(handle, pos, &entry))
       RETURN_FTEERR(FAIL);
       
    if ((entry.attribute & FA_LABEL) ||
        IsCurrentDir(entry)          ||
        IsPreviousDir(entry)         ||
        IsLFNEntry(&entry)           ||
        IsDeletedLabel(entry))
    {
        return TRUE;
    }
       
    IncrementFileCount();

    if ((entry.attribute & FA_HIDDEN) ||
        (entry.attribute & FA_SYSTEM))
    {
       firstcluster = GetFirstCluster(&entry);
       if (firstcluster)
       {
          if (!FileTraverseFat(handle, firstcluster, FixedClusterMarker, 
                               (void**) NULL))
          {
              RETURN_FTEERR(FAIL);
          }
       }
    }

    return TRUE;
}

static BOOL FixedClusterMarker(RDWRHandle handle,
                               CLUSTER label,
                               SECTOR  sector,
                               void**  structure)
{
    CLUSTER cluster;
    
    if (label);
    if (structure);
    
    cluster = DataSectorToCluster(handle, sector);
    if (!cluster)
       RETURN_FTEERR(FAIL);

    DrawOnDriveMap(cluster, UNMOVABLESYMBOL);
 
    return TRUE;
}


