/*    
   Crunch.c - Move all data to the front of the disk.

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

#include "fte.h"
#include "..\dtstruct\ClMovMap.h"
#include "defrmap.h"
#include "..\..\modlgate\expected.h"
#include "..\..\modlgate\custerr.h"

#if 1

static BOOL GetConsecutiveLength(RDWRHandle handle, CLUSTER cluster, unsigned long* length)
{
    CLUSTER label;
    unsigned long labelsinfat;
    
    *length = 1;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) 
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FALSE);
    }

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

#if 0

static BOOL MoveFittingToFreeSpace(RDWRHandle handle, CLUSTER cluster,
               CLUSTER lastfree, unsigned long length,
               BOOL* lastMoved)
{
    int value;
    CLUSTER current;
    unsigned long fileSize;
    VirtualDirectoryEntry* file;

    file = GetFastSelectMap();
     
    *lastMoved = TRUE;

    for (current = lastfree-1; current > cluster; current--)
    {
   if (!GetVFSBitfieldBit(file, current, &value))
       RETURN_FTEERR(FAIL);

   if (value) // First cluster of some file
   {
       fileSize  = CalculateFileSize(handle, current);
       if (fileSize == FAIL)
      RETURN_FTEERR(FAIL);

       if (fileSize == length)
       {
      DrawMoreOnDriveMap(cluster, WRITESYMBOL, length);
      DrawMoreOnDriveMap(current, READSYMBOL, length);

                if (!EnsureClustersFree(handle, cluster, length))
                    RETURN_FTEERR(FAIL);

      if (!RelocateClusterSequence(handle, current, cluster, length))
          RETURN_FTEERR(FAIL);

      DrawMoreOnDriveMap(cluster, OPTIMIZEDSYMBOL, length);
      DrawMoreOnDriveMap(current, UNUSEDSYMBOL, length);

      if (!ClearVFSBitfieldBit(file, current))
         RETURN_FTEERR(FAIL);

      return TRUE;
       }
       else
       {
      *lastMoved = FALSE;
       }
   }
    }

    RETURN_FTEERR(FALSE);
}

static BOOL MoveLastFittingToFreeSpace(RDWRHandle handle, CLUSTER cluster,
                   CLUSTER lastfree, unsigned long length,
                   unsigned long* skipped,
                   BOOL* lastMoved)
{
    int value;
    CLUSTER current;
    unsigned long fileSize;
    VirtualDirectoryEntry* file;
RETURN_FTEERR(FALSE);  
    file = GetFastSelectMap();

    *lastMoved = TRUE;

    for (current = lastfree-1; current > cluster; current--)
    {
   if (!GetVFSBitfieldBit(file, current, &value))
       RETURN_FTEERR(FAIL);

   if (value) // First cluster of some file
   {
       fileSize  = CalculateFileSize(handle, current);
       if (fileSize == FAIL) return FAIL;

       if (fileSize < length)
       {
      DrawMoreOnDriveMap(cluster, WRITESYMBOL, fileSize);
      DrawMoreOnDriveMap(current, READSYMBOL, fileSize);

                if (!EnsureClustersFree(handle, cluster, fileSize))
                    RETURN_FTEERR(FAIL);

      if (!RelocateClusterSequence(handle, current, cluster, fileSize))
          RETURN_FTEERR(FAIL);
      
      DrawMoreOnDriveMap(cluster, OPTIMIZEDSYMBOL, fileSize);
      DrawMoreOnDriveMap(current, UNUSEDSYMBOL, fileSize);

      if (!ClearVFSBitfieldBit(file, current))
         RETURN_FTEERR(FAIL);

      *skipped = fileSize;
      return TRUE;
       }
       else
       {
      *lastMoved = FALSE;
       }
   }    
    }

    RETURN_FTEERR(FALSE);
}

#endif

static BOOL GetFirstClusterOfLastFreeSpace(RDWRHandle handle,
                  CLUSTER* firstOfLastFree)
{
    unsigned long labelsinfat;
    CLUSTER current, label;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
        SetCustomError(WRONG_LABELSINFAT);        
        RETURN_FTEERR(FALSE);
    }

    *firstOfLastFree = labelsinfat;

    for (current = labelsinfat-1; current >= 2; current--)
    {
   if (!GetNthCluster(handle, current, &label))
   {
      SetCustomError(CLUSTER_RETRIEVE_ERROR);             
      RETURN_FTEERR(FALSE);
   }

   if (!FAT_FREE(label))
           return TRUE;

   *firstOfLastFree = current;
    }

    return TRUE;
}

BOOL CrunchVolume(RDWRHandle handle)
{
    BOOL isMovableS, isMovableP;
    CLUSTER seeker=2, placer=2, label, firstOfLastFree;
    unsigned long labelsinfat;
    unsigned long freeSpace, consecutiveSize, relocatingSize;
    unsigned long maxFreeSpace;  
    unsigned long totalfree;

    /* First see wether there is any free space */
    if (!GetFreeDiskSpace(handle, &totalfree))
        RETURN_FTEERR(FALSE);

    if (totalfree == 0)
        return TRUE;        // No free space => crunch done

    if (!GetFirstClusterOfLastFreeSpace(handle, &firstOfLastFree))
    {
        RETURN_FTEERR(FALSE);
    }

    if (!GetMaxRelocatableSize(handle, &maxFreeSpace))
    {
        SetCustomError(GET_MAXRELOC_FAILED);
        RETURN_FTEERR(FALSE);
    }

    if (maxFreeSpace == 0)
    {
        SetCustomError(INSUFFICIENT_FREE_DISK_SPACE);        
    }

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FALSE);
    }

    while (seeker < firstOfLastFree)
    {
        if (QuerySaveState())
            return TRUE;        /* The user requested the process to stop */

	IndicatePercentageDone(seeker, firstOfLastFree);

        if (!GetNthCluster(handle, seeker, &label))
        {
            SetCustomError(CLUSTER_RETRIEVE_ERROR);             
            RETURN_FTEERR(FALSE);
        }

        if (!IsClusterMovable(handle, seeker, &isMovableS))
            RETURN_FTEERR(FALSE);
        if (!IsClusterMovable(handle, placer, &isMovableP))
            RETURN_FTEERR(FALSE);

        if (!FAT_FREE(label) && !FAT_BAD(label) && isMovableS)
        {
            if (!GetNthCluster(handle, placer, &label))
            {
                SetCustomError(CLUSTER_RETRIEVE_ERROR);             
                RETURN_FTEERR(FALSE);
            }

            if (FAT_FREE(label))
            {
                freeSpace = CountFreeSpace(handle, placer, maxFreeSpace);
                if (freeSpace == FAIL) 
                {
                    SetCustomError(GET_FREESPACE_ERROR);             
                    RETURN_FTEERR(FALSE);
                }

                if (!GetConsecutiveLength(handle, seeker, &consecutiveSize))
                    RETURN_FTEERR(FALSE);

                if (placer + freeSpace == seeker)   /* Beware of bad and non movable clusters */
                    relocatingSize = consecutiveSize;
                else
                    relocatingSize = (freeSpace < consecutiveSize) ? freeSpace : consecutiveSize;

                DrawMoreOnDriveMap(placer, WRITESYMBOL, relocatingSize);
                DrawMoreOnDriveMap(seeker, READSYMBOL, relocatingSize);

                if (!EnsureClustersFree(handle, placer, relocatingSize))
                {
                    SetCustomError(VFS_RELOC_FAILED);
                    RETURN_FTEERR(FALSE);
                }

                if (!RelocateOverlapping(handle, seeker, placer, relocatingSize))
                {
                    SetCustomError(RELOC_OVERLAPPING_FAILED);
                    RETURN_FTEERR(FALSE);
                }

		DrawMoreOnDriveMap(seeker, UNUSEDSYMBOL, relocatingSize);
		DrawMoreOnDriveMap(placer, OPTIMIZEDSYMBOL, relocatingSize);

                seeker = seeker + relocatingSize - 1;
                placer = placer + relocatingSize - 1;
            }
            else if (FAT_BAD(label) || !isMovableP)
            {
                seeker--;
            }
            else
            {
                DrawOnDriveMap(placer, OPTIMIZEDSYMBOL);
            }
            placer++;
        }

        seeker++;
    }

    IndicatePercentageDone(labelsinfat, labelsinfat);
    return TRUE;
}

#endif

#if 0

static BOOL MoveFittingToFreeSpace(RDWRHandle handle, CLUSTER cluster,
               CLUSTER lastfree, unsigned long length,
               BOOL* lastMoved)
{
    int value;
    CLUSTER current;
    unsigned long labelsinfat, fileSize;
    VirtualDirectoryEntry* file;
  
    file = GetFastSelectMap();
     
    *lastMoved = TRUE;

    for (current = lastfree-1; current > cluster; current--)
    {
   if (!GetVFSBitfieldBit(file, current, &value))
       RETURN_FTEERR(FAIL);

   if (value) // First cluster of some file
   {
       fileSize  = CalculateFileSize(handle, current);
       if (fileSize == FAIL)
      RETURN_FTEERR(FAIL);

       if (fileSize == length)
       {
      DrawMoreOnDriveMap(cluster, WRITESYMBOL, length);
      DrawMoreOnDriveMap(current, READSYMBOL, length);

      if (!RelocateClusterSequence(handle, current, cluster, length))
          RETURN_FTEERR(FAIL);

      DrawMoreOnDriveMap(cluster, USEDSYMBOL, length);
      DrawMoreOnDriveMap(current, UNUSEDSYMBOL, length);

      if (!ClearVFSBitfieldBit(file, current))
         RETURN_FTEERR(FAIL);

      return TRUE;
       }
       else
       {
      *lastMoved = FALSE;
       }
   }
    }

    RETURN_FTEERR(FALSE);
}

static BOOL MoveLastFittingToFreeSpace(RDWRHandle handle, CLUSTER cluster,
                   CLUSTER lastfree, unsigned long length,
                   unsigned long* skipped,
                   BOOL* lastMoved)
{
    int value;
    CLUSTER current;
    unsigned long labelsinfat, fileSize;
    VirtualDirectoryEntry* file;
  
    file = GetFastSelectMap();

    *lastMoved = TRUE;

    for (current = lastfree-1; current > cluster; current--)
    {
   if (!GetVFSBitfieldBit(file, current, &value))
       RETURN_FTEERR(FAIL);

   if (value) // First cluster of some file
   {
       fileSize  = CalculateFileSize(handle, current);
       if (fileSize == FAIL) RETURN_FTEERR(FAIL);

       if (fileSize < length)
       {
      DrawMoreOnDriveMap(cluster, WRITESYMBOL, length);
      DrawMoreOnDriveMap(current, READSYMBOL, length);

      if (!RelocateClusterSequence(handle, current, cluster, fileSize))
          RETURN_FTEERR(FAIL);
      
      DrawMoreOnDriveMap(cluster, USEDSYMBOL, length);
      DrawMoreOnDriveMap(current, UNUSEDSYMBOL, length);

      if (!ClearVFSBitfieldBit(file, current))
         RETURN_FTEERR(FAIL);

      *skipped = fileSize;
      return TRUE;
       }
       else
       {
      *lastMoved = FALSE;
       }
   }    
    }

    RETURN_FTEERR(FALSE);
}

static BOOL GetFirstClusterOfLastFreeSpace(RDWRHandle handle,
                  CLUSTER* firstOfLastFree)
{
    unsigned long labelsinfat;
    CLUSTER current, label;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) RETURN_FTEERR(FALSE);

    *firstOfLastFree = labelsinfat;

    for (current = labelsinfat-1; current >= 2; current--)
    {
   if (!GetNthCluster(handle, current, &label))
           RETURN_FTEERR(FALSE);

   if (!FAT_FREE(label))
           return TRUE;

   *firstOfLastFree = current;
    }

    return TRUE;
}

BOOL CrunchVolume(RDWRHandle handle)
{
    BOOL wholeFound, lastMoved;
    unsigned long labelsinfat, skipped, length;
    CLUSTER current, label, firstFreeCluster=2, cluster;
    CLUSTER firstOfLastFree;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) RETURN_FTEERR(FALSE);

    if (CreateFastSelectMap(handle) == FAIL)
   RETURN_FTEERR(FALSE);

    if (!GetFirstClusterOfLastFreeSpace(handle, &firstOfLastFree))
       RETURN_FTEERR(FALSE);

    /* We crunch in two parts: */    
    do 
    {
         if (QuerySaveState())
            return TRUE;        /* The user requested the process to stop */

    IndicatePercentageDone(firstFreeCluster, firstOfLastFree);

    if (firstFreeCluster == firstOfLastFree)
       break;

   /*
       1) * go through the disk, find any free spaces
          * go back from the end of the disk, and try to find a file that is exactly as large as
       the free space.
          * relocate that file to the free space

       if there is no file large enough, move the last file on the disk, that is small enough to
       fit the free space!, to the free space.
   */
   for (current = firstFreeCluster; current < firstOfLastFree; current++)
   {
       if (!GetNthCluster(handle, current, &label))
      RETURN_FTEERR(FALSE);

       if (FAT_FREE(label))
       {
      if (!CalculateFreeSpace(handle, current, &length))
          RETURN_FTEERR(FALSE);

      switch (MoveFittingToFreeSpace(handle, current, firstOfLastFree, length, &lastMoved))
      {
      case TRUE:
          current += length-1;
          if (lastMoved)
             firstOfLastFree -= length;
          break;
      case FALSE:
          switch (MoveLastFittingToFreeSpace(handle, current, firstOfLastFree, length, &skipped, &lastMoved))
          {
          case TRUE:
         current += skipped-1;
         if (lastMoved)
            firstOfLastFree -= skipped;
         break;
          case FAIL:
         RETURN_FTEERR(FALSE);     
          }
          break;
      case FAIL:
          RETURN_FTEERR(FALSE);
      }  
       }
   } 

   /*
       2) find the first free spot, move the part after the spot to the front.
   */
   wholeFound = FALSE;
   for (current = firstFreeCluster; current < firstOfLastFree; current++)
   {
       if (!GetNthCluster(handle, current, &label))
      RETURN_FTEERR(FALSE);

       if (FAT_FREE(label))
       {
      for (cluster = current; cluster < firstOfLastFree; cluster++)
      {
             if (!GetNthCluster(handle, current, &label))
         RETURN_FTEERR(FALSE);
          
          if (FAT_NORMAL(label) || FAT_LAST(label))
          {
         wholeFound = TRUE;

         if (!GetConsecutiveLength(handle, cluster, &length))
             RETURN_FTEERR(FALSE);

         DrawMoreOnDriveMap(cluster, READSYMBOL, length);
         DrawMoreOnDriveMap(current, WRITESYMBOL, length);

         if (!RelocateOverlapping(handle, cluster, current, length))
             RETURN_FTEERR(FALSE);

         DrawMoreOnDriveMap(cluster, UNUSEDSYMBOL, length);
         DrawMoreOnDriveMap(current, USEDSYMBOL, length);

         firstFreeCluster += length;
         break;
          }
      }
      break;
       }
   }

   /* Repeat until all free space is at the end of the disk.  */
    } while (wholeFound);

    IndicatePercentageDone(labelsinfat, labelsinfat);
    return TRUE;
}

#endif
