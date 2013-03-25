/* CWILD.C - command line wildcard filename expansion for MS-DOS
 ******************************************************************************
 * 
 *  int _cwild()
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
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "dosglob.h"
/*
 *  Microsoft C defines these globals for the argument count and vector
 */
extern int __argc;
extern char ** __argv;

/*****************************************************************************
 * NAME
 *    nomemory
 * ARGUMENTS
 *    none
 * DESCRIPTION
 *    complain about running out of memory and die
 * RETURN VALUE
 *    none
 */
static void nomemory()
{
fprintf(stderr, "Not enough memory for storing command line arguments!\n");
exit(1);
}

/*****************************************************************************
 * NAME
 *    glob_compare
 * ARGUMENTS
 *    elem1 - pointer to first array element
 *    elem2 - pointer to second array element
 * DESCRIPTION
 *    compare two array elements for qsort() on an array of string pointers
 * RETURN VALUE
 *    whatever strcmp(*elem1,*elem2) returns
 */
static int glob_compare(const void *elem1, const void *elem2)
{
return( strcmp( *(const char **)elem1, *(const char **)elem2) );
}

/*****************************************************************************
 * NAME
 *    reformat_filename
 * ARGUMENTS
 *    name - file pathname
 * DESCRIPTION
 *    convert an MS-DOS file pathname to look more like a Unix file pathname
 *	1) characters all lowercase
 *      2) forward slashes for directory separators
 * RETURN VALUE
 *    pointer to the converted name (same as the input pointer)
 */
static char *reformat_filename(char *name)
{
char *p;

for ( p = name ; *p ; ++p )
    {
    if (*p == '\\')
	*p = '/';
    else if (isupper(*p))
	*p = tolower(*p);
    }
return( name );
}

/*****************************************************************************
 * NAME
 *    no_glob_chars
 * ARGUMENTS
 *    pattern - string to check for glob magic characters
 * DESCRIPTION
 *    check whether this pattern string has any glob magic characters: * ? []
 * RETURN VALUE
 *    1 if the pattern does not have any glob magic characters, 0 if it does
 */
static int no_glob_chars(const char *pattern)
{
char *p;

p = strpbrk(pattern, "*?[");
if (p == (char *)NULL)
    return( 1 );
if ((*p == '[') && (strchr(p, ']') == (char *)NULL))
    return( 1 );
return( 0 );
}

/*****************************************************************************
 * NAME
 *    _cwild
 * ARGUMENTS
 *    none
 * DESCRIPTION
 *    perform filename globbing of __argv[] during startup.  this replaces a
 *    function buried in the normal Microsoft C library.
 *    By Microsoft convention, the first letter of each element of __argv[]
 *    indicates whether or not globbing is to be performed for that name.
 *    If it is ", the name is passed unchanged (except for stripping the
 *    leading and trailing quotation marks).
 *    if the environment variable NOGLOB is defined, then filename globbing
 *    is not performed.  it doesn't matter what the value of NOGLOB is.
 * RETURN VALUE
 *    0 if successful, -1 if error occurs
 */
int _cwild()
{
int i, j, k;
int argc;
int glob_result;
char buffer[140];
char drive_buffer[3];
int len;
unsigned old_drive;
int skip, ch;
struct glob_array globdata[64];
int __orig_argc;
char **__orig_argv;

__orig_argc = __argc;
__orig_argv = __argv;

for ( argc = 0, i = 0 ; i < __argc ; ++i )
    {
    if ((__argv[i][0] == '"') || no_glob_chars(__argv[i]))
	{
	globdata[i].glob_count = 0;
	globdata[i].glob_vector = (char **)NULL;
	}
    else
	{
	strcpy(buffer, __argv[i] + 1);
	reformat_filename( buffer );
	if (dos_glob(buffer, &globdata[i]) != 0)
	    nomemory();
	}
    if (globdata[i].glob_count == 0)
	++argc;
    else
	{
	argc += globdata[i].glob_count;
	if (globdata[i].glob_count > 1)
	    qsort((void *)globdata[i].glob_vector,
		  (size_t)globdata[i].glob_count,
		  (size_t)sizeof(char *), glob_compare);
	}
    }
/*
 *  make a permanent copy of argv[]
 */
__argc = argc;
__argv = (char **)malloc((argc + 1) * sizeof(char *));
if (__argv == (char **)NULL)
    nomemory();
__argv[0] = _strdup( __orig_argv[0] + 1 );
if (__argv[0] == (char *)NULL)
    nomemory();
reformat_filename(__argv[0]);

for ( j = 1, i = 1 ; i < __orig_argc ; ++i )
    {
    if (globdata[i].glob_count == 0)
	{
	__argv[j] = _strdup( __orig_argv[i] + 1 );
	if (__argv[j] == (char *)NULL)
	    nomemory();
	++j;
	}
    else
	{
	for ( k = 0 ; k < globdata[i].glob_count ; ++k )
	    __argv[j++] = globdata[i].glob_vector[k];
	free( globdata[i].glob_vector );
	globdata[i].glob_count = 0;
	globdata[i].glob_vector = (char **)NULL;
	}
    }
__argv[__argc] = (char *)NULL;

return( 0 );
}
