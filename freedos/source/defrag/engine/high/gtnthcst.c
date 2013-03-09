/*    
   GetNthCluster.c - Function to return the nth cluster in a volume
                     regardless of which kind of FAT it contains.

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

#include "fte.h"

/**************************************************************
**                    GetNthCluster
***************************************************************
** Reads a cluster from the FAT and interprets according to the
** the type of FAT used.
***************************************************************/ 

BOOL GetNthCluster(RDWRHandle handle, CLUSTER n, CLUSTER* label)
{
   CLUSTER result;
   int fatlabelsize = GetFatLabelSize(handle);
   
   if (!ReadFatLabel(handle, n, &result))
      RETURN_FTEERROR(FALSE);
      
   switch (fatlabelsize)
   {
      case FAT12:
           if (FAT12_FREE(result))   
           {
              *label = FAT_FREE_LABEL;
              return TRUE;
           }
           if (FAT12_BAD(result))
           {
              *label = FAT_BAD_LABEL;
              return TRUE;
           }
           if (FAT12_LAST(result))
           {
              *label = FAT_LAST_LABEL;
              return TRUE;
           }
           break;
           
      case FAT16:
           if (FAT16_FREE(result))   
           {
              *label = FAT_FREE_LABEL;      
              return TRUE;
           }
           if (FAT16_BAD(result))
           {
              *label = FAT_BAD_LABEL;
              return TRUE;
           }
           if (FAT16_LAST(result))
           {
              *label = FAT_LAST_LABEL;             
              return TRUE;
           }
           break;
              
      case FAT32:
           if (FAT32_FREE(result))   
           {
              *label = FAT_FREE_LABEL;      
              return TRUE;
           }
           if (FAT32_BAD(result))
           {
              *label = FAT_BAD_LABEL;
              return TRUE;
           }
           if (FAT32_LAST(result))
           {
              *label = FAT_LAST_LABEL;             
              return TRUE;
           } 
           break; 
   }

   *label = result;
   return TRUE;
}
