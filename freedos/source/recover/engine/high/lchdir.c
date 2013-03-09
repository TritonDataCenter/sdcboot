/*    
   Lchdir.c - low change directory.
   
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
#include "../header/lchdir.h"

static int DirChanger(RDWRHandle handle,
                      struct DirectoryPosition* pos,
                      struct DirectoryEntry* dummy);

struct DirectoryPosition CurDir;

static int before = FALSE;

struct DirectoryPosition low_getcwd(void)
{
    return CurDir;
}

int low_chdir(RDWRHandle handle, char* path, CLUSTER* fatcluster,
              struct DirectoryPosition* pos)
{
    char* p;
    struct DirectoryEntry entry;

    if ((!before) || (path[0] == '\\'))
    {
       GetRootDirPosition(handle, 0, pos);
       before = TRUE;
    }

    if (path[0] == '\\') path++;

    while (*path)
    {
       p = strchr(path, '\\');
       if (p != NULL) *p = '\0';

       if (!GetDirectory(handle, pos, &entry)) return FALSE;
       *fatcluster = entry.firstclust;
       if (!SearchDirectory(handle, path, *fatcluster, pos, DirChanger))
          return FALSE;

       if (p != NULL) path = p+1;
    }

    *pos = CurDir;                /* Not changed if directory not found. */

    return TRUE;
}

static int DirChanger(RDWRHandle handle,
                      struct DirectoryPosition* pos,
                      struct DirectoryEntry* dummy)
{
       CurDir = *pos;
       return TRUE;
}
