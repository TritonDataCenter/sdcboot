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


#include "dostypes.h"		/* Byte Word Dword */
#include "cluster.h"		/* nextcluster, CTS */
#include "fatio.h"		/* readfat(drive, slot, * value) */
#include "diskio.h"		/* readsector(drive, sector, * buffer) */
#include "dir16.h"		/* struct dirent */
#include <ctype.h>		/* toupper */
#include <stdio.h>		/* printf */

#include "dirfind.h"

/* #define DIRFINDDEBUG 1 */

/* looks in the dir at cluster (0 for root) for the dir */
/* named name and returns a new value for cluster... */

int
dirfind1 (char *name, Word * cluster, struct DPB *dpb)
{
  Byte buffer[512];
  struct dirent dent;
  struct dirent *srch;
  Dword sector;
  int i, j, k, l, secinclust;
  Word myclust = cluster[0];

#ifdef DIRFINDDEBUG
  printf ("dirfind1(<%s>,<%u>,<dpb>)\n", name, cluster[0]);
#endif

  cluster[0] = 0xffff;		/* default is failure */
  if (myclust > dpb->maxclustnum)
    {
      printf ("Illegal initial cluster for dirfind1 %s: %u\n", name, myclust);
      return -1;
    }
  secinclust = 0;

  for (i = 0; i < 8; i++)
    {
      if ((name[0] != 0) && (name[0] != '.'))
	{
	  dent.f_name[i] = toupper (name[0]);
	  name++;
	}
      else
	{
	  dent.f_name[i] = ' ';	/* DOS pads with spaces */
	}
    }
  if (name[0] == '.')
    {
      name++;			/* skip over . marking the extension */
      for (i = 0; i < 3; i++)
	{
	  if (name[0])
	    {
	      dent.f_ext[i] = toupper (name[0]);
	      name++;
	    }
	  else
	    {
	      dent.f_ext[i] = ' ';	/* DOS pads with spaces */
	    }
	}
    }
  else
    {
      dent.f_ext[0] = ' ';
      dent.f_ext[1] = ' ';
      dent.f_ext[2] = ' ';
    }

  if (dent.f_name[0] == '?')
    {
      dent.f_name[0] = D_DELCHAR;	/* wildcard for del */
    }

  if (myclust == 0)
    {
      sector = dpb->numressec + (dpb->fats * dpb->secperfat);
    }
  else
    {
      sector = CTS (myclust, dpb);	/* sigh... */
    }

  for (i = 0;; i++)
    {				/* both for existing and deleted dirs */
      j = readsector (dpb->drive, sector, buffer);
      if (j < 0)
	{
	  printf ("Error reading sector %lu\n", sector);
	  return -1;
	}

      for (j = 0; j < 512; j += 32)
	{			/* scan one sector */
	  srch = (struct dirent *) &buffer[j];
	  if (srch->f_attrib & D_DIR)
	    {
	      l = 0;
	      for (k = 0; k < 8; k++)
		{
		  if (toupper (srch->f_name[k]) == dent.f_name[k])
		    {
		      l++;
		    }
		}
	      for (k = 0; k < 3; k++)
		{
		  if (toupper (srch->f_ext[k]) == dent.f_ext[k])
		    {
		      l++;
		    }
		}
	      if (l == (8 + 3))
		{
		  dent.f_attrib = 0;	/* evil string term */
		  cluster[0] = srch->cluster;
		  if (srch->cluster_hi)
		    {
		      printf ("Warning: ignored cluster_hi: %u\n",
			      srch->cluster_hi);
		    }
#ifdef DIRFINDDEBUG
		  printf ("Directory <%s> is at %u\n",
			  (char *) &dent.f_name[0], cluster[0]);
#endif
		  return 0;
		}		/* eof found */
	    }
	}			/* eof scan one sector */

      if ((buffer[512 - 32] == 0) && (buffer[512 - 64] == 0))
	{
	  dent.f_attrib = 0;	/* evil string term */
	  printf ("Seems to be EOF, but directory <%s> not found\n",
		  (char *) &dent.f_name[0]);
	  return -1;
	}

      if (sector < CTS (2, dpb))
	{			/* root directory is linear */
	  sector++;
	}
      else
	{
	  sector++;		/* clusters are linear inside */
	  secinclust++;
	  if (secinclust == dpb->maxsecinclust)
	    {			/* follow FAT chain */
	      myclust = nextcluster (myclust, dpb);
	      if (myclust == 0)
		{
		  dent.f_attrib = 0;	/* evil string term */
		  printf ("FAT chain EOF, but directory <%s> not found\n",
			  (char *) &dent.f_name[0]);
		  return -1;
		}
	      sector = CTS (myclust, dpb);	/* back on track */
	    }
	}			/* new sector determined */

    }				/* loop over sectors */

/* return -1; *//* not reached */
}


/* finds a dir, walks along the tree */

int
dirfind (char *name, Word * cluster, struct DPB *dpb)
{
  Word mycluster = 0;		/* start with root dir */
  char subname[8 + 1 + 3 + 1];	/* one part */
  int i;

  cluster[0] = 0xffff;		/* default is failure */
  /* Must be a path from root */
  /* However could technically be relative path from root - modified RP */
  if ((name[0] == '\\') || (name[0] == '/'))
    name++;

  while (name[0] != 0)
    {
      for (i = 0; (i < (8 + 1 + 3)) && (name[0] != '/') &&
	   (name[0] != '\\') && (name[0] != 0); i++)
	{
	  subname[i] = name[0];
	  name++;
	  subname[i + 1] = 0;
	}
      if ((name[0] == '\\') || (name[0] == '/'))
	{
	  name++;		/* skip over \\ or / as level separators */
	}
      if (dirfind1 (subname, &mycluster, dpb) < 0)
	{
	  printf ("Could not find subdirectory ...\\[%s]\\%s\n",
		  subname, name);
	  return -1;
	}
#ifdef DIRFINDDEBUG
      printf ("Searching on: cluster %u, directory ...\\%s\n",
	      mycluster, name);
#endif
    }

#ifdef DIRFINDDEBUG
  printf ("Full path points to cluster %u\n", mycluster);
#endif
  cluster[0] = mycluster;
  return 0;
}
