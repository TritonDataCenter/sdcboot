/*    
   Flldrvmp.c - fill drive map.

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

#include <stdlib.h>
#include <stdio.h>

#include "fte.h"
#include "infofat.h"
#include "expected.h"
#include "useful.h"
#include "..\..\modlgate\custerr.h"

unsigned GetDrawDrvMapFactor(RDWRHandle handle);

STATIC int ReportDriveSize(RDWRHandle handle);

STATIC int Reporter(RDWRHandle handle, CLUSTER label, SECTOR sector,
                    void** structure);

/************************************************************
***                        FillDriveMap
*************************************************************
*** Fills the drive map
*************************************************************/

struct drivemap
{
    unsigned ClustersPerBlock;
    int weigth;
};

int FillDriveMap(RDWRHandle handle)
{
    unsigned MaxBlock;
    struct drivemap pipe, *ppipe = &pipe;
    unsigned long labelsinfat;

    MaxBlock = GetMaximumDrvMapBlock();
    if (!MaxBlock) return TRUE;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat)
    {
        SetCustomError(WRONG_LABELSINFAT);
        RETURN_FTEERR(FALSE);
    }

    pipe.ClustersPerBlock = (unsigned)
            ((labelsinfat + (MaxBlock-1)) / MaxBlock);
    pipe.weigth = 0;
    
    SmallMessage("Calculating drive size.");
    if (ReportDriveSize(handle))
    {
       SmallMessage("Drawing drive map.");
       if (!LinearTraverseFat(handle, Reporter, (void**)&ppipe))
       {
     SetCustomError(DRAW_DRIVEMAP_ERROR);
          RETURN_FTEERR(FALSE);
       }
       return TRUE;
    }
    else
    {
       SetCustomError(GET_DRIVE_SIZE_FAILED);
       RETURN_FTEERR(FALSE);
    }
}

/************************************************************
***                        Reporter
*************************************************************
*** Looks at every label in the FAT and reports accordingly
*************************************************************/

static char acBuffer[50];                                    

STATIC int Reporter(RDWRHandle handle, CLUSTER label, SECTOR sector,
                    void** structure)
{
    int     symbol;
    CLUSTER cluster;    
    struct drivemap *pipe = *((struct drivemap **)structure);
    int currentweigth;
    unsigned long clustersindataarea;

    cluster = DataSectorToCluster(handle, sector);
    if (!cluster) RETURN_FTEERR(FAIL);

    /* Update status info in order not to appear stalled */
    clustersindataarea = GetClustersInDataArea(handle);
    if (!clustersindataarea) RETURN_FTEERR(FAIL);

    sprintf(acBuffer, "Drawing drive map (%lu/%lu).", 
                                               cluster, clustersindataarea);
//    SmallMessage(acBuffer);

    if (FAT_FREE(label))
    {
   symbol = UNUSEDSYMBOL;
   currentweigth = 0;
    }
    else if (FAT_BAD(label))
    {
   symbol = BADSYMBOL;
   currentweigth = 4;
    }
    else if (FAT_LAST(label))
    {
   symbol = USEDSYMBOL;
   currentweigth = 2;
    }
    else if (FAT_NORMAL(label))
    {
   symbol = USEDSYMBOL;
   currentweigth = 2;
    }
    else
    {
       RETURN_FTEERR(FAIL);
    }

    if (cluster % pipe->ClustersPerBlock == 0)
       pipe->weigth = 0;

    if (currentweigth > pipe->weigth)
    {
       DrawOnDriveMap(cluster, symbol);
       pipe->weigth = currentweigth;
    }

    return TRUE;
}

/************************************************************
***                        ReportDriveSize
*************************************************************
*** Reports the size of the volume.
*************************************************************/

STATIC int ReportDriveSize(RDWRHandle handle)
{
   unsigned long clusters;

   clusters = GetClustersInDataArea(handle);

   if (clusters == 0) RETURN_FTEERR(FALSE);

   DrawDriveMap(clusters);

   return TRUE;
}
