/*    
   Defrmap.c - speed up bitfields.

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

#include <stdlib.h>

#include "fte.h"
#include "..\dtstruct\ClMovMap.h"
#include "unfrgfls\IsFlFgtd.h"
#include "..\..\modlgate\custerr.h"

static BOOL FirstClusterMarker(RDWRHandle handle, 
                               struct DirectoryPosition* pos,
                               void** structure);
static BOOL ContinousFileMarker(RDWRHandle handle, 
                                struct DirectoryPosition* pos,
                                void** structure);
static BOOL ContinousClusterMarker(RDWRHandle handle, CLUSTER label,
                                   SECTOR datsector, void** structure);                                
                               

static VirtualDirectoryEntry* FastSelectMap = NULL;
static VirtualDirectoryEntry* NotFragmentedMap = NULL;

BOOL CreateFastSelectMap(RDWRHandle handle)
{
     unsigned long LabelsInFat;
     
     LabelsInFat = GetLabelsInFat(handle);
     if (!LabelsInFat)
     {    
         SetCustomError(WRONG_LABELSINFAT);     
         RETURN_FTEERR(FAIL);
     }

     FastSelectMap = CreateVFSBitField(handle, LabelsInFat);
     if (!FastSelectMap)
     {
         SetCustomError(VFS_ALLOC_FAILED);
         RETURN_FTEERR(FAIL);
     }

     if (!WalkDirectoryTree(handle, FirstClusterMarker, (void**) NULL))
     {
        SetCustomError(GET_FIRSTFILES_FAILED);
	RETURN_FTEERR(FAIL);
     }

     return TRUE;
}

static BOOL FirstClusterMarker(RDWRHandle handle, 
                               struct DirectoryPosition* pos,
                               void** structure)
{
    CLUSTER cluster;
    struct DirectoryEntry entry;
    BOOL isMovable;
    
    if (structure);
     
    if (!GetDirectory(handle, pos, &entry))
    {
       RETURN_FTEERR(FAIL);
    }

    /*
       Don't take LFN entries into consideration.
    */
    if (IsLFNEntry(&entry)) return TRUE;

    /*
       Be carefull for hidden or system files.
    */
    if ((entry.attribute & FA_HIDDEN) || (entry.attribute & FA_SYSTEM) ||
        IsDeletedLabel(entry))
    {
       return TRUE;
    }     
    
    cluster = GetFirstCluster(&entry);
    if (cluster)
    {
       if (!IsClusterMovable(handle, cluster, &isMovable))
          RETURN_FTEERR(FAIL);

       if (isMovable)
       {
          if (!SetVFSBitfieldBit(FastSelectMap, cluster))
	      RETURN_FTEERR(FAIL);
       }
    }
    
    return TRUE;
}

void DestroyFastSelectMap(void)
{
    if (FastSelectMap)
    {
       DestroyVFSBitfield(FastSelectMap);
       FastSelectMap = NULL;
    }
}

VirtualDirectoryEntry* GetFastSelectMap(void)
{
    return FastSelectMap;
}

BOOL SwapClustersInFastSelectMap(CLUSTER cluster1, CLUSTER cluster2)
{
     if (!FastSelectMap)
     {    
         SetCustomError(FIRST_CLUSTER_UPDATE_FAILED);
         RETURN_FTEERR(FALSE);
     }
     
     if (!SwapVFSBitfieldBits(FastSelectMap, cluster1, cluster2))
     {
         SetCustomError(FIRST_CLUSTER_UPDATE_FAILED);
	 RETURN_FTEERR(FALSE);
     }

     if (NotFragmentedMap)
        if (!SwapVFSBitfieldBits(NotFragmentedMap, cluster1, cluster2))
        {
            SetCustomError(FIRST_CLUSTER_UPDATE_FAILED);
	    RETURN_FTEERR(FALSE);
        }

     return TRUE;
}

BOOL CreateNotFragmentedMap(RDWRHandle handle)
{
     BOOL isMovable;
     unsigned long LabelsInFat, i;

     LabelsInFat = GetLabelsInFat(handle);
     if (!LabelsInFat) 
     {    
         SetCustomError(WRONG_LABELSINFAT);
         RETURN_FTEERR(FAIL);
     }

     NotFragmentedMap = CreateVFSBitField(handle, LabelsInFat);
     if (!NotFragmentedMap)
     {
         SetCustomError(VFS_ALLOC_FAILED);
         RETURN_FTEERR(FAIL);
     }

     if (!WalkDirectoryTree(handle, ContinousFileMarker, (void**) NULL))
     {
        SetCustomError(GET_CONTINOUS_FILES_ERROR);   
        RETURN_FTEERR(FAIL);
     }

     // The files that are not movable also count as not fragmented
     for (i=0; i < LabelsInFat; i++)
     {
	 if (!IsClusterMovable(handle, i, &isMovable))
	    RETURN_FTEERR(FAIL);

	 if (!isMovable)
	 {
	    if (!ClearVFSBitfieldBit(FastSelectMap, i))
            {
               SetCustomError(VFS_GET_FAILED);  
	       RETURN_FTEERR(FAIL);
            }
	    if (!SetVFSBitfieldBit(NotFragmentedMap, i))
            {
               SetCustomError(VFS_GET_FAILED);  
	       RETURN_FTEERR(FAIL);
            }
	 }
     }

     return TRUE;
}

void DestroyNotFragmentedMap()
{
     if (NotFragmentedMap)
     {
	DestroyVFSBitfield(NotFragmentedMap);
	NotFragmentedMap = NULL;
     }
}

static BOOL ContinousFileMarker(RDWRHandle handle, 
                                struct DirectoryPosition* pos,
                                void** structure)
{
    CLUSTER cluster;
    struct DirectoryEntry entry;
    BOOL isMovable;
    
    if (structure);
     
    if (!GetDirectory(handle, pos, &entry))
    {
       RETURN_FTEERR(FAIL);
    }

    /*
       Don't take LFN entries into consideration.
    */
    if (IsLFNEntry(&entry)) return TRUE;

    /*
       Be carefull for hidden or system files.
    */
    if ((entry.attribute & FA_HIDDEN) || (entry.attribute & FA_SYSTEM) ||
        IsDeletedLabel(entry))
    {
       return TRUE;
    }     
    
    cluster = GetFirstCluster(&entry);
    if (cluster)
    {
       if (!IsClusterMovable(handle, cluster, &isMovable))
          RETURN_FTEERR(FAIL);

       if (isMovable)
       {
          switch (IsFileFragmented(handle, cluster))
          {
            case FALSE:
                 if (!FileTraverseFat(handle, cluster, 
                                      ContinousClusterMarker,
                                      (void**) NULL))
                 {
                    RETURN_FTEERR(FAIL);
                 }
                 break;
                 
            case FAIL:
                 RETURN_FTEERR(FAIL);
          }
       }
    }
    
    return TRUE;     
}     

static BOOL ContinousClusterMarker(RDWRHandle handle, CLUSTER label,
                                   SECTOR datasector, void** structure)  
{
    CLUSTER cluster;
    
    if (label);
    if (structure);
    
    cluster = DataSectorToCluster(handle, datasector);
    if (!cluster) RETURN_FTEERR(FAIL);
      
    if (!SetVFSBitfieldBit(NotFragmentedMap, cluster))
	RETURN_FTEERR(FAIL);
    if (!ClearVFSBitfieldBit(FastSelectMap, cluster))
	RETURN_FTEERR(FAIL);
    
    return TRUE;
}

VirtualDirectoryEntry* GetNotFragmentedMap(void)
{
    return NotFragmentedMap; 
}

BOOL MarkFileAsContinous(CLUSTER place, unsigned long length)
{
     unsigned long i;
     
     for (i = 0; i < length; i++)
     {
	 if (!SetVFSBitfieldBit(NotFragmentedMap, place+i))
         {
             SetCustomError(VFS_SET_FAILED);
	     RETURN_FTEERR(FALSE);
         }
	 if (!ClearVFSBitfieldBit(FastSelectMap, place+i))
         {
             SetCustomError(VFS_SET_FAILED);
	     RETURN_FTEERR(FALSE);
         }
     }

     return TRUE;
}

BOOL MoveFragmentBlock(CLUSTER source, CLUSTER destination, unsigned long length)
{
     int value;
     unsigned long i;

     if (NotFragmentedMap && (source != destination))
     {
        for (i = 0; i < length; i++)
        {
            if (!GetVFSBitfieldBit(NotFragmentedMap, source+i, &value))
            {
                SetCustomError(VFS_SET_FAILED);
                RETURN_FTEERR(FALSE);
            }

            if (value)
            {
                if (!SetVFSBitfieldBit(NotFragmentedMap, destination+i))
                {
                    SetCustomError(VFS_SET_FAILED);
                    RETURN_FTEERR(FALSE);
                }
            }
            else
            {
                if (!ClearVFSBitfieldBit(NotFragmentedMap, destination+i))
                {
                    SetCustomError(VFS_SET_FAILED);
                    RETURN_FTEERR(FALSE);
                }            
            }

            if (!ClearVFSBitfieldBit(NotFragmentedMap, source+i))
            {
                SetCustomError(VFS_SET_FAILED);
                RETURN_FTEERR(FALSE);
            }            
        }
     }

     return TRUE;
}


