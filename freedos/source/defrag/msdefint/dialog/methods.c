/*    
   Methods.c - method dialog box.
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

#include "..\..\misc\bool.h"

#include "dialog.h"
#include "methods.h"
#include "..\screen\screen.h"
#include "..\winman\control.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\controls.h"
#include "..\winman\slctbtn.h"

#include "..\..\modlgate\defrpars.h"

#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

#define DIALOG_GRAYED_COLOR DARKGRAY

#define DIALOG_X       2
#define DIALOG_Y       1
#define DIALOG_X_LEN  76
#define DIALOG_Y_LEN  22
#define SELECTION_X   DIALOG_X + 4
#define TEXT_X        DIALOG_X + (DIALOG_X_LEN / 2) - 2
#define BUTTON_LENGTH 10

#define BUTTON_X1     DIALOG_X + (DIALOG_X_LEN / 2) - BUTTON_LENGTH - 2
#define BUTTON_X2     DIALOG_X + (DIALOG_X_LEN / 2) + 1

#define BUTTON_Y      DIALOG_Y + 20
#define PUSHLEN       (DIALOG_X_LEN / 2) - 2

#define HIGHLIGHTFOR WHITE

#define AMOFCONTROLS 20
#define AMOF_METHODS  9

static struct Control controls[AMOFCONTROLS];

#define CANCELBUTTON 19

extern struct CommandButton OkButton;
extern struct CommandButton CancelButton;

#define AD &controls[9]

static struct SelectButton
 SButtonsFull[] = 
   {{"No defragmentation",     FALSE, HIGHLIGHTFOR, PUSHLEN, 0, AD, AMOF_METHODS, NULL},
    {"Full Optimization",      FALSE, HIGHLIGHTFOR, PUSHLEN, 1, AD, AMOF_METHODS, NULL},
    {"Unfragment Files only",  FALSE, HIGHLIGHTFOR, PUSHLEN, 2, AD, AMOF_METHODS, NULL},
    {"Files first",            FALSE, HIGHLIGHTFOR, PUSHLEN, 3, AD, AMOF_METHODS, NULL},
    {"Directories first",      FALSE, HIGHLIGHTFOR, PUSHLEN, 4, AD, AMOF_METHODS, NULL},
    {"Directories with files", FALSE, HIGHLIGHTFOR, PUSHLEN, 5, AD, AMOF_METHODS, NULL},
    {"Crunch only",            FALSE, HIGHLIGHTFOR, PUSHLEN, 6, AD, AMOF_METHODS, NULL},
    {"Quick try",              FALSE, HIGHLIGHTFOR, PUSHLEN, 7, AD, AMOF_METHODS, NULL},
    {"Complete quick try",     FALSE, HIGHLIGHTFOR, PUSHLEN, 8, AD, AMOF_METHODS, NULL}};

static struct SelectButton
 SButtonsFAT32[] = 
   {{"No defragmentation",     FALSE, HIGHLIGHTFOR, PUSHLEN, 0, AD, 3, NULL},
    {"Quick try",              FALSE, HIGHLIGHTFOR, PUSHLEN, 1, AD, 3, NULL},
    {"Complete quick try",     FALSE, HIGHLIGHTFOR, PUSHLEN, 2, AD, 3, NULL}};

static struct Window MethodWin = {DIALOG_X, DIALOG_Y,
                                  DIALOG_X_LEN, DIALOG_Y_LEN,
                                  DIALOGBACKCOLOR, DIALOGFORCOLOR,
                                  " Select optimization method ",
                                  controls,
                                  AMOFCONTROLS};

static void Initialize(BOOL bForFAT32)
{
    int forcolor;

    if (bForFAT32)
        forcolor = DIALOG_GRAYED_COLOR;
    else
        forcolor = DIALOGFORCOLOR;

    controls[0] = CreateLabel("Do not perform any optimization",
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 2);        
        
    controls[1] = CreateLabel("Fully optimizes your disk",
                              forcolor, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 4);
                               
    controls[2] = CreateLabel("Unfragment files only",
                              forcolor, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 6);
    
    controls[3] = CreateLabel("Move all files to the front",
                              forcolor, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 8);
                              
    controls[4] = CreateLabel("Move all directories to the front",
                              forcolor, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 10);    
                               
    controls[5] = CreateLabel("Place directories and files together",
                              forcolor, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 12); 

    controls[6] = CreateLabel("Move all data to the front,",
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 14);
                              
    controls[7] = CreateLabel("Quickly try to move files consecutively",
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 16);  

    controls[8] = CreateLabel("Try harder to move files consecutively",
                              DIALOGFORCOLOR, DIALOGBACKCOLOR,
                              TEXT_X, DIALOG_Y + 18);  

    if (bForFAT32)
    {
        controls[9] = CreateSelectionButton(&SButtonsFAT32[0],
                                            DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 2);
        
        controls[10] = CreateSelectionButton(&SButtonsFAT32[1],
                                             DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                             SELECTION_X, DIALOG_Y + 16);

        controls[11] = CreateSelectionButton(&SButtonsFAT32[2],
                                             DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                             SELECTION_X, DIALOG_Y + 18);

        controls[12] = CreateLabel("( ) Full Optimization",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 4);

        controls[13] = CreateLabel("( ) Unfragment Files only",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 6);

        controls[14] = CreateLabel("( ) Files first",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 8);

        controls[15] = CreateLabel("( ) Directories first",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 10);

        controls[16] = CreateLabel("( ) Directories with files",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 12);

        controls[17] = CreateLabel("( ) Crunch only",
                                   forcolor, DIALOGBACKCOLOR,
                                   SELECTION_X, DIALOG_Y + 14);  
    
    }
    else
    {
        controls[9] = CreateSelectionButton(&SButtonsFull[0],
                                            DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 2);
        
        controls[10] = CreateSelectionButton(&SButtonsFull[1],
                                            forcolor, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 4);

        controls[11] = CreateSelectionButton(&SButtonsFull[2],
                                             forcolor, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 6);

        controls[12] = CreateSelectionButton(&SButtonsFull[3],
                                            forcolor, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 8);

        controls[13] = CreateSelectionButton(&SButtonsFull[4],
                                            forcolor, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 10);

        controls[14] = CreateSelectionButton(&SButtonsFull[5],
                                            forcolor, DIALOGBACKCOLOR,
                                            SELECTION_X, DIALOG_Y + 12);
 
        controls[15] = CreateSelectionButton(&SButtonsFull[6],
                                             forcolor, DIALOGBACKCOLOR,
                                             SELECTION_X, DIALOG_Y + 14);        
    
        controls[16] = CreateSelectionButton(&SButtonsFull[7],
                                             DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                             SELECTION_X, DIALOG_Y + 16);

        controls[17] = CreateSelectionButton(&SButtonsFull[8],
                                             DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                             SELECTION_X, DIALOG_Y + 18);
    
    }


    controls[18] = CreateCommandButton(&OkButton,
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X1, BUTTON_Y, TRUE, FALSE,
                                       FALSE);
     
    controls[19] = CreateCommandButton(&CancelButton,
                                       BUTTONFORCOLOR, BUTTONBACKCOLOR,
                                       BUTTON_X2, BUTTON_Y, FALSE, TRUE, 
                                       TRUE);
}

int AskOptimizationMethod(Method* method, BOOL bForFAT32)
{
    int control, result, i;

    PushHelpIndex(OPTIMIZATION_METHOD_INDEX);
    
    Initialize(bForFAT32);
     
    SetStatusBar(RED, WHITE, "                                            ");
    SetStatusBar(RED, WHITE, " Change the current optimization method.");
    
    if (bForFAT32)
    {
       switch (GetOptimizationMethod())
       {
       case 0:
            SButtonsFAT32[0].selected = TRUE;
            break;
       case 8:
            SButtonsFAT32[1].selected = TRUE;
            break;
       case 9:
            SButtonsFAT32[2].selected = TRUE;
            break;
       default:
            SButtonsFAT32[1].selected = TRUE;
       }
    }  
    else
        SButtonsFull[GetOptimizationMethod()].selected = TRUE;
    
    OpenWindow(&MethodWin);
    control = ControlWindow(&MethodWin);
    CloseWindow();

    if ((control == CANCELBUTTON) || (control == -1))
       result = FALSE;
    else
       result = TRUE;

    if (bForFAT32)
    {
       if (SButtonsFAT32[0].selected)
          *method = 0;
       if (SButtonsFAT32[1].selected)
          *method = 7;
       if (SButtonsFAT32[2].selected)
          *method = 8;
    }
    else
    {
       for (i = 0; i < AMOF_METHODS; i++)
       {
           if (SButtonsFull[i].selected)
           {
              *method = i;
           }
       }
    }
    
    for (i = 0; i < AMOF_METHODS; i++)
    {
        SButtonsFull[i].selected = FALSE;
    }

    for (i = 0; i < 3; i++)
    {
        SButtonsFAT32[i].selected = FALSE;
    }

    PopHelpIndex();
    
    return result;
}
