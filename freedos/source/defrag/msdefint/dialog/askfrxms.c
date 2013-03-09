/*    
   AskFrXMS.c - dialog that asks the user to install an XMS driver.
   Copyright (C) 2004 Imre Leber

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

#include "..\..\misc\bool.h"

#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\controls.h"
#include "..\winman\control.h"
#include "..\winman\cmdbtn.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define BUTTON_LENGTH 10

#define DIALOG_WIDTH    67
#define DIALOG_HEIGHT   10 
#define DIALOG_Y	((MAX_Y / 2) - (DIALOG_HEIGHT / 2))
#define DIALOG_X 	((MAX_X / 2) - (DIALOG_WIDTH  / 2))

#define TEXT_X		DIALOG_X+2
#define TEXT_Y		DIALOG_Y+2

#define CONTINUEBUTTON_X DIALOG_X + (DIALOG_WIDTH / 2) + 1
#define CONTINUEBUTTON_Y DIALOG_Y+8 

#define EXITBUTTON_X	DIALOG_X + (DIALOG_WIDTH / 2) - BUTTON_LENGTH - 2
#define EXITBUTTON_Y    CONTINUEBUTTON_Y

#define EXITBUTTON 4

#define CNTRLCNT 6

static struct Control Controls[CNTRLCNT];

static struct CommandButton ContinueButton = {BUTTON_LENGTH, "Continue", BUTTONHIGHLIGHTCOLOR};
static struct CommandButton ExitButton = {BUTTON_LENGTH, "Exit", BUTTONHIGHLIGHTCOLOR};

static struct Window askdialog = 
	{
	  DIALOG_X,
	  DIALOG_Y,
	  
	  DIALOG_WIDTH,
	  DIALOG_HEIGHT,
	  DIALOGWARNINGBACK,	
	  DIALOGWARNINGFOR,
	    
	  " No XMS driver found ",
	    
	  Controls,
	  CNTRLCNT  	    
	};

static void Initialise(void)
{
    Controls[0] = CreateLabel("You have not configured your extended memory as XMS.",
			      DIALOGWARNINGFOR, DIALOGWARNINGBACK,    
			      TEXT_X, TEXT_Y);
    
    Controls[1] = CreateLabel("This will lead to very poor performance in defragmentation speed",
			      DIALOGWARNINGFOR, DIALOGWARNINGBACK,    
			      TEXT_X, TEXT_Y+1);
    
    Controls[2] = CreateLabel("Defrag will run with this configuration, albeit very slow",
			      DIALOGWARNINGFOR, DIALOGWARNINGBACK,    
	                      TEXT_X, TEXT_Y+3);
    
    Controls[3] = CreateLabel("Are you sure you wish to continue?",
			      DIALOGWARNINGFOR, DIALOGWARNINGBACK,    
			      TEXT_X, TEXT_Y+4);
    
    Controls[4] = CreateCommandButton(&ExitButton, 
				      BUTTONFORCOLOR, BUTTONBACKCOLOR, 
				      EXITBUTTON_X, EXITBUTTON_Y,
				      TRUE, FALSE, FALSE);
    
    Controls[5] = CreateCommandButton(&ContinueButton, 
				      BUTTONFORCOLOR, BUTTONBACKCOLOR, 
				      CONTINUEBUTTON_X, CONTINUEBUTTON_Y,
				      FALSE, FALSE, FALSE);
     
}

int AskForXMSManager(void)
{
    int retVal;
    static BOOL initialised = FALSE;
    
    PushHelpIndex(SPEED_INDEX);
    
    if (!initialised)
    {
	Initialise();
	initialised = TRUE;
    }	
    
    SetStatusBar(RED, WHITE, "                                            ");
    SetStatusBar(RED, WHITE, " No XMS manager.");
    
    OpenWindow(&askdialog);
    retVal = ControlWindow(&askdialog);
    CloseWindow();
    
    PopHelpIndex();
    
    return retVal == EXITBUTTON;
}
