/*    
   Scrlog.c - screen logger.

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

#include <string.h>

#include "logman.h"
#include "lologman.h"

#include "..\screen\screen.h"
#include "..\event\event.h"

static void NextLine(void);

static int  ScreenLine = 0;

void ScrLogPrint(char* string)
{
     int i, len, col = 1;

     len = strlen(string);

     NextLine();

     for (i = 0; i < len; i++)
     {
         if (col == 81)
         {
            NextLine();
            col = 1;
         }

         if (string[i] == '\n')
         {
            NextLine();
            col = 1;
         }
         else
         {
            col++;
            PrintChar1(string[i]);
         }
     }
}

void ShowScreenLog(void)
{
     ClearEvent();
     ShowScreenPage(LOGPAGE);
     while (GetEvent() == 0);
     ShowScreenPage(MAINPAGE);
}

static void NextLine(void)
{
   if (ScreenLine == 24)
      Scroll1Up();
   else
      ScreenLine++;

   gotoxy1(0, ScreenLine);
}


