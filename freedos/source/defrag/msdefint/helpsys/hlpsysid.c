/*    
   Hlpsysid.c - help system indexing routines.

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

/*
   This code handles the memory for the back scroll buffer. These are
   the indexes when the user requests the previously viewed help pages.
*/


#define BACKSCROLLCOUNT 5

static int HelpSysIndexes[5];

static int IndexFillCount=0;

int GetHelpIndex(void)
{
    return HelpSysIndexes[IndexFillCount-1];
}

void RememberHelpIndex(int index)
{
    int i;

    if (IndexFillCount < BACKSCROLLCOUNT)
       HelpSysIndexes[IndexFillCount++] = index;
    else
    {
       for (i = 0; i < BACKSCROLLCOUNT-1; i++)
           HelpSysIndexes[i] = HelpSysIndexes[i+1];

       HelpSysIndexes[BACKSCROLLCOUNT-1] = index;
    }
}

int PreviousHelpIndex(void)
{
    IndexFillCount--;
    if (IndexFillCount < 1)
       IndexFillCount = 1;

    return GetHelpIndex();
}
