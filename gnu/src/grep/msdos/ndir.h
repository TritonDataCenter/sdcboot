/* NDIR.H - 4.2BSD directory functions for MS-DOS (header file)
 ******************************************************************************
 * Copyright 1987, 1995 by the Summer Institute of Linguistics, Inc.
 * Permission is granted to use, but not to sell, this code.
 */
#ifndef _NDIR_H_INCLUDED_
#define _NDIR_H_INCLUDED_

#define MAXNAMLEN 12

struct direct
    {
    long     d_ino;			/* MSDOS starting cluster number */
    unsigned d_reclen;			/* size of this record */
    unsigned d_namlen;			/* length of string in d_name */
    char     d_name[MAXNAMLEN+1];	/* actual filename */
    };

struct _dirfcb
    {
    char df_resv[21];		/* reserved area */
    char df_attr;		/* file attribute */
    char df_time[2];		/* file creation time */
    char df_date[2];		/* file creation date */
    char df_size[4];		/* file size */
    char df_name[13];		/* filename */
    };

typedef struct _dirdesc
    {
    char *dd_subdir;		/* drive and directory name */
    long dd_loc;		/* number of current entry in directory */
    long dd_oldloc;		/* old current entry number */
    struct direct dd_entry;	/* current directory entry */
    struct _dirfcb dd_fcb;	/* file control block for directory */
    } DIR;

/*
 *  function prototypes
 */
extern DIR *opendir(const char *dirname);
extern struct direct *readdir(DIR *dp);
extern void seekdir(DIR *dp, long loc);
extern long telldir(DIR *dp);
extern void rewinddir(DIR *dp);
extern int closedir(DIR *dp);

#endif /*_NDIR_H_INCLUDED_*/
