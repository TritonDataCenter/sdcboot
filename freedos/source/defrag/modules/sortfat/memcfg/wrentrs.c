/*    
   Wrentrs.c - write entries to disk.

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
#include <stdio.h>
#include <string.h>

#include "fte.h"
#include "expected.h"

int WriteSortedEntries(RDWRHandle handle, 
                       struct DirectoryEntry* entries,
                       SECTOR* sectors,
                       int amofentries)
{
     int i;
     struct DirectoryEntry backup;
     struct DirectoryPosition pos;
     static int IsRestoring = FALSE;
     CLUSTER cluster;
     SECTOR  datastart;

     /* First update the drive map */
     datastart = GetDataAreaStart(handle);
     if (!datastart) RETURN_FTEERR(FALSE);

     for (i = 0; i < (amofentries / ENTRIESPERSECTOR) +
		      ((amofentries % ENTRIESPERSECTOR) > 0); i++)
     {
	 if (sectors[i] >= datastart)
	 {
	    cluster = DataSectorToCluster(handle, sectors[i]) ;
	    DrawOnDriveMap(cluster, WRITESYMBOL);
	 }
     }

     for (i = 0; i < amofentries; i++)
     {
         if ((i % ENTRIESPERSECTOR) == 0)
         {
            pos.sector = sectors[i / ENTRIESPERSECTOR];
	    pos.offset = 0;
	 }
         else
            pos.offset++;

         if (!IsRestoring)
         {
            if (!GetDirectory(handle, &pos, &backup))
            {  /* Try restoring everything to it's original state. */                    
               if (!IsRestoring)       /* Avoid going in infinite loop! */
               {
                  IsRestoring = TRUE; 
                  WriteSortedEntries(handle, entries, sectors, i-1);
                  IsRestoring = FALSE;
                  RETURN_FTEERR(FALSE);
               }
            }
	 }

	 if (!WriteDirectory(handle, &pos, &entries[i]))
         {  /* Try restoring everything to it's original state. */                    
            if (!IsRestoring)       /* Avoid going in infinite loop! */
            {
               IsRestoring = TRUE; 
               WriteSortedEntries(handle, entries, sectors, i);
               IsRestoring = FALSE;
               RETURN_FTEERR(FALSE);
            }
         }
         
         memcpy(&entries[i], &backup, sizeof(struct DirectoryEntry));
     }

     /* We have written everything. */
     for (i = 0; i < (amofentries / ENTRIESPERSECTOR) +
		      ((amofentries % ENTRIESPERSECTOR) > 0); i++)
     {
	 if (sectors[i] >= datastart)
	 {
	    cluster = DataSectorToCluster(handle, sectors[i]) ;
	    DrawOnDriveMap(cluster, USEDSYMBOL);
	 }
     }
     return TRUE;
}
