/*    
   QuickTry.c - code to quick try optimize a drive.

   Copyright (C) 2007 Imre Leber

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
   email me at:  imre.leber@telenet.be
*/

/*
    Go through all directories on the volume.

    For each file:
    - see wether it is movable
    - see if it is fragmented
    - see if wether there is enough disk space left to move the file consecutively
    - move the file consecutively to the new location
*/

#include <stdlib.h>

#include "FTE.h"
#include "QuickTry\POWKTree.h"
#include "..\dtstruct\ClMovMap.h"
#include "unfrgfls\Isflfgtd.h"
#include "..\..\modlgate\expected.h"
#include "QuickTry\RelocFil.h"
#include "..\..\modules\infofat\infofat.h"

struct PipeStruct 
{
    BOOL bFileMoved;
    BOOL bDoComplete;
    unsigned long FileCount;
    unsigned long runCount;
};

static BOOL QuickTryWalker(RDWRHandle handle, struct DirectoryPosition* position, void** structure);
static BOOL IndicateFileAsContinous(RDWRHandle handle, CLUSTER firstcluster);
static BOOL CompleteQuickTryIndicateComplete(RDWRHandle handle, struct DirectoryPosition* position, void** structure);

BOOL QuickTryDefragmentVolume(RDWRHandle handle)
{
    struct PipeStruct pipe;
    struct PipeStruct* ppipe = &pipe;

    pipe.bFileMoved = FALSE;
    pipe.bDoComplete = FALSE;
    pipe.FileCount = 0;

    if (!PostOrderWalkDirectoryTree(handle, QuickTryWalker, (void**)&ppipe))
    {
        RETURN_FTEERR(FALSE);    
    }

    IndicatePercentageDone(100, 100);  

    return TRUE;
}

BOOL CompleteQuickTryDefragmentVolume(RDWRHandle handle)
{   
    struct PipeStruct pipe;
    struct PipeStruct* ppipe = &pipe;

    pipe.bDoComplete = TRUE;
    pipe.runCount = 0;

    do 
    {
        pipe.bFileMoved = FALSE;
        pipe.FileCount = 0;
        pipe.runCount++;

        if (!PostOrderWalkDirectoryTree(handle, QuickTryWalker, (void**)&ppipe))
        {
            RETURN_FTEERR(FALSE);    
        }
    } while (pipe.bFileMoved);

    /* We are now at the end, indicate all files continous 
    if (!WalkDirectoryTree(handle, CompleteQuickTryIndicateComplete, NULL))
    {
        RETURN_FTEERR(FALSE);    
    }
    */
    IndicatePercentageDone(100, 100);  

    return TRUE;    
}

static BOOL QuickTryWalker(RDWRHandle handle, struct DirectoryPosition* position, void** structure)
{
    struct DirectoryEntry entry;
    CLUSTER firstcluster;
    unsigned long fileSize;
    CLUSTER freespace;
    struct PipeStruct* pipe = *((struct PipeStruct**) structure); 
    
    if (QuerySaveState())
        return FALSE;        /* The user requested the process to stop */

    if (!GetDirectory(handle, position, &entry))
    {
        pipe->bFileMoved = FALSE;
        RETURN_FTEERR(FAIL);    
    }
    
    if ((entry.attribute & FA_LABEL) ||
        IsCurrentDir(entry)          ||
        IsPreviousDir(entry)         ||
        IsLFNEntry(&entry)           ||
        IsDeletedLabel(entry))
    {
        return TRUE;
    }

    pipe->FileCount++;

    /* Update the percentage complete bar */
    if (pipe->bDoComplete)
    {
        /* This is tricky!
           Assume at least two runs */
        if (pipe->runCount == 1)
            IndicatePercentageDone(pipe->FileCount, GetRememberedFileCount()*2);    
        else if (pipe->runCount == 2)
            IndicatePercentageDone(GetRememberedFileCount()+pipe->FileCount, GetRememberedFileCount()*2+1);    
        else
            IndicatePercentageDone(99, 100);    
    }
    else
    {
        IndicatePercentageDone(pipe->FileCount, GetRememberedFileCount());    
    }

    /* See if it is movable */
    if (!(entry.attribute & FA_HIDDEN) || (entry.attribute & FA_SYSTEM))
    {
        /* See if it is fragmented */
        firstcluster = GetFirstCluster(&entry);

        if (firstcluster)
        {
            fileSize = CalculateFileSize(handle, firstcluster);

            switch (IsFileFragmented(handle, firstcluster))
            {        
            case TRUE:
                /* See wether there is space to relocate the file */ 

                if (fileSize == FAIL)
                    RETURN_FTEERR(FAIL);

                if (fileSize > 0)
                {
                    switch (FindFirstFittingFreeSpace(handle, 
                        fileSize,
                        &freespace))
                    {
                    case FAIL:
                        RETURN_FTEERR(FAIL);
                    case FALSE:
                        /* Not enough space this time, if we are not coming back,
                        indicate it as continous */
                        if (!pipe->bDoComplete)
                        {
                            if (!IndicateFileAsContinous(handle, firstcluster))
                                RETURN_FTEERR(FAIL);
                        }
                        break;

                    case TRUE:
                        if (!RelocateKnownFile(handle, position, &entry, freespace))
                            RETURN_FTEERR(FAIL);

                        /* Indicate this file now continous */
                        DrawMoreOnDriveMap(firstcluster, OPTIMIZEDSYMBOL, fileSize);

                        pipe->bFileMoved = TRUE;
                        break;    
                    }
                }

            case FALSE:
                /* Indicate file as continous */

                DrawMoreOnDriveMap(firstcluster, OPTIMIZEDSYMBOL, fileSize);
                break;

            case FAIL:
                RETURN_FTEERR(FAIL);
            }
        }
    }

    return TRUE;
}

static BOOL CompleteQuickTryIndicateComplete(RDWRHandle handle, struct DirectoryPosition* position, void** structure)
{
    struct DirectoryEntry entry;
    CLUSTER firstcluster;

    /* No more files could be moved, indicate everything continous */
   
    if (!GetDirectory(handle, position, &entry))
        RETURN_FTEERR(FALSE);    
    
    if (IsCurrentDir(entry)  ||
        IsPreviousDir(entry) ||
        IsLFNEntry(&entry))
    {
        return TRUE;
    }

    if (!(entry.attribute & FA_HIDDEN) || (entry.attribute & FA_SYSTEM))
    {
        firstcluster = GetFirstCluster(&entry);
        IndicateFileAsContinous(handle, firstcluster);         
    }

    return TRUE;
}

static BOOL IndicateFileAsContinous(RDWRHandle handle, CLUSTER firstcluster)
{
    CLUSTER label = 0, start = firstcluster, cluster = firstcluster;
    unsigned long continousLength=1;
return TRUE;
    while (TRUE)
    {
        if (!GetNthCluster(handle, cluster, &label))
            return FALSE;

        if (FAT_FREE(label)) return FALSE;
        if (FAT_BAD(label))  return FALSE;

        if (FAT_LAST(label)) break;

        if (label != cluster+1)
        {
            DrawMoreOnDriveMap(start, OPTIMIZEDSYMBOL, continousLength);
            start = label;
            continousLength = 1;            
        }
        else
            continousLength++;

        cluster = label;
    }

   return TRUE; 
}


