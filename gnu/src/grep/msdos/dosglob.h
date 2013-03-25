/* DOSGLOB.H - filename globbing for MS-DOS (Microsoft C)
 ******************************************************************************
 * Copyright 1995 by the Summer Institute of Linguistics, Inc.
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
 */
#ifndef _DOSGLOB_H_INCLUDED_
#define _DOSGLOB_H_INCLUDED_

struct glob_array
    {
    unsigned glob_count;	/* number of elements in glob_vector */
    char **glob_vector;		/* array of pointers to globbed filenames */
    };

extern int dos_glob(char *pattern, struct glob_array *argarray);

#endif /*_DOSGLOB_H_INCLUDED_*/
