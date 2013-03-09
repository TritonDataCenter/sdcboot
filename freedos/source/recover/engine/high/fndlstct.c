/*    
   FndLstCt.c - Function to return the last free space on a volume.

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
    CLUSTER result;    
    BOOL error;
};


static BOOL FreeClusterFinder(RDWRHandle handle, CLUSTER label,
                              SECTOR datasector, void** structure);

/**************************************************************
**                    FindLastFreeCluster
***************************************************************
** Function that returns the last free cluster in the volume.
**
** This doesn't return the length.
***************************************************************/                             
                              
BOOL FindLastFreeCluster(RDWRHandle handle, CLUSTER* result)
{
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.result = 0;
    pipe.error  = FALSE;
    
    if (!LinearTraverseFat(handle, FreeClusterFinder, (void**) &ppipe))
       return FALSE;
    if (pipe.error)
       return FALSE;
    
    *result = pipe.result;
    return TRUE;
}

/**************************************************************
**                    FindLastFreeCluster
***************************************************************
** Higher order function that looks for the last free cluster.
**
** Notice that this function does not stop asking for the following
** label in the FAT.
***************************************************************/ 

static BOOL FreeClusterFinder(RDWRHandle handle, CLUSTER label,
                              SECTOR datasector, void** structure)
{
    CLUSTER cluster;
    struct Pipe* pipe = *((struct Pipe**) structure);    
    
    if (FAT_FREE(label))
    {
       cluster = DataSectorToCluster(handle, datasector);
       if (!cluster)
       {
          pipe->error = TRUE;
          return FALSE;
       }
       pipe->result = cluster;
    }
    
    return TRUE;
}
