/*
   clmovmap.c - operations on the fixed cluster map.

   Copyright (C) 2003, Imre Leber.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have recieved a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be.

*/

#include <assert.h>
#include <stdlib.h>

#include "fte.h"
#include "..\..\modlgate\expected.h"
#include "..\..\modlgate\custerr.h"

//static unsigned long WindowStart;

static VirtualDirectoryEntry* FixedClusterMap = NULL;

/*static*/ BOOL FixedFileMarker(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            void** structure);
/*static*/ BOOL FixedClusterMarker(RDWRHandle handle,
                               CLUSTER label,
                               SECTOR  sector,
                               void**  structure);

BOOL CreateFixedClusterMap(RDWRHandle handle)
{
    unsigned long labelsinfat;
    int fattype;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {    
        SetCustomError(WRONG_LABELSINFAT);   
        RETURN_FTEERR(FALSE);
    }

    FixedClusterMap = CreateVFSBitField(handle, labelsinfat);   
    if (!FixedClusterMap)
    {
        SetCustomError(VFS_ALLOC_FAILED);
        RETURN_FTEERR(FALSE);
    }

    if (!WalkDirectoryTree(handle, FixedFileMarker, (void**) NULL))
    {
       SetCustomError(GET_FIXEDFILE_FAILED); 
       RETURN_FTEERR(FALSE);
    }

    fattype = GetFatLabelSize(handle);
    if (fattype == 0)
    {
        SetCustomError(GET_FATTYPE_FAILED);
        RETURN_FTEERR(FALSE);
    }

    if (fattype == FAT32)
    {
        /* Mark the root directory as non movable */
        CLUSTER rootcluster = GetFAT32RootCluster(handle);
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
    }
/*
{
unsigned long i;
int bit;

for (i=0; i < labelsinfat; i++)
{
GetVFSBitfieldBit(FixedClusterMap, i, &bit);

if (bit)
printf("%lu\n", i);
 }

}
*/

    LogMessage("Fixed cluster map created.");
    return TRUE;
}

/*static*/ BOOL FixedFileMarker(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            void** structure)
{
    CLUSTER firstcluster;
    struct DirectoryEntry entry;

    if (structure);
    
    if (!GetDirectory(handle, pos, &entry))
       RETURN_FTEERR(FAIL);
       
    if ((entry.attribute & FA_LABEL) ||
        (IsLFNEntry(&entry))         ||
        (IsDeletedLabel(entry)))
       return TRUE;
       
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

/*static*/ BOOL FixedClusterMarker(RDWRHandle handle,
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

    if (!SetVFSBitfieldBit(FixedClusterMap, cluster))
        RETURN_FTEERR(FAIL);
 
    return TRUE;
}

void DestroyFixedClusterMap(void)
{
    if (FixedClusterMap)
    {
       DestroyVFSBitfield(FixedClusterMap);
       FixedClusterMap = NULL;
    }
}

BOOL IsClusterMovable(RDWRHandle handle, CLUSTER cluster, BOOL* isMovable)
{
    if (handle);
    
    assert(FixedClusterMap);
 
    if (!GetVFSBitfieldBit(FixedClusterMap, cluster, isMovable))
    {
	SetCustomError(VFS_GET_FAILED);
	RETURN_FTEERR(FALSE);
    }

    *isMovable = !*isMovable;
    return TRUE;
}
