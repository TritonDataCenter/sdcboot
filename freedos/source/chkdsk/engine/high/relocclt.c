/*    
   RelocClt.c - Function to relocate a cluster in a volume.

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

#include <assert.h>
#include <stdio.h>

#include "fte.h"

static VirtualDirectoryEntry* FatReferedMap=NULL;

static BOOL CreateFatReferedMap(RDWRHandle handle)
{
    unsigned long LabelsInFat;
    CLUSTER current, label;

    LabelsInFat = GetLabelsInFat(handle);
    if (!LabelsInFat) return FAIL;
        
    FatReferedMap = CreateVFSBitField(handle, LabelsInFat);
    if (!FatReferedMap) return FAIL;

    for (current = 2; current < LabelsInFat; current++)
    {
	if (!GetNthCluster(handle, current, &label))
	    return FALSE;

	if (FAT_NORMAL(label))
	{
	    if (!SetVFSBitfieldBit(FatReferedMap, label))
		return FALSE;	
	}    
    }

    return TRUE;
}

void DestroyFatReferedMap()
{
    if (FatReferedMap)
        DestroyVFSBitfield(FatReferedMap);

    FatReferedMap = NULL;
}

BOOL BreakFatMapReference(RDWRHandle handle, CLUSTER label)
{
    return ClearVFSBitfieldBit(FatReferedMap, label);	
}

BOOL CreateFatMapReference(RDWRHandle handle, CLUSTER label)
{
    return SetVFSBitfieldBit(FatReferedMap, label);
}

BOOL SwapFatMapReference(RDWRHandle handle, CLUSTER source, CLUSTER destination)
{
    int value;

    if (!GetVFSBitfieldBit(FatReferedMap, source, &value))
        return FALSE;
    if (!ClearVFSBitfieldBit(FatReferedMap, source))
        return FALSE;

    if (value)
    {
        if (!SetVFSBitfieldBit(FatReferedMap, destination))
            return FALSE;
    }
    else
    {
        if (!ClearVFSBitfieldBit(FatReferedMap, destination))
            return FALSE;
    }

    return TRUE;
}

/* This function returns FALSE if the destination is not free. */
BOOL RelocateCluster(RDWRHandle handle, CLUSTER source, CLUSTER destination)
{
   int labelsize, value;
   CLUSTER fatpos=0, freecluster, dircluster, label;
   struct DirectoryPosition dirpos;
   struct DirectoryEntry entry;
   BOOL IsInFAT = FALSE;
   SECTOR srcsector, destsector;
   unsigned long sectorspercluster;
   CLUSTER clustervalue;
   BOOL found, DOTSprocessed=FALSE;
   struct FSInfoStruct FSInfo;

   if (!FatReferedMap)
   {
	if (!CreateFatReferedMap(handle))
	    return FALSE;
   }

   /* See wether the destination is actually free. */
   if (!GetNthCluster(handle, destination, &label))
      RETURN_FTEERROR(FALSE);
   if (!FAT_FREE(label)) 
      RETURN_FTEERROR(FALSE);
   
   /* Do some preliminary calculations. */
   srcsector = ConvertToDataSector(handle, source);
   if (!srcsector)
      RETURN_FTEERROR(FALSE);
   destsector = ConvertToDataSector(handle, destination);
   if (!destsector)
      RETURN_FTEERROR(FALSE);
   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster)
      RETURN_FTEERROR(FALSE);
         
   /* Get the value that is stored at the source position in the FAT */
   if (!ReadFatLabel(handle, source, &clustervalue))
      RETURN_FTEERROR(FALSE);
   
   /* See where the cluster is refered */
   if (!GetVFSBitfieldBit(FatReferedMap, source, &value))
      return FALSE;

   if (value)
   {//CLUSTER fatpos1;

	if (!FindClusterInFAT(handle, source, &fatpos))
	    RETURN_FTEERROR(FALSE);
/*
        if (!FindClusterInFAT1(handle, source, &fatpos1))
            RETURN_FTEERROR(FALSE);

        if (fatpos != fatpos1)
            printf("hola");
*/
   }
      
   if (!fatpos)
   {
      if (!FindClusterInDirectories(handle, source, &dirpos, &found))
         RETURN_FTEERROR(FALSE);
      if (!found)
      {
          /* Note: on FAT32 this cluster may be pointing to the root directory.
                   We do not support relocating the root cluster at this time.          
          */
          
          
          RETURN_FTEERROR(FALSE);                /* Non valid cluster! */
      }
      else
      {
         /*
            This is the first cluster of some file. See if it is a directory
            and if it is, adjust the '.' entry of this directory and all
            of the '..' entries of all the (direct) subdirectories to point
            to the new cluster.
         */
         if (!GetDirectory(handle, &dirpos, &entry))
            RETURN_FTEERROR(FALSE);
            
         if (entry.attribute & FA_DIREC)
         {
            dircluster = GetFirstCluster(&entry);
            if (!AdaptCurrentAndPreviousDirs(handle, dircluster, destination))
            {
               RETURN_FTEERROR(FALSE);
            }
            DOTSprocessed = TRUE;
         }
      }
   }
   else
   {
      IsInFAT = TRUE;
   }

   /* Copy all sectors in this cluster to the new position */
   if (!CopySectors(handle, srcsector, destsector, sectorspercluster))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      RETURN_FTEERROR(FALSE);
   }
   
   /* Write the entry in the FAT */
   if (!WriteFatReference(handle, destination, clustervalue))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      RETURN_FTEERROR(FALSE);
   }

   if (!WriteFatLabel(handle, destination, clustervalue))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      RETURN_FTEERROR(FALSE);
   }

   /* Adjust the pointer to the relocated cluster */
   if (IsInFAT)
   {
      if (!WriteFatReference(handle, fatpos, destination))
      {
         RETURN_FTEERROR(FALSE);
      }

      if (!WriteFatLabel(handle, fatpos, destination))
      {
         RETURN_FTEERROR(FALSE);
      }

      if (!ClearVFSBitfieldBit(FatReferedMap, source))
	  return FALSE;
      if (!SetVFSBitfieldBit(FatReferedMap, destination))
	  return FALSE;
      //if (!IndicateFatClusterMoved(fatpos, source, destination))
      //   return FALSE;
   }
   else
   {
      CLUSTER label;

      if (!GetDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         RETURN_FTEERROR(FALSE);
      }
      
      SetFirstCluster(destination, &entry);
      if (!WriteDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         RETURN_FTEERROR(FALSE);
      }

      if (!IndicateDirEntryMoved(source, destination))
	  return FALSE;

     if (!GetNthCluster(handle, source, &label))
         return FALSE;

     //if (!IndicateFatClusterMoved(label, source, destination))
     //   return FALSE;
   }

   if (!WriteFatReference(handle, source, FAT_FREE_LABEL))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
   
      RETURN_FTEERROR(FALSE);
   }

   if (!WriteFatLabel(handle, source, FAT_FREE_LABEL))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
   
      RETURN_FTEERROR(FALSE);
   }

   if (!IndicateDirClusterMoved(handle, source, destination))
      RETURN_FTEERROR(FALSE);
      
   /* Adjust FSInfo on FAT32 */
   labelsize = GetFatLabelSize(handle);
   if (labelsize == FAT32)
   {
      if (!GetFreeClusterSearchStart(handle, &freecluster))
         RETURN_FTEERROR(FALSE);
         
      if (source < freecluster) /* source cluster became available */
      {
         if (!ReadFSInfo(handle, &FSInfo))
            RETURN_FTEERROR(FALSE);
    
         WriteFreeClusterStart(&FSInfo, source);
    
         if (!WriteFSInfo(handle, &FSInfo))
            RETURN_FTEERROR(FALSE);          
      }
      
      if ((freecluster == destination) && /* We are relocating to the first */
	  (destination < source))         /* free cluster */
      {
         CLUSTER dummy;     
 
         if (!FindFirstFreeSpace(handle, &dummy, &dummy))
            RETURN_FTEERROR(FALSE);
      }
   }
   
   return TRUE;
}
