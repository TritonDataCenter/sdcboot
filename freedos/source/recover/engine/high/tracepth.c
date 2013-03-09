/*    
   TracePth.c - routines to trace back the path name of a directory position.

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

/*******************************************************************
**                        GetSurroundingCluster
********************************************************************
** Returns the first cluster of the directory to which the given 
** position belongs.
********************************************************************/

CLUSTER GetSurroundingCluster(RDWRHandle handle, struct DirectoryPosition* pos)
{
    CLUSTER cluster, previous;
    
    if (IsRootDirPosition(handle, pos)) /* Not allowing root directory */
       return FALSE;                    /* position.                   */
    
    cluster = DataSectorToCluster(handle, pos->sector);
    if (!cluster) return FALSE;
    
    previous = cluster;
    
    for (;;)
    {
        if (!FindClusterInFAT(handle, cluster, &cluster))
           return FALSE;
           
        if (!cluster) return previous;
        
        previous = cluster;
    } 
}

/*******************************************************************
**                        AddToFrontOfString
********************************************************************
** Adds a string in front of an other string. 
********************************************************************/

void AddToFrontOfString(char* string, char* front)
{
   unsigned len1 = strlen(front);
   unsigned len2 = strlen(string);
   
   if (len2 && len1)
   {
      memmove(string+len1, string, len2+1);
      memcpy(string, front, len1);
   }
   else if (len1)
   {
      strcpy(string, front);
   }   
} 

/*******************************************************************
**                        FindPointingPosition
********************************************************************
** Goes through a directory and returns the position of the entry for 
** which points to the given cluster.
********************************************************************/

struct Pipe 
{
    CLUSTER firstcluster;
    struct DirectoryPosition* pos;
};

static BOOL PointingPositionFinder(RDWRHandle handle, 
                                   struct DirectoryPosition* pos,                                
                                   void** structure)
{
    struct DirectoryEntry entry;
    struct Pipe* pipe = *((struct Pipe**) structure);
    
    if (!GetDirectory(handle, pos, &entry))
       return FAIL;
    
    if ((IsDeletedLabel(entry)) ||
        (IsLFNEntry(&entry)))
    {
       return TRUE; 
    }
    
    if (GetFirstCluster(&entry) == pipe->firstcluster)
    {
       memcpy(pipe->pos, pos, sizeof(struct DirectoryPosition));     
       return FALSE;
    }
  
    return TRUE;
}

BOOL FindPointingPosition(RDWRHandle handle, CLUSTER parentcluster, 
                          CLUSTER cluster, struct DirectoryPosition* pos)
{
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.firstcluster = cluster;
    pipe.pos = pos;
    
    return TraverseSubdir(handle, parentcluster, PointingPositionFinder,
                          (void**) &ppipe, TRUE);
}
                  
/*******************************************************************
**                        TraceFullPathName
********************************************************************
** Traces back the fully qualified path name to which the given 
** directory position belongs.
********************************************************************/

BOOL TraceFullPathName(RDWRHandle handle, struct DirectoryPosition* pos,
                       char* result)
{
   struct DirectoryEntry entry;
   char userfile[14];
   BOOL lastfilename = TRUE;
   CLUSTER surroundingcluster, parentcluster;
   struct DirectoryPosition pos1;

   memcpy(&pos1, pos, sizeof(struct DirectoryPosition));   
   result[0] = 0;

   for (;;)
   {
       if (!GetDirectory(handle, &pos1, &entry))
          return FALSE;  
           
       /* Put the name of the directory in front of the resultant string. */
       ConvertEntryPath(userfile, entry.filename, entry.extension);
       
       if (!lastfilename)
          strcat(userfile, "\\");
       else
          lastfilename = FALSE;     
               
       AddToFrontOfString(result, userfile);
       
       /* If the current directory position is a root directory position
          return. */
       if (IsRootDirPosition(handle, &pos1))
       {
          AddToFrontOfString(result, "\\");
          return TRUE; 
       }
       
       surroundingcluster = GetSurroundingCluster(handle, &pos1);
       if (!surroundingcluster) return FALSE;
       parentcluster = LocatePreviousDir(handle, surroundingcluster);
       if (!parentcluster) return FALSE;
       
       if (!FindPointingPosition(handle, parentcluster, surroundingcluster,
                                 &pos1))
          return FALSE;
   }
}   

/*******************************************************************
**                        GetSurroundingFile
********************************************************************
** Returns the directory position of the file to which a cluster 
** belongs.
**
** Returns:
**      TRUE : surounding cluster found
**      FALSE: surrounding cluster not found
**      FAIL: media error
********************************************************************/  

BOOL GetSurroundingFile(RDWRHandle handle, CLUSTER cluster,
                           struct DirectoryPosition* result)
{
    BOOL found;
    CLUSTER previous, current = cluster;
    
    if (!cluster)
       return FALSE;
       
    while (cluster)
    {
        if (!FindClusterInFAT(handle, cluster, &cluster))
           return FAIL;
           
        previous = current;
    } 
    
    if (!FindClusterInDirectories(handle, previous, result, &found))
    {
       return FAIL;
    }
    
    return found;
}

/*******************************************************************
**                        TraceClusterPathName
********************************************************************
** Traces back the fully qualified path name to which the given 
** cluster belongs.
**
** Returns:
**      TRUE : path name traced
**      FALSE:  media error
********************************************************************/ 

BOOL TraceClusterPathName(RDWRHandle handle, CLUSTER cluster,
                          char* result)
{
    struct DirectoryPosition temp;
    
    if (GetSurroundingFile(handle, cluster, &temp) == TRUE)
    {
       if (!TraceFullPathName(handle, &temp, result))
          return FALSE;
          
       return TRUE;
    }
    else
    {
       return FALSE;
    }
}

/*******************************************************************
**                        TracePreviousDir
********************************************************************
** Traces back the directory position of the directory that contains 
** the given directory.
**
** Returns:
**      TRUE : path name traced
**      FALSE: root directory entered
**      FAIL: media error
********************************************************************/ 

BOOL TracePreviousDir(RDWRHandle handle, struct DirectoryPosition* thisPos,
                      struct DirectoryPosition* previousPos)
{
    CLUSTER surroundingcluster, parentcluster;
    
    if (IsRootDirPosition(handle, thisPos))
    {
       return FALSE; 
    }
    
    surroundingcluster = GetSurroundingCluster(handle, thisPos);
    if (!surroundingcluster) return FAIL;
    
    parentcluster = LocatePreviousDir(handle, surroundingcluster);
    if (!parentcluster) return FALSE;
    
    if (!FindPointingPosition(handle, parentcluster, surroundingcluster,
                              previousPos))
       return FAIL;
     
    return TRUE;
}
