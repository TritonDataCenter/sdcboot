/*    
   FndCiFAT.c - Function to return the position in the FAT that 
                contains the given label.

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

struct Pipe
{
   CLUSTER tofind;
   SECTOR sector;
   BOOL found;
};

static BOOL ClusterInFATFinder(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure);
 
/**************************************************************
**                       FindClusterInFAT
***************************************************************
** Goes through the FAT and returns the position in the FAT that
** has the given cluster as a label (i.e. the previous cluster
** in the file chain).
**
** If the cluster could not be found it returns 0 as the found 
** cluster.
**
** Only returns FALSE when there has been a media error. 
***************************************************************/
                              
BOOL FindClusterInFAT(RDWRHandle handle, CLUSTER tofind, CLUSTER* result)
{
   struct Pipe pipe, *ppipe=&pipe;
   CLUSTER cluster;
   
   pipe.tofind = tofind;
   pipe.sector = 0;
   pipe.found  = FALSE;
  
   if (!LinearTraverseFat(handle, ClusterInFATFinder, (void**) &ppipe))
      return FALSE;

   if (!pipe.found)
   {
      *result = 0;
      return TRUE;
   }
      
   if (!pipe.sector)                  /* This should never happen */
   {
      *result = 0;
      return FALSE;
   }
      
   cluster = DataSectorToCluster(handle, pipe.sector);
   if (!cluster) return FALSE;
   
   *result = cluster;
   return TRUE;
}

static BOOL ClusterInFATFinder(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure)
{
   struct Pipe* pipe = (struct Pipe*) *structure;
   
   handle = handle;
   
   if (pipe->tofind == label)
   {
      pipe->sector = datasector;
      pipe->found  = TRUE;
      return FALSE;
   }
   return TRUE;
}
