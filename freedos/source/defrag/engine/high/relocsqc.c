/*    
   RelocSqc.c - Function to relocate a sequence of clusters in a volume.

   Copyright (C) 2004 Imre Leber

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

#define COPY_BLOCK_SIZE 64 /* Maximum sectors in a block */

static BOOL RelocateConsecutiveClusterSeq(RDWRHandle handle, CLUSTER source, CLUSTER destination, unsigned long flength);
static BOOL GetNextConsecutiveChain(RDWRHandle handle, CLUSTER source, unsigned long maxlength, unsigned long* length);
static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long filesize, BOOL atend);
static BOOL CopyClusters(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long length);
static BOOL CleanFatLabels(RDWRHandle handle, CLUSTER dest, unsigned long length);
//static BOOL IndicateReferencesBroken(RDWRHandle handle, CLUSTER source, unsigned long length);
static BOOL IndicateReferencesCreated(RDWRHandle handle, CLUSTER destination, unsigned long length);
static BOOL IndicateClustersMoved(RDWRHandle handle, CLUSTER source, CLUSTER destination,
                                  unsigned long n);

BOOL RelocateClusterSequence(RDWRHandle handle, CLUSTER source, CLUSTER destination, unsigned long length)
{
    unsigned long flength;
   
    /* Small optimization for when length == 1*/
    if (length == 1)
    {
   return RelocateCluster(handle, source, destination);
    }
    
    /* Change the structures for the relocation */
    while (length > 0)
    {    
        if (!GetNextConsecutiveChain(handle, source, length, &flength))     
      RETURN_FTEERROR(FALSE);    

        if (!RelocateConsecutiveClusterSeq(handle, source, destination, flength))
       RETURN_FTEERROR(FALSE);

// if (!IndicateReferencesCreated(handle, destination, flength))
//     RETURN_FTEERROR(FALSE);

        if (!IndicateClustersMoved(handle, source, destination, flength))
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

BOOL RelocateOverlapping(RDWRHandle handle, CLUSTER source, CLUSTER destination, unsigned long length)
{
    CLUSTER label;
    unsigned long i;

    /* Part of the source is overlapping with the target which is free */

    /*
       Assumptions for now:
       - source is continous
       - target < source
       - length is only for one file
    */
 
//    if (!IndicateReferencesBroken(handle, source, length))
//       RETURN_FTEERROR(FALSE);

    if (!RelocateCluster(handle, source, destination))
   RETURN_FTEERROR(FALSE);

    if (length == 1) return TRUE;              /* If we have to relocate just one cluster, we're done */

    for (i=1; i < length; i++)
    {
        if (!SwapFatMapReference(handle, source+i, destination+i))
            RETURN_FTEERROR(FALSE);
        
        if (!GetNthCluster(handle, source+i, &label))
            RETURN_FTEERROR(FALSE);

        //if (!IndicateFatClusterMoved(label, source+i, destination+i))
        //    RETURN_FTEERROR(FALSE);
    }

    /* After that, just move all the data from the source to the destination,
       adding fat labels to the destination */
    if (!GetNthCluster(handle, source+length-1, &label))
   RETURN_FTEERROR(FALSE);

    if (!CopyClusters(handle, source+1, destination+1, length-1))
   RETURN_FTEERROR(FALSE);

    if (!MarkClusterChain(handle, source, destination, length-1, FAT_LAST(label)))
   RETURN_FTEERROR(FALSE);      

    /* In the end, remove all the fat labels from the source */
    if (!CleanFatLabels(handle, destination+length, source-destination))
   RETURN_FTEERROR(FALSE);        

    if (!IndicateReferencesCreated(handle, destination, length))
       RETURN_FTEERROR(FALSE);

//    if (!IndicateClustersMoved(handle, source, destination, length))
//       RETURN_FTEERROR(FALSE);

    return TRUE;
}

static BOOL RelocateConsecutiveClusterSeq(RDWRHandle handle, CLUSTER source, CLUSTER destination, unsigned long flength)
{
    CLUSTER label;
    unsigned long i;
    
    assert(flength>0);
    
    if (!RelocateCluster(handle, source, destination))
   RETURN_FTEERROR(FALSE);
    
    if (flength == 1) return TRUE;              /* If we have to relocate just one cluster, we're done */
    
        for (i=1; i < flength; i++)
        {
            if (!SwapFatMapReference(handle, source+i, destination+i))
                RETURN_FTEERROR(FALSE);
            
            if (!GetNthCluster(handle, source+i, &label))
                RETURN_FTEERROR(FALSE);

           // if (!IndicateFatClusterMoved(label, source+i, destination+i))
           //     RETURN_FTEERROR(FALSE);
        }


    /* The first cluster is relocated to the destination, the link to break is now there */
//    if (!IndicateReferencesBroken(handle, destination, 1))
//        RETURN_FTEERROR(FALSE);

//    if (!IndicateReferencesBroken(handle, source+1, flength-1))
//        RETURN_FTEERROR(FALSE);

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

static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long filesize, BOOL atend)
{
    CLUSTER label;
    unsigned long i;    
    
    for (i=0; i<filesize; i++)
    {
   if (!WriteFatReference(handle, dest+i, dest+i+1))
       RETURN_FTEERROR(FALSE);

   if (!WriteFatLabel(handle, dest+i, dest+i+1))
       RETURN_FTEERROR(FALSE);
    }

    if (atend)
    {           
        if (!WriteFatReference(handle, dest+filesize, FAT_LAST_LABEL))
      RETURN_FTEERROR(FALSE);
       
        if (!WriteFatLabel(handle, dest+filesize, FAT_LAST_LABEL))
      RETURN_FTEERROR(FALSE);
    }    
    else
    {
   if (!GetNthCluster(handle, source+filesize, &label))
       RETURN_FTEERROR(FALSE);

        if (!WriteFatReference(handle, dest+filesize, label))
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
   if (!WriteFatReference(handle, dest + i, FAT_FREE_LABEL))
       RETURN_FTEERROR(FALSE);

   if (!WriteFatLabel(handle, dest + i, FAT_FREE_LABEL))
       RETURN_FTEERROR(FALSE);
    }
   
    return TRUE;       
}

#if 0
static BOOL IndicateReferencesBroken(RDWRHandle handle, CLUSTER source, unsigned long length)
{
    CLUSTER label;

    while (length--)
    {
        if (!GetNthCluster(handle, source, &label))
      return FALSE;

   if (FAT_NORMAL(label))
   {
      if (!BreakFatMapReference(handle, label)) 
         return FALSE;
      //if (!BreakFatTableReference(handle, label)) 
           //  return FALSE;
   }
   source++;
    }

    return TRUE;
}
#endif

static BOOL IndicateReferencesCreated(RDWRHandle handle, CLUSTER destination, unsigned long length)
{
    CLUSTER label;
    while (length--)
    {
        if (!GetNthCluster(handle, destination, &label))
      return FALSE;

   if (FAT_NORMAL(label))
   {
      if (!CreateFatMapReference(handle, label))
         return FALSE;
      //if (!CreateFatTableReference(handle, destination, label)) 
      // return FALSE;
   }
    
   destination++;
    }

    return TRUE;
}

static BOOL IndicateClustersMoved(RDWRHandle handle, CLUSTER source, CLUSTER destination,
                                  unsigned long n)
{
    unsigned long i;

    for (i=0; i < n; i++)
    {
        if (!IndicateDirClusterMoved(handle, source++, destination++))
            return FALSE;    
    }

    return TRUE;
}
