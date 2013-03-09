/*    
   DfrgDrvr.c - driver for volume defragmentation.

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

#include "fte.h"
#include "..\..\modlgate\expected.h"
#include "..\dtstruct\clmovmap.h"
#include "..\..\modlgate\custerr.h"


static unsigned long GetUpperCluster(RDWRHandle handle);

BOOL DefragmentVolume(RDWRHandle handle,
                      BOOL (*select)(RDWRHandle handle,
                                     CLUSTER startfrom,
                                     CLUSTER* clustertoplace),
                      BOOL (*place)(RDWRHandle handle,
                                    CLUSTER clustertoplace,
                                    CLUSTER startfrom,
                                    CLUSTER* wentto))
{
     CLUSTER clustertoplace;
     CLUSTER cluster1 = 2, cluster2;

     CLUSTER uppercluster;

     uppercluster = GetUpperCluster(handle);
     if (!uppercluster) RETURN_FTEERR(FALSE);
	 
     for (;;)
     {
         if (QuerySaveState())
            return TRUE;        /* The user requested the process to stop */

	 switch (select(handle, cluster1, &clustertoplace))
         {
           case TRUE:
                /* Indicate how much of the disk that has already been optimized */
                IndicatePercentageDone(cluster1, uppercluster);
                
                switch (place(handle, clustertoplace, cluster1, &cluster2))
                {
                   case TRUE:
			cluster1 = cluster2;
			break;

                   case FALSE: /* Disk full. */
			return TRUE;

		   case FAIL:
			RETURN_FTEERR(FALSE);
		   
		   default:
		        assert(FALSE);
                }
                break;
                
           case FALSE: /* All done with defragmentation. */
                //CommitCache();
	        IndicatePercentageDone(cluster1, cluster1);
                return TRUE;

	   case FAIL:  /* An error occured. */
		RETURN_FTEERR(FALSE);
	   
	   default:
	        CommitCache();
	        assert(FALSE);
         }
     }
}

static unsigned long GetUpperCluster(RDWRHandle handle)
{
    BOOL isMovable;
    CLUSTER i, Upper = 2, label;
    unsigned long labelsinfat;    
    
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) 
    {   
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FALSE);
    }
	
    for (i=2; i<labelsinfat; i++)
    {
	if (!GetNthCluster(handle, i, &label))
        {
            SetCustomError(CLUSTER_RETRIEVE_ERROR);
	    RETURN_FTEERR(FALSE);
        }

	if (FAT_FREE(label) || FAT_BAD(label))
	    continue;
	
	if (!IsClusterMovable(handle, i, &isMovable))
        {            
	    RETURN_FTEERR(FALSE);
        }
	
	if (isMovable) Upper++;
    }
    
    for (i=2; (i<Upper) && (i<labelsinfat); i++)
    {
	if (!GetNthCluster(handle, i, &label))
        {
            SetCustomError(CLUSTER_RETRIEVE_ERROR);
	    RETURN_FTEERR(FALSE);
        }
	
	if (FAT_BAD(label))
	{
	   Upper++;
	   continue; 
	}
	
	if (!IsClusterMovable(handle, i, &isMovable))
	    RETURN_FTEERR(FALSE);
	
	if (!isMovable) Upper++;
    }
    
    return Upper;    
}
