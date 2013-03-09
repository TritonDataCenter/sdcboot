/*    
   Ordrdfrg.c - implementation for files first, directories first and
                directories together with files methods.

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fte.h"
#include "FllDfrag\FllDfrg.h"
#include "DfrgDrvr.h"
#include "..\dtstruct\ClMovMap.h"
#include "..\..\modlgate\expected.h"

struct Pipe
{
   CLUSTER cluster;
   struct  DirectoryEntry Entry;

   BOOL finddirs;
   BOOL found;
};

static BOOL FileSelector(RDWRHandle handle, struct DirectoryPosition* pos,
                         void** structure)
{
    char buffer[70], filename[14];
    BOOL isMovable;
    CLUSTER cluster;
    struct Pipe* pipe = *((struct Pipe**) structure);
    struct DirectoryEntry entry;

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

    if ((entry.attribute & FA_DIREC) && !pipe->finddirs)
       return TRUE;

    if (((entry.attribute & FA_DIREC) == 0) && pipe->finddirs)
       return TRUE;
    
    cluster = GetFirstCluster(&entry);
    if (cluster >= pipe->cluster)
    {
       if (!IsClusterMovable(handle, cluster, &isMovable))
          RETURN_FTEERR(FAIL);
    
       if (isMovable)
       {
          pipe->found = TRUE;

          ConvertEntryPath(filename, entry.filename, entry.extension);
          sprintf(buffer, " Defragmenting: %s", filename);
          SmallMessage(buffer);

          memcpy(&(pipe->Entry), &entry, sizeof(struct DirectoryEntry));
       
          RETURN_FTEERR(FALSE);
       }
    }
    
    return TRUE;
}

static BOOL FilesFirstSelector(RDWRHandle handle,
                               CLUSTER    startfrom,
                               CLUSTER*   clustertoplace)
{
    struct Pipe pipe, *ppipe = &pipe;

    pipe.cluster  = startfrom;
    pipe.found    = FALSE;
    pipe.finddirs = FALSE;

    if (!WalkDirectoryTree(handle, FileSelector, (void**) &ppipe))
    {
       RETURN_FTEERR(FAIL);
    }
    
    if (!pipe.found)
    {
       pipe.finddirs = TRUE;
       if (!WalkDirectoryTree(handle, FileSelector, (void**) &ppipe))
       {
          RETURN_FTEERR(FAIL);
       }
    }

    if (pipe.found)
       *clustertoplace = GetFirstCluster(&(pipe.Entry));
    
    return pipe.found;
}

static BOOL DirectoriesFirstSelector(RDWRHandle handle,
                                     CLUSTER    startfrom,
                                     CLUSTER*   clustertoplace)
{
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.cluster  = startfrom;
    pipe.found    = FALSE;
    pipe.finddirs = TRUE;

    if (!WalkDirectoryTree(handle, FileSelector, (void**) &ppipe))
    {
       RETURN_FTEERR(FAIL);
    }
    
    if (!pipe.found)
    {
       pipe.finddirs = FALSE;
       if (!WalkDirectoryTree(handle, FileSelector, (void**) &ppipe))
       {
          RETURN_FTEERR(FAIL);
       }
    }

    if (pipe.found)
       *clustertoplace = GetFirstCluster(&(pipe.Entry));

    return pipe.found;
}

static BOOL FindNextFile(RDWRHandle handle,
                         struct DirectoryPosition* pos,
                         void** structure)
{
    BOOL isMovable;
    char buffer[70], filename[14];
    CLUSTER cluster;
    struct DirectoryEntry entry;
    struct Pipe* pipe = *((struct Pipe**) structure);

    if (!GetDirectory(handle, pos, &entry))
       RETURN_FTEERR(FAIL);

    if (entry.attribute & FA_LABEL)
       return TRUE;
       
    if (IsDeletedLabel(entry))
       return TRUE;

    cluster = GetFirstCluster(&entry);
    if (cluster >= pipe->cluster)
    {
       if (!IsClusterMovable(handle, cluster, &isMovable))
          RETURN_FTEERR(FAIL);

       if (isMovable)
       {
          pipe->found = TRUE;

          ConvertEntryPath(filename, entry.filename, entry.extension);
          sprintf(buffer, " Defragmenting: %s", filename);
          SmallMessage(buffer);

          memcpy(&(pipe->Entry), &entry, sizeof(struct DirectoryEntry));
          RETURN_FTEERR(FALSE);
       }
    }
    
    return TRUE;
}

static BOOL DirSelector(RDWRHandle handle, struct DirectoryPosition* pos,
			void** structure)
{
    char buffer[70], filename[14];
    BOOL isMovable;
    CLUSTER cluster;
    struct Pipe* pipe = *((struct Pipe**) structure);
    struct DirectoryEntry entry;

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

    if ((entry.attribute & FA_DIREC) == 0)
       return TRUE;

    cluster = GetFirstCluster(&entry);
    if (cluster)
    {
       if (!TraverseSubdir(handle, cluster, FindNextFile, structure,
			   TRUE))
       {
	  RETURN_FTEERR(FAIL);
       }

       if (pipe->found)
       {
	  RETURN_FTEERR(FALSE);
       }
    }

    if (cluster >= pipe->cluster)
    {
       if (!IsClusterMovable(handle, cluster, &isMovable))
          RETURN_FTEERR(FAIL);
    
       if (isMovable)
       {
          pipe->found = TRUE;

          ConvertEntryPath(filename, entry.filename, entry.extension);
          sprintf(buffer, " Defragmenting: %s", filename);
          SmallMessage(buffer);

          memcpy(&(pipe->Entry), &entry, sizeof(struct DirectoryEntry));
       
          RETURN_FTEERR(FALSE);
       }
    }
    
    return TRUE;
}

static BOOL DirectoriesWithFilesSelector(RDWRHandle handle,
                                         CLUSTER    startfrom,
                                         CLUSTER*   clustertoplace)
{
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.cluster  = startfrom;
    pipe.found    = FALSE;
    pipe.finddirs = TRUE;

    if (!TraverseSubdir(handle, 0, FindNextFile, (void**) &ppipe, TRUE))
    {
       RETURN_FTEERR(FAIL);
    }

    if (pipe.found)
    {
       *clustertoplace = GetFirstCluster(&(pipe.Entry));
       return TRUE;
    }

    if (!WalkDirectoryTree(handle, DirSelector, (void**) &ppipe))
    {
       RETURN_FTEERR(FAIL);
    }

    if (pipe.found)
       *clustertoplace = GetFirstCluster(&(pipe.Entry));

    return pipe.found;
}

BOOL FilesFirstDefrag(RDWRHandle handle)
{
    return DefragmentVolume(handle, FilesFirstSelector, FullDefragPlace);
}

BOOL DirectoriesFirstDefrag(RDWRHandle handle)
{
    return DefragmentVolume(handle, DirectoriesFirstSelector, FullDefragPlace);
}


BOOL DirectoriesWithFilesDefrag(RDWRHandle handle)
{
    return DefragmentVolume(handle, DirectoriesWithFilesSelector,
                            FullDefragPlace);
}

