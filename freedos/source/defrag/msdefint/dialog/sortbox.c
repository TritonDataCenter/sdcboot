/*    
   Sortbox.c - sort order dialog.
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
#include "sortbox.h"
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

#define DIALOG_X      24
#define DIALOG_Y       3
#define DIALOG_X_LEN  32
#define DIALOG_Y_LEN  18
#define TEXT_X        DIALOG_X + 2
#define BUTTON_LENGTH 10

#define FRAME_X      TEXT_X
#define FRAME_LEN    DIALOG_X_LEN - 4
#define FRAME_TEXT_X TEXT_X + 2

#define FRAME_Y1     DIALOG_Y + 4
#define FRAME_Y_LEN1 6

#define FRAME_Y2     DIALOG_Y + 11
#define FRAME_Y_LEN2 3

#define BUTTON_X1     DIALOG_X + (DIALOG_X_LEN / 2) - BUTTON_LENGTH - 2
#define BUTTON_X2     DIALOG_X + (DIALOG_X_LEN / 2) + 1

#define BUTTON_Y      DIALOG_Y + 16

#define PUSHLEN       FRAME_LEN - 2

#define HIGHLIGHTFOR WHITE

#define AMOFCONTROLS 13

#define CANCELBUTTON 12

extern struct CommandButton OkButton;
extern struct CommandButton CancelButton;

static struct Control Controls[AMOFCONTROLS];

static struct Frame SortFrame      = {" sort criterium ", FRAME_LEN, FRAME_Y_LEN1};
static struct Frame DirectionFrame = {" sort order ", FRAME_LEN, FRAME_Y_LEN2};

#define CA &Controls[4]
#define DA &Controls[9]

static struct SelectButton 
    CButtons[] = {{"unsorted", FALSE, HIGHLIGHTFOR, PUSHLEN, 0, CA, 5, NULL},
                  {"name", FALSE, HIGHLIGHTFOR, PUSHLEN, 1, CA, 5, NULL},
                  {"extension", FALSE, HIGHLIGHTFOR, PUSHLEN, 2, CA, 5, NULL},
                  {"date & time", FALSE, HIGHLIGHTFOR, PUSHLEN, 3, CA, 5, NULL},
                  {"size", FALSE, HIGHLIGHTFOR, PUSHLEN, 4, CA, 5, NULL}};

static struct SelectButton 
    DButtons[] = {{"ascending", FALSE, HIGHLIGHTFOR, PUSHLEN, 0, DA, 2, NULL},
                  {"descending", FALSE, HIGHLIGHTFOR, PUSHLEN, 1, DA, 2, NULL}};


static struct Window SortBox = {DIALOG_X, DIALOG_Y,
                                DIALOG_X_LEN, DIALOG_Y_LEN,
                                DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                " File Sort ",
                                Controls,
                                AMOFCONTROLS};


static void Initialize(void)
{
     Controls[0] = CreateLabel("Select the order to sort files",
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               TEXT_X, DIALOG_Y+1);
     
     Controls[1] = CreateLabel("in each directory",
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               TEXT_X, DIALOG_Y+2);

     Controls[2] = CreateFrame(&SortFrame, 
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               FRAME_X, FRAME_Y1);
     
     Controls[3] = CreateFrame(&DirectionFrame, 
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               FRAME_X, FRAME_Y2);

     Controls[3] = CreateFrame(&DirectionFrame, 
                               DIALOGFORCOLOR, DIALOGBACKCOLOR,
                               FRAME_X, FRAME_Y2);

     Controls[4] = CreateSelectionButton(&CButtons[0], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y1+1);

     Controls[5] = CreateSelectionButton(&CButtons[1], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y1+2);

     Controls[6] = CreateSelectionButton(&CButtons[2], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y1+3);

     Controls[7] = CreateSelectionButton(&CButtons[3], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y1+4);

     Controls[8] = CreateSelectionButton(&CButtons[4], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y1+5);
     

     Controls[9] = CreateSelectionButton(&DButtons[0], 
                                         DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                         FRAME_TEXT_X, FRAME_Y2+1);
     
     
     Controls[10] = CreateSelectionButton(&DButtons[1], 
                                          DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                          FRAME_TEXT_X, FRAME_Y2+2);

     Controls[11] = CreateCommandButton(&OkButton, 
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X1, BUTTON_Y, TRUE, FALSE,
                                       FALSE);
     
     Controls[12] = CreateCommandButton(&CancelButton, 
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X2, BUTTON_Y, FALSE, TRUE, 
                                       TRUE);
}

int GetSortOptions(struct SortDialogStruct* sortdialog)
{
     int control, i, result;
     static int Initialized = FALSE;

     PushHelpIndex(FILE_SORT_INDEX);
     
     if (!Initialized)
     {
        Initialize();
        Initialized = TRUE;
     }
              
     SetStatusBar(RED, WHITE, "                                            ");
     SetStatusBar(RED, WHITE, " Specify file order.");
     
     CButtons[GetSortCriterium()].selected = TRUE;
     DButtons[GetSortOrder()].selected     = TRUE;

     OpenWindow(&SortBox);
     control = ControlWindow(&SortBox);
     CloseWindow();

     /* Interpret values. */
     if ((control == CANCELBUTTON) || (control == -1))
        result = FALSE;
     else
        result = TRUE;
     
     for (i = 0; i < 5; i++)
         if (CButtons[i].selected) 
         {
            sortdialog->SortCriterium = i;
            CButtons[i].selected = FALSE;
            break;
         }

     for (i = 0; i < 2; i++)
         if (DButtons[i].selected) 
         {
            sortdialog->SortOrder = i;
            DButtons[i].selected = FALSE;
            break;
         }

     PopHelpIndex();
     
     return result;
}
