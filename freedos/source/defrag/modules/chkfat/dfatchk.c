/*    
   CheckFAT.c - check disk integrity module.

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

#include <string.h>

#include "fte.h"
#include "checkfat.h"

/*
   Notice that this assumes that the number of clusters is even.
   
   TODO: fix this.
*/
int MultipleFatCheck(RDWRHandle handle)
{
   unsigned short sectorsperfat = GetSectorsPerFat(handle);
   unsigned char  numberoffats  = GetNumberOfFats(handle);

   unsigned long bytesinfat = GetBytesInFat(handle), compared = 0, rest;

   SECTOR fat1start = GetFatStart(handle), fat2start = fat1start, i, j;

   char buffer1[BYTESPERSECTOR];
   char buffer2[BYTESPERSECTOR];

   if ((sectorsperfat == 0) || (numberoffats == 0) || (fat1start == 0) ||
       (bytesinfat == 0))
      RETURN_FTEERR(FALSE);                                  /* Not succeeded. */

   if (numberoffats == 1) return TRUE; /* Succeeded when only one fat? */

   rest = bytesinfat % BYTESPERSECTOR;
   for (i = 0; i < numberoffats - 1; i++)
   {
       fat2start += sectorsperfat;

       for (j = 0; j < sectorsperfat; j++)
       {
           ReadSectors(handle, 1, fat1start + j, buffer1);
           ReadSectors(handle, 1, fat2start + j, buffer2);

           if (compared == bytesinfat - rest)
           {
              if (memcmp(buffer1, buffer2, (int) rest) != 0)
                 RETURN_FTEERR(FALSE);

              compared = 0;
           }
           else
           {
              if (memcmp(buffer1, buffer2, BYTESPERSECTOR) != 0)
                 RETURN_FTEERR(FALSE);

              compared += BYTESPERSECTOR;
           }
       }
   }
   return TRUE;
}
