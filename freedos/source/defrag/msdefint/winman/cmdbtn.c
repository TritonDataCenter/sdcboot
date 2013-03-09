/*    
   Cmdbtn.c - command buttons.
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

#include <dos.h>

#include "control.h"
#include "cmdbtn.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawCmdButton(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;

  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, control->backcolor, button->caption, 
        FALSE, TRUE);
}

static void OnEntering(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;

  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, button->highcolor, button->caption, 
        TRUE, TRUE);

  control->active = TRUE;
}

static void OnLeaving(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;

  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, control->backcolor, button->caption, 
        control->DefaultControl, TRUE);

  control->active = FALSE;
}

#define DOWN 0
#define UP   1

static int FakeMousePress(struct Control* control)
{
  int i, wait=TRUE, x, y, status;

  struct CommandButton* button = (struct CommandButton*)control->ControlData;

  ClearMouse();
  DrawButton(control->posx+1, control->posy, button->len,
        control->forcolor, button->highcolor, button->caption, 
        TRUE, FALSE);

  status = DOWN;
  
  while (wait)
        for (i = 0; i < GetNumSupportedButtons(); i++)
            if (MouseReleased(i))
               wait = FALSE;
            else
            {
               WhereMouse(&x, &y);

               if ((x < control->posx) || (x > control->posx+button->len) ||
                   (y < control->posy) || (y > control->posy))
               {
                  if (status == DOWN)
                  {
                     status = UP;
                     DrawButton(control->posx, control->posy, button->len,
                                control->forcolor, button->highcolor, button->caption,
                                TRUE, TRUE);
                  }
               }
               else if (status == UP)
               {
                  status = DOWN;

                  DrawButton(control->posx+1, control->posy, button->len,
                             control->forcolor, button->highcolor, button->caption,
                             TRUE, FALSE);
               }
            }

/*
  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, button->highcolor, button->caption, 
        TRUE, TRUE);
*/
  return status == DOWN;
}

static void FakeKeyboardPress(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;

  DrawButton(control->posx+1, control->posy, button->len,
        control->forcolor, button->highcolor, button->caption, 
        TRUE, FALSE);

  delay(100);

  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, button->highcolor, button->caption, 
        TRUE, TRUE);
}

static void OnDefault(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;
  
  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, control->backcolor, button->caption, 
        TRUE, TRUE);
}

static void OnUnDefault(struct Control* control)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;
  
  DrawButton(control->posx, control->posy, button->len,
        control->forcolor, control->backcolor, button->caption, 
        FALSE, TRUE);
}

static int LookAtEvent(struct Control* control, int event)
{
  struct CommandButton* button = (struct CommandButton*)control->ControlData;
  
  switch (event)
  {
    case MSLEFT:
    case MSRIGHT:
    case MSMIDDLE:
    if (PressedInRange(control->posx, control->posy, 
             control->posx + button->len-1, control->posy))
    {
       if (control->SecondPass)
       {
          return (FakeMousePress(control)) ? LEAVE_WINDOW: 
                    CONTROL_ACTIVATE;
       }
       else
          return REQUEST_AGAIN;
    }
    return NOT_ANSWERED;
    
    case ENTERKEY:
    if (!control->active && !control->DefaultControl) 
       return NOT_ANSWERED;
    
    FakeKeyboardPress(control);   
    return LEAVE_WINDOW;

    default:
    return NOT_ANSWERED;
  }
}

struct Control CreateCommandButton(struct CommandButton* button, 
               int forcolor, int backcolor, 
               int posx, int posy, 
               int Default, int beforedefault,
               int MustBeTab)
{
   struct Control result;

   InitializeControlValues(&result);

   result.forcolor  = forcolor;
   result.backcolor = backcolor;
   result.posx      = posx;
   result.posy      = posy;

   result.CanEnter      = TRUE;
   result.OnRedraw      = DrawCmdButton;
   result.OnEnter       = OnEntering;
   result.OnLeave       = OnLeaving;
   result.OnEvent       = LookAtEvent;
   result.OnNormalDraw  = DrawCmdButton;
   result.HandlesEvents = TRUE;

   result.DefaultControl = Default;
   result.BeforeDefault  = beforedefault;
   result.OnDefault      = OnDefault;
   result.OnUnDefault    = OnUnDefault;
   result.MustBeTab      = MustBeTab;

   result.ControlData = (void*) button;

   return result;
}

