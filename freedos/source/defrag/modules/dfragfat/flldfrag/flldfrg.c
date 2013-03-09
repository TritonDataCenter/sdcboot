/*    
   FllDfrg.c - code to fully optimize a drive.

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

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "fte.h"
#include "DfrgDrvr.h"
#include "FllDfrag\flldfrg.h"
#include "..\dtstruct\ClMovMap.h"
#include "..\..\modlgate\expected.h"
#include "defrmap.h"
#include "..\..\modlgate\custerr.h"

/* Public functions */
static BOOL SelectFile(RDWRHandle handle, CLUSTER startfrom,
                       CLUSTER* clustertoplace);                         

struct Pipe
{
   CLUSTER cluster;
   CLUSTER clustertoplace;

   BOOL found;
};

BOOL FullyDefragmentVolume(RDWRHandle handle)
{
    BOOL result;
    
    if (CreateFastSelectMap(handle) == FAIL)
       RETURN_FTEERR(FALSE);
    
    result = DefragmentVolume(handle, SelectFile, FullDefragPlace);
    DestroyFastSelectMap();
    
    return result;
}

static BOOL SelectFile(RDWRHandle handle, CLUSTER startfrom,
                       CLUSTER* clustertoplace)
{
    CLUSTER i;
    unsigned long LabelsInFat;
    VirtualDirectoryEntry* FastSelectMap;

    LabelsInFat = GetLabelsInFat(handle);
    if (!LabelsInFat)
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FAIL);
    }

    FastSelectMap = GetFastSelectMap();

    for (i = startfrom; i < LabelsInFat; i++)
    {
        BOOL value;

        if (!GetVFSBitfieldBit(FastSelectMap, i, &value))
        {
            SetCustomError(VFS_GET_FAILED);
            RETURN_FTEERR(FAIL);
        }

        if (value)
        {
            *clustertoplace = i;
            return TRUE;
        }
    }

    RETURN_FTEERR(FALSE);
}

static unsigned long CountFreeSpace(RDWRHandle handle, CLUSTER cluster, unsigned long maxInterested)
{
    CLUSTER label;
    unsigned long retLength=0;
    unsigned long labelsinfat;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) 
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FAIL);
    }

    for (; (cluster < labelsinfat) && (retLength < maxInterested); cluster++)
    {
   if (!GetNthCluster(handle, cluster, &label))
   {
       SetCustomError(CLUSTER_RETRIEVE_ERROR);
       RETURN_FTEERR(FAIL);
   }

   if (!FAT_FREE(label))
       break;

   retLength++;
    }

    return retLength;      
}

static BOOL SkipConsecutiveClusters(RDWRHandle handle, CLUSTER* cluster, unsigned long* spaceSkipped)
{
    CLUSTER label;
    unsigned long labelsinfat;

    *spaceSkipped = 0;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) 
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FALSE);
    }

    for (; *cluster < labelsinfat; )
    {
        if (!GetNthCluster(handle, *cluster, &label))
        {
           SetCustomError(CLUSTER_RETRIEVE_ERROR);
           RETURN_FTEERR(FALSE);
        }

   *cluster = *cluster + 1;
   *spaceSkipped = *spaceSkipped + 1;

        if (FAT_LAST(label))
           return TRUE;

   if (label != *cluster)
      return TRUE;
    }

    return TRUE;
}

static BOOL NextNonFreeEquals(RDWRHandle handle, CLUSTER start, CLUSTER totest)
{
    CLUSTER label;
    unsigned long labelsinfat;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
        SetCustomError(WRONG_LABELSINFAT);        
        RETURN_FTEERR(FAIL);
    }

    for (; start < labelsinfat; start++)
    {
        if (!GetNthCluster(handle, start, &label))
        {
           SetCustomError(CLUSTER_RETRIEVE_ERROR);
           RETURN_FTEERR(FAIL);
        }
   
   if (!FAT_FREE(label))
   {
      return start == totest;
   }
    }

    RETURN_FTEERR(FALSE);
}

static BOOL GetConsecutiveLength(RDWRHandle handle, CLUSTER cluster, unsigned long* length)
{
    CLUSTER label;
    unsigned long labelsinfat;
    
    *length = 1;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) RETURN_FTEERR(FALSE);
    
    for (; cluster < labelsinfat; cluster = cluster + 1)
    {
        if (!GetNthCluster(handle, cluster, &label))
        {
           SetCustomError(CLUSTER_RETRIEVE_ERROR);
           RETURN_FTEERR(FALSE);
        }

        if (FAT_LAST(label))
           return TRUE;

        if (label != cluster + 1)
           return TRUE;

        *length = *length + 1;
    }

    return TRUE;
}

BOOL FullDefragPlace(RDWRHandle handle, CLUSTER clustertoplace, CLUSTER clustertobereplaced, CLUSTER* stop)
{
    BOOL value;
    BOOL pastTheEnd;
    BOOL isMovable;
    unsigned long filesize;
    unsigned long relocatedSize=0;
    unsigned long spaceFree = 0;
    unsigned long blockingFileSize;
    unsigned long freeLength;
    unsigned long maxFreeSpace;    

    CLUSTER freespace, label;
    VirtualDirectoryEntry* FastSelectMap;

    unsigned long labelsinfat, length;

    FastSelectMap = GetFastSelectMap();

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FAIL);
    }

    /* Get the length of the file to relocate */

    filesize = CalculateFileSize(handle, clustertoplace);
    if (filesize == FAIL)
    {
        SetCustomError(GET_FILESIZE_FAILED);
        RETURN_FTEERR(FAIL);
    }

    *stop = clustertobereplaced + filesize;

    if (!GetMaxRelocatableSize(handle, &maxFreeSpace))
    {
        RETURN_FTEERR(FALSE);
    }

    if (maxFreeSpace == 0)
    {
        SetCustomError(INSUFFICIENT_FREE_DISK_SPACE);        
    }

    while (relocatedSize != filesize)
    {
        /* If part, or whole of the file is already in the right location,
        follow the file until it is no longer consecutive */

        if (clustertoplace == clustertobereplaced)
        {
            unsigned long spaceSkipped;

            if (!SkipConsecutiveClusters(handle, &clustertobereplaced, &spaceSkipped))
            {
                RETURN_FTEERR(FAIL);
            }

            clustertoplace = GetNthFileCluster(handle, clustertoplace, spaceSkipped,
                &pastTheEnd);
            if (!clustertoplace)
            {
                SetCustomError(GET_FILECLUSTER_FAILED);
                RETURN_FTEERR(FAIL);
            }

	    // clustertobereplaced += spaceSkipped;
	    relocatedSize += spaceSkipped;

            // Mark everything we skipped as continous
            DrawMoreOnDriveMap(clustertobereplaced, OPTIMIZEDSYMBOL, spaceSkipped);

            continue;
        }

        /* See how much space is free to store a part of the file chain */
        spaceFree = CountFreeSpace(handle, clustertobereplaced, filesize-relocatedSize);
        if (spaceFree == FAIL)
        {
            SetCustomError(GET_FREESPACE_ERROR);
            RETURN_FTEERR(FAIL);
        }

	if (spaceFree > maxFreeSpace)
	    spaceFree = maxFreeSpace;

        if (spaceFree)
        {
            /* Optimization:

            go through the free space and see wether we do not directly come up to
            the cluster to place.

            If we do, see how many clusters of clustertoplace are consecutive and 
            place it at the clustertobereplaced.
            */

	    switch (NextNonFreeEquals(handle, clustertobereplaced, clustertoplace))
	    {
	    case TRUE:
		if (!GetConsecutiveLength(handle, clustertoplace, &length))
		    RETURN_FTEERR(FAIL);

                if (length > maxFreeSpace)   length = maxFreeSpace;

                DrawMoreOnDriveMap(clustertoplace, READSYMBOL, length);
                DrawMoreOnDriveMap(clustertobereplaced, WRITESYMBOL, length);

		// Make sure the clusters are not taken by the virtual file system
		if (!EnsureClustersFree(handle, clustertobereplaced, length))
                {
                    SetCustomError(VFS_RELOC_FAILED);
		    RETURN_FTEERR(FAIL);
                }

                if (!RelocateOverlapping(handle, clustertoplace, clustertobereplaced, length))
                {
                    SetCustomError(RELOC_OVERLAPPING_FAILED);
                    RETURN_FTEERR(FAIL);
                }

                /* The start of the file is no longer located at this position */
                if (FastSelectMap)
                    ClearVFSBitfieldBit(FastSelectMap, clustertoplace);

                if (!MoveFragmentBlock(clustertoplace, clustertobereplaced, length))
                {                    
                    SetCustomError(RELOC_OVERLAPPING_FAILED);
                    RETURN_FTEERR(FAIL);
                }

		DrawMoreOnDriveMap(clustertoplace, UNUSEDSYMBOL, length);
		DrawMoreOnDriveMap(clustertobereplaced, OPTIMIZEDSYMBOL, length);

                clustertoplace = GetNthFileCluster(handle, clustertobereplaced, length,
                    &pastTheEnd);
                if (!clustertoplace)
                {
                    SetCustomError(GET_FILECLUSTER_FAILED);
                    RETURN_FTEERR(FAIL);
                }

		clustertobereplaced += length;

                relocatedSize += length;
                break;

            case FALSE:
                DrawMoreOnDriveMap(clustertoplace, READSYMBOL, spaceFree);
                DrawMoreOnDriveMap(clustertobereplaced, WRITESYMBOL, spaceFree);

		// Make sure the clusters are not taken by the virtual file system
		if (!EnsureClustersFree(handle, clustertoplace, spaceFree))
                {
                    SetCustomError(VFS_RELOC_FAILED);
		    RETURN_FTEERR(FAIL);
                }

                if (!RelocateClusterSequence(handle, clustertoplace, clustertobereplaced, spaceFree))
                {
                    SetCustomError(RELOC_SEQ_FAILED);
                    RETURN_FTEERR(FAIL);
                }

		DrawMoreOnDriveMap(clustertoplace, UNUSEDSYMBOL, spaceFree);
		DrawMoreOnDriveMap(clustertobereplaced, OPTIMIZEDSYMBOL, spaceFree);

                /* The start of the file is no longer located at this position */
                if (FastSelectMap)
                    ClearVFSBitfieldBit(FastSelectMap, clustertoplace);

                if (!MoveFragmentBlock(clustertoplace, clustertobereplaced, spaceFree))
                {                    
                    SetCustomError(RELOC_OVERLAPPING_FAILED);
                    RETURN_FTEERR(FAIL);
                }

		clustertoplace = GetNthFileCluster(handle, clustertobereplaced, spaceFree,
		    &pastTheEnd);
		if (!clustertoplace)
                {
                    SetCustomError(GET_FILECLUSTER_FAILED);
		    RETURN_FTEERR(FAIL);
                }

		relocatedSize += spaceFree;
		clustertobereplaced += spaceFree;
		break;

            case FAIL:
                RETURN_FTEERR(FAIL);

            }
        }  
        else
        {
            int couldnotmove = 0;

            /* See wether we come across something that should not be moved */
            for (; clustertobereplaced < labelsinfat; clustertobereplaced++)
            {
                if (!GetNthCluster(handle, clustertobereplaced, &label))
                {
                    SetCustomError(GET_FILECLUSTER_FAILED);
                    RETURN_FTEERR(FAIL);    
                }

                /* See wether the cluster is movable */
                if (!IsClusterMovable(handle, clustertobereplaced, &isMovable))
                    RETURN_FTEERR(FAIL);

                if (FAT_BAD(label) || !isMovable)
                {
                    *stop = *stop + 1;
                    clustertobereplaced++;
                    couldnotmove = 1;
                }
                else
                    break;
            }

	    if (!couldnotmove)
	    {
		/* No space, make space */

                /* Get the size of the rest of the file that is in the way */
                blockingFileSize = CalculateFileSize(handle, clustertobereplaced);
                if (blockingFileSize == FAIL)
                {
                    SetCustomError(GET_FILESIZE_FAILED);
                    RETURN_FTEERR(FAIL);
                }

                if (blockingFileSize > maxFreeSpace)
                    blockingFileSize = maxFreeSpace;

		/* Relocate it to the next fitting free space */
		switch (FindFirstFittingFreeSpace(handle, blockingFileSize, &freespace))
		{
		case FAIL:
                    SetCustomError(GET_FREESPACE_ERROR);
		    RETURN_FTEERR(FAIL);

                case TRUE:
                    DrawMoreOnDriveMap(clustertobereplaced, READSYMBOL, blockingFileSize);
                    DrawMoreOnDriveMap(freespace, WRITESYMBOL, blockingFileSize);

                    // Make sure the clusters are not taken by the virtual file system
                    if (!EnsureClustersFree(handle, freespace, blockingFileSize))
                    {
                        SetCustomError(VFS_RELOC_FAILED);
                        RETURN_FTEERR(FAIL);
                    }

		    if (!RelocateClusterSequence(handle, clustertobereplaced, freespace, blockingFileSize))
                    {
                        SetCustomError(RELOC_SEQ_FAILED);
                        RETURN_FTEERR(FAIL);
                    }

                    if (!MoveFragmentBlock(clustertobereplaced, freespace, blockingFileSize))
                    {                    
                        SetCustomError(RELOC_OVERLAPPING_FAILED);
                        RETURN_FTEERR(FAIL);
                    }

                    // If the cluster that is in the way is the first one of the file,
                    // then we moved that first cluster to the free space
                    if (FastSelectMap)
                    {
                        if (!GetVFSBitfieldBit(FastSelectMap, clustertobereplaced, &value))
                        {
                            SetCustomError(VFS_GET_FAILED);
                            RETURN_FTEERR(FAIL);
                        }

                        if (value)
                            if (!SetVFSBitfieldBit(FastSelectMap, freespace))
                            {
                                SetCustomError(VFS_SET_FAILED);
                                RETURN_FTEERR(FAIL);
                            }
                    }

		    DrawMoreOnDriveMap(clustertobereplaced, UNUSEDSYMBOL, blockingFileSize);
		    DrawMoreOnDriveMap(freespace, USEDSYMBOL, blockingFileSize);
		    break;

                case FALSE:
                    /* No fitting free space? Move as much as possible to the next free space */
                    if (!FindFirstFreeSpace(handle, &freespace, &freeLength))
                    {
                        SetCustomError(GET_FREESPACE_ERROR);
                        RETURN_FTEERR(FAIL);
                    }

                    if (freeLength > maxFreeSpace)
                        freeLength = maxFreeSpace;

                    DrawMoreOnDriveMap(clustertobereplaced, READSYMBOL, freeLength);
                    DrawMoreOnDriveMap(freespace, WRITESYMBOL, freeLength);

		    if (!EnsureClustersFree(handle, freespace, freeLength))
                    {
                        SetCustomError(VFS_RELOC_FAILED);
			RETURN_FTEERR(FAIL);
                    }

                    if (!RelocateClusterSequence(handle, clustertobereplaced, freespace, freeLength))
                    {
                        SetCustomError(RELOC_SEQ_FAILED);
                        RETURN_FTEERR(FAIL);
                    }

                    if (!MoveFragmentBlock(clustertobereplaced, freespace, freeLength))
                    {                    
                        SetCustomError(RELOC_OVERLAPPING_FAILED);
                        RETURN_FTEERR(FAIL);
                    }

		    // If the cluster that is in the way is the first one of the file,
		    // then we moved that first cluster to the free space
		    if (FastSelectMap)
		    {
                        int value; 

			if (!GetVFSBitfieldBit(FastSelectMap, clustertobereplaced, &value))
                        {
                            SetCustomError(VFS_GET_FAILED);
                            RETURN_FTEERR(FAIL);
                        }

                        if (value)
                        {
			    if (!SetVFSBitfieldBit(FastSelectMap, freespace))
                            {
                                SetCustomError(VFS_SET_FAILED);
			        RETURN_FTEERR(FAIL);
                            }
                        }
		    }

                    DrawMoreOnDriveMap(clustertobereplaced, UNUSEDSYMBOL, freeLength);
                    DrawMoreOnDriveMap(freespace, USEDSYMBOL, freeLength);
                }
            }
        }
    }

    return TRUE;
}

#if 0
BOOL FullDefragPlace(RDWRHandle handle, CLUSTER clustertoplace,
                     CLUSTER clustertobereplaced, CLUSTER* stop)
{
    BOOL pastTheEnd = FALSE, isMovable;
    CLUSTER label;

    /*
        Now try moving all the clusters to the front of the disk:
        - If the target cluster is free, just move it to the given location.
        - If the target cluster is not movable go on to the next cluster.
        - Otherwise swap the target cluster with the cluster in the file
          to defragment.
        - Do not move bad clusters.
        - Do not swap a cluster with itself.
    */
   
    clustertobereplaced--;
    while (!pastTheEnd)
    {
        do {
      clustertobereplaced++;

           if (!GetNthCluster(handle, clustertobereplaced, &label))
              RETURN_FTEERR(FAIL);     

      /* See wether the cluster is movable */
      if (!IsClusterMovable(handle, clustertobereplaced, &isMovable))
         RETURN_FTEERR(FAIL);

   } while (FAT_BAD(label) || !isMovable);

        DrawOnDriveMap(clustertoplace, READSYMBOL);
        DrawOnDriveMap(clustertobereplaced, WRITESYMBOL);

        SwapClustersInFastSelectMap(clustertoplace, clustertobereplaced);
        
   if (FAT_FREE(label))
   {
           if (!RelocateCluster(handle, clustertoplace, clustertobereplaced))
         RETURN_FTEERR(FAIL);

           DrawOnDriveMap(clustertoplace, UNUSEDSYMBOL);
           DrawOnDriveMap(clustertobereplaced, OPTIMIZEDSYMBOL);
   }
   else
   {
      if (clustertoplace != clustertobereplaced)
      {
              if (!SwapClusters(handle, clustertoplace, clustertobereplaced))
                 RETURN_FTEERR(FAIL);
           }
           
           DrawOnDriveMap(clustertoplace, USEDSYMBOL);
           DrawOnDriveMap(clustertobereplaced, OPTIMIZEDSYMBOL);
        }
   clustertoplace = GetNthFileCluster(handle, clustertobereplaced, 1,
                  &pastTheEnd);
   if (!clustertoplace) RETURN_FTEERR(FAIL);
    }

    *stop = clustertobereplaced+1;
    return TRUE;
}
#endif
