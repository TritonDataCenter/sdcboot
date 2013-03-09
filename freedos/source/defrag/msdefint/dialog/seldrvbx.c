/*    
   Seldrvbx.c - Drive selection box.
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

#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "..\screen\screen.h"
#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\..\misc\misc.h"
#include "..\..\modlgate\defrpars.h"

#include "..\winman\control.h"
#include "..\winman\controls.h"
#include "..\winman\cmdbtn.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\lolstbx.h"
#include "..\winman\vscrctrl.h"

#include "..\..\misc\bool.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define DIALOG_X_LEN  31
#define DIALOG_Y_LEN  11 
#define DIALOG_X      (MAX_X / 2) - (DIALOG_X_LEN / 2)
#define DIALOG_Y      (MAX_Y / 2) - (DIALOG_Y_LEN / 2)
#define TEXT_X        DIALOG_X+3
#define BUTTON_LENGTH 10
#define SELBOX_LENGTH 15
#define SELBOX_HEIGHT  7
#define SELBOX_X      DIALOG_X + 2
#define SELBOX_Y      DIALOG_Y + 4
#define BUTTON_X      DIALOG_X + SELBOX_LENGTH + 4
#define BUTTON_Y1     DIALOG_Y + 5
#define BUTTON_Y2     DIALOG_Y + 8
#define DRIVE_X       SELBOX_X + (SELBOX_LENGTH / 2) - 2

#define BARFORCOLOR   BLUE
#define BARBACKCOLOR  DIALOGBACKCOLOR

#define DRIVEBOX     1
#define OKBUTTON     2
#define CANCELBUTTON 3

#define AMOFCONTROLS 4

static struct Control controls[AMOFCONTROLS];

extern struct CommandButton OkButton;
extern struct CommandButton CancelButton;

static struct Window DriveBox = {DIALOG_X, DIALOG_Y,
                                 DIALOG_X_LEN, DIALOG_Y_LEN,
                                 DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                 " Select drive ",
                                 controls,
                                 AMOFCONTROLS};


static char DrvBuffer[27];

static struct DriveSelectionBox DBox = {WHITE, BLACK, DrvBuffer};
static struct LowListBox LBox = {0, 0, NULL, NULL, NULL, NULL, NULL, NULL};
static struct VerticalScrollControl VBox = 
              {SELBOX_LENGTH, SELBOX_HEIGHT, /* xlen, ylen         */
               0,0,0,                        /* BAR: Y1, Y2, X     */
               DIALOGBACKCOLOR,              /* Border back color. */
               BLUE,                         /* Border for color.  */
               0,                            /* Answers events.    */
               0,                            /* SavedEntry         */
               NULL,                         /* ControlData        */
               NULL,                         /* OneUp              */
               NULL,                         /* OneDown            */
               NULL,                         /* OnClick            */
               NULL,                         /* EntryChosen        */
               NULL,                         /* HandleEvent        */
               NULL,                         /* DrawControl        */
               NULL,                         /* OnEntering         */
               NULL,                         /* OnLeaving          */
               NULL};                        /* AdjustScrollBar    */


static void Initialize(void)
{
  controls[0] = CreateLabel("Choose drive to optimize.",
                            DIALOGFORCOLOR, DIALOGBACKCOLOR,
                            TEXT_X, DIALOG_Y + 2);
  
  controls[1] = CreateDriveSelectionBox(&DBox,
                                        &LBox,
                                        &VBox,
                                        SELBOX_X, SELBOX_Y,
                                        BLUE, DIALOGBACKCOLOR);

  
  controls[2] = CreateCommandButton(&OkButton,
                                    BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                    BUTTON_X, BUTTON_Y1, 
                                    TRUE, FALSE, FALSE);

  controls[3] = CreateCommandButton(&CancelButton,
                                    BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                    BUTTON_X, BUTTON_Y2, 
                                    FALSE, TRUE, FALSE);
}

char ShowDriveSelectionBox(void)
{
  int   control, i, len, index = 0;
  char  drive;
  
  static int Initialized = FALSE;

  PushHelpIndex(SELECT_DRIVE_INDEX);
  
  if (!Initialized)
  {
     Initialized = TRUE;
     Initialize();
  }
  
  drive = GetOptimizationDrive();
  len   = strlen(DrvBuffer);
  
  for (i = 0; i < len; i++)
  {
      if (drive == DrvBuffer[i])
      {
         index = i;
         break;
      }
  }

  if (index)
     if (index - LBox.top < SELBOX_HEIGHT - 2)
        LBox.CursorPos = index - LBox.top;
     else 
     {
        LBox.CursorPos = SELBOX_HEIGHT - 2;
        LBox.top       = index - (SELBOX_HEIGHT - 2);
     }
  else
  {
     LBox.CursorPos = 0;
     LBox.top       = 0;
  }

  OpenWindow(&DriveBox);
  control = ControlWindow(&DriveBox);
  CloseWindow();
        
  PopHelpIndex();
  
  if ((control == OKBUTTON) || (control == DRIVEBOX)) 
     return DrvBuffer[LBox.top+LBox.CursorPos];
  
  return 0;
}
