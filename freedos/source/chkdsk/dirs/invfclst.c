/*
   InvFClst.c - Checks the first cluster of a file.
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
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\errmsgs.h"
#include "..\errmsgs\PrintBuf.h"
#include "..\misc\useful.h"
#include "dirschk.h"


BOOL InitFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
    
     return TRUE;
}

BOOL PreProcesFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(fixit);
    UNUSED(filename);
    UNUSED(cluster);
    
    return TRUE;
}  

BOOL CheckFirstCluster(RDWRHandle handle,                   
                       struct DirectoryPosition* pos, 
                       struct DirectoryEntry* entry, 
                       char* filename,
                       BOOL fixit)
{  
    CLUSTER firstcluster;
    unsigned long labelsinfat;
    
    if (IsDeletedLabel(*entry) ||
       (IsLFNEntry(entry)))
    {
       return TRUE;
    }
    
    firstcluster = GetFirstCluster(entry);
       
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
       return FAIL;
    }
    
    if (((firstcluster < 2)                    ||
         (firstcluster >= labelsinfat)) &&
        (!((firstcluster == 0) &&
	 (IsPreviousDir(*entry) || entry->filesize == 0))))
    {
       ShowFileNameViolation(handle, filename,  
                                "%s has an invalid first cluster");
       if (fixit)
       { 
          MarkEntryAsDeleted(*entry);                    
          if (!WriteDirectory(handle, pos, entry))
             return FAIL;
       }
       
       return FALSE;
    }

    return TRUE;         
}       

BOOL PostProcesFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(fixit);
    UNUSED(filename);
    UNUSED(cluster);
   
    return TRUE;
}

void DestroyFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
    UNUSED(filename);   
    UNUSED(handle);
    UNUSED(cluster);
}  