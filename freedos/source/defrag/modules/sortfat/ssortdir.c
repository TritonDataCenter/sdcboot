/*    
   ssortdir.c - performs a selection sort on the given directory.

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
#include <stdlib.h>

#include "fte.h"
#include "sortcfgf.h"
#include "expected.h"

/*
** Generic routine to sort entries in a directory.
**
** Different implementatiotions should give the possibility 
** to both make use of all available memory and still allow
** for VERY low (< 8Kb) memory conditions.
**
** Returns FALSE if not been able to sort, TRUE otherwise.
**
** The reasons why I use a selection sort is:
** - It is a simple algorithm, so less can go wrong.
** - When assuming that we use a write-through cache, the time
**   to sort is mostly determined by the number of writes. With
**   selection sort the number of writes is at most 2*n (with n
**   the number of entries in the directory).
*/

/* First see if we should be sorting at all, we should not be sorting
   if this directory contains:
   - system files not all located at the start of the disk.
   - entries of different length.
*/
static int DirectoryMaybeSorted(void* entries, int totalentries)
{
   int systemfilefound = FALSE, nonsystemfilefound=FALSE, differentlength = FALSE;
   unsigned previouslength=0, currentlength, i;
   struct DirectoryEntry* entry = (struct DirectoryEntry*)
				     malloc(sizeof(struct DirectoryEntry));
   if (!entry) return -1;

   for (i = 0; i < totalentries; i++)
   {
       /* Count the number of slots in this entry. */
       for (currentlength = 0; currentlength < totalentries; currentlength++)
       {
	   if (!GetSlotToSort(entries, i, currentlength, entry))
	   {
	      free(entry);
	      return -1;
	   }

	   if (!IsLFNEntry(entry))
	      break;
       }
       if (entry->attribute & FA_SYSTEM)
       {
	  if (nonsystemfilefound)
	  {
	      systemfilefound = TRUE;
	  }
       }
       else
       {
	  nonsystemfilefound = TRUE;
       }

       if (i == 0)
       {
	  previouslength = currentlength;
       }
       else if (previouslength != currentlength)
       {
	  differentlength = TRUE;
       }
   }

   free(entry);
   return !(differentlength && systemfilefound);
}


static int SkipLFNs(void* entries,
		    int entry, int totalentries,
		    struct DirectoryEntry* result)
{
    unsigned j;

    for (j=0; j < totalentries; j++)
    {
      if (!GetSlotToSort(entries, entry, j, result))
         RETURN_FTEERR(FALSE);

      if (!IsLFNEntry(result))
         break;
    }
    return TRUE;
}

int SelectionSortEntries(void* entries, int amountofentries)
{
    int i, j, pos;
    struct DirectoryEntry seeker, min;

    switch (DirectoryMaybeSorted(entries, amountofentries))
    {
       case -1:
	    RETURN_FTEERR(FALSE);
       case FALSE:
	    return TRUE;
    }

    for (i = 0; i < amountofentries; i++)
    {
	pos = i;

	if (!GetEntryToSort(entries, i, &min))
	   RETURN_FTEERR(FALSE);
	if (!SkipLFNs(entries, i, amountofentries, &min))
	   RETURN_FTEERR(FALSE);

	if ((min.attribute & FA_SYSTEM) == 0)
	{
	   if ((i % 2) == 0)                       /* Cache optimization */
	   {
	      for (j = i; j < amountofentries; j++)
	      {
		  /* Update the interface */
		  //UpdateInterfaceState();

		  if (!GetEntryToSort(entries, j, &seeker))
		     RETURN_FTEERR(FALSE);
		  if (!SkipLFNs(entries, j, amountofentries, &seeker))
		     RETURN_FTEERR(FALSE);

		  if ((seeker.attribute & FA_SYSTEM) == 0)
		  {
		     if (FilterSortResult(CompareEntriesToSort(&seeker, &min))
			 == -1)
		     {
			pos = j;
			memcpy(&min, &seeker, sizeof(struct DirectoryEntry));
		     }
		  }
	      }
	   }
	   else
	   {
	      for (j = amountofentries-1; j >= i; j--)
	      {
		  /* Update the interface */
		  //UpdateInterfaceState();


		  if (!GetEntryToSort(entries, j, &seeker))
		     RETURN_FTEERR(FALSE);
		  if (!SkipLFNs(entries, j, amountofentries, &seeker))
		     RETURN_FTEERR(FALSE);

		  if ((seeker.attribute & FA_SYSTEM) == 0)
		  {
		     if (FilterSortResult(CompareEntriesToSort(&seeker, &min))
			== -1)
		     {
			pos = j;
			memcpy(&min, &seeker, sizeof(struct DirectoryEntry));
		     }
		  }
	      }
	   }

	   if (pos != i)
	      if (!SwapEntryForSort(entries, i, pos))     /* 2n writes */
		 RETURN_FTEERR(FALSE);
	}
    }
    return TRUE;
}
