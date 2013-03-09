/*    
   Drawscr.c - routine to draw the main screen.
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

#include <conio.h>

#include "FTE.h"

#include "screen.h"
#include "scrmask.h"

     
/*
    Draws the main screen for defrag.
*/

void DrawScreen ()
{
     ColorScreen (LIGHTBLUE);

     DrawText (1, 1, "     Optimize                                                        F1 = help  ", BLACK, WHITE);

     SetStatusBar (RED, WHITE, "                                                                FreeDOS defrag  ");
     DelimitStatusBar (BLACK, WHITE, 62);

     DrawSingleBox(5, 19, 35, 5, WHITE, BLUE, " Status ");
     DrawSingleBox(41, 19, 35, 5, WHITE, BLUE, " Legend ");

     ClearStatusBar ();
     DrawText(6, 22, "      Elapsed time: 00:00:00", WHITE, BLUE);
//     DrawText(6, 23, "        Full Optimization", WHITE, BLUE);
     DrawMethod("Full Optimization");

     DrawText(43, 20, "\t", BLINK+BLUE, WHITE);
     DrawText(44, 20, " - used", WHITE, BLUE);
     DrawText(43, 21, "r - reading", WHITE, BLUE);
     DrawText(43, 22, "b - bad", WHITE, BLUE);
     DrawText(43, 23, "Drive ?: 1 block =   0 clusters", WHITE, BLUE);

     DrawText(61, 20, "± - unused", WHITE, BLUE);
     DrawText(61, 21, "W - writing", WHITE, BLUE);
     DrawText(61, 22, "X - unmovable", WHITE, BLUE);
}
