/*    
   Rdentrs.c - read entries in memory.

   Copyright (C) 2000, 2002 Imre Leber

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

#include "fte.h"
#include "expected.h"

static int EntryGetter(RDWRHandle handle,
                       struct DirectoryPosition* pos,
                       void** buffer);

struct PipeStruct {

       unsigned epos;
       struct   DirectoryEntry* entries;

       unsigned spos;
       SECTOR*  sectors;
       
       BOOL error;
};


int ReadEntriesToSort(RDWRHandle handle, CLUSTER cluster,
                      struct DirectoryEntry* entries, SECTOR* sectors)
{
    struct PipeStruct pipe, *ppipe = &pipe;

    pipe.epos     = 0;
    pipe.entries  = entries;
    pipe.spos     = UINT_MAX;
    pipe.sectors  = sectors;
    pipe.error    = FALSE;
    
    if (!TraverseSubdir(handle, cluster, EntryGetter,
			(void**) &ppipe, TRUE))
       RETURN_FTEERR(FALSE);
    
    if (pipe.error)
       RETURN_FTEERR(FALSE);
       
    return TRUE;
}

static int EntryGetter(RDWRHandle handle,
                       struct DirectoryPosition* pos,
                       void** buffer)
{
    struct PipeStruct* pipe = *((struct PipeStruct**) buffer);
    
    if (!GetDirectory(handle, pos, &(pipe->entries[pipe->epos])))
    {
       pipe->error = TRUE;
       RETURN_FTEERR(FALSE);
    }
    pipe->epos++;

    if ((pipe->spos == UINT_MAX) || (pipe->sectors[pipe->spos] != pos->sector))
    {
       (pipe->spos)++;
       pipe->sectors[pipe->spos] = pos->sector;
       /* Mark it on the drive map. */
       if (!IsRootDirPosition(handle, pos))
       {
	  CLUSTER cluster = DataSectorToCluster(handle, pos->sector);
	  if (!cluster)
	  {
	     pipe->error = TRUE;
	     RETURN_FTEERR(FALSE);
	  }

	  DrawOnDriveMap(cluster, READSYMBOL);
       }
    }

    return TRUE;
}
