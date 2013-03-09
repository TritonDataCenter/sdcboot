/*    
   SwapClst.c - Functions to swap clusters in a volume.

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
  
#include <stdlib.h>
#include <string.h>
  
#ifdef __BORLANDC__
#include <alloc.h>
#endif	
  
#include "fte.h"
  
/*
   Beware of circular references:

	fatpos1 = cluster2
	cluster1 = clustervalue2

       A -> B -> C
      
       D -> A -> B

       D -> A -> B -> C => D -> B -> A -> C 


       A: C

       A: A        /
       B: B        A 

       D: B
         



	cluster1 = fatpos2
	clustervalue1 = cluster2

       A -> B -> C

       B -> C -> D

       A -> B -> C -> D => A -> C -> B -> D    


       C: C      B

       A: C       
       B: D

       B: B      /
       
*/ 
  BOOL SwapClusters (RDWRHandle handle, CLUSTER clust1, CLUSTER clust2) 
{      BOOL found, IsInFAT1 = FALSE, IsInFAT2 = FALSE, error = FALSE;
    CLUSTER freeclust, clustervalue1, fatpos1, fatpos2;      CLUSTER clustervalue2, dircluster1, dircluster2;      unsigned char sectorspercluster;      SECTOR startsector;
    struct DirectoryPosition dirpos1, dirpos2;
    struct DirectoryEntry entry;
    unsigned long neededmem;
    void *MemCluster;
    BOOL DOTSProcessed1, DOTSProcessed2;
    SECTOR srcsector, destsector;
  
    /* First check wether we shouldn't be using RelocateCluster instead. */ 
    if (!ReadFatLabel (handle, clust1, &clustervalue1))
	RETURN_FTEERROR(FALSE);
      if (!ReadFatLabel (handle, clust2, &clustervalue2))    	RETURN_FTEERROR(FALSE);
      if (FAT_FREE (clustervalue1) && (FAT_FREE (clustervalue2)))
    {
	return TRUE;
    }
      if (FAT_FREE (clustervalue1))
    {
	return RelocateCluster (handle, clust2, clust1);
    }
	    if (FAT_FREE (clustervalue2))
    {
	return RelocateCluster (handle, clust1, clust2);
    }
  
    /* See if we have enough memory to load the data for one cluster
       into memory.                                               */ 
    sectorspercluster = GetSectorsPerCluster (handle);
    if (!sectorspercluster)
	RETURN_FTEERROR(FALSE);
    neededmem = sectorspercluster * BYTESPERSECTOR;
    MemCluster = malloc (neededmem);
    if (MemCluster)
    {
	/* First prepare the first cluster. */ 
	/* See where the cluster is refered */ 
	/* In FAT? */ 
	if (!FindClusterInFAT (handle, clust1, &fatpos1))
	{
	    free (MemCluster);
	    RETURN_FTEERROR(FALSE);
	}

	/* No, then look if this is the first cluster of a file. */ 
	if (!fatpos1)
	{
	    if (!FindClusterInDirectories (handle, clust1, &dirpos1, &found))
	    {
		free (MemCluster);
		RETURN_FTEERROR(FALSE);
	    }
			
	    if (!found)
	    {
		free (MemCluster);
		RETURN_FTEERROR(FALSE);	/* Non valid cluster! */
	    }
	    else
	    {
		/*
		   Special case: if this is the first cluster of a directory
		   then adjust all the . and .. pointers to 
		   reflect the new position on the volume.
		*/ 
		if (!GetDirectory (handle, &dirpos1, &entry))
		{
		    free (MemCluster);
		    RETURN_FTEERROR(FALSE);
		}
	      		if (entry.attribute & FA_DIREC)
		{
		    dircluster1 = GetFirstCluster (&entry);
		  		    if (!AdaptCurrentAndPreviousDirs(handle, dircluster1, clust2))
		    {
			AdaptCurrentAndPreviousDirs (handle, dircluster1,
						     clust1);
			free (MemCluster);
			RETURN_FTEERROR(FALSE);
		    }
		    DOTSProcessed1 = TRUE;
		}	    	    }		}
	else
	{
	    IsInFAT1 = TRUE;
	}
      
	/* Then prepare the second cluster. */ 
	/* See where the cluster is refered */ 
	/* In FAT? */ 
	if (!FindClusterInFAT (handle, clust2, &fatpos2))
	{
	    if (DOTSProcessed1)
		AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);

		free (MemCluster);
		RETURN_FTEERROR(FALSE);
	    }
      	    /* No, then look if this is the first cluster of a file. */ 
	    if (!fatpos2)
	    {
		if (!FindClusterInDirectories (handle, clust2, &dirpos2, &found))
		{
		    if (DOTSProcessed1)
			AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
	      		    free (MemCluster);
		    RETURN_FTEERROR(FALSE);
		}
	  		if (!found)
		{
		    if (DOTSProcessed1)
			AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
	      		    free (MemCluster);
		    RETURN_FTEERROR(FALSE);	/* Non valid cluster! */
		}
		else
		{
		    /*
			Special case: if this is the first cluster of a directory
			then adjust all the . and .. pointers to 
			reflect the new position on the volume.
		    */ 
		    if (!GetDirectory (handle, &dirpos2, &entry))
		    {
			if (DOTSProcessed1)
			    AdaptCurrentAndPreviousDirs (handle, dircluster1,
							 clust1);
		  			free (MemCluster);
			RETURN_FTEERROR(FALSE);
		    }
	      		    if (entry.attribute & FA_DIREC)
		    {
			dircluster2 = GetFirstCluster (&entry);
		  			if (!AdaptCurrentAndPreviousDirs(handle, dircluster2, clust1))
			{
			    AdaptCurrentAndPreviousDirs (handle, dircluster2, clust2);
		      
			    AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
		      
			    free (MemCluster);
			    RETURN_FTEERROR(FALSE);
			}
			DOTSProcessed2 = TRUE;
		    }
		}
	}
	else
	{
	    IsInFAT2 = TRUE;
	}
      	startsector = ConvertToDataSector (handle, clust1);
	if (!startsector)
	{
	    if (DOTSProcessed1)
		AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
	  	    if (DOTSProcessed2)
		AdaptCurrentAndPreviousDirs (handle, dircluster2, clust2);
	
	    free (MemCluster);
	    RETURN_FTEERROR(FALSE);
	}
      
	/* Then copy the data of the second cluster to the new position */ 
	/* Copy all sectors in this cluster to the new position */ 
	srcsector = ConvertToDataSector (handle, clust2);      	if (!srcsector)
	{
	    if (DOTSProcessed1)
		AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
	  	    if (DOTSProcessed2)
		AdaptCurrentAndPreviousDirs (handle, dircluster2, clust2);
	  	    free (MemCluster);
	    RETURN_FTEERROR(FALSE);
	}
      	destsector = ConvertToDataSector (handle, clust1);
	if (!startsector)
	{
	    if (DOTSProcessed1)
		AdaptCurrentAndPreviousDirs (handle, dircluster1, clust1);
	  	    if (DOTSProcessed2)
		AdaptCurrentAndPreviousDirs (handle, dircluster2, clust2);
	  	    free (MemCluster);
	    RETURN_FTEERROR(FALSE);
	}
      
	/* AS OF THIS POINT WE WILL NOT BE ABLE TO BACKTRACK,
	   THEREFORE WE KEEP ON GOING EVEN IF THERE WERE ERRORS. */ 
	/* Change the pointers of the first cluster that has to be moved
	   to the second cluster. */ 
	/* Write the entry in the FAT */ 
	if (clust2 == clustervalue1)	/* Beware of circular references */
	{
	    if (!WriteFatLabel (handle, clust2, clust1))
		error = TRUE;
	}
	else
	{
	    if (!WriteFatLabel (handle, clust2, clustervalue1))
		error = TRUE;
	}
      
	/* Adjust the pointer to the relocated cluster */ 
	if (IsInFAT1)		/* the previous one in the file */
	{
	    if (fatpos1 != clust2)	/* Beware of circular references */
	    {
		if (!WriteFatLabel (handle, fatpos1, clust2))
		    error = TRUE;
	    }
	}
	else			/* or the directory entry to the file */
	{
	    if (!GetDirectory (handle, &dirpos1, &entry))
		error = TRUE;
	    if (GetFirstCluster (&entry) != clust2)
	    {
		SetFirstCluster (clust2, &entry);
		if (!WriteDirectory (handle, &dirpos1, &entry))
		    error = TRUE;
	    }
	}
      
	/* Change the pointers of the second cluster that has to be moved
	   to the first cluster. */ 
	/* Write the entry in the FAT */ 
	if (clust1 == clustervalue2)	/* Beware of circular references */
	{
	    if (!WriteFatLabel (handle, clust1, clustervalue1))
		error = TRUE;
	}
	else
	{
	    if (!WriteFatLabel (handle, clust1, clustervalue2))
		error = TRUE;
	}
      	/* Adjust the pointer to the relocated cluster */ 
	if (IsInFAT2)		/* the previous one in the file */
	{
	    if (fatpos2 != clust1)	/* Beware of circular references */
	    {
		if (!WriteFatLabel (handle, fatpos2, clust1))
		    error = TRUE;
	    }
	}
	else			/* or the directory entry to the file */
	{
	    if (!GetDirectory (handle, &dirpos2, &entry))
		error = TRUE;
	  	    if (GetFirstCluster (&entry) != clust1)	/* Beware of circular references */
	    {
		SetFirstCluster (clust1, &entry);
		if (!WriteDirectory (handle, &dirpos2, &entry))
		    error = TRUE;
	    }
	}
      	
	/* Read the cluster data in memory */
	if (!ReadDataSectors (handle, sectorspercluster, startsector, MemCluster))
	    error = TRUE;
			if (!CopySectors (handle, srcsector, destsector, sectorspercluster))
	    error = TRUE;
      	/* Move the data of the first cluster to the new position. */ 
	startsector = ConvertToDataSector (handle, clust2);
	if (!WriteDataSectors(handle, sectorspercluster, startsector, MemCluster))
	    error = TRUE;       /* Salvage what can be salvaged, i.e.
				   go on with moving everything. Everything
				   that is lost is lost.                    */
		
	/* Free the memory and return */ 
	free (MemCluster);
		if (error)
	    RETURN_FTEERROR(FALSE);
    }
    else
    {
	if (!FindLastFreeCluster (handle, &freeclust))
	    RETURN_FTEERROR(FALSE);
	if (!freeclust)
	{
	    SetFTEerror (FTE_MEM_INSUFFICIENT);
	    RETURN_FTEERROR(FALSE);
	}
      	if (!RelocateCluster (handle, clust1, freeclust))
	    RETURN_FTEERROR(FALSE);
	if (!RelocateCluster (handle, clust2, clust1))
	    RETURN_FTEERROR(FALSE);
	if (!RelocateCluster (handle, freeclust, clust1))
	    RETURN_FTEERROR(FALSE);
    }
      return TRUE;}
