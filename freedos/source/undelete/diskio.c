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


#include "diskio.h"

#define BIGDISK	1		/* this will require you to have nasm for DOS! */
  /* *** From 11/2002, NASM is no longer needed for BIGDISK/DOS *** */

#ifdef __MSDOS__		/* Turbo C thing */
#define DOS 1
#endif


#include <sys/types.h>		/* generic */
#include <sys/stat.h>		/* open */
#include <fcntl.h>		/* open */

#ifndef DOS
#include <unistd.h>		/* close, lseek, read, write */
#endif
#ifdef DOS
#include <io.h>			/* close, lseek, read, write */
#include <limits.h>		/* UINT_MAX */
#endif

#include "drives.h"		/* getdrive() */

#ifndef DOS
int imghand = 0;		/* yes, we use an image... */
char *imgname = "test.img";	/* ... because this is linux :-) */
#endif
#ifdef DOS
#include "dos.h"		/* absread, abswrite */
#include "io.h"			/* unix like files */
#endif

/* ************************************************************* */

#ifdef BIGDISK
#ifdef DOS

/*
 * Using an external .obj looked like this (was written in NASM ASM):
 * extern int writebig(Word drive, Dword which, Byte near * buffer);
 * extern int readbig(Word drive, Dword which, Byte near * buffer);
 * However, ASM is no longer needed :-).
 */

struct bigdisk			/* WARNING: Must not be padded for alignment by the compiler */
{
  Dword sector;
  Word count;
  Byte far *buffer;
};

int
writebig (Word drive, Dword which, Byte near * buffer)
{
  struct bigdisk buffer2;

/* Wrong #syntax, sigh...
 * #if (sizeof buffer2) - 10
 * #error Bigdisk control structure may not be padded for alignment!
 * #endif
 */

  buffer2.count = 1;
  buffer2.sector = which;
  buffer2.buffer = (Byte far *) buffer;
  return (sizeof (buffer2) != 10) ? -1 :
    abswrite (drive, 1, -1, (void *) &buffer2);
}

int
readbig (Word drive, Dword which, Byte near * buffer)
{
  struct bigdisk buffer2;

  buffer2.count = 1;
  buffer2.sector = which;
  buffer2.buffer = (Byte far *) buffer;

  return (sizeof (buffer2) != 10) ? -1 :
    absread (drive, 1, -1, (void *) &buffer2);
}

#endif
#endif

/* ************************************************************* */

int
writesector (int drive, Dword which, Byte * buffer)
{
#ifndef DOS
  if (drive != 0)
    {
      return -1;
    };
  if (!imghand)
    {
      imghand = open (imgname, O_RDWR, 0644);
      if (imghand < 0)
	{
	  return imghand;
	};
    };
  if ((long int) lseek (imghand, which << 9, SEEK_SET) < 0)
    {
      return -1;
    };
  if ((long int) write (imghand, buffer, 512) != 512)
    {
      return -1;
    };
  return 0;
#endif
#ifdef DOS
  if (which > UINT_MAX)
    {				/* UINT, not INT - 19oct02 */
#ifndef BIGDISK
      return -1;
#endif
#ifdef BIGDISK
      return writebig (drive, which, buffer);
#endif
    }
  return abswrite (drive, 1, (int) which, (void *) buffer);
#endif
}


int
readsector (int drive, Dword which, Byte * buffer)
{
#ifndef DOS
  if (drive != 0)
    {
      return -1;
    };
  if (!imghand)
    {
      imghand = open (imgname, O_RDWR, 0644);
      if (imghand < 0)
	{
	  return imghand;
	};
    }
  if ((long int) lseek (imghand, which << 9, SEEK_SET) < 0)
    {
      return -1;
    };
  if ((long int) read (imghand, buffer, 512) != 512)
    {
      return -1;
    };
  return 0;
#endif
#ifdef DOS
  if (which > UINT_MAX)
    {				/* UINT, not INT - 19oct02 */
#ifndef BIGDISK
      return -1;
#endif
#ifdef BIGDISK
      return readbig (drive, which, buffer);
#endif
    }
  return absread (drive, 1, (int) which, (void *) buffer);
#endif
}


int
openfiler (char *name)
{
#ifndef DOS
  if (name[1] == ':')
    {
      name += 2;
    };				/* skip over drive for UNIX */
  return open (name, O_RDONLY, 0644);
#endif
#ifdef DOS
  return open (name, O_RDONLY | O_BINARY);
#endif
}


int
openfilew (char *name)
{
  int stat;
  if (name[1] != ':')
    {
      printf ("Cannot write files on current drive!\n");
      return -1;
    }
  if (((name[0] - 'A') & 31) == getdrive (NULL))
    {
      printf ("Cannot write files on current drive!\n");
      return -1;
    }
#ifndef DOS
  name += 2;			/* skip over drive for UNIX */
  stat = open (name, O_RDWR | O_CREAT | O_EXCL, 0644);
  if (stat == -EEXIST)
    {
      printf ("Warning! Appending to an existing file!\n");
      stat = open (name, O_RDWR | O_CREAT | O_APPEND, 0644);
    }
#endif
#ifdef DOS
  stat = open (name, O_RDWR | O_CREAT | O_EXCL | O_BINARY);
#endif
  return stat;
}


int
closefile (int handle)
{
  return close (handle);
}


int
readfile (int handle, Byte * buffer)	/* no seek yet */
{
  if ((long int) read (handle, buffer, 512) != 512)
    {
      return -1;
    };
  return 0;
}


int
writefile (int handle, Byte * buffer)	/* no seek yet */
{
  if ((long int) write (handle, buffer, 512) != 512)
    {
      return -1;
    };
  return 0;
}
