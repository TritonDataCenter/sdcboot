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

#include <assert.h>
#include <string.h>
#include <ctype.h>

//#include "fte.h"

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
     char *dot, *delim, *delim2;
     size_t i;
    
     assert(path && filename && extension);

     dot = strchr(path, '.');       /* If the filename starts with . */
     if (dot == path) return;
     
     delim2 = strchr(path, '/');    /* The part BEFORE the first / or \ */
     delim  = strchr(path, '\\');

     if (!delim || (delim2 && (delim > delim2)))     /* Adjust to make delim1 point to the first / or \ */
        delim = delim2;     
     
     if (delim && (delim < dot)) dot = 0;      /* Only the extension of the part before / or \ */


     if (dot)                       /* If there is an extension */
     {
        /* filename */
        filelen = (int)(dot - path);
        memcpy(filename, path, filelen);
        memset(filename+filelen, ' ', 8 - filelen);

        /* extension */
        if (delim) 
	    extlen = (int) (delim - (dot+1));
	else
            extlen = strlen(dot+1);	    

        memcpy(extension, dot+1, extlen);
        memset(extension+extlen, ' ', 3 - extlen); /* Pad with spaces */
     }
     else
     {
        /* filename */
        if (delim) 
	   filelen = (int) (delim - path);
	else
	   filelen = strlen(path);
        
        memcpy(filename, path, filelen);
        memset(filename+filelen, ' ', 8 - filelen); /* Pad with spaces */

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

     assert(path && filename && extension);
    
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

#ifdef DEBUG

int main(int argc, char** argv)
{
    int i;
    char filename[8], extension[3], path[255];
    
    
    for (i=1; i<argc; i++)
    {
        ConvertUserPath(argv[i], filename, extension);
	
	printf("ENTRY: ");
	for (i=0; i<8; i++)
	    printf("%c", filename[i]);
	printf(".");
	for (i=0; i<3; i++)
	    printf("%c", extension[i]);
	printf("\n");
	
	
	ConvertEntryPath(path, filename, extension);
	printf("USER: %s", path);
    }    
}




#endif
