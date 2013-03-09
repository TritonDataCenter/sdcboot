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


#ifndef DRIVES_H
#define DRIVES_H

#include "dostypes.h"		/* Byte Word Dword */

struct DPB
{
  Byte drive, unit;		/* 0 is A: ... unit: for driver */
  Word bytepersec;		/* usually 512 */
  Byte maxsecinclust;		/* sectors per cluster - 1 */
  Byte shlclusttosec;		/* shl for cluster -> sector */
  Word numressec;		/* number of reserved sectors */
  /* 0x08 */
  Byte fats;			/* number of fats (1 or 2) */
  Word rootdirents;		/* 32by each */
  Word firstdatasec;		/* after root dir */
  Word maxclustnum;		/* if > 0xff6, fat16, else fat12  */
  /* num of data clusters + 1, as   */
  /* first data cluster is number 2 */
  Word secperfat;		/* sectors per fat - Byte for < DOS 4 */
  /* Word for Dos 4+, the mess starts   */
  /* 0x10 */
  /* < DOS 4: [0x0f] W startofrootdir, D devhdr, B mediaid...  */
  /* ... B used, D chain, (then Dos2: currdir Dos3: freeinfo)  */
  /*  DOS 4+: B ..., W startofrootdir, D devhdr, B mediaid ... */
  /* ... B used, D chain, (then freeinfo)                      */
  Byte other[64 + 2 + 4 + 6];
  /* Additions: not like in reality but ERIC LIKE */
  Dword FAT32_secperfat;
  Dword FAT32_firstdatasec;
  Dword FAT32_rootdirstart;
  Dword FAT32_clusters;
};

int getdrive (struct DPB *mydpb);
    /* int 21.19 -> get current drive, A is 0: ...       */
    /* int 21.1b -> al is secperclust, dx is numclust,   */
    /*              cx is bypersec, dsbx->media-id       */
    /* plus new int 21.32: returns for drive DL, being   */
    /* 0 for default and 1... for A..., a pointer DSBX,  */
    /* if al returned 0 for OK, to the DRIVE PARAM BLOCK */

void showdriveinfo (struct DPB *d);
int setdrive (int drive);	/* select another drive */
    /* int 21.0e.dl (A is 0...), returns lastdrive in AL */


/* int 21.3b.dsdx chdir allows a drive letter, but you have   */
/* to use int 21.... to go to the drive then */
/* int 21.47 get current dir does not return the "X:\\" part  */
/* int 21.47.dl(as 21.32).dssi -> ax=100/NC/dssi filled if ok */

/* TODO: start-sec of dir */
/* int 21.4exx.cx.dsdx: findfirst: cx is maximum flags, dsdx is */
/* name with maybe wildcards, returns attrib 40h if device...   */
/* also - if NC - returns data in DTA for findnext (varies...)  */
/* PLUS lots of other func return "sector of dir", but all seem */
/* to vary among DOS versions. Sigh... 21.1a sets DTA addr...   */

#endif
