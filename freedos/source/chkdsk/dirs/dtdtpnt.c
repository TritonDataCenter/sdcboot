/*
   DtDtPnt.c - '..' checking.
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

#include <stdio.h>
#include <string.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\printbuf.h"
#include "..\misc\useful.h"
#include "dirschk.h"

static BOOL CheckDblDots(RDWRHandle handle, CLUSTER firstcluster,
                         CLUSTER topointto, BOOL* invalid,
                         char* filename,
                         BOOL fixit);
                      

static CLUSTER topointto;

BOOL InitDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
     
     return TRUE;
}

BOOL PreProcesDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
     topointto = cluster;
     
     UNUSED(handle);
     UNUSED(fixit);
     UNUSED(filename);
    
     return TRUE;
}

BOOL CheckDirectoryForInvalidDblDots(RDWRHandle handle,                                   
                                     struct DirectoryPosition* pos, 
                                     struct DirectoryEntry* entry, 
                                     char* filename,
                                     BOOL fixit)
{
     CLUSTER firstcluster;
     BOOL invalid = FALSE;
     
     UNUSED(pos);
     
     if ((IsLFNEntry(entry))           ||
         IsDeletedLabel(*entry)        ||
         IsPreviousDir(*entry)         ||
         IsCurrentDir(*entry)          ||
         (entry->attribute & FA_LABEL) ||
         (entry->attribute & FA_DIREC) == 0) 
     {
        return TRUE;
     }

     firstcluster = GetFirstCluster(entry);
     if (firstcluster &&
         (!CheckDblDots(handle, firstcluster, topointto, 
                        &invalid, 
                        filename,
                        fixit)))
     {
        return FAIL;
     }
   
     return !invalid;
}

BOOL PostProcesDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(cluster);
    UNUSED(fixit);
    UNUSED(filename);
        
    return TRUE;
}

void DestroyDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
}

/*************************************************************************
**                             CheckDblDots
**************************************************************************
** Checks the directory pointed to by firstcluster to see wether the '..' 
** entry points to the directory pointed to by topointto.
**************************************************************************/

static BOOL CheckDblDots(RDWRHandle handle, CLUSTER firstcluster,
                         CLUSTER topointto, BOOL* invalid,
                         char* filename, BOOL fixit)
{
   struct DirectoryEntry entry;
   struct DirectoryPosition pos = {0,0};

   /* Get the position of the second directory entry */  
   if (!GetNthDirectoryPosition(handle, firstcluster, 1, &pos))
   {
      return FALSE;
   }
 
   /* See wether there was a second directory entry */
   if ((pos.sector == 0) && (pos.offset == 0))
   {
      ShowFileNameViolation(handle, filename,  
                            "%s is a directory without '..'");            
      return TRUE;
   }

   /* Read the second directory entry from disk. */
   if (!GetDirectory(handle, &pos, &entry))
      return FALSE;

   /* See wether it is a '..' entry */   
   if (!IsPreviousDir(entry))
   {
      ShowFileNameViolation(handle, filename,  
                            "%s is a directory without '..'");   
      return TRUE;
   }

   /* If it is a see wether it is pointing to the right directory */
   if (GetFirstCluster(&entry) != topointto)
   {
      ShowFileNameViolation(handle, filename,  
                            "The '..' entry in %s is not pointing to the right directory");              

      if (fixit)
      {
         SetFirstCluster(topointto, &entry);
         if (!WriteDirectory(handle, &pos, &entry))
            return FALSE;
      }
      else
      {
         *invalid = TRUE;
      }
   }

   return TRUE;
}