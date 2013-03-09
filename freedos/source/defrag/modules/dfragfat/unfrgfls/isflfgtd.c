/*    
   IsFlFgtd.c - Returns wether a cluster chain is fragmented.

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

#include "..\..\modlgate\custerr.h"

static BOOL FragmentationFinder(RDWRHandle handle, CLUSTER label,
                                SECTOR datsector, void** structure);
struct Pipe
{
    BOOL fragmented;
};

BOOL IsFileFragmented(RDWRHandle handle, CLUSTER firstclust)
{
   struct Pipe pipe, *ppipe = &pipe;
   
   pipe.fragmented = FALSE;
   
   if (!FileTraverseFat(handle, firstclust, FragmentationFinder,
                        (void**) &ppipe))
   {
      SetCustomError(IS_FILE_FRAGMENTED_FAILED);
      RETURN_FTEERR(FAIL);
   }
   return pipe.fragmented;
}

static BOOL FragmentationFinder(RDWRHandle handle, CLUSTER label,
                                SECTOR datasector, void** structure)
{
   CLUSTER cluster;
   struct Pipe* pipe = (struct Pipe*) *structure;

   if (FAT_NORMAL(label))
   {
      cluster = DataSectorToCluster(handle, datasector);
      if (!cluster) RETURN_FTEERR(FAIL);
      
      if (cluster != label-1)
      { 
         pipe->fragmented = TRUE;
         RETURN_FTEERR(FALSE);
      }    
  }

  return TRUE;              
}