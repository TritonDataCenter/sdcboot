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


#include "fatio.h"
#include <stdio.h>		/* printf */

Byte fatbuf1[1024];
Byte fatbuf2[1024];
Dword lastfatbuf = 0xffffffffL;	/* number of sector in buffer */


int
writefat (int drive, Word slot, Word value, struct DPB *dpb)
{
  Word oldvalue;
  Dword offs;			/* intermediate values have 17 or 18 bits */
  Dword fatsector = dpb->numressec;	/* skip reserved */

  readfat (drive, slot, &oldvalue, dpb);

  if (dpb->maxclustnum > 0xff6)
    {				/* fat16 */
      offs = slot;
      offs <<= 1;
    }
  else
    {				/* fat12 */
      offs = slot;
      offs = ((offs << 1) + offs) >> 1;	/* 1.5by/slot! */
    }
  fatsector += offs >> 9;	/* assume 512 by per sector */
  offs &= 511;			/* may be 511 for FAT12, sigh */

  if (dpb->maxclustnum > 0xff6)
    {				/* fat16 */
      fatbuf1[(Word) offs + 1] = (value >> 8) & 0xFF;
      fatbuf1[(Word) offs] = value & 0xFF;
    }
  else
    {				/* fat12 */
      if ((slot & 1) == 0)
	{			/* even */
	  fatbuf1[(Word) offs + 1] |= (value >> 8) & 0x0F;
	  fatbuf1[(Word) offs] = value & 0xFF;
	}
      else
	{			/* odd */
	  fatbuf1[(Word) offs + 1] = (value >> 4) & 0xFF;
	  fatbuf1[(Word) offs] |= (value << 4) & 0xF0;
	}
    }

  fatbuf2[(Word) offs + 1] = fatbuf1[(Word) offs + 1];
  fatbuf2[(Word) offs] = fatbuf1[(Word) offs];


  if (writesector (drive, fatsector, fatbuf1) < 0)
    {
      printf ("Error writing FAT");
      return -1;
    }
  if (writesector (drive, fatsector + 1, fatbuf1 + 512) < 0)
    {
      printf ("Error writing FAT");
      return -1;
    }
  if (dpb->fats > 1)
    {
      if (writesector (drive, fatsector + dpb->secperfat, fatbuf2) < 0)
	{
	  printf ("Error writing FAT");
	  return -1;
	}
      if (writesector (drive, fatsector + dpb->secperfat + 1,
		       fatbuf2 + 512) < 0)
	{
	  printf ("Error writing FAT");
	  return -1;
	}
    }

  return 0;
}

int
readfat (int drive, Word slot, Word * value, struct DPB *dpb)
{
  Word val1;
  Word val2;
  Dword offs;			/* intermediate values have 17 or 18 bits */
  Dword fatsector = dpb->numressec;	/* skip reserved */
  value[0] = 0;

  if (dpb->maxclustnum > 0xff6)
    {				/* fat16 */
      offs = slot;
      offs <<= 1;
    }
  else
    {				/* fat12 */
      offs = slot;
      offs = ((offs << 1) + offs) >> 1;	/* 1.5by/slot! */
    }
  fatsector += offs >> 9;	/* assume 512 by per sector */
  offs &= 511;			/* may be 511 for FAT12, sigh */

  if (fatsector != lastfatbuf)
    {
      if (readsector (drive, fatsector, fatbuf1) < 0)
	{
	  return -1;
	}
      if (readsector (drive, fatsector + 1, fatbuf1 + 512) < 0)
	{
	  return -1;
	}
      if (dpb->fats > 1)
	{
	  if (readsector (drive, fatsector + dpb->secperfat, fatbuf2) < 0)
	    {
	      return -1;
	    }
	  if (readsector (drive, fatsector + dpb->secperfat + 1,
			  fatbuf2 + 512) < 0)
	    {
	      return -1;
	    }
	}
      lastfatbuf = fatsector;	/* buffer this */
    }				/* else already in buffer */

  if (dpb->maxclustnum > 0xff6)
    {				/* fat16 */
      val1 = fatbuf1[(Word) offs + 1];
      val1 <<= 8;
      val1 += fatbuf1[(Word) offs];
      val2 = fatbuf2[(Word) offs + 1];
      val2 <<= 8;
      val2 += fatbuf2[(Word) offs];
    }
  else
    {				/* fat12 */
      if ((slot & 1) == 0)
	{
	  val1 = fatbuf1[(Word) offs + 1] & 15;
	  val1 <<= 8;
	  val1 += fatbuf1[(Word) offs];
	  val2 = fatbuf2[(Word) offs + 1] & 15;
	  val2 <<= 8;
	  val2 += fatbuf2[(Word) offs];
	}
      else
	{
	  val1 = fatbuf1[(Word) offs + 1];
	  val1 <<= 4;
	  val1 |= fatbuf1[(Word) offs] >> 4;
	  val2 = fatbuf2[(Word) offs + 1];
	  val2 <<= 4;
	  val2 |= fatbuf2[(Word) offs] >> 4;
	}
    }

  value[0] = val1;

  if (dpb->fats < 2)
    {
      val2 = val1;
    }
  if (val1 != val2)
    {
      printf ("FAT inconsistency: #1 %u or #2 %u ???\n", val1, val2);
      printf ("always using FAT1 in this case!\n");
      printf ("Slot: %u  offset: %lu  sectors: %lu/%lu\n",
	      slot, offs, fatsector, fatsector + dpb->secperfat);
    }

  return 0;
}

/* fat12 packs 3, 4, 5, 6 as bytes 3,40,0, 5,60,0, ... */
