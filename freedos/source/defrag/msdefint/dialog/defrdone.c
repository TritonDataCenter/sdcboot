/*    
   defrdone.c - defragmentation done dialog.

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

#include "dialog.h"

#include "..\screen\screen.h"
#include "..\winman\control.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\controls.h"
#include "..\winman\slctbtn.h"

#include "..\..\misc\bool.h"

#define DIALOG_X_LEN 34
#define DIALOG_Y_LEN 10
#define DIALOG_X      ((MAX_X / 2) - (DIALOG_X_LEN / 2))
#define DIALOG_Y      ((MAX_Y / 2) - (DIALOG_Y_LEN / 2))
#define TEXT_X        DIALOG_X + 4
#define FRAME_X       DIALOG_X + 2
#define BUTTON_LENGTH 10
#define BUTTON_X      DIALOG_X + (DIALOG_X_LEN / 2) - (BUTTON_LENGTH / 2)
#define BUTTON_Y      DIALOG_Y + 8
#define PUSHLEN       28
#define FRAME_HEIGHT   4
#define FRAME_WIDTH   30

#define OKBUTTON 5

#define AMOFCONTROLS 6

static struct Control controls[AMOFCONTROLS];

extern struct CommandButton OkButton;

#define DEFAULT_OPTION 1

#define AD &controls[2]

static struct SelectButton SelectButtons[] =
	  {{"Reboot the computer.", FALSE, WHITE, PUSHLEN, 0, AD, 3, NULL},
	   {"Exit defrag.", TRUE, WHITE, PUSHLEN, 1, AD, 3, NULL},
           {"Continue using defrag.", FALSE, WHITE, PUSHLEN, 2, AD, 3, NULL}};

static struct Frame ActionsFrame =
           {" You can now ", FRAME_WIDTH, FRAME_HEIGHT};


static struct Window DoneDialog = {DIALOG_X, DIALOG_Y,
                                   DIALOG_X_LEN, DIALOG_Y_LEN,
                                   DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                   "",
                                   controls,
                                   AMOFCONTROLS};

static void Initialize(void)
{
 controls[0] = CreateLabel("Your drive has been optimized.",
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           FRAME_X, DIALOG_Y+1);

 controls[1] = CreateFrame(&ActionsFrame,
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           FRAME_X, DIALOG_Y+3);

 controls[2] = CreateSelectionButton(&SelectButtons[0],
                                     DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                     TEXT_X, DIALOG_Y+4);

 controls[3] = CreateSelectionButton(&SelectButtons[1],
                                     DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                     TEXT_X, DIALOG_Y+5);

 controls[4] = CreateSelectionButton(&SelectButtons[2],
                                     DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                     TEXT_X, DIALOG_Y+6);

 controls[5] = CreateCommandButton(&OkButton,
                                   BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                   BUTTON_X, BUTTON_Y, TRUE, FALSE,
                                   FALSE);
}

int ReportDefragDone(void)
{
    static int Initialized = FALSE;

    int control, i, result;

    if (!Initialized)
    {
       Initialize();
       Initialized = TRUE;
    }
     
    SetStatusBar(RED, WHITE, "                                            ");
    SetStatusBar(RED, WHITE, " Defragmentation done.");
    
    OpenWindow(&DoneDialog);
    do {
       control = ControlWindow(&DoneDialog);
    } while (control != OKBUTTON);
    CloseWindow();

    for (i = 0; i < 3; i++)
        if (SelectButtons[i].selected)
        {
           result = i;
           SelectButtons[i].selected = FALSE;
        }

    SelectButtons[DEFAULT_OPTION].selected = TRUE;
    return result;
}
