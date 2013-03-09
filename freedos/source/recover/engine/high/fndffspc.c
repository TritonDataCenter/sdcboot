/*    
   FndFFSpc.c - functions to return the free spaces on a volume.

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

struct Pipe
{
   CLUSTER space;  
   CLUSTER startfrom;
   
   unsigned long length;   
     
   BOOL error;
};

static CLUSTER CurrentFreeSpace = 0;

static BOOL FindFreeSpaceDirectly(RDWRHandle handle, CLUSTER start,
                                  unsigned long * length);
static BOOL FirstFreeSpaceFinder(RDWRHandle handle, CLUSTER label, 
                                 SECTOR datasector, void** structure);

                                 
/**************************************************************
**                    FindFirstFreeSpace
***************************************************************
** Searches for the first free cluster in the volume and returns 
** the length. 
***************************************************************/
                                 
BOOL FindFirstFreeSpace(RDWRHandle handle, CLUSTER* space,
                        unsigned long* length)
{ 
    int labelsize;
    struct FSInfoStruct* FSInfo;
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.space     = 0;
    pipe.length    = 0;
    
    /*
    ** Get the first cluster where to search from the FSInfo
    ** structure.
    **
    ** Search starts at the cluster returned in pipe.startfrom.
    */
    if (!GetFreeClusterSearchStart(handle, &(pipe.startfrom)))
       return FALSE;
    
     /*
    ** See if the position in the FSInfo structure points to 
    ** free block.
    */   
    if (!FindFreeSpaceDirectly(handle, pipe.startfrom, length))
       return FALSE;
     
    /*
    ** If it does, we are all done.
    */       
    if (*length)
    {
       *space = pipe.startfrom;
       
       /* Remember where to start looking when looking for the
          next free block */
       CurrentFreeSpace = *space + *length;
       return TRUE;
    }        
              
    pipe.error = FALSE;
    
    /*
    ** Look through the FAT to find the first free space.
    */
    if (!LinearTraverseFat(handle, FirstFreeSpaceFinder, (void**) &ppipe))
       return FALSE;
    if (pipe.error)
       return FALSE;
       
    /*
    ** Return and remember where we found the free space.
    */
    CurrentFreeSpace = *space = pipe.space;
    *length = pipe.length;
    
    /* 
    ** If this is FAT32 and the volume is not read/only, update the
    ** FSInfo structure  
    */
    if (*length)
    {
       labelsize = GetFatLabelSize(handle);
       if ((labelsize == FAT32) && 
           (GetReadWriteModus(handle) == READINGANDWRITING))
       {
          FSInfo = AllocateFSInfo();
          if (!FSInfo) return FALSE;
         
          if (!ReadFSInfo(handle, FSInfo))
          {
             FreeFSInfo(FSInfo);
             return FALSE;
          }
    
          WriteFreeClusterStart(FSInfo, CurrentFreeSpace);
    
          if (!WriteFSInfo(handle, FSInfo))
          {
             FreeFSInfo(FSInfo);
             return FALSE;
          }  
          FreeFSInfo(FSInfo);
       }
    }
     
    CurrentFreeSpace += *length;  
    return TRUE;
}

/**************************************************************
**                    FindNextFreeSpace
***************************************************************
** Searches for the first free cluster in the volume and returns 
** the length. 
***************************************************************/

BOOL FindNextFreeSpace(RDWRHandle handle, CLUSTER* space,
                       unsigned long* length)
{
    struct Pipe pipe, *ppipe = &pipe;
    
    pipe.space     = 0;
    pipe.startfrom = CurrentFreeSpace;
    pipe.length    = 0;
    
    pipe.error = FALSE;
    
    if (!LinearTraverseFat(handle, FirstFreeSpaceFinder, (void**) &ppipe))
       return FALSE;
    if (pipe.error)
       return FALSE;
       
    CurrentFreeSpace = *space = pipe.space;
    *length = pipe.length;
    CurrentFreeSpace += *length;
        
    return TRUE;
}

/**************************************************************
**                    FindFreeSpaceDirectly
***************************************************************
** Returns how much free space (clusters) there is starting at 
** the given cluster. 
**
** This may be 0.
***************************************************************/

static BOOL FindFreeSpaceDirectly(RDWRHandle handle, CLUSTER start,
                                  unsigned long * length)
{
    CLUSTER numberofclusters, i, label;
    *length = 0;

    numberofclusters = GetLabelsInFat(handle);
    if (numberofclusters == 0) return FALSE;
    
    for (i = start; i < numberofclusters; i++)
    {
       if (!GetNthCluster(handle, i, &label))
          return FALSE;
       
       if (label == FAT_FREE_LABEL)
          *length = *length+1;       
       else 
          break;
    }
    
    return TRUE; 
}

/**************************************************************
**                    FirstFreeSpaceFinder
***************************************************************
** Higher order function that does the hard work.
***************************************************************/
          
static BOOL FirstFreeSpaceFinder(RDWRHandle handle, CLUSTER label, 
                                 SECTOR datasector, void** structure)
{
    CLUSTER cluster;
    struct Pipe *pipe = *((struct Pipe**) structure);
    
    cluster = DataSectorToCluster(handle, datasector);
    if (!cluster)
    {        
       pipe->error = TRUE;       
       return FALSE;
    }
    
    if (cluster < pipe->startfrom)
       return TRUE;
       
    if (FAT_FREE(label))
    {
       if (!pipe->space) pipe->space = cluster;
       pipe->length++; 
    }
    else
    {
      if (pipe->space)
         return FALSE;  /* Have the data we are looking for */
    }  
    
    return TRUE;
}
