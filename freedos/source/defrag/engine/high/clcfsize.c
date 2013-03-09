/*    
   ClcFSize.c - calculate file size (in clusters).

   Copyright (C) 2003 Imre Leber

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

static BOOL FileSizeCalculator(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure);

/*******************************************************************
**                        CalculateFileSize
********************************************************************
** Calculates the size of a file in clusters
********************************************************************/

unsigned long CalculateFileSize(RDWRHandle handle, CLUSTER firstcluster)                              
{
   unsigned long result=0, *presult = &result;

   if (!FileTraverseFat(handle, firstcluster, FileSizeCalculator,
                        (void**) &presult))
      RETURN_FTEERROR(FAIL);

   return result;
}

static BOOL FileSizeCalculator(RDWRHandle handle, CLUSTER label,
                               SECTOR datasector, void** structure)
{
   unsigned long* presult = *((unsigned long**) structure);

   structure = structure,
   label = label,
   handle = handle,
   datasector=datasector;

   (*presult)++;

   return TRUE;
}
