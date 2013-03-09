/*
  FreeDOS special UNDELETE tool (and mirror, kind of)
  Copyright (C) 2001, 2002  Eric Auer <eric@CoLi.Uni-SB.DE>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, 
  USA. Or check http://www.gnu.org/licenses/gpl.txt on the internet.
*/


#include "drives.h"		/* DPB structure */
#include "fatio.h"		/* readfat */
#include <stdio.h>		/* printf */

#define CTS clustertosector
Dword
clustertosector (Word cluster, struct DPB *dpb)
{
  Dword sector;
  if ((cluster < 2) || (cluster > dpb->maxclustnum))
    {
      printf ("Cluster number %u out of range [2..%u]\n",
	      cluster, dpb->maxclustnum);
      return 0;
    }
  sector = cluster - 2;		/* make it 0 based */
  sector <<= dpb->shlclusttosec;
  /*
     sector += dpb->numressec + (dpb->secperfat * dpb->fats);
     sector += (dpb->rootdirents >> 4); 
 *//* rootdirents>>4 because of 512/32 being 1<<4... */
  sector += dpb->firstdatasec;
  return sector;
}

/* ***************************************************** */
/* Screen output of nextcluster has been modified for new interface,
   Less info but less scary for innocents - RP */
Word
nextcluster (Word cluster, struct DPB * dpb)
{				/* follow FAT chain or guess! */
  Word oldcluster = cluster;
  int state;

/*  printf ("%d->", oldcluster); */
  state = readfat (dpb->drive, oldcluster, &cluster, dpb);
  if (state < 0)
    {
      printf ("FAT read error\n");
      return 0;
    }

  if (cluster > dpb->maxclustnum)
    {
      printf ("EOF\n");
      return 0;
    }
  if (cluster == 0)
    {
      printf (".");
/*      printf ("NIL/");  *//* if we come from empty... (undelete) */
      do
	{
	  oldcluster++;
	  state = readfat (dpb->drive, oldcluster, &cluster, dpb);
	  if (state < 0)
	    {
	      printf ("FAT read error\n");
	      return 0;
	    }
	  if (cluster != 0)
	    {
	      printf ("-");
	    }
	}
      while (cluster != 0);	/* skip over used clusters */
      cluster = oldcluster;	/* return next empty cluster! */
    }				/* else stay inside FAT chain */

  /* printf("%d "); */
  return cluster;
}
