/*
   Fexist.c - file existance checking.
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
#include "../header/strnicmp.h"

struct Pipe
{
   char* filename;
   char* extension;

   BOOL exists;
};

static BOOL ExistanceChecker(RDWRHandle handle, struct DirectoryPosition* pos,
                             void** structure);

/**************************************************************
**                    LoFileNameExists
***************************************************************
** Returns wether a directory exists in the directory of which
** firstcluster is the first cluster.
**
** If firstcluster is 0, the root directory is assumed.
**
** Returns:
**     TRUE  if a file with that name exists
**     FALSE if a file with that name does not exists
**
**     FAIL  if there was an error accessing the volume
***************************************************************/

BOOL LoFileNameExists(RDWRHandle handle, CLUSTER firstcluster,
                      char* filename, char* extension)
{
   struct Pipe pipe, *ppipe = &pipe;

   pipe.filename  = filename;
   pipe.extension = extension;
   pipe.exists    = FALSE;

   if (!TraverseSubdir(handle, firstcluster, ExistanceChecker,
                       (void**) &ppipe, TRUE))
      return FAIL;

   return pipe.exists;
}

static BOOL ExistanceChecker(RDWRHandle handle, struct DirectoryPosition* pos,
                             void** structure)
{
   struct Pipe* pipe = *((struct Pipe**) structure);
   struct DirectoryEntry entry;

   if (!GetDirectory(handle, pos, &entry))
      return FAIL;

   if (IsLFNEntry(&entry))
      return TRUE;

   if ((strnicmp(pipe->extension, entry.extension, 3) == 0) &&
       (strnicmp(pipe->filename, entry.filename, 8) == 0))
   {
      pipe->exists = TRUE;
      return FALSE;
   }

   return TRUE;
}
