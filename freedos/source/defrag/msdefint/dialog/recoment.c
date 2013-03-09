/*    
   Recoment.c - recommending optimization method dialog.

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
#include "..\winman\control.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\controls.h"

#include "..\..\misc\bool.h"

#define DIALOG_X_LEN 41
#define DIALOG_Y_LEN 10
#define DIALOG_X      ((MAX_X / 2) - (DIALOG_X_LEN / 2))
#define DIALOG_Y      ((MAX_Y / 2) - (DIALOG_Y_LEN / 2))
#define TEXT_X        DIALOG_X + 3
#define BUTTON_X1     DIALOG_X + 5
#define BUTTON_X2     DIALOG_X + 22
#define BUTTON_Y      DIALOG_Y + 8

#define BUTTON_LENGTH 15

#define AMOFCONTROLS 7

#define OPTIMIZEBUTTON 1

static struct Control controls[AMOFCONTROLS];

static struct CommandButton OptimizeButton = {BUTTON_LENGTH,
                                              "Optimize",
                                              BUTTONHIGHLIGHTCOLOR};

static struct CommandButton ConfigureButton = {BUTTON_LENGTH,
                                               "Configure",
                                               BUTTONHIGHLIGHTCOLOR};


static struct Window RecommendDialog = {DIALOG_X, DIALOG_Y,
                                        DIALOG_X_LEN, DIALOG_Y_LEN,
                                        DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                        " Recommendation ",
                                        controls,
                                        AMOFCONTROLS};


static void Initialize(void)
{
    controls[0] = CreateLabel("Recommended optimization method:",
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y+4);

    controls[1] = CreateCommandButton(&OptimizeButton,
                                      BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                      BUTTON_X1, BUTTON_Y,
                                      FALSE, FALSE, FALSE);

    controls[2] = CreateCommandButton(&ConfigureButton,
                                      BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                      BUTTON_X2, BUTTON_Y,
                                      FALSE, FALSE, FALSE);
}

int RecommendMethod(int fragmentation, char drive, char* recommendation)
{
    char buffer[40];
    char* drv = "?";
    int   posx, control;

    static int Initialized = FALSE;

    if (!Initialized)
    {
       Initialize();
       Initialized = TRUE;
    }

    SetStatusBar(RED, WHITE, "                                            ");
    SetStatusBar(RED, WHITE, " Recommending defragmentation method.");

    itoa(fragmentation, buffer, 10);
    strcat(buffer, "% of drive ");
    drv[0] = drive;
    strcat(buffer, drv);
    strcat(buffer, ": is not fragmented.");

    controls[3] = CreateLabel(buffer,
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y+2);

    posx = (DIALOG_X_LEN / 2) - (strlen(recommendation) / 2) + DIALOG_X; 
    
    controls[4] = CreateLabel(recommendation,
                              BLUE, DIALOGBACKCOLOR,
                              posx, DIALOG_Y+6);

    OpenWindow(&RecommendDialog);
    control = ControlWindow(&RecommendDialog);
    CloseWindow();

    if (control == OPTIMIZEBUTTON) 
       return TRUE;
    else
       return FALSE;
}
