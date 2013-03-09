/*
   LFN.c - long file name functions.
   Copyright (C) 2000 Imre Leber

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

#include "../../misc/bool.h"
#include "../header/rdwrsect.h"
#include "../header/direct.h"

unsigned char CalculateSFNCheckSum(struct DirectoryEntry* entry)
{
   short i;
   unsigned char result = 0;

   for (i = 0; i < 8; i++)
       result = ((result & 1) ? 0x80 : 0) + (result >> 1) +
                 entry->filename[i];
                 
   for (i = 0; i < 3; i++)
       result = ((result & 1) ? 0x80 : 0) + (result >> 1) +
                 entry->extension[i];
   
   return result;
}
