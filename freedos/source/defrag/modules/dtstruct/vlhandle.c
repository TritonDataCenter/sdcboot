/*
   clmovmap.c - operations on the fixed cluster map.

   Copyright (C) 2003, Imre Leber.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have recieved a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be.

*/

#include <stdlib.h>

#include "fte.h"
#include "vlhandle.h"
#include "clmovmap.h"
#include "expected.h"
#include "custerr.h"

static RDWRHandle handle = (RDWRHandle) NULL;

BOOL InstallOptimizationDrive(char* drive)
{
   int fatlabelsize;
   
   if (handle)
      RejectOptimizationDrive();
   
#ifdef DEBUG_IMAGEFILE
   if (!InitReadWriteSectors("defrag.img", &handle)) 
      RETURN_FTEERR(FALSE);
#else      
   if (!InitReadWriteSectors(drive, &handle)) 
      RETURN_FTEERR(FALSE); 
#endif
  
   /* Create a few data structures */

   /* Since on FAT32, we only support Quick try methods we 
      do not need to create the fixed cluster map there */

   fatlabelsize = GetFatLabelSize(handle);
   if (fatlabelsize != FAT32)
   {
      if (!CreateFixedClusterMap(handle))
         RETURN_FTEERR(FALSE);
   }

   return TRUE;
}

void RejectOptimizationDrive(void)
{
   /* Destroy a few data structures */     
   DestroyFixedClusterMap();
   
   CloseReadWriteSectors(&handle);  
}

RDWRHandle GetCurrentVolumeHandle(void)
{
   if (!handle)
   {
       LogMessage("Error: handle not initialised");
       SetCustomError(HANDLENOTINITIALISED);
   }
      
   return handle;
}
