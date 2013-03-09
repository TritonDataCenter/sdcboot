/*    
   drvsltbx.c - drive selection box.
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
#include <string.h>

#include "control.h"
#include "controls.h"
#include "vscrctrl.h"
#include "lolstbx.h"
#include "drvsltbx.h"

#include "..\screen\screen.h"
#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\..\misc\bool.h"

#include "..\..\misc\misc.h"

static int AmOfEntries (struct Control* control)
{
  struct VerticalScrollControl* VBox = 
                    (struct VerticalScrollControl*) control->ControlData;
  struct LowListBox* LBox = (struct LowListBox*) VBox->ControlData;
  struct DriveSelectionBox* box = 
                               (struct DriveSelectionBox*) LBox->ControlData;

  return strlen(box->Drives);
}

static void DrawEntry(int x, int y, char drive, int len,
                      int forcolor, int backcolor, int BarColor)
{
   char* buf = "[-?-]";
   
   buf[2] = drive;

   DrawSequence(x, y, len, ' ', backcolor, BarColor);

   DrawText(x + (len / 2) - 3, y, buf, forcolor, backcolor);
}

static void PrintEntry(struct Control* control, int top, int index)
{
  struct VerticalScrollControl* VBox = 
                    (struct VerticalScrollControl*) control->ControlData;
  struct LowListBox* LBox = (struct LowListBox*) VBox->ControlData;
  struct DriveSelectionBox* box = 
                               (struct DriveSelectionBox*) LBox->ControlData;

  DrawEntry(control->posx+1,
            control->posy+index-top+1, 
            box->Drives[index],
            VBox->xlen-2,
            box->ForColor, control->backcolor, control->backcolor);
}

static void SelectEntry(struct Control* control, int top, int index)
{
  struct VerticalScrollControl* VBox = 
                    (struct VerticalScrollControl*) control->ControlData;
  struct LowListBox* LBox = (struct LowListBox*) VBox->ControlData;
  struct DriveSelectionBox* box = 
                               (struct DriveSelectionBox*) LBox->ControlData;

  DrawEntry(control->posx+1,
            control->posy+index-top+1, 
            box->Drives[index],
            VBox->xlen-2,
            box->HighColor, VBox->BorderBackC, VBox->BorderBackC);

  VBox->AdjustScrollBar(control, index, AmOfEntries(control));
}

static void LeaveEntry(struct Control* control, int top, int index)
{
  struct VerticalScrollControl* VBox = 
                    (struct VerticalScrollControl*) control->ControlData;
  struct LowListBox* LBox = (struct LowListBox*) VBox->ControlData;
  struct DriveSelectionBox* box = 
                               (struct DriveSelectionBox*) LBox->ControlData;

  DrawEntry(control->posx+1,
            control->posy+index-top+1, 
            box->Drives[index],
            VBox->xlen-2,
            control->backcolor, VBox->BorderBackC, VBox->BorderBackC);
}

struct Control CreateDriveSelectionBox(struct DriveSelectionBox* box,
                                       struct LowListBox* LBox,
                                       struct VerticalScrollControl* VBox,
                                       int posx, int posy,
                                       int forcolor, int backcolor)
{
     struct Control result;

     result = CreateLowListBox(LBox, VBox, posx, posy, forcolor, backcolor);

     LBox->ControlData = (void*) box;

     LBox->AmOfEntries = AmOfEntries;
     LBox->PrintEntry  = PrintEntry;
     LBox->SelectEntry = SelectEntry;
     LBox->LeaveEntry  = LeaveEntry;
     LBox->EnterEntry  = SelectEntry;

     GetDriveNames(box->Drives);
     
     return result;
}
