/*
   Slctbtn.c - selection buttons.
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
#include "control.h"
#include "slctbtn.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawSlctButton(struct Control* control)
{
  struct SelectButton* button = (struct SelectButton*) control->ControlData;

  DrawSelectionButton(control->posx, control->posy,
            control->forcolor, control->backcolor,
            button->selected, button->Caption);
}

/* Function called by other selection buttons. */
static void OnUnSelect(struct Control* control)
{
  struct SelectButton* button = (struct SelectButton*) control->ControlData;

  if (button->selected) 
  {
     button->selected = FALSE;
     DrawSlctButton(control);
  }
}

static void OnEntering(struct Control* control)
{
  struct SelectButton* button = (struct SelectButton*) control->ControlData;

  DrawSelectionButton(control->posx, control->posy,
            button->HighColor, control->backcolor,
            button->selected, button->Caption);

  control->active = TRUE;
}

static void OnLeaving(struct Control* control)
{
  struct SelectButton* button = (struct SelectButton*) control->ControlData;

  DrawSelectionButton(control->posx, control->posy,
            control->forcolor, control->backcolor,
            button->selected, button->Caption);

  control->active = FALSE;
}

static void BroadCastSelection(struct Control* control)
{
  int i;
  struct SelectButton* button = (struct SelectButton*) control->ControlData;
  struct SelectButton* button1;

  for (i = 0; i < button->AmInGroup; i++)
      if (i != button->position)
      {

    button1 = (struct SelectButton*) button->others[i].ControlData;
    
    if (button1->selected) button1->ClearSelection(&button->others[i]);
      }
}                          

static int LookAtEvent(struct Control* control, int event)
{
  struct SelectButton* button = (struct SelectButton*)control->ControlData;

  switch (event)
  {
    case MSLEFT:
    case MSRIGHT:
    case MSMIDDLE:
    if (PressedInRange(control->posx, control->posy, 
             control->posx + button->PushLength-1, 
             control->posy))
    {
       BroadCastSelection(control);
       button->selected = TRUE;
       DrawSlctButton(control);
       return CONTROL_ACTIVATE;
    }
    return NOT_ANSWERED;
    
    case SPACEKEY:
    if (!control->active) return NOT_ANSWERED;

        BroadCastSelection(control);
        button->selected = TRUE;
        DrawSlctButton(control);
        return CONTROL_ACTIVATE;

    default:
    return NOT_ANSWERED;
  }
}

struct Control CreateSelectionButton(struct SelectButton* button, 
                 int forcolor, int backcolor, 
                 int posx, int posy)
{
   struct Control result;

   InitializeControlValues(&result); 
   
   result.forcolor  = forcolor;
   result.backcolor = backcolor;
   result.posx      = posx;
   result.posy      = posy;

   result.CanEnter      = TRUE;
   result.HandlesEvents = TRUE;
   result.OnRedraw      = DrawSlctButton;
   result.OnEnter       = OnEntering;
   result.OnLeave       = OnLeaving;
   result.OnEvent       = LookAtEvent;
   result.OnNormalDraw  = DrawSlctButton;

   result.ControlData    = (void*) button;
   button->ClearSelection = OnUnSelect;

   return result;
}

