/*    
   Extevent.c - external events.
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

#include "..\event\event.h"
#include "..\dialog\msgbxs.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"
#include "..\logman\logman.h"

#include "..\dialog\hlpdialg.h"

void CheckExternalEvent(int event)
{
     static int Before = FALSE;

     /* Recursion not allowed */
     if (Before) return; else Before = TRUE;
     
     SetClipIndex(1);

     switch (event)
     {
       case F1:
            ShowHelpSystem();
            break;

       case CTRL_F2:
       case CTRL_CURSOR_LEFT:
       case CTRL_CURSOR_RIGHT:
            HideMouse();
            ShowScreenLog();
            ShowMouse();
            break;
       
       case MSMIDDLE:
       case MSRIGHT:
       case MSLEFT:
            if (PressedInRange(68, 1, 80, 1))
               ShowHelpSystem();
            break;
     }
     
     SetClipIndex(0);
     
     Before = FALSE;
}
