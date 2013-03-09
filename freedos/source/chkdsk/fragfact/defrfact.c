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

#include <dir.h>
#include <string.h>
#include <stdio.h>

#include "fte.h"
#include "bool.h"

static int FactorCalculator(RDWRHandle handle, CLUSTER cluster,
                            SECTOR sector, void** structure);

static int FileFactCalculator(RDWRHandle handle,
                              struct DirectoryPosition* position,
                              void** structure);

struct PipeStruct {
    CLUSTER total;
    CLUSTER consecutive;
};

/************************************************************
***                 PrintFileDefragFactors
*************************************************************
*** Prints the defragmentation factor of the indicated files.
*************************************************************/

int PrintFileDefragFactors(RDWRHandle handle, char* abspath)
{
    return WalkWildcardPath(handle, abspath, 0, FileFactCalculator, NULL);
}

/************************************************************
***                    FileFactCalculator
*************************************************************
*** If this file is included in the count then it goes through
*** the given files and scans for the number of consecutive 
*** clusters (and prints them off).
*************************************************************/

static int FileFactCalculator(RDWRHandle handle,
                              struct DirectoryPosition* position,
                              void** structure)
{
    int i, sum;
    CLUSTER firstcluster;
    struct PipeStruct pipe = {0,0}, *ppipe = &pipe;
    
    struct DirectoryEntry entry;

    structure = structure;
    
    if (!GetDirectory(handle, position, &entry))
    {
       return FAIL;
    }

    if (entry.attribute & FA_LABEL) return TRUE;
    if (IsLFNEntry(&entry))         return TRUE;
    if (IsDeletedLabel(entry))      return TRUE;
    if (IsCurrentDir(entry))        return TRUE;
    if (IsPreviousDir(entry))       return TRUE;

    firstcluster = GetFirstCluster(&entry);
    if (firstcluster == 0) return TRUE;
    
    if (!FileTraverseFat(handle, firstcluster, FactorCalculator,
                         (void**) &ppipe))
    {   
       return FAIL;
    }

    if (pipe.total)
    {
       sum = 100 - (int)(((unsigned long) pipe.consecutive * 100) / pipe.total);
    }
    else
    {
       sum = 0;
    }
    for (i = 0; i < 8; i++)
        printf("%c", entry.filename[i]);

    printf("  ");

    for (i = 0; i < 3; i++)
        printf("%c", entry.extension[i]);

    /* Print the attributes */
    printf(" ");
    if (entry.attribute & FA_ARCH)
       printf("A");
    else
       printf("-");

    if (entry.attribute & FA_RDONLY)
       printf("R");
    else
       printf("-");

    if (entry.attribute & FA_HIDDEN)
       printf("H");
    else
       printf("-");

    if (entry.attribute & FA_SYSTEM)
       printf("S");
    else
       printf("-");

    if (entry.attribute & FA_DIREC)
       printf("D");
    else
       printf("-");

    printf ("  (%d%%)\n", sum);

    return TRUE;
}

/************************************************************
***                  FactorCalculator
*************************************************************
*** Goes through the given file and counts the number of 
*** consecutive clusters.
*************************************************************/

static int FactorCalculator(RDWRHandle handle, CLUSTER cluster,
                            SECTOR sector, void** structure)
{
    struct PipeStruct *pipe = *((struct PipeStruct**) structure);
    CLUSTER now = DataSectorToCluster(handle, sector);
    if (!now) return FAIL;

    pipe->total++;

    if ((now == cluster - 1) || (FAT_LAST(cluster))) pipe->consecutive++;

    return TRUE;
}