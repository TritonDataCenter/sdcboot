/*    
   MkAbsPath.c - Make relative path absolute.

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

#ifdef __BORLANDC__
#include <dir.h>
#else
#define MAXPATH 80
#endif

#include "fte.h"

BOOL GetPreviousDir(char* direct)
{
   char *p, *q, *r;
   
   p = strrchr(direct, '/');    /* Get the last '/' */
   q = strrchr(direct, '\\');   /* Get the last '\' */
   
   /* Check wether the '/' or '\' is the last character in the string */
   r = strchr(direct, 0);
   if (p == r-1)
   {   
      *p = 0;                   /* If so take the one before that */
      p = strrchr(direct, '/');    
   }
   
   if (q == r-1)
   {     
      *q = 0;                   /* If so take the one before that */
      q = strrchr(direct, '\\');      
   }
   
   if (q > p) p = q;            /* Take wathever came last, '/' or '\' */
   
   if (p == 0) return FALSE;    /* Return if there is no previous dir */
   
   *p = 0;                      /* Truncate the string on the last '/' and 
                                   '\' */
                                   
   return TRUE;
}

static char* SkipDots(char* relative)
{
    while (relative[0] == '.' && 
               ((relative[1] == '/') || (relative[1] == '\\')))
          relative += 2;
          
    if ((relative[0] == '.') && (relative[1] == '.'))
    {
       relative += 2;     
       if ((relative[0] == '/') || (relative[0] == '\\'))
          relative++;
    }
    else
       return NULL;     
    
    while (relative[0] == '.' && 
               ((relative[1] == '/') || (relative[1] == '\\')))
          relative += 2;

    return relative;
}

BOOL MakeAbsolutePath(char* workingdir, char* relative, char* result)
{
    char* rel = relative;
    char* temp;
      
    if ((relative[0] == '/') || (relative[0] == '\\'))
    {
       strcpy(result, relative); /* path already absolute */
       return TRUE;
    }
    
    strcpy(result, workingdir);
    if (workingdir[0] == 0) return FALSE;
    
    temp = strchr(result, 0);
    if ((*(temp-1) != '/') && (*(temp-1) != '\\'))
       strcat(result, "\\");     
      
    while ((temp = SkipDots(rel)) != NULL)
    {
       if (!GetPreviousDir(result)) 
          return FALSE;     
          
       rel = temp;   
    }

    if ((strlen(result) + strlen(rel)) > MAXPATH)
       return FALSE;
       
    strcat(result, rel);
    
    return TRUE;
}
