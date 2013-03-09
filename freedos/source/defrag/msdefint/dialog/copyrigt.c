/*    
   copyrigt.c - copyright dialog.

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

#include "..\dialog\copyrigt.h"

#include "..\screen\screen.h"
#include "..\dialog\dialog.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\cmdbtn.h"
#include "..\winman\control.h"
#include "..\winman\window.h"
#include "..\winman\controls.h"
#include "..\winman\timlabel.h"
#include "..\..\misc\bool.h"
#include "..\..\misc\version.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define DIALOG_X_LEN  70
#define DIALOG_Y_LEN  10
#define DIALOG_X      ((MAX_X / 2) - (DIALOG_X_LEN / 2))
#define DIALOG_Y      ((MAX_Y / 2) - (DIALOG_Y_LEN / 2))
#define TEXT_X        DIALOG_X + 2
#define BUTTON_LENGTH 10
#define BUTTON_X      DIALOG_X + (DIALOG_X_LEN / 2) - (BUTTON_LENGTH / 2)
#define BUTTON_Y      DIALOG_Y + 8

#define AMOFCONTROLS 7

static struct Control Controls[AMOFCONTROLS];

static char* Contributors[] = CONTRIBUTORS;

static struct TimeLabel CLabel = {Contributors, 0, 0, AMOFCONTRIBUTORS, 
                                  LONGESTCONTRIBUTOR};

extern struct CommandButton OkButton;

struct Window CopyRight = {DIALOG_X, DIALOG_Y,
                                  DIALOG_X_LEN, DIALOG_Y_LEN,
                                  DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                  "",
                                  Controls,
                                  AMOFCONTROLS};

static void Initialise(void)
{
 Controls[0] = CreateLabel("defrag " VERSION, 
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           34, DIALOG_Y+1);
 
 Controls[1] =  CreateHLine(DIALOG_X+1, DIALOG_Y+2, DIALOG_X_LEN-1, 
                            DIALOGFORCOLOR, DIALOGBACKCOLOR);

 Controls[2] = CreateLabel("Copyright 2000, 2002, 2003, 2004, 2006, 2007, 2009 Imre Leber",
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           TEXT_X, DIALOG_Y+3);

 Controls[3] = CreateLabel("contributors:",
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           TEXT_X, DIALOG_Y+5);

 Controls[4] = CreateTimeLabel(&CLabel, 
                               DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                               TEXT_X+14, DIALOG_Y+5);
/* 
 Controls[5] = CreateLabel("web site: " WEBSITE,
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           TEXT_X, DIALOG_Y+7);

 Controls[6] = CreateLabel("project hosted at: " PROJECT,
                           DIALOGFORCOLOR, DIALOGBACKCOLOR, 
                           TEXT_X, DIALOG_Y+8);
*/ 
 Controls[5] =  CreateHLine(DIALOG_X+1, DIALOG_Y+9, DIALOG_X_LEN-1,
                            DIALOGFORCOLOR, DIALOGBACKCOLOR);
 
 Controls[6] = CreateCommandButton(&OkButton, BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                   BUTTON_X, BUTTON_Y, FALSE, FALSE, FALSE);
}

void ShowCopyRight()
{
     static int Initialised = FALSE;

     PushHelpIndex(ABOUT_FREEDOS_INDEX);
     
     if (!Initialised) 
     {
        Initialise();
        Initialised = TRUE;
     }

     SetStatusBar(RED, WHITE, "                                            ");
     SetStatusBar(RED, WHITE, " Display the copyright notice.");

     OpenWindow((struct Window*)&CopyRight);
     ControlWindow((struct Window*)&CopyRight);
     CloseWindow();
     
     PopHelpIndex();
}
