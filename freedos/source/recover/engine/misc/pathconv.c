/*
   Pathconv.c - conversion of file names between the file system and the
                user.
                
   Copyright (C) 2002 Imre Leber

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

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/

#include <string.h>
#include <ctype.h>

#include "fte.h"

/**************************************************************
**                   ConvertUserPath
***************************************************************
** Converts the user string (e.g. "defrag.txt") to the two
** strings pointed to by filename and extension, in such
** a way as they are stored in directory entries.
**************************************************************/

void ConvertUserPath(char* path, char* filename, char* extension)
{
     int extlen, filelen;
     char *dot, *delim1, *delim2, *p, *q;
	 size_t i;

     dot = strchr(path, '.');
     if (dot == path) return;
     
     delim1 = strchr(path, '/');
     delim2 = strchr(path, '\\');

     if ((delim1 < dot) || (delim2 < dot))
        dot = 0;

     if (delim1 > delim2)
        delim1 = delim2;

     if ((dot) && (!delim1 || (dot < delim1)))
     {
        /* filename */
        filelen = (int)(dot - path - 1);
        memcpy(filename, path, filelen);
        memset(filename+filelen, ' ', 8 - filelen);

        /* extension */
        extlen = strlen(dot+1);
        p = strchr(dot+1, '\\');
        q = strchr(dot+1, '/');

        if (q && ((p > q) || (!p))) p = q;
        if (p) extlen = (int) (p - (dot+1));

        memcpy(extension, dot+1, extlen);
        memset(extension+extlen, ' ', 3 - extlen);
     }
     else
     {
        /* filename */
        filelen = strlen(path);
        p = strchr(path, '\\');
        q = strchr(path, '/');

        if (q && ((p > q) || (!p))) p = q;
        if (p) filelen = (int) (p - path);
        
        memcpy(filename, path, filelen);
        memset(filename+filelen, ' ', 8 - filelen);

        /* no extension */
        memset(extension, ' ', 3);
     }
	 
	 for (i = 0; i < strlen(filename); i++)
		 filename[i] = toupper(filename[i]);

     for (i = 0; i < strlen(extension); i++)
         extension[i] = toupper(extension[i]);
}

/**************************************************************
**                    ConvertEntryPath
***************************************************************
** Converts the filename and extension as stored in a directory
** entry to a user printable string and stores it in path (
** wich has to be 13 bytes long).
**************************************************************/

void ConvertEntryPath(char* path, char* filename, char* extension)
{
     char* p;

     /* Copy the filename */
     memcpy(path, filename, 8);
     p = path+7;
     while (*p == ' ') p--;

     if (memcmp(extension, "   ", 3) != 0)
     {
         p++; *p = '.'; p++;

         /* Copy the extension */
         memcpy(p, extension, 3);
         p += 2;
         while (*p == ' ') p--;
     }
     *(p+1) = '\0';
}
