/*    
   Cmpfuncs.c - compare entry functions.

   Copyright (C) 2000, 2002 Imre Leber

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

#include "fte.h"


/*
**  Return  1 if entry1 > entry2
**  Return -1 if entry1 < entry2
**  Return  0 if entry1 = entry2
*/

int CompareNames(struct DirectoryEntry* entry1,
		 struct DirectoryEntry* entry2)
{
    int result;
    char char1, char2;

    char1 = entry1->filename[0];
    char2 = entry2->filename[0];
    
    if (char1 == 0x05) entry1->filename[0] = 0xe5;
    if (char2 == 0x05) entry2->filename[0] = 0xe5;

    result = memcmp(entry1->filename, entry2->filename, 8);

    entry1->filename[0] = char1;
    entry2->filename[0] = char2;

    if (result > 0) return  1;
    if (result < 0) return -1;
    return 0;
}

int CompareExtension(struct DirectoryEntry* entry1,
		     struct DirectoryEntry* entry2)
{
    int result;

    result = memcmp(entry1->extension, entry2->extension, 3);

    if (result > 0) return  1;
    if (result < 0) return -1;
    return 0;
}

int CompareSize(struct DirectoryEntry* entry1,
		struct DirectoryEntry* entry2)
{
    if (entry1->filesize > entry2->filesize) return  1;
    if (entry1->filesize < entry2->filesize) return -1;
    return 0;
}

int CompareDateTime(struct DirectoryEntry* entry1,
		    struct DirectoryEntry* entry2)
{
    if (entry1->LastWriteDate.year  > entry2->LastWriteDate.year)   return  1;
    if (entry1->LastWriteDate.year  < entry2->LastWriteDate.year)   return -1;
    if (entry1->LastWriteDate.month > entry2->LastWriteDate.month)  return  1;
    if (entry1->LastWriteDate.month < entry2->LastWriteDate.month)  return -1;
    if (entry1->LastWriteDate.day   > entry2->LastWriteDate.day)    return  1;
    if (entry1->LastWriteDate.day   < entry2->LastWriteDate.day)    return -1;


    if (entry1->LastWriteTime.hours  > entry2->LastWriteTime.hours)  return  1;
    if (entry1->LastWriteTime.hours  < entry2->LastWriteTime.hours)  return -1;
    if (entry1->LastWriteTime.minute > entry2->LastWriteTime.minute) return  1;
    if (entry1->LastWriteTime.minute < entry2->LastWriteTime.minute) return -1;
    if (entry1->LastWriteTime.second > entry2->LastWriteTime.second) return  1;
    if (entry1->LastWriteTime.second < entry2->LastWriteTime.second) return -1;

    return 0;
}

int AscendingFilter (int x) {return  x;}
int DescendingFilter(int x) {return -x;}

