/*    
   Menu.c - main menu.
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
#include "menu.h"
#include "msgbxs.h"
#include "..\event\event.h"
#include "..\screen\screen.h"
#include "..\mouse\mouse.h"
#include "..\keyboard\keyboard.h"
#include "..\winman\control.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\helpsys\hlpindex.h"
#include "..\helpsys\idxstack.h"

static int SavedCurrent = 0;

static char* MenuItems[] = {"Begin optimization          ALT-B",
                            "-",
                            "Drive...",
                            "Optimization method...",
                            "-",
                            "File sort...",
                            "Map legend...",
                            "-",
                            "About FreeDOS defrag...",
                            "eXit                        ALT-X"};

static char* informations[] = {" Begin disk optimization on current drive.",
                               "",
                               " Change current drive.",
                               " Change the current optimization method.",
                               "",
                               " Specify file order.",
                               " Show the map symbol definitions.",
                               "",
                               " Display the copyright notice.",
                               " Exit the FreeDOS defragmenter."};

static char ChangePositions[] = {2, 0, 2, 2, 0, 2, 2, 0, 2, 3};

static int HelpIndexes[] = {BEGIN_OPTIMIZATION_INDEX,  
                            0,
                            SHOW_DRIVES_INDEX,        
                            SHOW_OPTIMIZATION_INDEX,
                            0,
                            SHOW_SORTOPTIONS_INDEX,
                            SHOW_MAPLEGEND_INDEX,  
                            0,
                            SHOW_ABOUTDIALOG_INDEX,  
                            EXIT_DEFRAG_INDEX};

static void DrawItem (int index, int forcolor, int backcolor, 
                      int shortcutcolor)
{
       char buf[LONGESTITEM+4];
       buf[LONGESTITEM+3] = 0;

       if (MenuItems[index][0] == '-')
          memset(buf, 'Ä', LONGESTITEM+2);
       else
       {
          memset(buf, ' ', LONGESTITEM+3);
          memcpy(&buf[1], MenuItems[index], strlen(MenuItems[index]));
       } 
       
       DrawText(MENU_X+1, MENU_Y+index+1, buf, forcolor, backcolor);

       if (ChangePositions[index])
          ChangeCharColor(MENU_X+(int)ChangePositions[index], MENU_Y+index+1,
                          shortcutcolor, backcolor);
}

static void DrawMenu (void)
{
       int i;

       /* Draw surounding box. */
       DrawSingleBox(MENU_X, MENU_Y, LONGESTITEM+4, AMOFITEMS+1,
                     MENUFORCOLOR, MENUBACKCOLOR, "");

       /* Draw menu items. */
       for (i = 0; i < AMOFITEMS; i++)
           DrawItem(i, MENUFORCOLOR, MENUBACKCOLOR, 
                       MENUSHORTCUTCOLOR);
}

int MainMenu(void)
{
    int current, event, leave = 0;

    current = SavedCurrent;

    CopyScreen (MENU_X, MENU_Y-1, MENU_X+LONGESTITEM+4, MENU_Y+AMOFITEMS+1);

    InvertLine (MENU_CAPTION_X1, MENU_CAPTION_X2, MENU_CAPTION_Y);

    ClearEvent();
    DrawMenu();
    while (AltKeyDown());

    while (!leave)
    {
        SetStatusBar(RED, WHITE, "                                            ");
        SetStatusBar(RED, WHITE, informations[current]);

        DrawItem(current, MENUHIGHLIGHTFOR, MENUHIGHLIGHTBACK, 
                          MENUHIGHLIGHTFOR);
        while (((event = GetEvent()) == 0) || (event == MSMIDDLE));
        DrawItem(current, MENUFORCOLOR, MENUBACKCOLOR,
                          MENUSHORTCUTCOLOR);
        
        switch (event)
        {
           case SPACEKEY:
           case ENTERKEY: leave = 1;
                          SavedCurrent = current;
                          current++;
                          break;

           case ALTKEY:
           case ESCAPEKEY: leave = 1;
                           SavedCurrent = current;
                           current = NONESELECTED;
                           break;

           case PAGEUP:
           case HOME:
                current = 0;
                break;

           case PAGEDOWN:
           case END:
                current = AMOFITEMS-1;
                break;

           case CURSORUP:
                if (current == 0)
                   current = AMOFITEMS-1;
                else
                   current--;
                if (MenuItems[current][0] == '-')
                   current--;
                break;

           case CURSORDOWN:
                if (current == AMOFITEMS-1)
                   current = 0;
                else
                   current++;
                if (MenuItems[current][0] == '-')
                   current++;
                break;

           case 'b':
           case 'B':
           case ALT_B:
                    current = BEGINOPTIMIZATION;
                    leave = 1;
                    break;

           case 'd':
           case 'D':
           case ALT_D:
                    current = CHANGEDRIVE;
                    leave = 1;
                    break;

           case 'o':
           case 'O':
           case ALT_O:
                    current = CHANGEMETHOD;
                    leave = 1;
                    break;

           case 'f':
           case 'F':
           case ALT_F:
                    current = SPECIFYFILEORDER;
                    leave = 1;
                    break;

           case 'm':
           case 'M':
           case ALT_M:
                    current = SHOWMAP;
                    leave = 1;
                    break;

           case 'a':
           case 'A':
           case ALT_A:
                    current = DISPLAYCOPYRIGHT;
                    leave = 1;
                    break;

           case 'x':
           case 'X':
           case ALT_X:
                    current = EXITDEFRAG;
                    leave = 1;
                    break;

           case MSLEFT:
           case MSRIGHT:
                    if (PressedInRange(68, 1, 80, 1)) break;

                    SavedCurrent = current;
                    if (!PressedInRange(MENU_X+1, MENU_Y+1,
                                        MENU_X+LONGESTITEM+3,
                                        MENU_Y+AMOFITEMS))
                    {
                       current = NONESELECTED;
                       leave = 1;
                       break;
                    }

                   current = GetPressedY() - MENU_Y;
                   SavedCurrent = (MenuItems[current-1][0] == '-') 
                                ? SavedCurrent
                                : current - 1;
                   leave   = 1;
                   break;
        }
        
        PushHelpIndex(HelpIndexes[current]);
        CheckExternalEvent(event);
        PopHelpIndex();
    }

    PasteScreen (0, 0);
    return current;
}
