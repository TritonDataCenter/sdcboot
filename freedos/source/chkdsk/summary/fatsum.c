/*
   fatsum.c - fat summary gattering.
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

#include <string.h>

#include "fte.h"

#include "fatsum.h"

static BOOL FATSummaryGetter(RDWRHandle handle,
                             CLUSTER label, SECTOR datasector,
                             void** info);

BOOL GetFATSummary(RDWRHandle handle, struct FatSummary* info)
{
    info->totalnumberofclusters = 0;
    info->numoffreeclusters     = 0;
    info->numofbadclusters      = 0;

    return LinearTraverseFat(handle, FATSummaryGetter, (void**)&info);
}

static BOOL FATSummaryGetter(RDWRHandle handle,
                             CLUSTER label, SECTOR datasector,
                             void** structure)
{
    struct FatSummary **info = (struct FatSummary**) structure;

    datasector = datasector;
    handle     = handle;

    (*info)->totalnumberofclusters++;

    if (FAT_FREE(label))
       (*info)->numoffreeclusters++;
    if (FAT_BAD(label))  (*info)->numofbadclusters++;

    return TRUE;
}
