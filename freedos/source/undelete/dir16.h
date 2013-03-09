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


#ifndef DIR16_H
#define DIR16_H

#include <stdio.h>		/* printf */
#include "dostypes.h"		/* Byte Word Dword */
#include "drives.h"		/* struct DPB */

#define D_RO 1			/* readonly */
#define D_HIDE 2		/* hidden */
#define D_SYS 4			/* sys file */
#define D_VOL 8			/* volume label */
#define D_DIR 16		/* - subdirectory! - */
#define D_ARCH 32		/* for backup stuff. */
/* a windoze long dir entry would be RHSV: */
#define D_LONG 15		/* long file name */

#define D_DELCHAR 0xe5		/* deleted dir entries start like this */

typedef Word Date;
#define YofDate(d) ( ((d)>>9) + 1980 )
#define MofDate(d) ( ((d)>>5) & 15 )
#define DofDate(d) ( (d) & 31 )

typedef Word Time;
#define HofTime(t) ( (t)>>11 )
#define MofTime(t) ( ((t)>>5) & 63 )
#define SofTime(t) ( ((t) & 31) << 1 )	/* yes, weird but DOS */


/* FAT file system directory entry */
struct dirent
{
  Byte f_name[8];		/* Filename */
  Byte f_ext[3];		/* Filename extension */
  Byte f_attrib;		/* File Attribute, see D_... defines */
  Byte other1[2];		/* File case, ms time */
  Byte other2[6];		/* Words for Ctime and Cdate, Atime, */
  Word cluster_hi;		/* and hi word of cluster (fat32)    */
  Time f_time;			/* Time file created/updated */
  Date f_date;			/* Date file created/updated */
  Word cluster;			/* Starting cluster (min 2) */
  Dword f_size;			/* File size in bytes */
};

enum InteractiveMode
{ IM_NORMAL, IM_LIST, IM_ALL };

void printdirents (Byte * buffer, struct DPB *dpb);
int interactivedirents (enum InteractiveMode im,
			const char *directory,
			int drive,
			Dword sector, Byte * buffer, struct DPB *dpb,
			const char *externalDest);
void appendSlashIfNeeded (char *directory);
void
Follow (Word cluster, const char *filename,
	const char *fileext, const char *directory, Dword size);

#endif
