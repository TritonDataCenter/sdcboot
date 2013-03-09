/* Wrapper to the info-unzip API */

/* Copyright (C) 1998,1999,2000,2001 Jim Hall <jhall@freedos.org> */

/*
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
*/

#ifdef __WATCOMC__
#include <screen.h>
#include <process.h>
#define fnmerge _makepath
#else
#include <conio.h>
#endif
#include <stdio.h>          /* for system(), free() */
#include <errno.h>
#include <string.h>
#include "unzip.h"          /* for UzpMain() */
int UzpMain( int argc, char **argv );
#include "dir.h"
#include "log.h"
#include <fcntl.h>
#include <dos.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef __WATCOMC__
#define FA_HIDDEN _A_HIDDEN
#define fnsplit _splitpath
#endif

static FILE *pkglst;

/* returns the unextracted size of all files in package (actual size, does not include wasted space) */
static unsigned long getUnexectractedPkgSize(unzFile uf)
{
  uLong i;
  unz_global_info pglobal_info;
  unz_file_info file_info;
  unsigned long pkgSize = 0;
  char curfilename[260];

  if (unzGetGlobalInfo(uf, &pglobal_info) == UNZ_OK)
    for (i = 0; i < pglobal_info.number_entry; i++)
    {
      if (unzGetCurrentFileInfo(uf, &file_info, curfilename, 260, NULL, 0,
                                NULL, 0) != UNZ_OK)
        return (pkgSize < 1)?1:pkgSize;

      fprintf (pkglst, "%s\n", curfilename);
      pkgSize += file_info.uncompressed_size;

      if ((i+1) < pglobal_info.number_entry && unzGoToNextFile(uf) != UNZ_OK)
          return (pkgSize < 1)?1:pkgSize;
    }

  return (pkgSize < 1)?1:pkgSize; /* 1 is to prevent divide by zero */
}

int
unzip_file (const char *zipfile, const char *fromdir, char *destdir)
{
  char full_zipfile[MAXPATH],
       full_ziplist[MAXPATH],
       buffer[MAXPATH],
       buffer2[MAXPATH];  /* path to a zipfile */
  int ret;
  unzFile uf;
  unsigned long pkgsize = 0L, dfree = 0L;
#ifndef __WATCOMC__
  struct dfree freesp;
#else
  struct diskfree_t freesp;
#endif

  /* set 3=full_zipfile and 5=destdir */

  char *argv[4] = {"unz.c", "-qq", "-o", "full_zipfile"};

  /* create the zip file name */

  fnsplit (zipfile, NULL, NULL, buffer2, NULL);
  fnmerge (full_zipfile, "", fromdir, zipfile, ".ZIP");

  /* set 3=full_zipfile and 5=destdir */
  argv[3] = full_zipfile;    /* pointer assignment */

  sprintf(buffer,"%s%sPACKAGES",destdir,(destdir[strlen(destdir)-1]=='\\')?"":"\\");
  if(access(buffer, 0) != 0) {
      mkdir(buffer);
#ifdef __WATCOMC__
      _dos_setfileattr (buffer, FA_HIDDEN);
#else
      _chmod(buffer, 1, FA_HIDDEN);
#endif
  }
  fnmerge (full_ziplist, "", buffer, buffer2, ".LST");


  log ("\n< full zipfile=\"%s\"/>\n", full_zipfile);
  log ("< full ziplist=\"%s\"/>\n", full_ziplist);

  pkglst = fopen (full_ziplist, "wt");
  if (pkglst == NULL) return -2;

  uf = unzOpen (full_zipfile);
  if (uf == NULL) return -1;

  pkgsize = getUnexectractedPkgSize(uf);

  /* Set the cursor to location so errors can be read, not proper fix. */
  gotoxy(2, 11);

#ifdef __WATCOMC__
  _dos_getdiskfree (0, &freesp);
#else
  getdfree (0, &freesp);
#endif

  dfree = freesp.avail_clusters;
  dfree *= freesp.sectors_per_cluster;
  dfree *= freesp.bytes_per_sector;
  dfree -= 4096L; /* allow a little leeway */

  log("<package size=\"%lu\"/>\n<disk free=\"%lu\"/>\n", pkgsize, dfree);

  if (pkgsize > dfree) return 3;

  unzClose (uf);

  ret = UzpMain (4, argv);      /* the Unzip code */
  gotoxy (2, 12);
  if (ret < 0) log ("< spawn errno=\"%i\"/>\n", errno);

  fclose (pkglst);

  /* Done */

  return (ret);
}
