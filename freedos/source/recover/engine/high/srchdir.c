/*    
   srchdir.c - directory search routines.
   
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

#include <stdlib.h>
#include <string.h>

#include "../header/rdwrsect.h"
#include "../header/direct.h"
#include "../header/bool.h"
#include "../header/fat.h"
#include "../header/subdir.h"

static int DirSearcher(RDWRHandle handle, struct DirectoryPosition* pos,
                       void** buffer);

struct PipeStruct {
    int        found;
    char*      filename;
    char*      extension;

    int        func(RDWRHandle handle, struct DirectoryPosition* pos,
                    struct DirectoryEntry* entry);

    int        error;
};

int SearchDirectory(RDWRHandle handle, char* dir, CLUSTER* fatcluster,
                    struct DirectoryPosition* pos,
                    int    func(RDWRHandle handle,
                                struct DirectoryPosition* pos,
                                struct DirectoryEntry* entry))
{
    int i, j;
    struct PipeStruct pipe, *ppipe = &pipe;

    char filename[8], extension[3];

    pipe.found      = FALSE;
    pipe.filename   = filename;
    pipe.extension  = extension;
    pipe.error      = FALSE;
    pipe.func       = func;

    memset(filename, ' ', 8);
    for (i = 0; i < 8; i++)
        if ((dir[i] == '.') || (dir[i] == '\0'))
           break;
        else
           filename[i] = dir[i];

    memset(extension, ' ', 3);

    for (j = 0; j < 3; j++)
        if (dir[i+j] == '.')
           continue;
        else if (dir[i+j] == '\0')
           break;
        else
           extension[j] = dir[i];

    if (IsRootDirPosition(handle, pos))
    {
       if (!TraverseRootDir(handle, DirSearcher,
                            (void**) &ppipe, TRUE)) return FALSE;
       return pipe.found;
    }
    else
    {
       if (!TraverseSubdir(handle, *fatcluster, DirSearcher,
                           (void**) &ppipe, TRUE)) return FALSE;

       return pipe.found;
    }
}

static int DirSearcher(RDWRHandle handle,
                       struct DirectoryPosition* pos, void** buffer)
{
    struct DirectoryEntry entry;
    struct PipeStruct *pipe = *((struct PipeStruct**) buffer);

    if (!GetDirectory(handle, pos, &entry))
    {
       pipe->error = TRUE;
       return FALSE;
    }

    if ((memcpy(pipe->filename, entry.filename, 8) == 0) &&
        (memcpy(pipe->extension, entry.extension, 3) == 0))
    {
       pipe->found = TRUE;
       CurDir      = *pos;

       pipe->error = pipe->func(handle, pos, &entry);
       return FALSE;                                   /* Do not continue.*/
    }

    return TRUE;
}
