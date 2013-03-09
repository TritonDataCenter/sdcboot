/*    
   Scrmask.c - routines to be able to easily fill the different pieces
               on the screen.

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
   email me at:  imre.leber@vub.ac.be
*/

#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "FTE.h"

#include "screen.h"

void DrawTime (int hours, int minutes, int seconds)
{
     char   buf[9];

     buf[0] = (hours / 10) + '0';
     buf[1] = (hours % 10) + '0';
     buf[2] = ':';
     buf[3] = (minutes / 10) + '0';
     buf[4] = (minutes % 10) + '0';
     buf[5] = ':';
     buf[6] = (seconds / 10) + '0';
     buf[7] = (seconds % 10) + '0';
     buf[8] = 0;
     
     DrawText(26, 22, buf, WHITE, BLUE); 
}

void DrawBlockSize (CLUSTER size)
{
     char buf[33];
     ultoa (size, buf, 10);

     DrawText(65-strlen(buf), 23, buf, WHITE, BLUE);
}

void DrawMethod (char* method)
{
     int  pos;
     pos = 22 - (strlen(method) / 2);
     
     DrawSequence(6, 23, 30, ' ', WHITE, BLUE);
     DrawText(pos, 23, method, WHITE, BLUE);

     if ((strcmp(method, "Quick Try") == 0) ||
         (strcmp(method, "Complete quick try") == 0))
	 {
		 DrawText(6, 20, "File    2                       0%", WHITE, BLUE);
	 }
	 else
	 {
		 DrawText(6, 20, "Cluster 2                       0%", WHITE, BLUE);
	 }
}

void DrawFunctionKey(int key, char* action)
{
     char buf[50];

     sprintf(buf, "F%d = %s", key, action);

     DrawText(79-strlen(buf), 1, buf, BLACK, WHITE);
}

void SetStatusOnBar (int thiscluster, int endcluster)
{
     char buf[35];
     unsigned pos,percent; 
     
     unsigned long now, end;
     

     for (pos = 0; pos < 34; pos++) buf[pos] = 'Û';

     
     now = thiscluster;
     end = endcluster;

	 while (now > 0x7fffffffl/100)
	 	{
	 	now /=2;
	 	end /= 2;
	 	}
     
     percent = (unsigned)(100*now/end);
     pos     = (unsigned)(34*now/end);

     buf[pos] = 0;
     
     DrawText(6, 21, buf, WHITE, BLUE);

     itoa(thiscluster, buf, 10);
     DrawText(14, 20, buf, WHITE, BLUE);

     itoa((int) percent, buf, 10);
     
     if (percent >= 10)
        DrawText(36, 20, buf, WHITE, BLUE);
     else 
        DrawText(37, 20, buf, WHITE, BLUE);
}

void ClearStatusBar ()
{
     DrawStatusBar(6, 21, 34, WHITE, BLUE);
}

void DrawStatus(CLUSTER cluster, CLUSTER amountofclusters)
{
     char CursorChar;
     int pos, len, i;
     int percentage = (int) (cluster * 100 / amountofclusters);
     char buffer[33], perbuf[4];

     pos = 6 + ((percentage * 34) / 100);

     for (i = 6; i <= pos; i++)
     {
         ChangeCursorPos(i, 21);
         CursorChar = ReadCursorChar();
         if (CursorChar == '±')
         {
            DrawText(i, 21, "Û", WHITE, BLUE);
         }
     }

     sprintf(perbuf, "%d", percentage);
     len = strlen(perbuf);
     DrawText(39-len, 20, perbuf, WHITE, BLUE);

     sprintf(buffer, "%lu", cluster);
     DrawText(14, 20, buffer, WHITE, BLUE);
}

void DrawCurrentDrive (char drive)
{
     char buf[2];
     buf[1] = 0;
     buf[0] = drive;
     DrawText(49, 23, buf, WHITE, BLUE);
}
