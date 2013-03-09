/*
   Locpath.c - filee locating routines.
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
   char* filename;
   char* extension;

   BOOL found;
   struct DirectoryPosition* result;
};

static BOOL FileLocater(RDWRHandle handle, struct DirectoryPosition* pos,
                        void** structure);

/**************************************************************
**                      LocateFilePosition
***************************************************************
** Locates a file in a directory and returns the directory
** position.
**
** Returns:
**        TRUE if file is found
**             then the position is returned in the location
**             pointed to by pos
**
**        FALSE if file not found
**
**        FAIL on media error
**************************************************************/

BOOL LocateFilePosition(RDWRHandle handle, CLUSTER firstcluster,
                        char* filename, char* extension,
                        struct DirectoryPosition* pos)
{
   struct Pipe pipe, *ppipe = &pipe;

   pipe.filename  = filename;
   pipe.extension = extension;
   pipe.found     = FALSE;
   pipe.result    = pos;

   if (!TraverseSubdir(handle, firstcluster, FileLocater,
                       (void**) &ppipe, FALSE))
      RETURN_FTEERROR(FAIL);

   return pipe.found;
}

static BOOL FileLocater(RDWRHandle handle, struct DirectoryPosition* pos,
                        void** structure)
{
   BOOL retVal = TRUE;
    
   struct Pipe* pipe = *((struct Pipe**) structure);
   struct DirectoryEntry* entry;

   entry = AllocateDirectoryEntry();
   if (!entry) RETURN_FTEERROR(FAIL);

   if (!GetDirectory(handle, pos, entry))
   {
      FreeDirectoryEntry(entry);
      RETURN_FTEERROR(FAIL);
   }

   /* 
   
       I guess it is safe not to check for validity of the entry.
   
       This way we can realy just search about anything!
      
   */
   
   
   if ((memcmp(pipe->filename, entry->filename, 8) == 0) &&
       (memcmp(pipe->extension, entry->extension, 3) == 0))
   {
      pipe->found  = TRUE;
      memcpy(pipe->result, pos, sizeof(struct DirectoryPosition));

      retVal = FALSE;
   }

   FreeDirectoryEntry(entry);
   return retVal;
}

/**************************************************************
**                      LocatePathPosition
***************************************************************
** Tries to locate a certain absolute path in the volume and
** returns the position.
**
** Returns:
**        TRUE if path is completely found
**             then the position is returned in the location
**             pointed to by pos
**
**        FALSE if path is not completely found
**
**        FAIL on media error
**
** Handles both '/' and '\' file delimiters.
**************************************************************/

BOOL LocatePathPosition(RDWRHandle handle, char* abspath,
                        struct DirectoryPosition* pos)
{
     return LocateRelPathPosition(handle, abspath, 0, pos);
}

/**************************************************************
**                      LocateRelPathPosition
***************************************************************
** Tries to locate a certain absolute path in the volume and
** returns the position.
**
** Returns:
**        TRUE if path is completely found
**             then the position is returned in the location
**             pointed to by pos
**
**        FALSE if path is not completely found
**
**        FAIL on media error
**
** Handles both '/' and '\' file delimiters.
**************************************************************/

BOOL LocateRelPathPosition(RDWRHandle handle, char* relpath,
                           CLUSTER firstcluster, struct DirectoryPosition* pos)
{
   struct DirectoryPosition result;
   char filename[8], extension[3];
   struct DirectoryEntry* entry;
   
   char *p1, *p2;

   if (relpath[0] == 0) return FALSE;

   if ((relpath[0] == '\\') || (relpath[0] == '/'))
      relpath++;

   entry = AllocateDirectoryEntry();
   if (!entry) RETURN_FTEERROR(FAIL);   
   
   while (relpath && relpath[0])
   {
      ConvertUserPath(relpath, filename, extension);

      switch (LocateFilePosition(handle, firstcluster,
                                 filename, extension,
                                 &result))
      {
         case FALSE:
              return FALSE;
         case TRUE:
              break;
         case FAIL:
              RETURN_FTEERROR(FAIL);
      }
      
      p1 = strchr(relpath, '\\');
      p2 = strchr(relpath, '/');

      if ((p1 == p2) && (p2 == 0))	/* If there was no / or \ */
      {
         relpath = strchr(relpath, '0');
      }      
      else if ((!p2) 		||	/* If there was no /, or */ 
	       ((p1 < p2) && p1))	/* If \ came before / making sure that there was a \ */
      {
	 relpath = p1+1;    	  
      }      
      else
      {
	 relpath = p2+1;  	  
      }

      if (!GetDirectory(handle, &result, entry))
      {
            FreeDirectoryEntry(entry);
            RETURN_FTEERROR(FAIL);
      }

      firstcluster = GetFirstCluster(entry);
      if (!firstcluster)           	/* Invalid entry in file system? */
      {
         FreeDirectoryEntry(entry);
         return FALSE;
      }
   }

   FreeDirectoryEntry(entry);
   memcpy(pos, &result, sizeof(struct DirectoryPosition));
   return TRUE;
}
