/* DOSGLOB.C - filename globbing (wildcard expansion) for MS-DOS (Microsoft C)
 ******************************************************************************
 * 
 *	int dos_glob(char *pattern, struct glob_array *globs)
 *
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
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dos.h>
#include <direct.h>
#include "dosglob.h"

/*****************************************************************************
 * NAME
 *    dos_glob
 * ARGUMENTS
 *    pattern - pathname string containing wildcards to expand
 *    globs   - address of data structure to hold any results
 * DESCRIPTION
 *    Expand the wildcards like Unix, including those in directory slots of
 *    the pattern pathname.
 * RETURN VALUE
 *    0 if okay, 1 if Out of Memory
 *    any actual results are stored in globs->glob_count and globs->glob_vector
 */
#define FINDALL (_A_HIDDEN | _A_RDONLY | _A_SUBDIR | _A_SYSTEM)
int dos_glob(char *pattern, struct glob_array *globs)
{
struct _find_t fileinfo;
char findname[_MAX_PATH];
char *p;
int i, j;
char *firstglob, *nextdir;
int ch;
size_t length;
unsigned num_expanded = 0;
unsigned size_expanded = 0;
char **expanded = NULL;
struct glob_array subdirs;

globs->glob_count = 0;
globs->glob_vector = (char **)NULL;
/*
 *  find the first glob special character
 */
for ( firstglob = pattern ; *firstglob ; ++firstglob )
    {
    if ((*firstglob == '*') || (*firstglob == '?') || (*firstglob == '['))
	break;
    }
nextdir = strchr(firstglob, '/');
if (nextdir != (char *)NULL)
    *nextdir = '\0';
ch = *firstglob;
*firstglob = '\0';
/*
 *  find the last drive/directory portion before the first glob character
 */
p = strrchr(pattern, '/');
if (p == (char *)NULL)
    p = strrchr(pattern, ':');
*firstglob = ch;		/* restore the first glob character */
if (p != (char *)NULL)
    {
    length = p - pattern + 1;
    if (length+12 >= _MAX_PATH)
	return 0;		/* pattern too long, quit now */
    memcpy(findname, pattern, length);
    }
else
    length = 0;
if (ch == '\0')
    strcpy(findname+length, pattern+length);
else
    strcpy(findname+length, "????????.???");
if (_dos_findfirst(findname, FINDALL, &fileinfo))
    {
    if (nextdir != (char *)NULL)
	*nextdir = '/';		/* restore the original pattern contents */
    return 0;			/* match failed */
    }
do  {
    if ((fileinfo.name[0] == '.') && (pattern[length] != '.'))
	continue;			/* avoid . and .. as matches */
    strcpy(findname+length, fileinfo.name);
    _strlwr(findname+length);
    if (fnmatch(pattern, findname, 0) != 0)
	continue;
    if (nextdir == (char *)NULL)
	{
	if (num_expanded >= size_expanded)
	    {
	    size_expanded += 64;
	    if (expanded == (char **)NULL)
		expanded = (char **)malloc(size_expanded * sizeof(char *));
	    else
		expanded = (char **)realloc(expanded,
					       size_expanded * sizeof(char *));
	    if (expanded == (char **)NULL)
		goto nomemory;
	    }
	expanded[num_expanded] = _strdup(findname);
	if (expanded[num_expanded] == (char *)NULL)
	    goto nomemory;
	++num_expanded;
	}
    else if (fileinfo.attrib & _A_SUBDIR)
	{
	*nextdir = '/';		/* restore the original pattern contents */
	if (strlen(findname) + strlen(nextdir) + 1 >= _MAX_PATH)
	    continue;			/* pattern has gotten too long */
	strcat(findname, nextdir);
	if (dos_glob(findname, &subdirs) != 0)
	    goto nomemory;
	if (globs->glob_vector == (char **)NULL)
	    {
	    globs->glob_count = subdirs.glob_count;
	    globs->glob_vector = subdirs.glob_vector;
	    }
	else
	    {
	    globs->glob_vector = (char **)realloc(globs->glob_vector,
		    (globs->glob_count + subdirs.glob_count) * sizeof(char *));
	    for (j=globs->glob_count, i=0 ; i < subdirs.glob_count ; ++i, ++j)
		globs->glob_vector[j] = subdirs.glob_vector[i];
	    globs->glob_count += subdirs.glob_count;
	    free( subdirs.glob_vector );
	    }
	*nextdir = '\0';
	}
    } while (_dos_findnext(&fileinfo) == 0);
if (nextdir == (char *)NULL)
    {
    globs->glob_count = num_expanded;
    if (num_expanded < size_expanded)
	{
	expanded = realloc(expanded, num_expanded * sizeof(char *));
	if (expanded == (char **)NULL)
	    goto nomemory;
	}
    globs->glob_vector = expanded;
    }
else
    *nextdir = '/';		/* restore the original pattern contents */
return 0;

nomemory:
if (nextdir != (char *)NULL)
    *nextdir = '/';		/* restore the original pattern contents */
for ( i = 0 ; i < globs->glob_count ; ++i )
    {
    if (globs->glob_vector[i] != (char *)NULL)
	free( globs->glob_vector[i] );
    }
if (globs->glob_vector != (char **)NULL)
    free( globs->glob_vector );
globs->glob_count = 0;
globs->glob_vector = (char **)NULL;
return 1;
}
