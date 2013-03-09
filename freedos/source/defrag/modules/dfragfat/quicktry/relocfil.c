/*    
   RelocFil.c - Function to relocate a file on the volume.

   Copyright (C) 2007 Imre Leber

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
#include "FTE.h"

#define COPY_BLOCK_SIZE 64

static BOOL RelocateFirstCluster(RDWRHandle handle, 
                                 struct DirectoryPosition* pos,
                                 struct DirectoryEntry* entry,
                                 CLUSTER destination);
static BOOL GetNextConsecutiveChain(RDWRHandle handle, CLUSTER source, unsigned long maxlength, unsigned long* length);

static BOOL Relocate1Cluster(RDWRHandle handle,
                             CLUSTER source,
                             CLUSTER destination,
                             CLUSTER pSource);

static BOOL RelocateConsecutiveClusterSeq(RDWRHandle handle, 
                                          CLUSTER source, 
                                          CLUSTER destination,                                           
                                          unsigned long flength);

static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long filesize, BOOL atend);

static BOOL CopyClusters(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long length);

static BOOL CleanFatLabels(RDWRHandle handle, CLUSTER dest, unsigned long length);

BOOL RelocateKnownFile(RDWRHandle handle, 
                       struct DirectoryPosition* pos,
                       struct DirectoryEntry* entry,
                       CLUSTER destination)
{
    /* We assume destination long enough to relocate the complete file */
    unsigned long length, flength;
    CLUSTER firstcluster;
    CLUSTER source;
    
    firstcluster = GetFirstCluster(entry);
    length = CalculateFileSize(handle, firstcluster);
    if (length == FAIL) 
        RETURN_FTEERROR(FALSE);    

    /* Small optimization for when length == 1 */

    if (!RelocateFirstCluster(handle, pos, entry, destination))
        RETURN_FTEERROR(FALSE);

    if (length == 1) return TRUE;    
    length--;
    
    /* Change the structures for the relocation */
    firstcluster = GetFirstCluster(entry);  // Value is now different!
    if (!GetNthCluster(handle, firstcluster, &source))
       return FALSE;
    
    destination++;

    while (length > 0)
    {    
        if (!GetNextConsecutiveChain(handle, source, length, &flength))	    
	   RETURN_FTEERROR(FALSE);		

        if (!RelocateConsecutiveClusterSeq(handle, source, destination, flength))
	    RETURN_FTEERROR(FALSE);

        if (!GetNthCluster(handle, destination + flength-1, &source))
	    return FALSE;

	if (FAT_LAST(source))
	    return length == flength;

	destination += flength;
	length -= flength;
    }
      
    return TRUE;
}

static BOOL RelocateFirstCluster(RDWRHandle handle, 
                                 struct DirectoryPosition* pos,
                                 struct DirectoryEntry* entry,
                                 CLUSTER destination)
{
   int labelsize;
   CLUSTER fatpos=0, freecluster, label;
   BOOL IsInFAT = FALSE;
   SECTOR srcsector, destsector;
   unsigned long sectorspercluster;
   CLUSTER clustervalue;
   BOOL DOTSprocessed=FALSE;
   struct FSInfoStruct FSInfo;
   CLUSTER source;

   /* Do some preliminary calculations. */
   source = GetFirstCluster(entry);

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
   
   /* We know the first cluster is refered to in the directory entry given */
    /*
    This is the first cluster of some file. See if it is a directory
    and if it is, adjust the '.' entry of this directory and all
    of the '..' entries of all the (direct) subdirectories to point
    to the new cluster.
    */
    if (entry->attribute & FA_DIREC)
    {
        if (!AdaptCurrentAndPreviousDirs(handle, source, destination))
        {
            RETURN_FTEERROR(FALSE);
        }
        DOTSprocessed = TRUE;
    }

   /* Copy all sectors in this cluster to the new position */
   if (!CopySectors(handle, srcsector, destsector, sectorspercluster))
   {
      RETURN_FTEERROR(FALSE);
   }
   
   /* Write the entry in the FAT */
   if (!WriteFatLabel(handle, destination, clustervalue))
   {
      RETURN_FTEERROR(FALSE);
   }

   SetFirstCluster(destination, entry);
   if (!WriteDirectory(handle, pos, entry))
   {
        RETURN_FTEERROR(FALSE);
   }

//   if (!GetNthCluster(handle, source, &label))
//        return FALSE;

   if (!WriteFatLabel(handle, source, FAT_FREE_LABEL))
   {
      RETURN_FTEERROR(FALSE);
   }

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

static BOOL GetNextConsecutiveChain(RDWRHandle handle, CLUSTER source, unsigned long maxlength, unsigned long* length)
{
    CLUSTER current = source, label;
    unsigned long retLength;
    
    if (!GetNthCluster(handle, source, &label))
	RETURN_FTEERROR(FALSE);
    
    for (retLength=1; 
	 FAT_NORMAL(label) && (label == ++current) && (retLength < maxlength);
         retLength++)    
    {
	if (!GetNthCluster(handle, current, &label))
	    RETURN_FTEERROR(FALSE);
    }
    
    *length = retLength;
    return TRUE;    
}

static BOOL Relocate1Cluster(RDWRHandle handle,
                             CLUSTER source,
                             CLUSTER destination,
                             CLUSTER pSource)
{
   int labelsize;
   CLUSTER fatpos=0, freecluster, label;
   //BOOL IsInFAT = FALSE;
   SECTOR srcsector, destsector;
   unsigned long sectorspercluster;
   CLUSTER clustervalue;
   BOOL DOTSprocessed=FALSE;
   struct FSInfoStruct FSInfo;

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


   /* Copy all sectors in this cluster to the new position */
   if (!CopySectors(handle, srcsector, destsector, sectorspercluster))
   {
      RETURN_FTEERROR(FALSE);
   }
   
   if (!WriteFatLabel(handle, destination, clustervalue))
   {
      RETURN_FTEERROR(FALSE);
   }

   /* Adjust the pointer to the relocated cluster */
   if (!WriteFatLabel(handle, pSource, destination))
   {
      RETURN_FTEERROR(FALSE);
   }

   if (!WriteFatLabel(handle, source, FAT_FREE_LABEL))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, source, source);
   
      RETURN_FTEERROR(FALSE);
   }

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

static BOOL RelocateConsecutiveClusterSeq(RDWRHandle handle, 
                                          CLUSTER source, 
                                          CLUSTER destination,                                           
                                          unsigned long flength)
{
    CLUSTER label;
    CLUSTER pSource = destination - 1; /* Only valid in this context */
    
    assert(flength>0);
    
    if (!Relocate1Cluster(handle, source, destination, pSource))
	RETURN_FTEERROR(FALSE);
    
    if (flength == 1) return TRUE;              /* If we have to relocate just one cluster, we're done */

    /* The first cluster is relocated to the destination, the link to break is now there */
    /* After that, just move all the data from the source to the destination,
       adding fat labels to the destination */
    if (!GetNthCluster(handle, source+flength-1, &label))
	RETURN_FTEERROR(FALSE);
    
    if (!CopyClusters(handle, source+1, destination+1, flength-1))
	RETURN_FTEERROR(FALSE);    
    
    if (!MarkClusterChain(handle, source, destination, flength-1, FAT_LAST(label)))
	RETURN_FTEERROR(FALSE);      
  
    /* In the end, remove all the fat labels from the source */
    if (!CleanFatLabels(handle, source, flength))
	RETURN_FTEERROR(FALSE);        
    
    return TRUE;
}

static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long filesize, BOOL atend)
{
    CLUSTER label;
    unsigned long i;    
    
    for (i=0; i<filesize; i++)
    {
	if (!WriteFatLabel(handle, dest+i, dest+i+1))
	    RETURN_FTEERROR(FALSE);
    }

    if (atend)
    {    		 
        if (!WriteFatLabel(handle, dest+filesize, FAT_LAST_LABEL))
	   RETURN_FTEERROR(FALSE);
    }    
    else
    {
	if (!GetNthCluster(handle, source+filesize, &label))
	    RETURN_FTEERROR(FALSE);

	if (!WriteFatLabel(handle, dest+filesize, label))
	    RETURN_FTEERROR(FALSE);
    }
     
    return TRUE;    
}

static BOOL CopyClusters(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long length)
{
    SECTOR ssource, sdest;
    unsigned long sectorspercluster, i, blocks, totallength;
    unsigned rest;    
    
    ssource = ConvertToDataSector(handle, source);
    if (!ssource) RETURN_FTEERROR(FALSE);
	
    sdest = ConvertToDataSector(handle, dest);
    if (!sdest) RETURN_FTEERROR(FALSE);
	
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERROR(FALSE);
    
    totallength = length * sectorspercluster;
    
    blocks = totallength / COPY_BLOCK_SIZE;
    rest   = (unsigned)(totallength % COPY_BLOCK_SIZE);    
    
    for (i=0; i<blocks; i++)
    {    
	if (!CopySectors(handle, ssource, sdest, COPY_BLOCK_SIZE))
	    RETURN_FTEERROR(FALSE);

        ssource += COPY_BLOCK_SIZE;
        sdest   += COPY_BLOCK_SIZE;
    }
    
    if (rest > 0)
    {
	if (!CopySectors(handle, ssource, sdest, rest))
	    RETURN_FTEERROR(FALSE);	
    }
    
    return TRUE;    
}

static BOOL CleanFatLabels(RDWRHandle handle, CLUSTER dest, unsigned long length)
{
    unsigned long i;
    
    for (i=0; i<length; i++)
    {
	if (!WriteFatLabel(handle, dest + i, FAT_FREE_LABEL))
	    RETURN_FTEERROR(FALSE);
    }
	
    return TRUE;       
}
