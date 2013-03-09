/*    
   Defrpars.c - remember defragmentation parameters.

   Copyright (C) 2000 Imre Leber

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

#include <ctype.h>

#include "fte.h"

#include "..\modules\dtstruct\vlhandle.h"

/* Drive to optimize. */
static char DriveToOptimize = -1;

/* Sort parameters. */
static int SortCriterium = -1;
static int SortOrder = -1;

/* Optimization method. */
static int OptimizationMethod = -1;

int SetOptimizationDrive(char drive)
{
     char drv[3];

     drv[0] = drive;
     drv[1] = ':';
     drv[2] = '\0';
        
     DriveToOptimize = toupper(drive);
     
     return InstallOptimizationDrive(drv);
}

char GetOptimizationDrive(void)
{
     return DriveToOptimize;
}

void SetSortOptions(int criterium, int order)
{
     SortCriterium = criterium;
     SortOrder     = order;
}

int GetSortCriterium (void)
{
    return SortCriterium;
}

int GetSortOrder (void)
{
    return SortOrder;
}

void SetOptimizationMethod(int method)
{
    OptimizationMethod = method;
}

int GetOptimizationMethod(void)
{
    return OptimizationMethod;
}