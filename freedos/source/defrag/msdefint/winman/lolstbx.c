/*    
   Lolstbx.c - base for list box implementations.
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
#include <time.h>

#include "control.h"
#include "controls.h"
#include "vscrctrl.h"
#include "lolstbx.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\..\misc\bool.h"

static void FillListBox(struct Control* control)
{
   int    i;
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   int AmofEntries = box->AmOfEntries(control);
   
   for (i = 0; (i < VBox->ylen-2) && (i + box->top < AmofEntries); i++)
       box->PrintEntry(control, box->top, i+box->top);
   
   if (AmofEntries) box->SelectEntry(control, box->top, 
                                     box->CursorPos+box->top);
}

static void ScrollOneDown(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   if (VBox->ylen + box->top -2 < box->AmOfEntries(control))
   {
      box->top++;
      FillListBox(control);
   }
}

static void ScrollOneUp(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   if (box->top > 0)
   {
      box->top--;
      FillListBox(control);
   }
}

static void OneUp(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   if (box->CursorPos > 0)
   {
      box->PrintEntry(control, box->top, box->top+box->CursorPos);
      box->CursorPos--;
      box->SelectEntry(control, box->top, box->CursorPos+box->top);
   }
   else
      ScrollOneUp(control);
}

static void OneDown(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;
   
   if ((box->CursorPos < VBox->ylen-3) && 
       (box->AmOfEntries(control) > box->top+box->CursorPos+1))
   {
      box->PrintEntry(control, box->top, box->top+box->CursorPos);
      box->CursorPos++;
      box->SelectEntry(control, box->top, box->CursorPos+box->top);
   }
   else
      ScrollOneDown(control);
}

static int OnClick(struct Control* control)
{
   int pos;
   static clock_t PressedTime = 0;
   clock_t now;
   static int expecting=FALSE; /* Acount for midnight activation */
   static int SavedTop=-1;
      
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;
  
   box->PrintEntry(control, box->top, box->top+box->CursorPos);

   pos = GetPressedY() - control->posy-1;  
   
   if (expecting)
   {
       if ((box->top == SavedTop) && (pos == box->CursorPos))
       {        
          expecting = FALSE;   
          now = clock();
          if (now - PressedTime < 15)
             return LEAVE_WINDOW;
       }
       else
          SavedTop = -1;     
   }
      
   if (pos < box->AmOfEntries(control))
   {       
      expecting = TRUE;     
      PressedTime = clock();
      SavedTop = box->top;
      box->CursorPos = pos;
   }
   
   box->SelectEntry(control, box->top, box->CursorPos+box->top);

   return EVENT_ANSWERED;
}

static void EntryChosen(struct Control* control, int index)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;
   int origtop = box->top;
   int count = box->AmOfEntries(control), middle;
   
   int line = (((index * 100) / (VBox->BarY2 - VBox->BarY1)) * 
               count) / 100;
   
   box->PrintEntry(control, box->top, box->top+box->CursorPos);
   
   // See wether the line is visible on screen.
   // If so put the cursor on that position.
   
   middle = (VBox->ylen-3) / 2;
   if ((count < VBox->ylen-3)||
       ((box->top < line) && (box->top + (VBox->ylen-3) < line)))
   {
      box->CursorPos = line - box->top;  
   }
   // See wether it is not on the first control page above the middle.
   if (line < middle)
   {
      box->top = 0;
      box->CursorPos = line;
   }
   // Else see if the line can be put in the middle of the control
   else if (line - middle + (VBox->ylen-3) <= count)
   {
      box->top = line - middle;
      box->CursorPos = middle;
   }
   // If not show the last entries with the cursor on the indicated line.
   else 
   {
      box->top = count - VBox->ylen-3;
      box->CursorPos = line - box->top;
   }
   
   if (origtop != box->top)
      FillListBox(control);
      
   box->SelectEntry(control, box->top, box->CursorPos+box->top);
}

static void OnEntering(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   box->EnterEntry(control, box->top, box->top+box->CursorPos);
}

static void OnLeaving(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;
   
   struct LowListBox* box = (struct LowListBox*) VBox->ControlData;

   box->LeaveEntry(control, box->top, box->top+box->CursorPos);
}

struct Control CreateLowListBox(struct LowListBox* box,
                                struct VerticalScrollControl* VBox,
                                int posx, int posy,
                                int forcolor, int backcolor)
{
   struct Control result;

   result = CreateVScrollBox(VBox, posx, posy, forcolor, backcolor);

   VBox->ControlData = (void*) box;
   
   VBox->AnswersEvents = FALSE;

   VBox->OneUp       = OneUp;
   VBox->OneDown     = OneDown;
   VBox->OnClick     = OnClick;
   VBox->EntryChosen = EntryChosen;
   VBox->DrawControl = FillListBox;
   VBox->OnEntering  = OnEntering;
   VBox->OnLeaving   = OnLeaving;

   return result;
}
