/*    
   checkbox.c - check boxes
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
/*- NOT TESTED YET - NOT TEStED YET - NOT TESTED YET - NOT TESTED YET -*/

#include "control.h"
#include "checkbox.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawCheckBox(int x, int y, char* caption, int checked,
			 int forcolor, int backcolor)
{
   if (checked)
      DrawText(x, y, "[X]", forcolor, backcolor);
   else
      DrawText(x, y, "[ ]", forcolor, backcolor);

   DrawText(x+4, y, caption, forcolor, backcolor);
}

static void OnDrawBox(struct Control* control)
{
  struct CheckBox* box = (struct CheckBox*) control->ControlData;

  DrawCheckBox(control->posx, control->posy, box->caption, box->Checked,
	       control->forcolor, control->backcolor);
}

static void OnEntering(struct Control* control)
{
  struct CheckBox* box = (struct CheckBox*) control->ControlData;

  DrawCheckBox(control->posx, control->posy, box->caption, box->Checked,
	       control->forcolor, control->backcolor);

  control->active = TRUE;
}

static void OnLeaving(struct Control* control)
{
  struct CheckBox* box = (struct CheckBox*) control->ControlData;

  DrawCheckBox(control->posx, control->posy, box->caption, box->Checked,
	       control->forcolor, control->backcolor);

  control->active = FALSE;
}

static int LookAtEvent(struct Control* control, int event)
{
  struct CheckBox* box = (struct CheckBox*)control->ControlData;

  switch (event)
  {
    case MSLEFT:
    case MSRIGHT:
    case MSMIDDLE:
	 if (PressedInRange(control->posx, control->posy, 
			    control->posx + box->PushLength-1,
			    control->posy))
	 {
	    box->Checked = TRUE;
	    DrawCheckBox(control->posx, control->posy, box->caption,
			 TRUE, control->forcolor, control->backcolor);
	    return CONTROL_ACTIVATE;
	 }
	 return NOT_ANSWERED;
	 
    case SPACEKEY:
	 if (!control->active) return NOT_ANSWERED;

	 box->Checked = !box->Checked;
	 DrawCheckBox(control->posx, control->posy, box->caption,
		      box->Checked, control->forcolor, control->backcolor);
	 return CONTROL_ACTIVATE;

    default:
	 return NOT_ANSWERED;
  }
}

struct Control CreateCheckBox(struct CheckBox* box,
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
   result.OnRedraw      = OnDrawBox;
   result.OnEnter       = OnEntering;
   result.OnLeave       = OnLeaving;
   result.OnEvent       = LookAtEvent;
   result.OnNormalDraw  = OnDrawBox;

   result.ControlData    = (void*) box;

   return result;
}
