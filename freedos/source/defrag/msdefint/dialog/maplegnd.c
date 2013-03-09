/*    
   Maplgnd.c - map legend.

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


#include "..\dialog\maplegnd.h"

#include "..\screen\screen.h"
#include "..\dialog\dialog.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\cmdbtn.h"
#include "..\winman\control.h"
#include "..\winman\controls.h"
#include "..\..\misc\bool.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define DIALOG_X_LEN  55
#define DIALOG_Y_LEN  13
#define DIALOG_X      (MAX_X / 2) - (DIALOG_X_LEN / 2)
#define DIALOG_Y      (MAX_Y / 2) - (DIALOG_Y_LEN / 2)
#define TEXT_X        DIALOG_X + 2
#define BUTTON_LENGTH 10
#define BUTTON_X      DIALOG_X + (DIALOG_X / 2) - (BUTTON_LENGTH / 2)
#define BUTTON_Y      DIALOG_Y + 11

#define AMOFCONTROLS 12

extern struct CommandButton OkButton;

static struct Control MapControls[AMOFCONTROLS];

static struct Window MapLegend = {DIALOG_X, DIALOG_Y,
                                  DIALOG_X_LEN, DIALOG_Y_LEN,
                                  MAGENTA, WHITE,
                                  "",
                                  MapControls,
                                  AMOFCONTROLS};

static void Initialise(void)
{
 MapControls[0] = 
            CreateLabel("disk map legend", WHITE, MAGENTA, 34, DIALOG_Y+1);
 MapControls[1] = 
            CreateHLine(DIALOG_X+1, DIALOG_Y+2, DIALOG_X_LEN-1, WHITE, MAGENTA);
 MapControls[2] = 
            CreateLabel("\t", MAGENTA+BLINK, WHITE, TEXT_X, DIALOG_Y+3);
 MapControls[3] = 
            CreateLabel(" - disk space used by files", WHITE, MAGENTA, TEXT_X+1, DIALOG_Y+3);
 MapControls[4] = 
            CreateLabel("\t", MAGENTA+BLINK, YELLOW, TEXT_X, DIALOG_Y+4);
 MapControls[5] = 
            CreateLabel(" - disk space optimized already.", WHITE, MAGENTA, TEXT_X+1, DIALOG_Y+4);
 MapControls[6] = 
            CreateLabel("± - unused disk space.", WHITE, MAGENTA, TEXT_X, DIALOG_Y+5);
 MapControls[7] = 
            CreateLabel("X - disk space used by files that will not be moved.", WHITE, MAGENTA, TEXT_X, DIALOG_Y+6);
 MapControls[8] = 
            CreateLabel("b - bad disk space (untouched by defrag).", WHITE, MAGENTA, TEXT_X, DIALOG_Y+7);
 MapControls[9] = 
            CreateLabel("r - disk space that is being read.", WHITE, MAGENTA, TEXT_X, DIALOG_Y+8);
 MapControls[10] = 
            CreateLabel("w - disk space that is being rewritten.", WHITE, MAGENTA, TEXT_X, DIALOG_Y+9);
 MapControls[11] =
            CreateCommandButton(&OkButton, BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                BUTTON_X, BUTTON_Y, FALSE, FALSE, FALSE);
}

void ShowMapLegend()
{
     static int Initialised = FALSE;

     PushHelpIndex(MAP_LEGEND_INDEX);
     
     if (!Initialised) 
     {
        Initialise();
        Initialised = TRUE;
     }

     SetStatusBar(RED, WHITE, "                                            ");
     SetStatusBar(RED, WHITE, " Show the map symbol definitions.");

     OpenWindow(&MapLegend);
     ControlWindow(&MapLegend);
     CloseWindow();
     
     PopHelpIndex();
}

