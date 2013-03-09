/*    
   FndCiFAT.c - Function to return the position in the FAT that 
                contains the given label.

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

/*

struct Pipe
{
   CLUSTER tofind;
   SECTOR sector;
   BOOL found;
};

static BOOL ClusterInFATFinder(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure);

*/

/**************************************************************
**                       FindClusterInFAT
***************************************************************
** Goes through the FAT and returns the position in the FAT that
** has the given cluster as a label (i.e. the previous cluster
** in the file chain).
**
** If the cluster could not be found it returns 0 as the found 
** cluster.
**
** Only returns FALSE when there has been a media error. 
***************************************************************/

static VFSArray table;
static BOOL initialised = FALSE;

static BOOL CreateFatReferedTable(RDWRHandle handle)
{
    CLUSTER current, label;
    unsigned long labelsinfat;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;

    if (!CreateVFSArray(handle, labelsinfat, sizeof(CLUSTER), &table)) return FALSE;    
    
    for (current=2; current < labelsinfat; current++)
    {
	if (!GetNthCluster(handle, current, &label))
	   return FALSE;		

	if (FAT_NORMAL(label))
	{
	    if (!SetVFSArray(&table, label, (char*)&current))
		return FALSE;
	}
    }

    return TRUE;
}

void DestroyFatReferedTable()
{
    if (initialised)
    {
        DestroyVFSArray(&table);    
        table.entry = 0;
        initialised = FALSE;
    }
}

BOOL WriteFatReference(RDWRHandle handle, CLUSTER cluster, CLUSTER label) 
{ 
    CLUSTER next, ClNull = 0; 

    if (table.entry)
    {
        if (!GetNthCluster(handle, cluster, &next)) 
            return FALSE; 

        if (FAT_NORMAL(label))
        {
           if (!SetVFSArray(&table, label, (char*)&cluster)) 
              return FALSE; 
        }
    }

    return TRUE; 
} 

/*
BOOL IndicateFatClusterMoved(CLUSTER fatpos, CLUSTER source, CLUSTER destination)
{
#if 0
    CLUSTER pointer;

    if (table.entry != 0)
    {
        if (!GetVFSArray(&table, source, (char*)&pointer))
	    return FALSE;

        if (!SetVFSArray(&table, destination, (char*)&pointer))
	    return FALSE;

        pointer = 0;

        if (!SetVFSArray(&table, source, (char*)&pointer))
	    return FALSE;

        if (FAT_NORMAL(fatpos))
        {
           if (!SetVFSArray(&table, fatpos, (char*)&destination))
              return FALSE;
        }
    }
#endif
    return TRUE;
}

BOOL BreakFatTableReference(RDWRHandle handle, CLUSTER label)
{
#if 0
    CLUSTER cluster = 0;

    if (table.entry != 0)
       return SetVFSArray(&table, label, (char*)&cluster);	
    else
#endif
       return TRUE; /* We realy did change it on the disk * /
}

BOOL CreateFatTableReference(RDWRHandle handle, CLUSTER destination, CLUSTER label)
{
#if 0
    if (table.entry != 0)
	return SetVFSArray(&table, label, (char*)&destination);
    else
#endif
	return TRUE;
}
*/
BOOL FindClusterInFAT(RDWRHandle handle, CLUSTER tofind, CLUSTER* result)
{
    if (!initialised) 
    {
	if (!CreateFatReferedTable(handle))
	    return FALSE;

	initialised = TRUE;
    }
    
    if (!GetVFSArray(&table, tofind, (char*) result))
	return FALSE;

    return TRUE;
}



BOOL FindClusterInFAT1(RDWRHandle handle, CLUSTER tofind, CLUSTER* result)
{
   unsigned long labelsinfat;
   CLUSTER cluster, label;    
    
   labelsinfat = GetLabelsInFat(handle);
   if (!labelsinfat) RETURN_FTEERROR(FALSE);
       
   for (cluster = 2; cluster < labelsinfat; cluster++)
   {
       if (!GetNthCluster(handle, cluster, &label))
	   RETURN_FTEERROR(FALSE);
       
       if (label == tofind)
       {
	   *result = cluster;
	   return TRUE;
       }   
   }
   
   *result = 0;
   return TRUE;    
}

#if 0	// old implementations (just in case)

BOOL FindClusterInFAT(RDWRHandle handle, CLUSTER tofind, CLUSTER* result)
{
   struct Pipe pipe, *ppipe=&pipe;
   CLUSTER cluster;
   
   pipe.tofind = tofind;
   pipe.sector = 0;
   pipe.found  = FALSE;
  
   if (!LinearTraverseFat(handle, ClusterInFATFinder, (void**) &ppipe))
      RETURN_FTEERROR(FALSE);

   if (!pipe.found)
   {
      *result = 0;
      return TRUE;
   }
           
   *result = DataSectorToCluster(handle, pipe.sector);
   return (*result) != 0;
}

static BOOL ClusterInFATFinder(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure)
{
   struct Pipe* pipe = (struct Pipe*) *structure;
   
   handle = handle;
   
   if (pipe->tofind == label)
   {
      pipe->sector = datasector;
      pipe->found  = TRUE;
      RETURN_FTEERROR(FALSE);
   }
   return TRUE;
}

#endif
