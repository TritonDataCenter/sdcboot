/*    
   Eventbtn.c - event buttons.
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
#include "controls.h"
#include "cmdbtn.h"
#include "eventbtn.h"

static void DrawEventButton(struct Control* control)
{
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   ebtn->control->OnRedraw(ebtn->control);
}

static void OnEntering(struct Control* control)
{
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   ebtn->control->OnEnter(ebtn->control);
}

static void OnLeaving(struct Control* control)
{
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   ebtn->control->OnLeave(ebtn->control);
}

static void OnDefault(struct Control* control)
{
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   ebtn->control->OnDefault(ebtn->control);
}

static void OnUnDefault(struct Control* control)
{
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   ebtn->control->OnUnDefault(ebtn->control);
}

static int LookAtEvent(struct Control* control, int event)
{
   int    result;
   struct EventButton* ebtn = (struct EventButton*) control->ControlData;

   result = ebtn->control->OnEvent(ebtn->control, event);

   if (result == LEAVE_WINDOW) return ebtn->OnPress(ebtn->EventData);
   return result;
}

struct Control CreateEventButton(struct EventButton* event,
                                 struct CommandButton* button,
                                 struct Control* control,
				 int forcolor, int backcolor,
				 int posx, int posy,
				 int Default, int beforedefault,
				 int MustBeTab)
{
   struct Control result;

   *control = CreateCommandButton(button, forcolor, backcolor,
                                  posx, posy, Default, beforedefault,
                                  MustBeTab);


   result = CreateCommandButton(button, forcolor, backcolor,
                                posx, posy, Default, beforedefault,
                                MustBeTab);

   result.OnRedraw      = DrawEventButton;
   result.OnEnter       = OnEntering;
   result.OnLeave       = OnLeaving;
   result.OnEvent       = LookAtEvent;
   result.OnNormalDraw  = DrawEventButton;
   result.OnDefault     = OnDefault;
   result.OnUnDefault   = OnUnDefault;

   result.ControlData = (void*) event;
   event->control     = control;

   return result;
}


