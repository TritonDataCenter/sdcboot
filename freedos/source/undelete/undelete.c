/*   	FreeDOS special UNDELETE tool (and mirror, kind of)
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

#include <conio.h>		/* getch */
#include <stdlib.h>		/* atoi, strtol */
#include <string.h>		/* strcmp, strcasecmp */
#include <stdio.h>		/* printf */
#include <dir.h>		/* getcurdir */
#include <dos.h>
#include <ctype.h>		/* toupper */

#include "dostypes.h"		/* Byte Word Dword */
#include "fatio.h"		/* {read,write}fat(drive, slot, {*,} value) */
#include "diskio.h"		/* r/w file+sector */
#include "drives.h"		/* DPB, getdrive(DPB * dpb), setdrive(0...) */
#include "dir16.h"		/* dirent structure, Date/Time macros */
#include "cluster.h"		/* nextcluster, clustertosector/CTS */
#include "dirfind.h"		/* dirfind, dirfind1 */

/* {read,write}sector(drive,which,*buffer), openfile{r,w}(*name) */
/* closefile(handle), {read,write}file(handle,*buffer) */

/* ***************************************************** */

int advancedmain (int argc, char **argv);
void help (void);
int processDirectory (char *currentdir, char *newdir);

/* Borland C++ users can do this: */
#define strcasecmp stricmp
/* Some compilers may not have stricmp: */
#ifndef strcasecmp		/* seems to be avail for Unix only... */
int
strcasecmp (const char *s1, const char *s2)
{				/* case insensitive string compare */
  int x;
  while (s1[0] && s2[0])
    {
      x = (toupper (s1[0]) - toupper (s2[0]));
      if (x != 0)
	{
	  return (x < 0 ? -1 : 1);
	}
      s1++;
      s2++;
    }
  if (s1[0] != s2[0])
    {
      x = (toupper (s1[0]) - toupper (s2[0]));
      return (x < 0 ? -1 : 1);
    }
  else
    {
      return 0;
    }
}
#endif

int
main (int argc, char **argv)
{
  struct DPB dpb;
  int drive;
  char directory[MAXPATH];
  Dword sector;
  Word cluster;
  Byte buffer[512];
  int state, j;
  int dirAlreadyGiven;
  enum InteractiveMode interactiveMode;
  int foundSomething = 0;
  char *externalDestination = NULL;

  /* Advanced commands for Undelete: */
  if (argc > 1)
    {
      if (!strcasecmp (argv[1], "/SYSSAVE")
	  || !strcasecmp (argv[1], "/DIRSAVE")
	  || !strcasecmp (argv[1], "/FOLLOW")
	  || !strcasecmp (argv[1], "/EXTRACT"))
	{
	  return advancedmain (argc, argv);
	}
    }

  drive = getdrive (&dpb);
  /* always operates on current drive */

  if ((dpb.bytepersec != 512) || (drive < 0))
    {
      printf ("\n"
	      "ERROR: current (source) drive has unsupported format.\n"
	      "       UNDELETE only works from FAT12/16 disks\n");
      return -1;
    }

  /* Get current working directory: */
  getcurdir (0, directory);
  /* Doesn't usually put in the starting '\\': */
  if (strlen (directory))
    {
      if (directory[0] != '\\')
	for (j = strlen (directory); j >= 0; j--)
	  directory[j + 1] = directory[j];
      directory[0] = '\\';
    }

  dirAlreadyGiven = 0;
  interactiveMode = IM_NORMAL;
  for (j = 1; j < argc; j++)
    {
      if (argv[j][0] == '/' || argv[j][0] == '-')
	{
	  if (strcasecmp (argv[j] + 1, "LIST") == 0)
	    interactiveMode = IM_LIST;
	  else if (strcasecmp (argv[j] + 1, "ALL") == 0)
	    interactiveMode = IM_ALL;
	  else if (argv[j][1] == 'E')
	    externalDestination = argv[j] + 2;
	  else
	    {
	      help ();
	      return 0;
	    }
	}
      else
	{
	  /* This can handle absolute and relative paths.
	     TO DO: wild card file names. */
	  char drive[MAXDRIVE];
	  char dir[MAXDIR];
	  char file[MAXFILE];
	  char ext[MAXEXT];
	  int flags;

	  if (dirAlreadyGiven)
	    {
	      printf ("Only one path per use of Undelete is allowed\n");
	      return 0;
	    }
	  dirAlreadyGiven = 1;

	  flags = fnsplit (argv[j], drive, dir, file, ext);

	  if (flags & DRIVE)
	    {
	      if (toupper (drive[0]) != ('A' + getdisk ()))
		{
		  printf ("Cannot operate from drive %s "
			  "(UNDELETE only operates from the current drive)\n",
			  drive);
		  return 1;
		}
	    }

	  if (flags & DIRECTORY)
	    {
	      if (processDirectory (directory, dir) == -1)
		{
		  printf ("Invalid path given\n");
		  return 0;
		}
	    }

	  if ((flags & FILENAME) && !(flags & EXTENSION)
	      && !(flags & WILDCARDS))
	    {
	      struct ffblk dummy;
	      /* See if filename is actually directory */
	      processDirectory (directory, file);
	      if (findfirst (directory, &dummy, FA_DIREC) != 0)
		{
		  /* it isn't, cut off the filename again: */
		  char *p = strrchr (directory, '\\');
		  if (p)
		    *p = '\0';
		  else
		    *directory = '\0';
		}
	    }
	}
    }

  printf ("Searching directory: %s\n", directory);
  dirfind (directory, &cluster, &dpb);
  if (cluster == 0)
    sector = dpb.numressec + (dpb.fats * dpb.secperfat);
  else
    sector = CTS (cluster, &dpb);

  j = 0;			/* sector in cluster */
  while (1)
    {
      state = readsector (drive, sector, buffer);
      if (state < 0)
	{
	  printf ("ERROR: could not read sector.\n");
	  return -1;
	}

      if (interactivedirents (interactiveMode, directory,
			      drive, sector, buffer, &dpb,
			      externalDestination) == 0)
	foundSomething = 1;


      if ((buffer[512 - 32] == 0) && (buffer[512 - 64] == 0))
	{
/*          printf ("\nSeems to be EOF\n");        could be done better... */
	  if (!foundSomething)
	    printf ("No undeletable files found.\n");
	  return 0;
	}

      if (sector < CTS (2, &dpb))
	{			/* root directory is linear */
	  sector++;
	}
      else
	{
	  sector++;		/* clusters are linear */
	  j++;
	  if (j == dpb.maxsecinclust)
	    {			/* follow FAT chain */
	      cluster = nextcluster (cluster, &dpb);	/* MAGIC */
	      /* nextcluster stays in used or unused clusters, */
	      /* depending which cluster it is "called from" */

	      if (cluster == 0)
		{
/*                  printf ("\nDone (EOF encountered)\n"); */
		  printf ("No undeletable files found.\n");
		  return 0;
		}
	      sector = CTS (cluster, &dpb);	/* back on track */
	    }
	}
    }
}


/* The follow makes currentdir point to the new directory, taking into
	 account any .. or . etc */
int
processDirectory (char *currentdir, char *newdir)
{
  char *ptr1 = newdir;
  char *ptr2;
  int numBack = 0;

  if (*ptr1 == '\\')
    strcpy (currentdir, ptr1);
  else
    {
      /* Relative path */
      /* Consider the .. and . at the beginnning: */
      while (*ptr1 == '.')
	{
	  ptr2 = strchr (ptr1, '\\');

	  if (ptr2 != NULL)
	    *ptr2 = '\0';

	  if (strcasecmp (ptr1, "..") == 0)
	    numBack++;
	  else if (strcasecmp (ptr1, ".") != 0)
	    {
	      printf ("Invalid path\n");
	      return -1;	/* Cannot have anything else start with . */
	    }

	  if (ptr2 != NULL)
	    {
	      *ptr2 = '\\';
	      ptr1 = ptr2 + 1;
	    }
	  else
	    *ptr1 = '\0';	/* Makes us finish */
	}

      while (numBack)
	{
	  ptr2 = strrchr (currentdir, '\\');

	  if (ptr2)
	    {
	      *ptr2 = '\0';
	      numBack--;
	    }
	  else if (*currentdir != '\0')
	    {
	      *currentdir = '\0';
	      numBack--;
	    }
	  else
	    return -1;		/* Cannot go back, can't be a valid path */
	}

      /* OK, now we can concatenate the relative path to the current dir */
      appendSlashIfNeeded (currentdir);
      strcat (currentdir, ptr1);
    }

  /* Check this is a valid path */
  ptr1 = currentdir;
  while (ptr1 != NULL)
    {
      /* Search through the finished string. If there are any more .. or . then
         throw a wobbler... */
      if (*ptr1 == '\0')
	return 0;

      if (*ptr1 == '\\')
	ptr1++;

      if (*ptr1 == '.')
	{
	  return -1;
	}

      ptr1 = strchr (ptr1, '\\');
    }

  return 0;
}


int
advancedmain (int argc, char **argv)	/* argv[0] is self... */
{
  int arg4 = -1;		/* size argument, optional */
  int mode = 0;			/* target selection: used or unused clusters, see the code. */
  Byte buffer[512];
  Dword sector;
  Word cluster;
  Word nextclust;
  Dword howmany;
  int handle, i, j, state;
  struct DPB dpb;
  int drive;

  if (argc > 5)
    {
      help ();
      printf ("Error: Too many args\n");
      return -1;
    };
  if (argc < 4)
    {
      help ();
      printf ("Error: Too few args\n");
      return -1;
    };

  drive = getdrive (&dpb);
  showdriveinfo (&dpb);
  /* always operates on current drive */

  if ((dpb.bytepersec != 512) || (drive < 0))
    {
      printf ("\nERROR: current=source drive has unsupported format!\n");
      printf ("\nPlease start UNDELETE from the drive which you want\n");
      printf ("to extract/undelete data from. Undelete /? for help.\n");
      /* help(); */
      return -1;
    }

  /* Advanced Commands */
  /* arg0: self                                        */
  /* arg1: SYSSAVE FOLLOW DIRSAVE                      */
  /* arg2: FAT1 FAT2 ROOT BOOT for SYSSAVE, starting   */
  /*       cluster for FOLLOW, directory for DIRSAVE,  */
  /*       (some filespec for UNDELETE, never dir!?)   */
  /* arg3: filename, where to save the specified thing */
  /* arg4: size for follow in clusters, may guess...   */
  /*       (follow a deleted file -> guess your way)   */
  /*       No arg4 for SYSSAVE. Size in sectors for    */
  /*       DIRSAVE. Arg4 is always optional.           */

  if (argc == 5)
    {
      arg4 = atoi (argv[4]);
      /* strtol(argv[4],NULL,10); base 10, end irrelevant */
      if (arg4 < 0)
	{
	  printf ("ERROR: Negative size?\n");
	  help ();
	  return -1;
	};
    };


  if (!strcasecmp (argv[1], "/SYSSAVE"))
    {
      if (arg4 != -1)
	{
	  help ();
	  printf
	    ("ERROR: No size may be given by the user for syssave mode.\n");
	  return -1;
	}			/* no arg4 for syssave */
      howmany = 0;
      if (!strcasecmp (argv[2], "BOOT"))
	{
	  howmany = dpb.numressec;
	  sector = 0;		/* first sectors */
	};
      if (!strcasecmp (argv[2], "FAT1"))
	{
	  howmany = dpb.secperfat;
	  sector = dpb.numressec;	/* skip over reserved */
	}
      if (!strcasecmp (argv[2], "FAT2"))
	{
	  howmany = dpb.secperfat;
	  sector = dpb.numressec + howmany;
	}
      if (!strcasecmp (argv[2], "ROOT"))
	{
	  howmany = dpb.rootdirents >> 4;	/* 512/32 */
	  sector = dpb.numressec + (dpb.secperfat * dpb.fats);
	}
      if (howmany == 0)
	{
	  printf
	    ("ERROR: You must specify one of root, boot, fat1 or fat2 as syssave type!\n");
	  help ();
	  return -1;
	};			/* illegal sysssave */

      handle = openfilew (argv[3]);
      if (handle < 0)
	{
	  printf
	    ("ERROR: Could not open target file. Hint: overwriting is disallowed.\n");
	  return -1;
	};
      printf ("Saving from sector %ld on...\n", sector);

      for (i = 0; i < howmany; i++)
	{
	  state = readsector (drive, sector, buffer);
	  if (state < 0)
	    {
	      printf ("ERROR: could not read sector.\n");
	      return -1;
	    }
	  state = writefile (handle, buffer);
	  if (state < 0)
	    {
	      printf ("ERROR: could not write to file, disk full?\n");
	      return -1;
	    }
	  sector++;
	  printf (".");
	}

      printf ("\nDone\n");
      (void) closefile (handle);
      return 0;
    }

/* ***************************************************** */

  mode = 0;			/* set up magic to FOLLOW mode */
  if (!strcasecmp (argv[1], "/EXTRACT"))
    {
      mode = 1;			/* set up magic to EXTRACT mode */
    }

/* ***************************************************** */

  if ((mode == 1) || (!strcasecmp (argv[1], "/FOLLOW")))
    {
      if ((arg4 <= 0) && (mode == 0))
	{			/* FOLLOW mode */
	  printf
	    ("Using autodetect for size: This may save too many clusters if the\n");
	  printf
	    ("file in question is in front of an area with other empty clusters, and\n");
	  printf
	    ("too few clusters if the file in question was fragmented. TAKE CARE!\n");
	  arg4 = -1;
	}			/* default size is autodetect */
      /* was 1 cluster in old versions. */
      if ((arg4 <= 0) && (mode == 1))
	{			/* EXTRACT mode */
	  printf ("Reading all or part of an EXISTING file or directory.\n");
	  printf
	    ("Ends where the FAT chain of this item ends, autodetected.\n");
	  arg4 = 0;
	}			/* default size is autodetect */
      cluster = atoi (argv[2]);

      handle = openfilew (argv[3]);
      if (handle < 0)
	{
	  printf
	    ("ERROR: Could not open target file. Hint: overwriting is disallowed.\n");
	  return -1;
	};
      printf ("Following FAT or empty chain until EOF or count reached...\n");

      if (mode == 1)
	{			/* EXTRACT mode */
	  printf ("Reading an existing FAT chain\n");
	  state = readfat (drive, cluster, &nextclust, &dpb);
	  if (state < 0)
	    {
	      printf
		("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
	      return -1;
	    }
	  if (nextclust == 0)
	    {
	      printf ("ERROR: There is no FAT chain on this spot!\n");
	      return -1;
	    }
	}
      else
	{			/* FOLLOW mode */
	  printf ("Reading from empty areas according to FAT\n");
	  state = readfat (drive, cluster, &nextclust, &dpb);
	  if (state < 0)
	    {
	      printf
		("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
	      return -1;
	    }
	  if (nextclust > 0)
	    {
	      printf ("This area is not empty, skipping...\n");
	      while (nextclust > 0)
		{
		  cluster++;
		  state = readfat (drive, cluster, &nextclust, &dpb);
		  if (state < 0)
		    {
		      printf
			("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
		      return -1;
		    }
		}
	      printf ("Starting with cluster %u\n", cluster);
	    }
	}

      for (i = 0; (arg4 <= 0) || (i < arg4); i++)
	{			/* 0 or none is auto */
	  sector = CTS (cluster, &dpb);

	  if (!sector)
	    {
	      return -1;
	    }

	  for (j = 0; j <= dpb.maxsecinclust; j++)
	    {
	      state = readsector (drive, sector, buffer);
	      if (state < 0)
		{
		  printf ("ERROR: could not read sector.\n");
		  return -1;
		}
	      state = writefile (handle, buffer);
	      if (state < 0)
		{
		  printf ("ERROR: could not write to file - disk full?\n");
		  return -1;
		}
	      sector++;
	    }
	  cluster = nextcluster (cluster, &dpb);	/* MAGIC */
	  /* nextcluster stays in used or unused clusters, */
	  /* depending which cluster it is "called from" */

	  if (cluster == 0)
	    {
	      printf ("\nDone (FAT chain EOF encountered)\n");
	      (void) closefile (handle);
	      return 0;
	    }
	}

      printf ("\nDone\n");
      (void) closefile (handle);
      return 0;
    }

/* ***************************************************** */

  if (!strcasecmp (argv[1], "/DIRSAVE"))
    {
      if (arg4 < 0)
	{
	  printf
	    ("Using autodetection for directory size. Will terminate when\n");
	  printf
	    ("it finds a few unused directory entries in a row. Should work!\n");
	  arg4 = -1;
	}			/* new default is now autodetect */
      /* old default size was 1 sector */

      if ((argv[2][0] <= '9') && (argv[2][0] >= '0'))
	{			/* cluster */
	  cluster = atoi (argv[2]);
	  sector = CTS (cluster, &dpb);

	  state = readfat (drive, cluster, &nextclust, &dpb);
	  if (state < 0)
	    {
	      printf
		("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
	      return -1;
	    }

	  if (nextclust != 0)
	    {
	      printf
		("Found to be reading an existing FAT chain. Staying within.\n");
	      mode = 0;		/* mode for EXISTING directories */
	    }
	  else
	    {
	      printf ("Reading from empty areas according to FAT\n");
	      state = readfat (drive, cluster, &nextclust, &dpb);
	      if (state < 0)
		{
		  printf
		    ("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
		  return -1;
		}
	      if (nextclust > 0)
		{
		  printf ("This area is not empty, skipping...\n");
		  while (nextclust > 0)
		    {
		      cluster++;
		      state = readfat (drive, cluster, &nextclust, &dpb);
		      if (state < 0)
			{
			  printf
			    ("ERROR: FAT reading engine failed. Only FAT12/16 work.\n");
			  return -1;
			}
		    }
		  sector = CTS (cluster, &dpb);
		}
	      mode = 1;		/* mode for DELETED directories */
	    }

	}
      else
	{			/* path */
	  mode = 0;		/* path mode also means EXISTING directory */
	  /* *** PATCH 11/2002 because FreeCOM cannot strip *** */
	  /* *** the "x:" drive spec from  %_CWD%  variable *** */
	  if (argv[2][1] == ':')
	    {
	      printf
		("Dirsave: Ignoring the drive you selected, using current!\n");
	      argv[2]++;	/* skip drive letter, */
	      argv[2]++;	/* skip ":" part, too */
	    }
	  /* *** END OF PATCH *** */
	  if ((!strcasecmp (argv[2], "\\")) || (!strcasecmp (argv[2], "/")))
	    {
	      printf ("Dirsave: selected root directory\n");
	      cluster = 0;
	    }
	  else
	    {

	      if (dirfind (argv[2], &cluster, &dpb) < 0)
		{		/* main MAGIC */

		  printf
		    ("ERROR: Specified directory not found in dirsave mode.\n");
		  printf
		    ("Remember that the directory name must start with \\\n");
		  printf
		    ("Neither relative names nor drive selection allowed here.\n");
		  return -1;
		}
	    }

	  if (cluster == 0)
	    {
	      sector = dpb.numressec + (dpb.fats * dpb.secperfat);
	    }
	  else
	    {
	      sector = CTS (cluster, &dpb);
	    }
	}

      printf ("DIRSAVE starting with sector %ld, cluster %u\n",
	      sector, cluster);
      j = 0;			/* sector in cluster */

      handle = openfilew (argv[3]);
      if (handle < 0)
	{
	  printf
	    ("ERROR: Could not open target file. Hint: overwriting is disallowed.\n");
	  return -1;
	};
      printf ("Saving DIR until EOF or count reached...\n");

      for (i = 0; (arg4 <= 0) || (i < arg4); i++)
	{			/* 0 or none is auto */
	  state = readsector (drive, sector, buffer);
	  if (state < 0)
	    {
	      printf ("ERROR: could not read sector.\n");
	      return -1;
	    }
	  state = writefile (handle, buffer);
	  if (state < 0)
	    {
	      printf ("ERROR: could not write to file - disk full?\n");
	      return -1;
	    }

	  printdirents (buffer, &dpb);	/* also print dirents for user */

	  if (arg4 <= 0)
	    {			/* if auto, detect end */
	      if ((buffer[512 - 32] == 0) && (buffer[512 - 64] == 0))
		{
		  printf ("\nSeems to be EOF\n");	/* could be done better... */
		  (void) closefile (handle);
		  return 0;
		}
	    }

	  if (sector < CTS (2, &dpb))
	    {			/* root directory is linear */
	      sector++;
	    }
	  else
	    {
	      sector++;		/* clusters are linear */
	      j++;
	      if (j == dpb.maxsecinclust)
		{		/* follow FAT chain */

		  cluster = nextcluster (cluster, &dpb);	/* MAGIC */
		  /* nextcluster stays in used or unused clusters, */
		  /* depending which cluster it is "called from" */

		  if (cluster == 0)
		    {
		      printf ("\nDone (EOF encountered)\n");
		      (void) closefile (handle);
		      return 0;
		    }

		  sector = CTS (cluster, &dpb);	/* back on track */

		}
	    }

	}

      printf ("\nDone\n");
      (void) closefile (handle);
      return 0;
    }

  return -1;
}

/* ***************************************************** */

void
help (void)			/* Human-readable thanks to: Emanuele Cipolla, 2002. */
{
  printf
    ("FreeDOS Undelete - (C) Copyright 2001-2002 Eric Auer <eric@coli.uni.sb.de>\n");
  printf
    ("Undelete is free software; it is distributed under the terms of the GNU\n");
  printf
    ("General Public License. See COPYING for details. FAT12/16 only.\n\n");

  printf
    ("SYNOPSIS: UNDELETE [path] [/ALL|/LIST] [/Edrive[:\directory]]\n\n");

  printf
    ("Prompts to undelete recoverable files from the current working directory,\n");

  printf ("or from the given path.\n\n");

  printf ("   /ALL        Automatically undelete all recoverable\n");

  printf ("               files without prompting first.\n\n");

  printf ("   /LIST       Lists the recoverable files without\n");

  printf ("               prompting for recovery.\n\n");

  printf ("  /E           Exports any undeleted files to\n");

  printf ("               an external disk, and optional directory.\n");

  printf
    ("               With this option, the source disk isn't modified.\n\n");

  printf ("Press any key...\n");
  getch ();

  printf ("For advanced users:\n\n");

  printf
    ("ADVANCED SYNOPSIS: undelete [/action] [what] [destination] [optional size]\n\n");

  printf ("Possible [/action]s:\n\n");

  printf
    ("follow  Looks for a deleted file (skips over used clusters!) starting at the\n");
  printf
    ("        cluster [what] and saves data to a file given as [destination]. The\n");
  printf
    ("        output of DIRSAVE helps you to find the right cluster number.\n");
  printf
    ("extract Like FOLLOW, but follows a still existing file according to FAT.\n");
  printf
    ("dirsave Like all above, but saves a directory to a file. Directory [what]\n");
  printf
    ("        must be given by absolute path (starting with \\) OR by cluster\n");
  printf
    ("        number. Also shows a technical directory listing on the screen!\n");
  printf
    ("syssave Saves the 1st or 2nd FAT, boot sector or root directory. No [size]\n");
  printf
    ("        allowed. Select fat1, fat2, boot, or root in [what]. \"Mirror\" mode.\n\n");

  printf ("Specifying no [size] or [size] 0 will cause autodetection.\n");
  printf ("Unit of [size] is clusters for FOLLOW, sectors for DIRSAVE).\n");
  printf
    ("DIRSAVE works the same for both existing and deleted directories.\n");
  printf
    ("[Destination] must be on another drive than the current drive. Data is\n");
  printf
    ("always read/recovered from the drive from which undelete is invoked.\n");
}
