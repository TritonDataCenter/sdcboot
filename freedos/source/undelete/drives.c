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

#include "drives.h"
#include "stdio.h"		/* printf... */
#include "diskio.h"		/* we are now (11/2002) using readsector! */

#ifdef __MSDOS__		/* Turbo C thing */
#define DOS
#endif

#ifdef DOS
#include "dos.h"		/*  getfatd... */
#include "dir.h"		/* getdisk, setdisk */
/* We use both the boot sector and getfatd() of Turbo C here */
#endif

void
showdriveinfo (struct DPB *d)
{
  if (d == (struct DPB *) 0)
    return;

  printf ("Drive %c: info: fats=%u maxsecinclust=%u",
	  d->drive + 'A', d->fats, d->maxsecinclust);
  printf (" numressec=%u\nshlclusttosec=%u\n",
	  d->numressec, d->shlclusttosec);
  /* SHLCLUSTTOSEC is written on its own line for easy parsing */
  /* it is the only really important thing for calcluating the */
  /* FOLLOW values from DIRSAVE output (file recovering)...    */
  printf ("rootdirents=%u firstdatasec=%u", d->rootdirents, d->firstdatasec);
  printf (" maxclustnum=%u, secperfat=%u\n", d->maxclustnum, d->secperfat);

  if (d->FAT32_rootdirstart != 0)
    {
      printf ("FAT32 info: secperfat=%lu, firstdatasec=%lu,\n",
	      d->FAT32_secperfat, d->FAT32_firstdatasec);
      printf ("FAT32 root dir starts in CLUSTER %lu\n",
	      d->FAT32_rootdirstart);
      printf ("FAT32 volume has %lu clusters\n", d->FAT32_clusters);
    }
  printf ("\n");
}

int
getdrive (struct DPB *mydpb)
{
  struct DPB *dPB = mydpb;	/* see RBIL 61, tables 01395 and */

  /* 01787 (extended DPB, Win95/FAT32)... */
  /* [xdpb+0x29]d is "firstdatasec", [xdpb+0x2d]d is "maxclustnum", */
  /* [xdpb+0x31]d is "secUSEDbyFAT", [xdpb+0x35]d is "rootdisclust" */

  /* Changed 9 Nov 2002 to unify DOS and UNIX versions */
  /* Old version used fixed 1.44 MB geometry for UNIX  */

  /* in DPB, secperfat is WORD for >= DOS 4 but BYTE otherwise */

  Dword x;			/* sigh... must be 17bits, bad luck for DOS */
  /* now that we have it, we use 32bits :-)   */
  Word w;

  /* parts of struct fatinfo dtab... no longer used anymore: */
  /* Word fi_sclus; sectors per cluster - originally a char  */
  /* Word fi_nclus; clusters on drive (int) */
  /* Word fi_bysec; bytes per sector (int)  */

  /* with DOS, we could getfatd(&dtab) with a struct fatinfo dtab */
  /* and get char fi_sclus, char fi_fatid, int fi_nclus and */
  /* int fi_bysec - especially fi_nclus is a nice service */
  /* (sec/clust, id, clusters, by/sec) */

  Byte bootsec[512];		/* UNSIGNED! */

  if (dPB == ((struct DPB *) 0))
    {
      return -1;		/* very cautious :-) */
    }

#ifdef INITwithDEFAULT
  dPB->drive = 0;
  dPB->unit = 0;
  dPB->bytepersec = 512;
  dPB->maxsecinclust = 0;
  dPB->shlclusttosec = 0;
  dPB->numressec = 1;		/* (the boot sector) */
  dPB->fats = 2;
  dPB->rootdirents = 224;	/* 0xe0: 14sec */
  dPB->firstdatasec = 33;	/* 0x21: 1+9+9+14 */
  dPB->maxclustnum = 2848;	/* 0xb20<0xff6, thus FAT12 */
  dPB->secperfat = 9;		/* about 8.34 used */
  /* and root dir starts at sector 0x13 = 19 = 1+9+9 */
#endif

  dPB->FAT32_secperfat = 0L;
  dPB->FAT32_firstdatasec = 0L;
  dPB->FAT32_rootdirstart = 0L;
  dPB->FAT32_clusters = 0L;

  dPB->unit = 0;

  dPB->drive =
#ifndef DOS
    0;				/* we only sim a: */
#endif
#ifdef DOS
  getdisk ();
#endif

  if (readsector (dPB->drive, 0, bootsec) < 0)
    {
      printf ("Could not read boot sector!\n");
      dPB->numressec = 1;
      dPB->fats = 2;
      dPB->rootdirents = 224;
      return -1;
    }

#ifdef DEBUGDUMP
  for (w = 0; w < 0x30; w++)
    {
      if ((w & 15) == 0)
	printf ("BOOTSECTOR %4.4x: ", w);
      printf ("%2.2hx ", bootsec[w]);
      if ((w & 15) == 15)
	printf ("\n");
    }
  printf ("\n");
#endif

  /* Boot sector: [0b]w by/sec  [0d]b sec/clust  [0e]w res.sec. */
  /* [10]b numfats  [11]w rootdirents  [13]w totalsecs/0  [15]b */
  /* media descr.  [16]w sec/fat/0 [18]w sec/track [1a]w heads  */
  /* [1c]d hidden  [20]d totalsecs-long                         */
  /* so we have a "BPB" at bootsec[0x0b], see RBIL table 01663. */

  /* before DOS 3.0, [1c] was a word                            */
  /* ONLY DOS 4.0, [2a]w is cylinders, [2c]b type, [2d]b attrib */
  /* [20]d is only used by >= DOS 4.0/4.01 (>32MB "big FAT16"). */
  /* FAT32 (Win95): [24]d is sec/fat (if [16]w is 0),           */
  /* [26]w flags: bit 7 "do not copy active to other fats", bit */
  /* 0..3 number of active fat, [28]w file system version, 0.0  */
  /* for Win95-OSR2, [2a]d STARTING CLUSTER OF ROOT DIRECTORY   */
  /* [2c]w "fsinfo sector number or -1" [2e]w "backup boot sec  */
  /* number or -1" [30]..[3a] reserved...                       */

  w = bootsec[0x0c];
  w <<= 8;
  w |= bootsec[0x0b];		/* byte/sector */
  dPB->bytepersec = w;
  /* fi_bysec = w; *** unused */
  if (w != 512)
    {
      printf ("Error: %dby/sec, not 512\n", w);
      return -1;
    }

  w = bootsec[0x0d];		/* sectors per cluster */
  if (w == 0)
    w = 256;			/* special, FreeDOS */
  /* fi_sclus = w; *** unused *** Turbo C fi_sclus is only char! */
  dPB->maxsecinclust = w - 1;
  dPB->shlclusttosec = 0;
  while (w > 1)
    {
      dPB->shlclusttosec++;
      w >>= 1;
    }

  w = bootsec[0x0f];
  w <<= 8;
  w |= bootsec[0x0e];		/* reserved sectors */
  /* normally 1 (the boot sector itself) */
  if (w != 1)
    {
      printf ("Warning: %u, not 1 reserved sector\n", w);
    }
  dPB->numressec = w;

  w = bootsec[0x10];		/* number of FAT copyies */
  if ((w < 1) || (w > 2))
    {
      printf ("Warning: %d, not 1 or 2 FAT copies\n", w);
    }
  dPB->fats = w;

  w = bootsec[0x12];
  w <<= 8;
  w |= bootsec[0x11];
  dPB->rootdirents = w;		/* root directory entries */

  x = bootsec[0x17];
  x <<= 8;
  x |= bootsec[0x16];
  dPB->secperfat = x;		/* secperfat: bootsectors value */
  /* Maybe we should &= 255 if DOS version is < 4 here? */

  if (x == 0)
    {
      x = bootsec[0x27];
      x <<= 8;
      x |= bootsec[0x26];
      x <<= 8;
      x |= bootsec[0x25];
      x <<= 8;
      x |= bootsec[0x24];
      printf ("Error: FAT32 not yet supported!\n");
      printf ("*** Sectors per FAT: %lu ***\n", x);
      dPB->FAT32_secperfat = x;
      dPB->FAT32_firstdatasec = (x * dPB->fats) + dPB->numressec;
      printf ("*** First CLUSTER starts on sector: %lu ***\n",
	      dPB->FAT32_firstdatasec);
      x = bootsec[0x2d];
      x <<= 8;
      x |= bootsec[0x2c];
      x <<= 8;
      x |= bootsec[0x2b];
      x <<= 8;
      x |= bootsec[0x2a];
      printf ("*** ROOT directory starts on CLUSTER: %lu ***\n", x);
      dPB->FAT32_rootdirstart = x;
      w = bootsec[0x29];
      w <<= 8;
      w |= bootsec[0x28];
      printf ("Filesystem version (0 for Win95): %u\n", w);
      w = bootsec[0x26];
      if ((w & 0x80) != 0)
	{
	  printf ("Mirroring off, only active FAT (1..16) is %d", w & 15);
	}
      if (dPB->rootdirents != 0)
	{
	  printf ("Expecting 0 FAT16 root directory entries, found %u\n",
		  dPB->rootdirents);
	}
      dPB->rootdirents = 0;
      /* would be an idea to honour "activefat" in dPB->... as well */
    }				/* END "FAT32 detected" */

  /* IMPORTANT: POSITION OF FIRST DATA SECTOR */
  /* Overridden by FAT32_ values if FAT32_rootdirstart is in use  */
  /* (in that case, dPB->secperfat will be 0 and                  */
  /* dPB->FAT32_secperfat and dPB->FAT32_firstdatasec are used... */

  dPB->firstdatasec = dPB->numressec + (dPB->fats * dPB->secperfat);
  dPB->firstdatasec += (dPB->rootdirents + 15) >> 4;

  x = bootsec[0x14];
  x <<= 8;
  x |= bootsec[0x13];
  if (x == 0)
    {				/* total size of disk/partition, 0 for 32bit */
      x = bootsec[0x23];
      x <<= 8;
      x |= bootsec[0x22];
      x <<= 8;
      x |= bootsec[0x21];
      x <<= 8;
      x |= bootsec[0x20];
      /* total partition size, 32 bit value!!! */
      /* (and also valid for FAT32 partitions) */
    }

  x -= (dPB->secperfat != 0) ? dPB->firstdatasec : dPB->FAT32_firstdatasec;
  /* subtract non-cluster area */
  /* now divide by cluster size */
  x >>= dPB->shlclusttosec;	/* /= fi_sclus (problematic!) */
  /* or better: shift by cluster shift */
  if (x > 65534L)
    {
      printf ("FAT32 number of clusters: %lu\n", x);
      dPB->FAT32_clusters = x;
      x = 65534L;
      if (dPB->secperfat != 0)
	{
	  printf ("Error: FAT32ish size but no FAT32 magic!\n");
	}
      dPB->secperfat = 0;	/* add FAT32 magic */
    }

  /* dtab.fi_nclus = x; *** unused *** number of clusters */
  /* dtab.fi_nclus is definitely easier to get through getfatd() */

  x = x + 2 - 1;		/* max cluster number (start: 2) */
  dPB->maxclustnum = x;

  if (dPB->secperfat != 0)
    {
      /* re-calculate sec per fat and check sanity for FAT12/FAT16: */
      /* uses up to 17 bit long intermediate values... */
      /* using >> 9 because we only allow 512 byte per sec anyway! */
      if (x < 0xff6)
	{			/* FAT12: 1.5by/entry */
	  x /* secperfat */  = ((dPB->bytepersec - 1) + (((x << 1) + x) >> 1))
	    >> 9;		/* / dPB->bytepersec */
	}
      else
	{
	  x /* secperfat */  = ((dPB->bytepersec - 1) + (x << 1))
	    >> 9;		/* / dPB->bytepersec */
	};
      if (dPB->secperfat != x /* secperfat */ )
	{
	  printf ("Sec per FAT not as predicted??? ");
	  printf ("Predicted %lu, but boot says %u\n", x, dPB->secperfat);
	  return -1;		/* re-calc "firstdatasec" or better, wimp out! */
	  /* reason: changed "maxclustnum" and "fi_nclus" */
	  /* which may in turn change "secperfat" again... */
	}
    }				/* END "secperfat sanity check for FAT12/FAT16" */

  if (dPB->secperfat == 0)
    {
      printf ("Stopping here - FAT32 not yet supported\n");
      return -1;
    }

  return (dPB->drive);		/* 0 is a:, and so on */
}


int
setdrive (int drive)		/* select another drive */
{
#ifndef DOS
  if (drive != 0)
    {
      return -1;
    };
  return 0;			/* lastdrive is A: */
#endif
#ifdef DOS
  return setdisk (drive) - 1;
#endif
}
