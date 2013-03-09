/*
   ScnRtDir.c - perform checks on the root directory.
   Copyright (C) 2003 Imre Leber

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

#include "fte.h"
#include "..\misc\useful.h"
#include "scnsbdrs.h"
#include "..\chkdrvr.h"
#include "dirschk.h"

struct Pipe
{ 
   BOOL fixit;
   BOOL result;
};

STATIC BOOL RootDirChecker(RDWRHandle handle, 
                           struct DirectoryPosition* pos,
                           void** structure);
                           
BOOL PerformRootDirChecks(RDWRHandle handle, BOOL fixit)
{
   int fatlabelsize;
   struct Pipe pipe, *ppipe = &pipe;
   
   fatlabelsize = GetFatLabelSize(handle);
   
   switch (fatlabelsize)
   {
      case FAT12:
      case FAT16:
           pipe.fixit  = fixit;
           pipe.result = TRUE;
           
           InitClusterEntryChecking(handle, 0, "The root directory");
           switch (PreProcessClusterEntryChecking(handle, 0, "The root directory", fixit))
           {
              case FALSE:
                   pipe.result = FALSE;
                   break;
               
              case FAIL:
                   DestroyClusterEntryChecking(handle, 0, "The root directory");
                   return FAIL;
           }           
           
           if (!TraverseRootDir(handle, RootDirChecker, (void**) &ppipe, TRUE))
           {
              DestroyClusterEntryChecking(handle, 0, "The root directory");
              return FAIL;
           }
              
           switch (PostProcessClusterEntryChecking(handle, 0, "The root directory", fixit))
           {
               case FALSE:
                    pipe.result = FALSE;
                    break;
               
               case FAIL:
                    pipe.result = FAIL;
           }
       
           DestroyClusterEntryChecking(handle, 0, "The root directory");              
              
           return pipe.result;
                   
      case FAT32:
           return ScanFileChain(handle, 0, 
                                (struct DirectoryPosition*) NULL, 
                                (struct DirectoryEntry*) NULL, 
                                "\\",
                                fixit);
              
      default:
           return FAIL;
   }
}

STATIC BOOL RootDirChecker(RDWRHandle handle, 
                           struct DirectoryPosition* pos,
                           void** structure)
{
   struct Pipe *pipe = *((struct Pipe**) structure);
   struct DirectoryEntry entry;
   
   if (!GetDirectory(handle, pos, &entry))
      return FAIL;

   switch (PerformEntryChecks(handle, pos, &entry, "", pipe->fixit))
   {
      case FAIL:
           return FAIL;
           
      case FALSE:
           pipe->result = FALSE;
   }

   return TRUE;
}