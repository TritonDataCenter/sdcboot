/*    
   RelocChn.c - Function to relocate a chain of clusters in a volume.

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

#include "fte.h"

static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER dest, unsigned long filesize);
static BOOL CleanFatLabelChain(RDWRHandle handle, CLUSTER source);
static BOOL CopyClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest);
static BOOL CopyConsecutiveClusters(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long length);
static BOOL GetNextConsecutiveChain(RDWRHandle handle, CLUSTER source, unsigned long* length);

/*
 Relocates a complete cluster chain starting at source to the free space started at dest.
 
 The destination must have enough free space to contain the complete cluster chain.
*/


BOOL RelocateClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest)
{
    unsigned long filesize;
    
    /* Start by relocating the first cluster */
    if (!RelocateCluster(handle, source, dest))
	RETURN_FTEERROR(FALSE);
    
    /* After that, just move all the data from the source to the destination,
       adding fat labels to the destination */

    filesize = CalculateFileSize(handle, source);
    if (filesize == FAIL)
	RETURN_FTEERROR(FALSE);
    
    if (!MarkClusterChain(handle, dest, filesize))
	RETURN_FTEERROR(FALSE);
    
    if (!CopyClusterChain(handle, source, dest))
	RETURN_FTEERROR(FALSE);
    
    /* In the end, remove all the fat labels from the source */
    if (!CleanFatLabelChain(handle, source))
	RETURN_FTEERROR(FALSE);
   
    return TRUE;
}
    

static BOOL MarkClusterChain(RDWRHandle handle, CLUSTER dest, unsigned long filesize)
{
    unsigned long i;
    
    for (i=0; i<filesize; i++)
    {
	if (!WriteFatLabel(handle, dest+i, dest+i+1))
	    RETURN_FTEERROR(FALSE);
    }
    
    if (!WriteFatLabel(handle, dest+filesize, FAT_LAST_LABEL))
	RETURN_FTEERROR(FALSE);
    
    return TRUE;    
}

static BOOL CleanFatLabelChain(RDWRHandle handle, CLUSTER source)
{
    CLUSTER label;
    
    do {
	
	if (!GetNthCluster(handle, source, &label))
	    RETURN_FTEERROR(FALSE);	
	
	if (!WriteFatLabel(handle, source, FAT_FREE_LABEL))
	    RETURN_FTEERROR(FALSE);	
	
	source = label;
	
    } while (FAT_NORMAL(label));
    
    return TRUE;
}

static BOOL CopyClusterChain(RDWRHandle handle, CLUSTER source, CLUSTER dest)
{
    unsigned long length;
    
    do {
        if (!GetNextConsecutiveChain(handle, source, &length))
	   RETURN_FTEERROR(FALSE);
    
	if (!CopyConsecutiveClusters(handle, source, dest, length))
	    RETURN_FTEERROR(FALSE);
	
	dest += length;
	
	if (!GetNthCluster(handle, source+length-1, &source))
	    RETURN_FTEERROR(FALSE);
	
    } while (FAT_NORMAL(source));
    
    
    return TRUE;
}

BOOL CopyConsecutiveClusters(RDWRHandle handle, CLUSTER source, CLUSTER dest, unsigned long length)
{
    SECTOR ssource, sdest;
    unsigned long sectorspercluster, i, blocks;
    unsigned rest;
    
    ssource = ConvertToDataSector(handle, source);
    if (!ssource) RETURN_FTEERROR(FALSE);
	
    sdest = ConvertToDataSector(handle, dest);
    if (!sdest) RETURN_FTEERROR(FALSE);
	
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERROR(FALSE);
    
    blocks = (length * sectorspercluster) / 32768L;
    rest   = (unsigned)((length * sectorspercluster) % 32768L);
    
    for (i=0; i<blocks; i++)
    {
	if (!CopySectors(handle, ssource, sdest, 32768U))
	    RETURN_FTEERROR(FALSE);
	
	ssource += 32768L;
	sdest   += 32768L;
    }
    
    if (rest > 0)
    {
	if (!CopySectors(handle, ssource, sdest, rest))
	    RETURN_FTEERROR(FALSE);
    }
    
    return TRUE;    
}

static BOOL GetNextConsecutiveChain(RDWRHandle handle, CLUSTER source, unsigned long* length)
{
    CLUSTER current = source, label;
    
    if (!GetNthCluster(handle, source, &label))
	RETURN_FTEERROR(FALSE);
    
    *length = 0;
    
    while (FAT_NORMAL(label))
    {
	if (label != current+1)
	{
	   return TRUE;        	    	    
	}	
	
	(*length)++;
	
	if (!GetNthCluster(handle, current, &label))
	    RETURN_FTEERROR(FALSE);
    }
    
    (*length)++;
    return TRUE;    
}
