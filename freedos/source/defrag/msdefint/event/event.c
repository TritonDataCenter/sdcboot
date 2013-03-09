/*    
   Event.c - general event service.
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

#include <time.h>

#include "..\mouse\mouse.h"
#include "..\keyboard\keyboard.h"
#include "..\..\misc\bool.h"

#include "event.h"

/*
   Return any event that occured.
*/
static int MsPressed = FALSE;

unsigned GetEvent()
{
    static int button;
    static int RememberButton;
       
    int i, released;
    static clock_t PressedTick=0;
    static time_t PressedTime;
    
    if (AltKeyPressed())    
       return ALTKEY;
    if (KeyPressed())       
       return ReadKey();
    if ((button = AnyButtonPressed()) != -1) 
    {
       time(&PressedTime);
       MsPressed = TRUE;
       RememberButton = button;
       return button+MSEVENTBASE;
    }

    if (MsPressed)
    {
       for (i = 0; i < 3; i++)
           if (MouseReleased(i))
              released = TRUE;
       
       if (!released)
       {
          clock_t tick;     
          time_t elapsed;
          time(&elapsed);
          if (elapsed != PressedTime)
          {
             tick = clock();
             if (tick != PressedTick)
             {        
                PressedTick = tick; 
                return RememberButton+MSRPTBASE;
             }
          }
       }
       else
       {
          MsPressed = FALSE;
       }
    }

    return 0;
}

/*
    Clears all pending events.
*/

void ClearEvent()
{
    MsPressed = FALSE;    
    ClearKeyboard();
    ClearMouse();
}
