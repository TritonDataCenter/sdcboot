/*    
   FndCiDir.c - Function to return the directory that contains
                the given cluster as first cluster in the corresponding
                file.

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
   CLUSTER tofind;
   struct DirectoryPosition* pos;
   BOOL* found;
};

static BOOL ClusterInDirectoryFinder(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure);

/**************************************************************
**                    FindClusterInDirectories
***************************************************************
** Goes through all the directories on the volume and returns
** the position of the directory entry of the file that starts
** at the given cluster.
**               
** If the entry was not found, pos is not changed.                              
***************************************************************/

static VFSArray table;
static VirtualDirectoryEntry* ClustersInDirectoriesMap;
static BOOL initialised = FALSE;

#if 1

static BOOL CreateDirReferedTable(RDWRHandle handle)
{    
    unsigned long labelsinfat;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;

    if (!CreateVFSArray(handle, labelsinfat, sizeof(struct DirectoryPosition), &table))
       return FALSE;

    ClustersInDirectoriesMap = CreateVFSBitField(handle, labelsinfat);
    if (!ClustersInDirectoriesMap) return FALSE;

    if (!WalkDirectoryTree(handle, ClusterInDirectoryFinder, (void**)0))
       RETURN_FTEERROR(FALSE);

    return TRUE;
}

void DestroyDirReferedTable()
{
    if (initialised)
    {
        DestroyVFSArray(&table);
        DestroyVFSBitfield(ClustersInDirectoriesMap);
        table.entry = 0;
    }

    initialised = FALSE;
}

static BOOL ClusterInDirectoryFinder(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure)
{
   CLUSTER firstcluster, cluster;
   struct DirectoryEntry* entry;
      
   if (structure);

   entry = AllocateDirectoryEntry();
   if (!entry)
   {        
      RETURN_FTEERROR(FAIL);
   }
   
   switch (IsRootDirPosition(handle, pos))
   {
   case FALSE:
        cluster = DataSectorToCluster(handle, pos->sector);
        if (!SetVFSBitfieldBit(ClustersInDirectoriesMap, cluster))
            RETURN_FTEERROR(FAIL);
        break;
   case -1:
        RETURN_FTEERROR(FAIL);
   }

   if (!GetDirectory(handle, pos, entry))
   {
      FreeDirectoryEntry(entry);
      RETURN_FTEERROR(FAIL);
   }
      
   if (!IsLFNEntry(entry)     &&
       !IsCurrentDir(*entry)  &&
       !IsPreviousDir(*entry) &&
       !IsDeletedLabel(*entry))
   {   
      firstcluster = GetFirstCluster(entry);
   
      if (!SetVFSArray(&table, firstcluster, (char*)pos))
      {
    FreeDirectoryEntry(entry);
    return FAIL;   
      }
   }
   
   FreeDirectoryEntry(entry);
   return TRUE;
}          

BOOL FindClusterInDirectories(RDWRHandle handle, CLUSTER tofind, 
                              struct DirectoryPosition* result,
                              BOOL* found)
{
    struct DirectoryPosition pos;
    if (!initialised) 
    {
   if (!CreateDirReferedTable(handle))
       return FALSE;

   initialised = TRUE;
    }
    
    if (!GetVFSArray(&table, tofind, (char*)&pos))
   return FALSE;

    if ((pos.offset == 0) && (pos.sector == 0))
   *found = FALSE;
    else
    {
        *found = TRUE;
   memcpy(result, &pos, sizeof(struct DirectoryPosition));
    }

    return TRUE;    
}

BOOL IndicateDirEntryMoved(CLUSTER source, CLUSTER destination)
{
    struct DirectoryPosition pointer;

    if (table.entry == 0)
        return TRUE;

    if (!GetVFSArray(&table, source, (char*)&pointer))
   return FALSE;

    if (!SetVFSArray(&table, destination, (char*)&pointer))
   return FALSE;

    memset(&pointer, 0, sizeof(pointer));

    if (!SetVFSArray(&table, source, (char*)&pointer))
   return FALSE;

    return TRUE;
}

BOOL IndicateDirClusterMoved(RDWRHandle handle, CLUSTER source, CLUSTER destination)
{
   /* See wether the source is in a directory and if it is, go through
      the table, changing every sector in the source cluster to 
      a sector in the destination cluster                              
   
      Don't forget to change the map itself!
   */ 
  
   BOOL isIn=FALSE;
   unsigned long sectorspercluster, i;
   SECTOR sourcesector, destsector;
   unsigned long labelsinfat;
   struct DirectoryPosition pos;

   if (ClustersInDirectoriesMap)
   {
      if (!GetVFSBitfieldBit(ClustersInDirectoriesMap, source, &isIn))
          return FALSE;
   }

   if (isIn)
   {
       sectorspercluster = GetSectorsPerCluster(handle);
       if (!sectorspercluster) return FALSE;

       sourcesector = ConvertToDataSector(handle, source);
       if (!sourcesector) return FALSE;
   
       destsector = ConvertToDataSector(handle, destination);
       if (!destsector) return FALSE;
   
       labelsinfat = GetLabelsInFat(handle);
       if (!labelsinfat) return FALSE;

       for (i=0; i < labelsinfat; i++)
       {
           if (!GetVFSArray(&table, i, (char*)&pos))
         return FALSE;

           if ((pos.sector >= sourcesector) && (pos.sector < sourcesector+sectorspercluster))
           {
              pos.sector = destsector + pos.sector - sourcesector; 

              if (!SetVFSArray(&table, i, (char*)&pos))
            return FALSE;
           }
       }

       if (!ClearVFSBitfieldBit(ClustersInDirectoriesMap, source))
           return FALSE;
       if (!SetVFSBitfieldBit(ClustersInDirectoriesMap, destination))
           return FALSE;
   }

   return TRUE;
}

#endif

#if 1
static BOOL ClusterInDirectoryFinder1(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure);

BOOL FindClusterInDirectories1(RDWRHandle handle, CLUSTER tofind, 
                              struct DirectoryPosition* result,
                              BOOL* found)
{
  struct Pipe pipe, *ppipe = &pipe;
  
  pipe.tofind = tofind;
  pipe.pos    = result;
  pipe.found  = found; 
  
  *found = FALSE;
  if (!WalkDirectoryTree(handle, ClusterInDirectoryFinder1, (void**)&ppipe))
     RETURN_FTEERROR(FALSE);
  
  return TRUE;
}

static BOOL ClusterInDirectoryFinder1(RDWRHandle handle,
                                     struct DirectoryPosition* pos,
                                     void** structure)
{
   CLUSTER firstcluster;
   struct Pipe* pipe = *((struct Pipe**) structure);
   struct DirectoryEntry* entry;
      
   entry = AllocateDirectoryEntry();
   if (!entry)
   {        
      RETURN_FTEERROR(FAIL);
   }
   
   if (!GetDirectory(handle, pos, entry))
   {
      FreeDirectoryEntry(entry);
      RETURN_FTEERROR(FAIL);
   }
      
   if (!IsLFNEntry(entry)     &&
       !IsCurrentDir(*entry)  &&
       !IsPreviousDir(*entry) &&
       !IsDeletedLabel(*entry))
   {   
      firstcluster = GetFirstCluster(entry);
   
      if (pipe->tofind == firstcluster)
      {
    memcpy(pipe->pos, pos, sizeof(struct DirectoryPosition));
         *(pipe->found)  = TRUE;
         FreeDirectoryEntry(entry);
         return FALSE;
      }
   }
   
   FreeDirectoryEntry(entry);
   return TRUE;
}          

                              
#endif
