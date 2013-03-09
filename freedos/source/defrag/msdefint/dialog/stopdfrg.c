/*    
   StopDfrg.c - query the user wether to stop defragmentation.
   Copyright (C) 2002 Imre Leber

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

#include "..\..\modlgate\defrpars.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define DIALOG_X      20
#define DIALOG_Y       8
#define DIALOG_X_LEN  41
#define DIALOG_Y_LEN   8
#define TEXT_X        DIALOG_X + 3
#define BUTTON_LENGTH 10

#define BUTTON_X1     DIALOG_X + (DIALOG_X_LEN / 2) - BUTTON_LENGTH - 2
#define BUTTON_X2     DIALOG_X + (DIALOG_X_LEN / 2) + 1

#define BUTTON_Y      DIALOG_Y + 6

#define HIGHLIGHTFOR WHITE

#define AMOFCONTROLS 5

#define YESBUTTON 3

struct CommandButton YesButton =  {BUTTON_LENGTH, "Yes ",
                                   BUTTONHIGHLIGHTCOLOR};

struct CommandButton NoButton =  {BUTTON_LENGTH, "No",
                                  BUTTONHIGHLIGHTCOLOR};

static struct Control Controls[AMOFCONTROLS];

static struct Window QueryBox = {DIALOG_X, DIALOG_Y,
                                DIALOG_X_LEN, DIALOG_Y_LEN,
                                DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                "",
                                Controls,
                                AMOFCONTROLS};


static void Initialize(void)
{
     Controls[0] = CreateLabel("You have pressed the key to stop the",
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               TEXT_X, DIALOG_Y+1);

     Controls[1] = CreateLabel("defragmentation process.",
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               TEXT_X, DIALOG_Y+2);

                               
     Controls[2] = CreateLabel("Are you sure you want to stop?",
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               TEXT_X, DIALOG_Y+4);
     

     Controls[3] = CreateCommandButton(&YesButton,
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X1, BUTTON_Y, TRUE, FALSE,
                                       FALSE);
     
     Controls[4] = CreateCommandButton(&NoButton,
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X2, BUTTON_Y, FALSE, TRUE, 
                                       FALSE);
}

int QueryUserStop(void)
{
     int control, result;
     static int Initialized = FALSE;

     PushHelpIndex(USER_STOP_INDEX);
     
     if (!Initialized)
     {
        Initialize();
        Initialized = TRUE;
     }
              
     SetStatusBar(RED, WHITE, "                                            ");
     SetStatusBar(RED, WHITE, " Defragmentation process stopped.");
     
     OpenWindow(&QueryBox);
     control = ControlWindow(&QueryBox);
     CloseWindow();

     /* Interpret values. */
     if (control == YESBUTTON)
        result = TRUE;
     else
        result = FALSE;

     PopHelpIndex();
     
     return result;
}
