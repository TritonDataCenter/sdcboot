/*    
   AdCPDirs.c - Function to adapt '.' and '..' entries when the first
                cluster of directory is about to change.

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

static BOOL DirectoryAdaptor(RDWRHandle handle,
                             struct DirectoryPosition* pos,
                             void** structure);

static BOOL PreviousDirAdaptor(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               void** structure);

/*
** This function adapts the first cluster of '.' and '..' entries to
** a change of the first cluster of a directory to a different place.
**
** This function is to be called BEFORE it is actually moved.
*/

BOOL AdaptCurrentAndPreviousDirs(RDWRHandle handle,
                                 CLUSTER oldcluster,
                                 CLUSTER newcluster)
{
   CLUSTER* pcluster = &newcluster;
    
   return TraverseSubdir(handle, oldcluster, DirectoryAdaptor,
                         (void**) &pcluster, TRUE);
}

static BOOL DirectoryAdaptor(RDWRHandle handle,
                             struct DirectoryPosition* pos,
                             void** structure)
{
     struct DirectoryEntry* entry;
     CLUSTER newcluster = **((CLUSTER**) structure);
     
     entry = AllocateDirectoryEntry();
     if (!entry) RETURN_FTEERROR(FAIL);

     if (!GetDirectory(handle, pos, entry))
     {
        FreeDirectoryEntry(entry);     
        RETURN_FTEERROR(FAIL);
     }

     if ((IsLFNEntry(entry)) || IsDeletedLabel(*entry))
     {
        FreeDirectoryEntry(entry);
        return TRUE;
     }

     if (entry->attribute & FA_DIREC)
     {
        if (IsCurrentDir(*entry))
        {
           SetFirstCluster(newcluster, entry);
           if (!WriteDirectory(handle, pos, entry))
           {
              FreeDirectoryEntry(entry);
              RETURN_FTEERROR(FAIL);
           }
        }
        else
        {
           CLUSTER firstcluster = GetFirstCluster(entry);
           if (!IsPreviousDir(*entry) && firstcluster)
           {
              if (!TraverseSubdir(handle, firstcluster,
                                  PreviousDirAdaptor, structure, TRUE))
              {
                 FreeDirectoryEntry(entry);
                 RETURN_FTEERROR(FAIL);
              }
           }
        }
     }

     FreeDirectoryEntry(entry);
     return TRUE;
}

static BOOL PreviousDirAdaptor(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               void** structure)
{
     BOOL retVal=TRUE;
     struct DirectoryEntry* entry;
     CLUSTER newcluster = **((CLUSTER**) structure);

     entry = AllocateDirectoryEntry();
     if (!entry) RETURN_FTEERROR(FAIL);

     if (!GetDirectory(handle, pos, entry))
     {
        FreeDirectoryEntry(entry);     
        RETURN_FTEERROR(FAIL);
     }

     if (IsLFNEntry(entry) || IsDeletedLabel(*entry))
     {
        FreeDirectoryEntry(entry);     
        return TRUE;
     }

     if (IsPreviousDir(*entry))
     {
        SetFirstCluster(newcluster, entry);
        if (!WriteDirectory(handle, pos, entry))
        {        
           retVal = FAIL;
        }
	else
	{        
	   retVal = FALSE;
	}
     }
     
     FreeDirectoryEntry(entry);
  
     if (retVal == FAIL)
	 RETURN_FTEERROR(FAIL);
     
     return retVal;     
}

