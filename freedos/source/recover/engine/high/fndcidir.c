/*    
   FndCiDir.c - Function to return the directory that contains
                the given cluster as first cluster in the corresponding
                file.

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

#include "fte.h"

struct Pipe
{
   CLUSTER tofind;
   struct DirectoryPosition* pos;
   BOOL found;
   
   BOOL error;
};

static BOOL ClusterInDirectoryFinder(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure);

/**************************************************************
**                    FindClusterInDirectories
***************************************************************
** Goes through all the directories on the volume and returns
** the position of the directory entry of the file that starts
** at the given cluster.
***************************************************************/

BOOL FindClusterInDirectories(RDWRHandle handle, CLUSTER tofind, 
                              struct DirectoryPosition* result,
                              BOOL* found)
{
  struct Pipe pipe, *ppipe = &pipe;
  struct DirectoryPosition pos;
  
  pipe.tofind = tofind;
  pipe.pos    = &pos;
  pipe.found  = FALSE;
  
  pipe.error  = FALSE;

  if (!WalkDirectoryTree(handle, ClusterInDirectoryFinder, (void**)&ppipe))
     return FALSE;

  if (pipe.error)
     return FALSE;
  
  if (!pipe.found)
     *found = FALSE;
  else
  {
     memcpy(result, &pos, sizeof(struct DirectoryPosition));
     *found = TRUE;
  }
     
  return TRUE;
}

static BOOL ClusterInDirectoryFinder(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure)
{
   CLUSTER firstcluster;
   struct Pipe* pipe = *((struct Pipe**) structure);
   struct DirectoryEntry* entry;
      
   entry = AllocateDirectoryEntry();
   if (!entry)
   {        
      pipe->error = TRUE;
      return FALSE;  
   }
   
   if (!GetDirectory(handle, pos, entry))
   {
      pipe->error = TRUE;
      FreeDirectoryEntry(entry);
      return FALSE;
   }
      
   if (!IsLFNEntry(entry)     &&
       !IsCurrentDir(*entry)  &&
       !IsPreviousDir(*entry) &&
       !IsDeletedLabel(*entry))
   {   
      firstcluster = GetFirstCluster(entry);
   
      if (pipe->tofind == firstcluster)
      {
	 memcpy(pipe->pos, pos, sizeof(struct DirectoryPosition));
         pipe->found  = TRUE;
         FreeDirectoryEntry(entry);
         return FALSE;
      }
   }
   
   FreeDirectoryEntry(entry);
   return TRUE;
}          

                              
