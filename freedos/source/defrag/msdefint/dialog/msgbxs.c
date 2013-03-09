/*    
   Msgbxs.c - generic message boxes.
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

#include <stdlib.h>                             /* For MACRO max. */
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

#define max(a,b)        (((a) > (b)) ? (a) : (b))


#define DIALOG_HEIGHT 6
#define DIALOG_Y      (MAX_Y / 2) - (DIALOG_HEIGHT / 2)
#define TEXT_Y        DIALOG_Y + 2
#define BUTTON_Y      DIALOG_Y + 4
#define BUTTON_LENGTH 10

#define MAXCNTRLCNT 6

static struct Control Controls[MAXCNTRLCNT];

static struct CommandButton CmdButtons[]
          = {{BUTTON_LENGTH, NULL, 0},
             {BUTTON_LENGTH, NULL, 0},
             {BUTTON_LENGTH, NULL, 0},
             {BUTTON_LENGTH, NULL, 0},
             {BUTTON_LENGTH, NULL, 0}};


static struct Window GenericMsgBxs = 
                                  {0, DIALOG_Y,      /* top right          */
                                   0, DIALOG_HEIGHT, /* height of dialog   */
                                   0, 0,             /* colors             */
                                   NULL,              /* caption            */
                                   Controls,         /* controls           */
                                   0};               /* amount of controls */


static int MessageBox(char* msg, int btncount, char** buttons,
                      int forcolor, int backcolor,
                      int btnforc, int btnbackc, int btnhighc,
                      char* caption)
{
    int slen, dwidth, btnarea, dialog_x, text_x, i, btn_x, result;
    
    /* Calculate dialog width. */
    slen     = strlen(msg);
    btnarea  = ((btncount-1) * (BUTTON_LENGTH+2)) + BUTTON_LENGTH+1;
    dwidth   = max(btnarea, slen) + 4;
    dialog_x = (MAX_X / 2) - (dwidth / 2);
    
    GenericMsgBxs.caption       = caption;
    GenericMsgBxs.x             = dialog_x;
    GenericMsgBxs.xlen          = dwidth;
    GenericMsgBxs.SurfaceColor  = backcolor;
    GenericMsgBxs.FrameColor    = forcolor;
    GenericMsgBxs.AmofControls  = btncount+1;

    /* Put the text in the dialog. */
    text_x = (slen > btnarea)
           ? dialog_x + 2
           : dialog_x + ((btnarea / 2) - (slen / 2)) + 2;
    
    Controls[0] = CreateLabel(msg, forcolor, backcolor, text_x, TEXT_Y);

    /* Put the buttons on the screen. */
    btn_x = dialog_x + (dwidth / 2) - (btnarea / 2);
    
    for (i = 0; i < btncount; i++) 
    {
        CmdButtons[i].caption   = buttons[i];
        CmdButtons[i].highcolor = btnhighc;

        Controls[i+1] = CreateCommandButton(&CmdButtons[i], 
                                            btnforc, btnbackc,
                                            btn_x, BUTTON_Y,
                                            FALSE, FALSE,
                                            FALSE);
        
        btn_x += BUTTON_LENGTH + 2;
    }

    OpenWindow(&GenericMsgBxs);
    result = ControlWindow(&GenericMsgBxs);
    CloseWindow();

    return result;
}

void InformUser(char* msg)
{
     char* button[] = {"Ok"};

     MessageBox(msg, 1, button, DIALOGFORCOLOR, DIALOGBACKCOLOR,
                BUTTONFORCOLOR, BUTTONBACKCOLOR, BUTTONHIGHLIGHTCOLOR,
                " Message ");
}

int WarningBox(char* msg, int btncount)
{
     char* buttons[] = {"Ok", "Cancel"};

     return MessageBox(msg, btncount, buttons, DIALOGWARNINGFOR, DIALOGWARNINGBACK,
                       BUTTONFORCOLOR, BUTTONBACKCOLOR, BUTTONHIGHLIGHTCOLOR,
                      " Warning ");
}

int ErrorBox(char* msg, int btncount, char** buttons)
{
     return MessageBox(msg, btncount, buttons, DIALOGERRORFOR, DIALOGERRORBACK,
                       BUTTONFORCOLOR, BUTTONBACKCOLOR, BUTTONHIGHLIGHTCOLOR,
                      " Error ");
}

#define MODAL_HEIGHT 4
#define MODAL_Y      (MAX_Y / 2) - (MODAL_HEIGHT / 2)

void ShowModalMessage(char* msg)
{
     int dialog_len, dialog_x;

     dialog_len = strlen(msg) + 4;
     dialog_x   = (MAX_X / 2) - (dialog_len / 2);

     ShowDialog(dialog_x, MODAL_Y, dialog_len, MODAL_HEIGHT,
                " Message ", DIALOGFORCOLOR, DIALOGBACKCOLOR);

     StaticLabel(dialog_x + 2, MODAL_Y + 2, msg);
}
