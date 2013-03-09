/*    
   Defrfact.c - calculate defragmentation factor.

   Copyright (C) 2000, 2002 Imre Leber

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
#include "bool.h"
#include "infofat.h"
#include "..\..\modlgate\custerr.h"

/************************************************************
***                        GetDefragFactor
*************************************************************
*** Calculates the defragmentation factor.
*************************************************************/ 

int GetDefragFactor(RDWRHandle handle)
{
    int result;
    unsigned long UsedClusters=0;
    unsigned long Fragments=0;
    unsigned long LabelsInFat, clusterno;
    CLUSTER label;
    
    LabelsInFat = GetLabelsInFat(handle);
    if (!LabelsInFat)
    {
        SetCustomError(WRONG_LABELSINFAT);
        return 255;
    }

    for (clusterno = 2; clusterno < LabelsInFat; clusterno++)
    {
	if (!GetNthCluster(handle, clusterno, &label))
        {
           SetCustomError(CLUSTER_RETRIEVE_ERROR);   
	   return 255;
        }

	if (!FAT_FREE(label) && !FAT_BAD(label))
           UsedClusters++;

	if (FAT_NORMAL(label))
	{
	   if (label > LabelsInFat) /* Check the validity of the FAT labels */
	   {
              SetCustomError(WRONG_LABEL_IN_FAT);   
	      return 255;
	   }
	   else
	   {
	      if (label != clusterno + 1)
		 Fragments++;
	   }
	}
    }

    if (UsedClusters == 0)
       return 100;              /* An empty volume is not fragmented */
  
    result = (int) (100-(100 * Fragments / UsedClusters));

    /* Check for rounding errors */
    if ((Fragments > 0) && (result == 100)) result = 99;

    return result;
}        
