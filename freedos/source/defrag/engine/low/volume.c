/*
   Volume.c - volume label manipulation code.
   Copyright (C) 2000 Imre Leber

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

#include <assert.h>
#include <string.h>

#include "../../misc/bool.h"
#include "../header/fat.h"
#include "../header/rdwrsect.h"
#include "../header/direct.h"
#include "../header/boot.h"
#include "../header/fatconst.h"
#include "../header/fsinfo.h"
#include "../header/subdir.h"
#include "../header/ftemem.h"
#include "../header/traversl.h"
#include "../header/fteerr.h"

struct Pipe
{
   struct DirectoryPosition* pos;
   
   BOOL   found;
};

static BOOL VolumeGetter(RDWRHandle handle,
                         struct DirectoryPosition* pos,
                         void** structure);

/*******************************************************************
**                     GetRootDirVolumeLabel
********************************************************************
** Returns the volume label that is stored in the root directory.
**
** Returns:
**         TRUE  if there was a label in the root directory
**               then the position of the volume label is returned in
**               volumepos
**
**         FALSE if there was no such label
**
**         FAIL  if there was an error accessing the volume
********************************************************************/

BOOL GetRootDirVolumeLabel(RDWRHandle handle,
                           struct DirectoryPosition* volumepos)
{
   struct Pipe pipe, *ppipe = &pipe;
       
   assert(volumepos);

   pipe.found = FALSE;
   pipe.pos   = volumepos;

   if (!TraverseRootDir(handle, VolumeGetter, (void**) &ppipe, TRUE))
      RETURN_FTEERR(FAIL);
   
   assert(!pipe.found || (volumepos->sector && volumepos->offset < 16));
   return pipe.found;
}

static BOOL VolumeGetter(RDWRHandle handle,
                         struct DirectoryPosition* pos,
                         void** structure)
{
   struct Pipe *pipe = *((struct Pipe**) structure);
   struct DirectoryEntry* entry;

   assert (pos->sector && (pos->offset < 16));
   
   entry = AllocateDirectoryEntry();
   if (!entry) RETURN_FTEERR(FAIL);

   if (!GetDirectory(handle, pos, entry))
   {
      FreeDirectoryEntry(entry);
      RETURN_FTEERR(FAIL);
   }

   if (IsLFNEntry(entry))
   {
      FreeDirectoryEntry(entry);
      return TRUE;
   }
   
   if (IsDeletedLabel(*entry))
   {
      FreeDirectoryEntry(entry);
      return TRUE;
   }
   
   if (entry->attribute & FA_LABEL)
   {
      pipe->found = TRUE;
      memcpy(pipe->pos, pos, sizeof(struct DirectoryPosition));

      FreeDirectoryEntry(entry);
       RETURN_FTEERR(FALSE);
   }

   FreeDirectoryEntry(entry);
   return TRUE;
}

