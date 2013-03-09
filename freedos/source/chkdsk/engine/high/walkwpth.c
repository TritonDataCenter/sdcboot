/*
   WalkWPth.c - wildcard higher order functions.
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

static BOOL WildcardCaller(RDWRHandle handle, struct DirectoryPosition* pos,
                          void** structure);

struct Pipe
{
    char* wildcard;
    BOOL (*func)(RDWRHandle handle,
                 struct DirectoryPosition* pos,
                 void** structure);
    void** structure;
};                          
                          
BOOL WalkWildcardPath(RDWRHandle handle, char* path, CLUSTER firstcluster,
                      BOOL (*func)(RDWRHandle handle,
                                   struct DirectoryPosition* pos,
                                   void** structure), 
		      void** structure)
{
   int i, len;
   char* temp;
   struct Pipe pipe, *ppipe = &pipe;
   struct DirectoryPosition abspos;
   CLUSTER poscluster = 0;
   struct DirectoryEntry* entry;
   
   pipe.func = func;
   pipe.structure = structure;
   
   len = strlen(path);
   for (i = len-1; i >= 0; i--)
       if ((path[i] == '/') || (path[i] == '\\'))
          break;     
   
   if (i > 0)
   {
      temp = (char*) FTEAlloc(i+1);
      memcpy(temp, path, i);
      temp[i] = 0;
   
      switch (LocateRelPathPosition(handle, temp,
                                    firstcluster, &abspos))
      {
         case FALSE: /* Path not found */
              FTEFree(temp);
              return FALSE;
         case FAIL:
              FTEFree(temp);   
              RETURN_FTEERROR(FAIL); 
      }
   
      FTEFree(temp);
 
      entry = AllocateDirectoryEntry();
      if (!entry) RETURN_FTEERROR(FAIL);
   
      if (!GetDirectory(handle, &abspos, entry))
      {
         FreeDirectoryEntry(entry);      
         RETURN_FTEERROR(FAIL);
      }    
      
      poscluster = GetFirstCluster(entry);
      FreeDirectoryEntry(entry);
      
      if (!poscluster)
         return TRUE;
   }
  
   pipe.wildcard = path+i+1;	/* notice that i may be -1 */
      
   if (!TraverseSubdir(handle, poscluster, WildcardCaller,
                       (void**) &ppipe, TRUE))
      RETURN_FTEERROR(FAIL);
      
   return TRUE;
}

static BOOL WildcardCaller(RDWRHandle handle, struct DirectoryPosition* pos,
                          void** structure)
{
   struct Pipe* pipe = *((struct Pipe**) structure);
   struct DirectoryEntry* entry;
   
   entry = AllocateDirectoryEntry();
   if (!entry) RETURN_FTEERROR(FAIL);
   
   if (!GetDirectory(handle, pos, entry))
   {
      FreeDirectoryEntry(entry);      
      RETURN_FTEERROR(FAIL);   
   }
   
   if (IsLFNEntry(entry)     ||
       IsPreviousDir(*entry) ||
       IsCurrentDir(*entry))
   {
      FreeDirectoryEntry(entry);
      return TRUE;     
   }    
   
   if (MatchWildcard(pipe->wildcard, entry->filename, entry->extension))
   {
      pipe->func(handle, pos, pipe->structure);
   }
    
   FreeDirectoryEntry(entry);
   return TRUE;
}
