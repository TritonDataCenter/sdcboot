/*    
   Vscrctrl.c - base for vertical scroll controls.
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
#include "vscrctrl.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawScrollBox(struct Control* control)
{
  struct VerticalScrollControl* box = 
                      (struct VerticalScrollControl*)control->ControlData;

  DrawVScrollBox(control->posx, control->posy, box->xlen, box->ylen, 
                 control->forcolor, control->backcolor, 
                 box->BorderForC, box->BorderBackC);

  box->DrawControl(control);
}

static void OnNormalDraw(struct Control* control)
{
  struct VerticalScrollControl* box = 
                      (struct VerticalScrollControl*)control->ControlData;

  box->OnLeaving(control);
}

static void OnEntering(struct Control* control)
{
  struct VerticalScrollControl* box = 
                      (struct VerticalScrollControl*)control->ControlData;

  control->active = TRUE;
  box->OnEntering(control);
}

static void OnLeaving(struct Control* control)
{
  struct VerticalScrollControl* box = 
                      (struct VerticalScrollControl*)control->ControlData;

  box->OnLeaving(control);
    
  control->active = FALSE;
}


static int LookAtEvent(struct Control* control, int event)
{
  static int DownButton = FALSE, UpButton = FALSE;       
 
  struct VerticalScrollControl* box = 
                       (struct VerticalScrollControl*)control->ControlData;
  
  switch (event)
  {
    case MSLEFT:
    case MSRIGHT:
    case MSMIDDLE:
                  DownButton = UpButton = FALSE;
                  
                  /* See wether the press was inside the box. */
                  if (PressedInRange(control->posx, control->posy,
                                     control->posx+box->xlen, 
                                     control->posy+box->ylen-1))
                  {
                     if (PressedInRange(box->BarX, box->BarY1, 
                                        box->BarX, box->BarY2)) 
                     {
                        /* See wether the up arrow has been pressed. */
                        if (GetPressedY() == box->BarY1)
                        {
                           box->OneUp(control);
                           UpButton = TRUE;
                        }
                        
                        /* See wether the down arrow has been pressed. */
                        else if (GetPressedY() == box->BarY2)
                        {
                           box->OneDown(control);
                           DownButton = TRUE;
                        }

                        /* See wether the bar has been pressed. */
                        else
                           box->EntryChosen(control, GetPressedY()-box->BarY1);
                     }
                     else if (PressedInRange(control->posx+1, 
                                             control->posy+1,
                                             control->posx+box->xlen-1,
                                             control->posy+box->ylen-2))
                          switch (box->OnClick(control))
                          {
                            case NOT_ANSWERED:
                                 return NOT_ANSWERED;
                            case LEAVE_WINDOW:
                                 return LEAVE_WINDOW;
                            default:
                                 return CONTROL_ACTIVATE;     
                          }
                  }
                  else 
                     return NOT_ANSWERED;

    case MSRPTLEFT:
    case MSRPTRIGHT:
    case MSRPTMIDLE:  
                  if (UpButton)
                  {        
                     box->OneUp(control);
                     return EVENT_ANSWERED;
                  }
                  if (DownButton)
                  {
                     box->OneDown(control);
                     return EVENT_ANSWERED;
                  }
                  return NOT_ANSWERED;
                
    case CURSORUP:
                  if (control->active) 
                  {
                     box->OneUp(control);
                     return EVENT_ANSWERED;
                  }
                  break;
    
    case CURSORDOWN:
                  if (control->active) 
                  {
                     box->OneDown(control);
                     return EVENT_ANSWERED;
                  }
                  break;

  }

  if (box->AnswersEvents) return box->HandleEvent(control, event);
  
  return NOT_ANSWERED;
}

/* Call back. */
static void AdjustScrollBar(struct Control* control, int index, 
                            int amofentries)
{
  struct VerticalScrollControl* box = 
                       (struct VerticalScrollControl*)control->ControlData;
 
  DrawStatusOnBar(box->BarX, control->posy, 
                  box->BorderBackC, box->BorderForC,
                  amofentries, box->SavedEntry+1, box->ylen, '²');

  DrawStatusOnBar(box->BarX, control->posy, 
                  box->BorderBackC, box->BorderForC,
                  amofentries, index+1, box->ylen, 'Û');

  box->SavedEntry = index;
}
                        
struct Control CreateVScrollBox(struct VerticalScrollControl* box, 
                                int posx, int posy, 
                                int forcolor, int backcolor)
{
   struct Control result;

   InitializeControlValues(&result);

   result.forcolor  = forcolor;
   result.backcolor = backcolor;
   result.posx      = posx;
   result.posy      = posy;

   result.CanEnter      = TRUE;
   result.CursorUsage   = CURSOR_SELF;
   result.OnRedraw      = DrawScrollBox;
   result.OnEnter       = OnEntering;
   result.OnLeave       = OnLeaving;
   result.OnEvent       = LookAtEvent;
   result.OnNormalDraw  = OnNormalDraw;
   result.HandlesEvents = TRUE;
   
   box->SavedEntry      = 1;
   box->AdjustScrollBar = AdjustScrollBar;
   
   box->BarY1           = posy+1;
   box->BarY2           = posy+box->ylen-2;
   box->BarX            = posx+box->xlen;

   result.ControlData = (void*) box;

   return result;
}

   
