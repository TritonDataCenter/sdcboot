/* NDIR.C - Unix directory functions for MS-DOS
 ******************************************************************************
 *
 * DIR *	  opendir(const char *dirname)
 * struct direct *readdir(DIR *dp)
 * void		  seekdir(DIR *dp, long loc)
 * long		  telldir(DIR *dp)
 * void		  rewinddir(DIR *dp)
 * int		  closedir(DIR *dp)
 *
 ******************************************************************************
 * edit history is at the end of the file
 ******************************************************************************
 * Copyright 1987, 1995 by the Summer Institute of Linguistics, Inc.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */
#include <string.h>
#include <stdlib.h>		/* For _MAX_PATH */
#include <errno.h>		/* For errno */
#include <sys/types.h>
#include <sys/stat.h>
#include <dos.h>		/* For _dos_getdiskfree(), ... */

#include "ndir.h"		/* Microsoft C has an unrelated <direct.h> */

#define NUL '\0'
/*
 *  BDOS function codes
 */
#define	SETDTA		 0x1A00		/* set disk transfer address */
#define GETDTA		 0x2F00		/* get disk transfer address */
#define	FINDFIRST	 0x4E00		/* find first file */
#define	FINDNEXT	 0x4F00		/* find next file */

#define ALL_FILES_DIRS	(_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR)

/************************************************************************
 * NAME
 *    opendir
 * ARGUMENTS
 *    dirname - NUL-terminated pathname of directory to open
 * DESCRIPTION
 *    Prepare to read from a directory.
 * RETURN VALUE
 *    pointer to opened DIR structure, or NULL if error
 */
DIR *opendir(dirname)
const char *dirname;
{
register char *p;
register DIR *dp;
unsigned cdev;
char cwd[_MAX_PATH], *q;
char path[_MAX_PATH];
const char *pdir;
union _REGS inr, outr;
struct _SREGS segr;
char __far *pc;		/* need both segment and offset for system call */
struct _stat stbuf;
/*
 *  make sure we have a nonempty directory name, and memory to work with
 */
if ((dirname == NULL) || (dirname[0] == NUL))
    return( (DIR *)NULL );
dp = (DIR *)malloc( sizeof(DIR) );
if (dp == (DIR *)NULL)
    return( (DIR *)NULL );
/*
 *  expand the given directory to a full pathname
 */
if (_fullpath(path, dirname, _MAX_PATH) == (char *)NULL)
    return( (DIR *)NULL );
for ( p = strchr(path,'\\') ; p ; p = strchr(p,'\\') )
    *p++ = '/';
if (strcmp(&path[1],":/") != 0)
    {
    p = strrchr( path, '/' );
    if ((p != (char *)NULL) && (*(p+1) == NUL))
	*p = NUL;
    }
_strlwr(path);
/*
 *  check whether we have a directory (or what looks like one)
 */
if (strcmp(&path[1],":/") != 0)
    {
    if (_stat(path,&stbuf) == -1)
	goto die;			/* can't find it */
    if (!(stbuf.st_mode & _S_IFDIR))
	goto die;			/* not a directory */
    }
/*
 *  set the directory name and location
 */
dp->dd_subdir = malloc( (unsigned)strlen(path)+1 );
if (dp->dd_subdir == NULL)
    goto die;
strcpy( dp->dd_subdir, path );
dp->dd_loc = 0L;			/* beginning of directory */
dp->dd_oldloc = 0L;			/* beginning of directory */
/*
 *  return pointer to DIR struct
 */
return( dp );
/*
 *  single return point for errors
 */
die:
free((char *)dp);
return( (DIR *)NULL );
}

/************************************************************************
 * NAME
 *    readdir
 * ARGUMENTS
 *    dp - pointer to opened DIR structure
 * DESCRIPTION
 *    Read the next entry from the opened directory
 * RETURN VALUE
 *    address of a direct structure which contains the filename, or NULL
 *    if error or end of directory
 */
struct direct *readdir(dp)
DIR *dp;
{
union _REGS inr, outr;
struct _SREGS segr;
char __far *pf;		/* need both segment and offset for system calls */
char path[_MAX_PATH];
long x;
register char *p;
unsigned save_dx, save_ds;

if (dp == (DIR *)NULL)
    return( (struct direct *)NULL );

if (dp->dd_loc < 0)
    dp->dd_loc = 0;
/*
 *  save the current DTA value and set a new one
 */
inr.x.ax = GETDTA;
_intdosx( &inr, &outr, &segr );
save_dx = outr.x.dx;
save_ds = segr.ds;
inr.x.ax = SETDTA;
pf = (char __far *)&dp->dd_fcb;
inr.x.dx = _FP_OFF(pf);
segr.ds  = _FP_SEG(pf);
_intdosx( &inr, &outr, &segr );

if ((dp->dd_loc == 0) || (dp->dd_loc != dp->dd_oldloc))
    {
    strcpy( path, dp->dd_subdir );
    if (strcmp(&path[1],":/")!=0)
	strcat( path, "/" );
    strcat( path, "????????.???");
    inr.x.ax = FINDFIRST;
    inr.x.cx = ALL_FILES_DIRS;
    pf = &path[0];
    inr.x.dx = _FP_OFF(pf);
    segr.ds  = _FP_SEG(pf);
    _intdosx( &inr, &outr, &segr );
    if (outr.x.cflag)
	return( (struct direct *)NULL );
    x = 1L;
    }
else
    x = dp->dd_oldloc;

if (dp->dd_loc > 0)
    {
    while ( x <= dp->dd_loc )
	{
	++x;
	inr.x.ax = FINDNEXT;
	_intdos( &inr, &outr );
	if (outr.x.cflag)
	    return( (struct direct *)NULL );
	}
    }
dp->dd_loc = dp->dd_oldloc = x;
/*
 *  copy from the disk transfer area to the current entry area
 */
dp->dd_entry.d_reclen = sizeof(struct direct);
strcpy( dp->dd_entry.d_name, dp->dd_fcb.df_name );
_strlwr( dp->dd_entry.d_name );
dp->dd_entry.d_namlen = strlen(dp->dd_entry.d_name);
dp->dd_entry.d_ino = 1L;
/*
 *  restore the original DTA value
 */
inr.x.ax = SETDTA;
inr.x.dx = save_dx;
segr.ds  = save_ds;
_intdosx( &inr, &outr, &segr );

return( &(dp->dd_entry) );
}

/************************************************************************
 * NAME
 *    seekdir
 * ARGUMENTS
 *    dp  - pointer to opened DIR structure
 *    loc - desired location within the directory
 * DESCRIPTION
 *    set the location within an opened directory
 * RETURN VALUE
 *    none
 */
void seekdir(dp,loc)
DIR *dp;
long loc;
{
if (dp != (DIR *)NULL)
    dp->dd_loc = loc;
}

/************************************************************************
 * NAME
 *    telldir
 * ARGUMENTS
 *    dp - pointer to opened DIR structure
 * DESCRIPTION
 *    Get the current location within an opened directory
 * RETURN VALUE
 *    current location within an opened directory
 */
long telldir(dp)
DIR *dp;
{
return( (dp != (DIR *)NULL) ? dp->dd_loc : -1L );
}

/************************************************************************
 * NAME
 *    rewinddir
 * ARGUMENTS
 *    dp - pointer to opened DIR structure
 * DESCRIPTION
 *    Reset the opened directory to its beginning.
 * RETURN VALUE
 *    none
 */
void rewinddir(dp)
DIR *dp;
{
if (dp != (DIR *)NULL)
    dp->dd_loc = 0L;
}

/************************************************************************
 * NAME
 *    closedir
 * ARGUMENTS
 *    dp - pointer to opened DIR structure
 * DESCRIPTION
 *    close a previously opened DIR structure, freeing the memory space
 * RETURN VALUE
 *    0 for success, -1 for error
 */
int closedir(dp)
DIR *dp;
{
if (dp != (DIR *)NULL)		/* surely, there's more to it than this! */
    free(dp);
return 0;			/* don't worry about errors... */
}

/******************************************************************************
 * EDIT HISTORY
 ******************************************************************************
 *  6-Mar-87	directory functions originally written by Stephen McConnel
 * 18-May-87	SRMc - fix bug for referencing default directory on another
 *			drive
 *  3-Jan-89	SRMc - fix for Microsoft C
 * 10-Feb-95	SRMc - rename BSD4.H to NDIR.H
 *		     - add function prototypes to NDIR.H
 *		     - merge functions into a single source file
 *		     - fix for Microsoft C 7.0+
 *		     - changed closedir() to return an int (always 0)
 *		     - convert filenames to lowercase
 *		     - don't bother adding '.' to end of filenames that lack
 *			an extension
 */
