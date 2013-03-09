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

#include <stdlib.h>		/* itoa */
#include <dos.h>		/* _argv */
#include <string.h>		/* strcpy, strcat */
#include <dir.h>		/* findfirst */
#include <process.h>		/* exit */
#include <ctype.h>		/* toupper */
#include <conio.h>		/* getch and getche */
#include "dir16.h"
#include "fatio.h"		/* readfat */
#include "cluster.h"		/* nextcluster */

int advancedmain (int argc, char **argv);

/* if printdirents knows the DPB, it checks un-delete-ability */

void
printdirents (Byte * buffer, struct DPB *dpb)
{
  int i, j, state;
  char x;
  Word fatchain, unichar;
  struct dirent *DE;
  char *DEarray;
  int lfnwhere[13] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

  j = 0;
  for (i = 1; i < 512; i++)
    {
      if (buffer[i] == buffer[i - 1])
	{
	  j++;
	}
    }
  if (j > 500)
    {
      printf ("printdirents: empty sector\n");
      return;
    }

  for (i = 0; i < 512; i += 32)
    {
      if (buffer[i] != 0)
	{
	  printf ("\n");
	  DE = (struct dirent *) (buffer + i);
	  if (DE->f_name[0] == D_DELCHAR)
	    {
	      if (dpb != (struct DPB *) 0)
		{
		  state = readfat (dpb->drive, DE->cluster, &fatchain, dpb);
		  if ((state < 0) || (fatchain != 0))
		    {
		      printf ("Del/Lost:  ");	/* first cluster reused */
		    }
		  else
		    {
		      printf ("UnDelAble: ");	/* undelete possible */
		    }
		}
	      else
		{
		  printf ("Deleted:   ");	/* deleted, did not check */
		}
	    }
	  else
	    {
	      if ((DE->f_attrib & D_LONG) == D_LONG)
		{
		  printf ("LFNpseudo: ");	/* part of a LFN -> meaning of */
		  /* most fields is void/changed */
		}
	      else
		{
		  printf ("           ");	/* normal file */
		}
	    }

	  for (j = 0; j < 8; j++)
	    {
	      x = DE->f_name[j];
	      if (x < 32)
		{
		  x = '?';
		}
	      if (x > 126)
		{
		  x = '?';
		}
	      printf ("%c", x);
	    }
	  printf (".");
	  for (j = 0; j < 3; j++)
	    {
	      x = DE->f_ext[j];
	      if (x < 32)
		{
		  x = '?';
		}
	      if (x > 126)
		{
		  x = '?';
		}
	      printf ("%c", x);
	    }
	  printf (" %2.2d:%2.2d:%2.2d", HofTime (DE->f_time),
		  MofTime (DE->f_time), SofTime (DE->f_time));
	  printf (" %2.2d.%2.2d.%d", DofDate (DE->f_date),
		  MofDate (DE->f_date), YofDate (DE->f_date));
	  printf (" @%5.5d, size %10.10ld", DE->cluster, DE->f_size);

	  if ((DE->f_attrib & D_LONG) == D_LONG)
	    {
	      printf (" <LONG>\nPartial name: <");
	      /* *** Better LFN display added 11/2002 *** */
	      /* See table 01355 of RBIL (intlist) for more. */
	      DEarray = (char *) DE;
	      for (j = 0; j < 13; j++)
		{
		  x = DEarray[lfnwhere[j] + 1];
		  unichar = x;
		  x = DEarray[lfnwhere[j]];
		  unichar = (unichar << 8) + x;
		  if (unichar < 32)
		    {
		      x = '?';
		    }
		  if (unichar > 126)
		    {
		      x = '?';
		    }
		  if (unichar > 255)
		    {
		      x = '*';
		    }
		  if (unichar == 0xffff)
		    {
		      x = ' ';
		    }
		  printf ("%c", x);
		}
	      x = DEarray[0];
	      printf ("> Part=%2.2d (%s,%s) ", x & 0x3f,
		      ((x & 0x40) != 0) ? "EOF" : "---",
		      ((x & 0x80) != 0) ? "DEL" : "---");
	      printf ("Shortname checksum=0x%hX", DEarray[0x0d]);
	    }
	  else
	    {
	      if (DE->f_attrib & D_RO)
		{
		  printf (" ro");
		}
	      if (DE->f_attrib & D_HIDE)
		{
		  printf (" hide");
		}
	      if (DE->f_attrib & D_SYS)
		{
		  printf (" sys");
		}
	      if (DE->f_attrib & D_VOL)
		{
		  printf (" label");
		}
	    }
	  if (DE->f_attrib & D_DIR)
	    {
	      printf (" dir");
	    }
	  if (DE->f_attrib & D_ARCH)
	    {
	      printf (" a");
	    }
	  if (DE->cluster_hi)
	    {
	      if ((DE->f_attrib & D_LONG) != D_LONG)
		printf ("Warning: ClusterHI=%x ", DE->cluster_hi);
	    }
	}
      else
	{
	  printf ("<eof>");
	}
    }
  printf ("\n");
}

/* Undelete by rebuilding the FAT chain: */
/* Uses writefat and Eric's nextchain: -RP */
int
interactivedirents (enum InteractiveMode interactiveMode,
		    const char *directory,
		    int drive, Dword sector, Byte * buffer, struct DPB *dpb,
		    const char *externalDirectory)
{
  int i, j, state;
  char x;
  Word fatchain;
  struct dirent *DE;
  struct ffblk dummyffblk;
  char filespec[MAXPATH];
  char response = 0;

  j = 0;
  for (i = 1; i < 512; i++)
    {
      if (buffer[i] == buffer[i - 1])
	{
	  j++;
	}
    }
  if (j > 500)
    {
/*      printf ("interactive dirents: empty sector\n"); */
      return -1;
    }

  for (i = 0; i < 512; i += 32)
    {
      if (buffer[i] != 0)
	{
	  DE = (struct dirent *) (buffer + i);
	  if (DE->f_name[0] == D_DELCHAR)
	    {

	      if (dpb != (struct DPB *) 0)
		{
		  state = readfat (dpb->drive, DE->cluster, &fatchain, dpb);
		  if (!((state < 0) || (fatchain != 0)))
		    {
		      for (j = 0; j < 8; j++)
			{
			  x = DE->f_name[j];
			  if (x < 32)
			    {
			      x = '?';
			    }
			  if (x > 126)
			    {
			      x = '?';
			    }
			  printf ("%c", x);
			}
		      printf (".");
		      for (j = 0; j < 3; j++)
			{
			  x = DE->f_ext[j];
			  if (x < 32)
			    {
			      x = '?';
			    }
			  if (x > 126)
			    {
			      x = '?';
			    }
			  printf ("%c", x);
			}
		      printf (" (%2.2d:%2.2d:%2.2d,", HofTime (DE->f_time),
			      MofTime (DE->f_time), SofTime (DE->f_time));
		      printf (" %2.2d.%2.2d.%d, ", DofDate (DE->f_date),
			      MofDate (DE->f_date), YofDate (DE->f_date));
		      printf (" size: %ld) can be undeleted.\n", DE->f_size);
		      switch (interactiveMode)
			{
			case IM_LIST:
			  response = 'N';
			  break;

			case IM_ALL:
			  response = 'Y';

			  /* Find a free character that won't produce a duplicate
			     filename: */
			  DE->f_name[0] = '#';
			  strcpy (filespec, directory);
			  appendSlashIfNeeded (filespec);
			  strcat (filespec, (char *) DE->f_name);
			  if (strlen ((char *) DE->f_ext))
			    {
			      strcat (filespec, ".");
			      strcat (filespec, (char *) DE->f_ext);
			    }
			  if (findfirst (filespec, &dummyffblk, 0) != 0)
			    break;

			  DE->f_name[0] = '%';
			  strcpy (filespec, directory);
			  appendSlashIfNeeded (filespec);
			  strcat (filespec, (char *) DE->f_name);
			  if (strlen ((char *) DE->f_ext))
			    {
			      strcat (filespec, ".");
			      strcat (filespec, (char *) DE->f_ext);
			    }
			  if (findfirst (filespec, &dummyffblk, 0) != 0)
			    break;

			  DE->f_name[0] = '&';
			  strcpy (filespec, directory);
			  appendSlashIfNeeded (filespec);
			  strcat (filespec, (char *) DE->f_name);
			  if (strlen ((char *) DE->f_ext))
			    {
			      strcat (filespec, ".");
			      strcat (filespec, (char *) DE->f_ext);
			    }
			  if (findfirst (filespec, &dummyffblk, 0) != 0)
			    break;

			  for (DE->f_name[0] = '1'; DE->f_name[0] <= '9';
			       (DE->f_name[0])++)
			    {
			      strcpy (filespec, directory);
			      appendSlashIfNeeded (filespec);
			      strcat (filespec, (char *) DE->f_name);
			      if (strlen ((char *) DE->f_ext))
				{
				  strcat (filespec, ".");
				  strcat (filespec, (char *) DE->f_ext);
				}
			      if (findfirst (filespec, &dummyffblk, 0) != 0)
				break;
			    }
			  if (findfirst (filespec, &dummyffblk, 0) != 0)
			    break;

			  for (DE->f_name[0] = 'A'; DE->f_name[0] <= 'Z';
			       (DE->f_name[0])++)
			    {
			      strcpy (filespec, directory);
			      appendSlashIfNeeded (filespec);
			      strcat (filespec, (char *) DE->f_name);
			      if (strlen ((char *) DE->f_ext))
				{
				  strcat (filespec, ".");
				  strcat (filespec, (char *) DE->f_ext);
				}
			      if (findfirst (filespec, &dummyffblk, 0) != 0)
				break;
			    }
			  if (findfirst (filespec, &dummyffblk, 0) != 0)
			    break;

			  printf ("Error: Ran out of free characters.\n");
			  exit (0);

			default:
			case IM_NORMAL:
			  printf ("Undelete (Y/N/Esc)?\n");	/* undelete possible */
			  do
			    {
			      response = getch ();
			      if (response == 27)
				exit (0);
			    }
			  while (toupper (response) != 'Y'
				 && toupper (response) != 'N');
			  break;
			}

		      if (response == 'Y' || response == 'y')
			{
			  Word oldcluster, newcluster;
			  Dword size;
			  if (interactiveMode == IM_NORMAL)
			    {
			      printf ("Enter first character of name:");
			      DE->f_name[0] = toupper (getche ());
			    }
			  else
			    printf ("First character will be: %c",
				    DE->f_name[0]);
			  printf ("\nThinking: ");
			  if (externalDirectory)
			    {
			      Follow (DE->cluster, (char *) DE->f_name,
				      (char *) DE->f_ext,
				      externalDirectory, DE->f_size);
			    }
			  else
			    {
			      newcluster = DE->cluster;
			      size = 0;
			      do
				{
				  size +=
				    dpb->bytepersec * (dpb->maxsecinclust +
						       1);
				  oldcluster = newcluster;
				  newcluster = nextcluster (oldcluster, dpb);
				  if (newcluster && (size < DE->f_size))
				    writefat (dpb->drive, oldcluster, newcluster, dpb);	/* next in chain */
				}
			      while (newcluster && (size < DE->f_size));
			      writefat (dpb->drive, oldcluster, 0xFFFF, dpb);	/* EOF */

			      if (size < DE->f_size)
				DE->f_size = size;

			      writesector (drive, sector, buffer);
			      printf ("done\n");
			    }
			}
		    }
		}
	    }
	}
    }

  if (response == 0)		/* mustn't have detected anything */
    return -1;
  else
    return 0;
}


void
appendSlashIfNeeded (char *directory)
{
  if (strlen (directory))
    {
      if (directory[strlen (directory) - 1] != '\\')
	{
	  strcat (directory, "\\");
	}
    }
  else
    {
      strcat (directory, "\\");
    }
}


void
Follow (Word cluster, const char *filename,
	const char *fileext, const char *directory, Dword size)
{
  char strcluster[20], strsize[40];
  char filespec[MAXPATH];
  char *arguments[5];

  sprintf (strcluster, "%u", cluster);
  sprintf (strsize, "%lu", size);
  strcpy (filespec, directory);

  if (strlen (directory) == 0)
  {
    printf("/E must specify a non-current drive");
    exit(1);
  }
  if (strlen (directory) == 1)	/* Just a drive letter */
  {
    strcat (filespec, ":");
  }
  else /* should be either D: or D:\DIR */
  {
    if (directory[1] != ':')
    {
      printf("/E must specify a non-current drive");
      exit(1);
    }

    if (strlen (directory) > 2)
    {
      /* Append '\\' if necessary, because it is a directory */
      if (directory[strlen(directory)-1] != '\\')
      strcat (filespec, "\\");
    }
  }

  strcat (filespec, filename);

  if (strlen (fileext))
    {
      strcat (filespec, ".");
      strcat (filespec, fileext);
    }

  arguments[0] = _argv[0];
  arguments[1] = "/FOLLOW";
  arguments[2] = strcluster;
  arguments[3] = filespec;
  arguments[4] = strsize;
  advancedmain (5, arguments);
}
