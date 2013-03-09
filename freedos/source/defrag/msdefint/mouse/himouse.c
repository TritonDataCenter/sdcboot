/*    
   Himouse.c - higher mouse routines.
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

#include "mouse.h"
#include "..\..\misc\bool.h"
#include "..\..\environ\dosemu.h"

static int ReleasedX, ReleasedY;
static int PressedX, PressedY;
static int LockedX, LockedY;

static int LockCounter = 0;

/*
    Clears the Press and Release counts.
*/

void ClearMouse ()
{
    int i, x, y, pressreleases;

    for (i = 0; i < 3; i++)
    {
        CountButtonPresses(i, &pressreleases, &x, &y);
        CountButtonReleases(i, &pressreleases, &x, &y);
    }
}

/*
    Returns how many times a certain button was pressed since the last
    call and remember the position of the mouse cursor.
*/

int MousePressed (int button)
{
    int presses;

    CountButtonPresses (button, &presses, &PressedX, &PressedY);
    
    return presses;
}

/*
    Returns how many times a certain button was released since the last
    call and remember the position of the mouse cursor.
*/
int MouseReleased (int button)
{
    int releases;

    CountButtonReleases (button, &releases, &ReleasedX, &ReleasedY);
    WhereMouse(&ReleasedX, &ReleasedY);

    return releases;
}

/*
    Returns wether the position of the last button press is in a certain 
    range.
*/

int PressedInRange (int x1, int y1, int x2, int y2)
{
    return ((PressedX >= x1) && (PressedX <= x2) &&
            (PressedY >= y1) && (PressedY <= y2));
}

/*
    Returns wether the position of the last button release is in a certain 
    range.
*/
int ReleasedInRange (int x1, int y1, int x2, int y2)
{
    return ((ReleasedX >= x1) && (ReleasedX <= x2) &&
            (ReleasedY >= y1) && (ReleasedY <= y2));
}

/*
    Returns wether any button was pressed and returns that button or 
    -1 if no button pressed.
*/

int AnyButtonPressed ()
{
    int i;	

    for (i = 0; i < GetNumSupportedButtons(); i++)
        if (MousePressed(i)) return i;

    return -1;
}

/*
    Return the number of buttons supported.

    In DOSemu (middle mouse button flaky and not supported)
*/
int GetNumSupportedButtons()
{
    static int InDosEmu = 3;
    
    if (InDosEmu == 3)	/* Work around a bug in DOSemu */
    {
	InDosEmu = is_dosemu() != 0;	
    }
    
    return (InDosEmu) ? 2: 3;    
}

/*
    Returns the x position back where the mouse was last pressed.
*/

int GetPressedX ()
{
    return PressedX;
}

/*
    Returns the y position back where the mouse was last pressed.
*/

int GetPressedY ()
{
    return PressedY;
}

/*
    Returns the x position back where the mouse was last released.
*/
int GetReleasedX ()
{
    return ReleasedX;
}

/*
    Returns the x position back where the mouse was last pressed.
*/
int GetReleasedY ()
{
    return ReleasedY;
}

/*
     Hides the mouse so that the screen can be updated.
*/
void LockMouse(int x1, int y1, int x2, int y2)
{
     if (LockCounter == 0)
     {
        WhereMouse(&LockedX, &LockedY);

        /* Lock the mouse to it's position */
        MouseWindow(LockedX, LockedY, LockedX, LockedY);

        if ((LockedX >= x1) && (LockedX <= x2) &&
            (LockedY >= y1) && (LockedY <= y2))
        {
            HideMouse();
        }
     }

     LockCounter++;
}

/*
     Unhides the mouse.
*/
void UnLockMouse()
{
    LockCounter--;

    if (LockCounter == 0)
    {
       ShowMouse();
       MouseGotoXY(LockedX, LockedY);

       MouseWindow(1, 1, 80, 25);
    }
}

/*
     Waits until the mouse is released and then looks wether it is 
     released in a certain range.
*/

int CheckMouseRelease (int x1, int y1, int x2, int y2)
{
     int i, wait = TRUE;

     while (wait)
         for (i = 0; i < 3; i++)
             if (MouseReleased(i)) 
                wait = FALSE;

     return ReleasedInRange(x1, y1, x2, y2);
}
