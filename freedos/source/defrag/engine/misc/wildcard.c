/*
   Wilcard.c - wild card matching.
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

#include "../../misc/bool.h"

#include "fte.h"


static BOOL MatchFilePart(char* joker, char* part, int partlen);

/************************************************************
**                       MatchWildCard
*************************************************************
** This function tries to match a filename and extension against
** a certain wildcard.
**
** The filename must be a 8 byte and the extension must be a 3
** byte buffer.
**
** The wild cards are interpreted as:
**      x.y
**
** If there is a '.' then
**    If it is the last char in the string then the extension
**    must be empty, and the filename is matched.
**    If it is not the char in the string the filename and
**    extension are matched.
**
** If there is no '.' then it just matches the file name.
*************************************************************/

BOOL MatchWildcard(char* joker, char* filename, char* extension)
{
   char* dot = strchr(joker, '.');
   char first[9];

   assert(joker && filename && extension); 
    
   if (dot)
   {
      if (dot[1] == 0)
      {
         if (memcmp(extension, "   ", 3) != 0)
            RETURN_FTEERROR(FALSE);	

	 strcpy(first, joker);
         first[strlen(first)] = 0;
         return MatchFilePart(first, filename, 8);
      }
      else
      {
	 memcpy(first, joker, (int) (dot-joker));
	 first[(int)(dot-joker)] = 0;

         return (MatchFilePart(first, filename, 8) &&
                 MatchFilePart(dot+1, extension, 3));
      }
   }
   else
   {
      return MatchFilePart(joker, filename, 8);
   }
}

/************************************************************
**                       MatchWildCard
*************************************************************
** This function tries to match a filename or an extension against
** a certain wildcard.
**
** The wild card consists of characters interpreted as:
**        If there is an * then everything until the end of the
**        string is matched.
**
**        If there is an ? then any characters is matched.
**
**        Any other character must have the same char in the
**        part.
*************************************************************/


static BOOL MatchFilePart(char* joker, char* part, int partlen)
{
   int i, len = strlen(joker);
    
   assert(joker && part); 

   if (strlen(joker) > (size_t)partlen)
      RETURN_FTEERROR(FALSE);	

   for (i = 0; i < len; i++)
   {
       if (joker[i] == '*')
          return TRUE;
          
       if ((joker[i] != '?') && (toupper(joker[i]) != toupper(part[i])))
          RETURN_FTEERROR(FALSE);	
   }

   for (; i < partlen; i++)
       if (part[i] != ' ')
          RETURN_FTEERROR(FALSE);	

   return TRUE;
}
