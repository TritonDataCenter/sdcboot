/*
   FileChn.c - File entries creation code.
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
    BOOL found;
    struct DirectoryPosition* pos;
};

static CLUSTER FindFreeCluster(RDWRHandle handle, CLUSTER spil);

static BOOL AddRootDirectory(RDWRHandle handle,
                             struct DirectoryPosition* pos);
                             
static BOOL AddSubDirectory(RDWRHandle handle,
                            CLUSTER firstcluster,
                            struct DirectoryPosition* pos);

static BOOL GetLastEntryInCluster(RDWRHandle handle, CLUSTER cluster,
                                  struct DirectoryPosition* pos);
                                  
static BOOL InitialiseNewCluster(RDWRHandle handle, CLUSTER cluster);

static BOOL GetFirstDeletedEntry(RDWRHandle handle, CLUSTER firstcluster,
                                 struct DirectoryPosition* pos);

static BOOL DeletedEntryFinder(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               void** structure);

/*******************************************************************
**                        CreateFileChain
********************************************************************
** Allocates a new cluster on the volume and fills it into the given
** directory entry.
**
** Returns  TRUE  if a new cluster is allocated
**                the allocated cluster is returned in newcluster
**
**          FALSE if a new cluster could not be allocated
**
**          FAIL  if there was an error
********************************************************************/

BOOL CreateFileChain(RDWRHandle handle, struct DirectoryPosition* pos,
                     CLUSTER *newcluster)
{
   CLUSTER freespace;
   unsigned long length;
   struct DirectoryEntry entry;

   if (!FindFirstFreeSpace(handle, &freespace, &length))
      return FAIL;

   if (length == 0)
      return FALSE;

   if (!GetDirectory(handle, pos, &entry))
      return FAIL;

   if (!WriteFatLabel(handle, freespace, FAT_LAST_LABEL))
      return FAIL;

   SetFirstCluster(freespace, &entry);
   if (!WriteDirectory(handle, pos, &entry))
   {
      if (!WriteFatLabel(handle, freespace, FAT_FREE_LABEL))
         return FAIL;
   
      return FAIL;
   }

   *newcluster = freespace;
   return TRUE;
}

/*******************************************************************
**                        ExtendFileChain
********************************************************************
** Allocates a new cluster on the volume and adds it to the given file
** chain.
**
** Returns  TRUE  if a new cluster is allocated
**                the allocated cluster is returned in newcluster
**
**          FALSE if a new cluster could not be allocated
**
**          FAIL  if there was an error
********************************************************************/

BOOL ExtendFileChain(RDWRHandle handle, CLUSTER firstcluster,
                     CLUSTER* newcluster)
{
   CLUSTER current = firstcluster, label = current, freeclust;

   /* Search the end of the file chain. */
   while (FAT_NORMAL(label))
   {
      current = label;         
      if (!GetNthCluster(handle, current, &label))
         return FAIL;
   }

   /* Sanity check */
   if (!FAT_LAST(label))
   {
      SetFTEerror(FTE_FILESYSTEM_BAD);
      return FAIL;
   }

   /* Search a new free spot in the file system (intelligently). */
   freeclust = FindFreeCluster(handle, current);
   if (freeclust == FALSE) 		 return FALSE;
   if (freeclust == 0xFFFFFFFF)  return FAIL;

   /* Add the free cluster to the file chain. */
   if (!WriteFatLabel(handle, current, freeclust))
      return FAIL;

   if (!WriteFatLabel(handle, freeclust, FAT_LAST_LABEL))
   {
      /* Back track */
      WriteFatLabel(handle, current, FAT_LAST_LABEL);
      return FAIL;
   }

   *newcluster = freeclust;
   return TRUE;
}

static CLUSTER FindFreeCluster(RDWRHandle handle, CLUSTER spil)
{
   CLUSTER i, label;
   unsigned long clustersindataarea;

   clustersindataarea = GetLabelsInFat(handle);
   if (!clustersindataarea) return FALSE;

   /* Search forward */
   for (i = spil; i < clustersindataarea; i++)
   {
      if (!GetNthCluster(handle, i, &label))
         return FAIL;

      if (FAT_FREE(label)) return i;
   }

   /* Search backward. */
   for (i = spil; i >= 2; i--)
   {
      if (!GetNthCluster(handle, i, &label))
         return 0xFFFFFFFF;

      if (FAT_FREE(label)) return i;
   }

   return FALSE;
}

/*******************************************************************
**                        AddDirectory
********************************************************************
** Adds a directory entry to the given directory.
**
** Returns  TRUE  if the directory entry
**                the allocated cluster is returned in newpos
**
**          FALSE if no new directory could be added
**                (not enough disk space or not enough entries in the
**                 root directory)
**
**          FAIL  if there was an error
**
** if firstcluster == 0 the root directory is assumed.
**
** if no directory entry could be allocated that was not used before,
** then the position of the first deleted entry is returned.
********************************************************************/

BOOL AddDirectory(RDWRHandle handle, CLUSTER firstcluster,
                  struct DirectoryPosition* pos)
{
   CLUSTER rootcluster;

   if (!firstcluster)
   {
      switch (GetFatLabelSize(handle))
      {
          case FAT12:
          case FAT16:
               return AddRootDirectory(handle, pos);
          
          case FAT32:
               rootcluster = GetFAT32RootCluster(handle);
               if (!rootcluster) return FAIL;
               
               return AddSubDirectory(handle, rootcluster, pos);

          default:
               return FAIL;
      }
   }

   return AddSubDirectory(handle, firstcluster, pos);
}

/* Only FAT12/16 */
static BOOL AddRootDirectory(RDWRHandle handle,
                             struct DirectoryPosition* pos)
{
   unsigned short numentries, i, firstdeleted;
   struct DirectoryEntry* entry;

   numentries = GetNumberOfRootEntries(handle);
   if (!numentries) return FAIL;

   firstdeleted = numentries;

   entry = AllocateDirectoryEntry();
   if (!entry) return FAIL;

   for (i = 0; i < numentries; i++)
   {
       if (!ReadDirEntry(handle, i, entry))
       {
          FreeDirectoryEntry(entry);
          return FAIL;
       }

       if (IsLastLabel(*entry))
       {
          FreeDirectoryEntry(entry);
          
          if (!GetRootDirPosition(handle, i, pos))
             return FAIL;
          else
             return TRUE;
       }

       if (IsDeletedLabel(*entry))
       {
          firstdeleted = i;
	  break; 
       }
   }

   if (firstdeleted < numentries)
   {
      if (!GetRootDirPosition(handle, firstdeleted, pos))
         return FAIL;
      else
         return TRUE;
   }

   return FALSE;
}

static BOOL AddSubDirectory(RDWRHandle handle,
                            CLUSTER firstcluster,
                            struct DirectoryPosition* pos)
{
   CLUSTER current, previous = firstcluster, newcluster;
   
   /* Get the last cluster of this directory. */
   if (!GetNthCluster(handle, firstcluster, &current))
      return FAIL;

   while (!FAT_LAST(current))
   {
       /* Sanity check */
       if (FAT_FREE(current) || FAT_BAD(current))
       {
          SetFTEerror(FTE_FILESYSTEM_BAD);
          return FAIL;
       }

       previous = current;

      if (!GetNthCluster(handle, current, &current))
         return FAIL;
   }

   /* If it is not completely full, return the position of the last free
      entry. */
   switch (GetLastEntryInCluster(handle, previous, pos))
   {
      case TRUE:
           return TRUE;
      case FAIL:
           return FAIL;
   }

   /* If it is completely full, allocate a new cluster and initialise it. */
   switch (ExtendFileChain(handle, previous, &newcluster))
   {
      case TRUE:
           if (!InitialiseNewCluster(handle, newcluster))
              return FAIL;

           pos->sector = ConvertToDataSector(handle, newcluster);
           pos->offset = 0;

           return TRUE;

      case FAIL:
           return FAIL;
   }
           
   /* If allocation was unsuccessfull, see wether a deleted entry can be found,
      if so return that entry. Otherwise return FALSE as we weren't able to add
      to the directory.*/
   return GetFirstDeletedEntry(handle, firstcluster, pos);
}

static BOOL GetLastEntryInCluster(RDWRHandle handle, CLUSTER cluster,
                                  struct DirectoryPosition* pos)
{
   struct DirectoryEntry* entries;
   unsigned char sectorspercluster;
   SECTOR target, i;
   unsigned entriespersector, j;

   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster) return FAIL;

   entriespersector = BYTESPERSECTOR / sizeof(struct DirectoryEntry);

   target = ConvertToDataSector(handle, cluster);
   if (!target) return FAIL;

   entries = (struct DirectoryEntry*) AllocateSector(handle);
   if (!entries) return FAIL;

   for (i = 0; i < sectorspercluster; i++)
   {
       if (!ReadDataSectors(handle, 1, target+i, (void*) entries))
       {
          FreeSectors((SECTOR*) entries);
          return FAIL;
       }

       for (j = 0; j < entriespersector; j++)
       {
           if (IsLastLabel(entries[j]))
           {
              pos->sector = target+i;
              pos->offset = j;
              FreeSectors((SECTOR*) entries);
              return TRUE;
           }
       }
   }

   FreeSectors((SECTOR*) entries);
   return FALSE;
}

static BOOL InitialiseNewCluster(RDWRHandle handle, CLUSTER cluster)
{
    void* buf;
    SECTOR target;
    unsigned char sectorspercluster, i;

    buf = (void*) AllocateSector(handle);
    if (!buf) return FALSE;

    memset(buf, 0, BYTESPERSECTOR);

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster)
    {
       FreeSectors((SECTOR*) buf);
       return FALSE;
    }

    target = ConvertToDataSector(handle, cluster);

    for (i = 0; i < sectorspercluster; i++)
    {
        if (!WriteDataSectors(handle, 1, target, buf))
        {
           FreeSectors((SECTOR*) buf);
           return FALSE;
        }
    }

    FreeSectors((SECTOR*) buf);
    return TRUE;
}

static BOOL GetFirstDeletedEntry(RDWRHandle handle, CLUSTER firstcluster,
                                 struct DirectoryPosition* pos)
{
    struct Pipe pipe, *ppipe = &pipe;

    pipe.found = FALSE;
    pipe.pos   = pos;

    if (!TraverseSubdir(handle, firstcluster, DeletedEntryFinder,
                        (void**) &ppipe, TRUE))
       return FAIL;

    return pipe.found;
}

static BOOL DeletedEntryFinder(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               void** structure)
{
    struct Pipe* pipe = *((struct Pipe**) structure);
    struct DirectoryEntry* entry;

    entry = AllocateDirectoryEntry();
    if (!entry) return FAIL;

    if (!GetDirectory(handle, pos, entry))
    {
       FreeDirectoryEntry(entry);
       return FAIL;
    }

    if (IsDeletedLabel(*entry))
    {
       FreeDirectoryEntry(entry);
       pipe->found  = TRUE;
       memcpy(pipe->pos, pos, sizeof(pos));

       return FALSE;
    }

    FreeDirectoryEntry(entry);
    return TRUE;
}
