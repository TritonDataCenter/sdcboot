/*    
   Winman.c - window manager.
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

#include <dos.h>

#include "control.h"
#include "window.h"
#include "winman.h"
#include "..\event\event.h"
#include "..\dialog\dialog.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

#define WAIT_TIME 500

void OpenWindow(struct Window* winpars)
{
     int i;
     
     /* Draw the window. */
     ShowDialog(winpars->x, winpars->y, winpars->xlen, winpars->ylen,
                winpars->caption, winpars->FrameColor, winpars->SurfaceColor);

     /* Draw the controls. */
     for (i = 0; i < winpars->AmofControls; i++)
         if (winpars->controls[i].Visible)
            winpars->controls[i].OnRedraw(&winpars->controls[i]);
}

#define CurrentControl (winpars->controls[ActiveControl])

static int NextControl(int ActiveControl, struct Control* controls,
                       int AmofControls, int IsTab, int step)
{
     do {
         ActiveControl = (ActiveControl + step) % AmofControls;

         if (ActiveControl == -1) ActiveControl = AmofControls - 1; 

     } while (!controls[ActiveControl].CanEnter ||
               (controls[ActiveControl].CanEnter &&
                (controls[ActiveControl].MustBeTab && !IsTab)));

     return ActiveControl;
}

static int FirstControl(struct Window* winpars)
{
     int i;

     for (i = 0; i < winpars->AmofControls; i++)
         if (winpars->controls[i].CanEnter) 
            return i;

     return -1;
}

static int LastControl(struct Window* winpars)
{
     int i;

     for (i = winpars->AmofControls-1; i >= 0; i--)
         if (winpars->controls[i].CanEnter) 
            return i;

     return -1;
}

static int FindDefaultControl(struct Window* winpars)
{
     int i;

     for (i = 0; i < winpars->AmofControls; i++)
         if (winpars->controls[i].DefaultControl) 
            return i;
     
     return -1;
}

static int BroadCastEvent(struct Control* controls, 
                           int AmofControls, 
                           int event, int* AnsweringControl)
{
    int i, result; 
    
    for (i = 0; i < AmofControls; i++)
        if (controls[i].HandlesEvents)
           if ((result = controls[i].OnEvent(&controls[i], event)) != 0)
           {
              *AnsweringControl = i; 
              return result;
           }

    return result;
}

static void AdjustDefaultControl(struct Window* winpars, int ActiveControl,
                                 int Default, int DefaultActive)
{
   CurrentControl.OnEnter(&CurrentControl);

   if ((Default != -1) && (ActiveControl != Default))
   {
      if (DefaultActive)
      {
         if (CurrentControl.BeforeDefault)
            winpars->controls[Default].OnUnDefault(
                                                &winpars->controls[Default]);
      }
      else 
         if (!CurrentControl.BeforeDefault)
            winpars->controls[Default].OnDefault(
                                                &winpars->controls[Default]);
   }
}


int ControlWindow(struct Window* winpars)
{
     int ActiveControl = FirstControl(winpars), event, leave = 0;
     int step, answering, Default = FindDefaultControl(winpars);
     int DefaultActive, result, again, tabpressed;

     if (ActiveControl == -1) 
     {
	delay(WAIT_TIME);
        return -1;
     }
     
     ClearEvent();

     if (!CurrentControl.BeforeDefault && (Default > 0))
        winpars->controls[Default].OnDefault(&winpars->controls[Default]);
     
     CurrentControl.OnEnter(&CurrentControl);

     /* Main loop. */
     while (!leave)
     {
         step = 0;
         
         event = GetEvent();
  
         if (event == ESCAPEKEY)
         {
            CurrentControl.OnLeave(&CurrentControl);
            answering = -1;
            break;
         }

         DefaultActive = !CurrentControl.BeforeDefault;

         switch (event)
         {
          case TABKEY:
          case SHIFTTAB:
          case HOME:
          case CURSORUP:
          case CURSORLEFT:
          case CURSORRIGHT:
          case END:
          case CURSORDOWN:
               if (CurrentControl.CursorUsage == CURSOR_SELF)
               {
                  if (CurrentControl.OnEvent(&CurrentControl, event) == 
                      EVENT_ANSWERED)
                     continue;
               }
               CurrentControl.OnLeave(&CurrentControl);
         }
        
         switch (event)
         {
          case HOME:
               ActiveControl = FirstControl(winpars);
               step = 2;
               break;

          case END:
               ActiveControl = LastControl(winpars);
               step = 2;
               break;

          case TABKEY:
          case CURSORRIGHT:
          case CURSORDOWN:
               step = 1;
               break;

          case SHIFTTAB:
          case CURSORLEFT:
          case CURSORUP:
               step = -1;
               break;
         }

         tabpressed = (event == TABKEY) || (event == SHIFTTAB);
         
         if ((step == -1) || (step == 1)) 
            ActiveControl = NextControl(ActiveControl, winpars->controls,
                                        winpars->AmofControls, tabpressed, 
                                        step);
         
         if (step)
         {
            CurrentControl.OnEnter(&CurrentControl);

            AdjustDefaultControl(winpars, ActiveControl, Default, 
                                 DefaultActive);

            continue;
         }
                  
         result = CurrentControl.OnEvent(&CurrentControl, event);
         
         if (result == NOT_ANSWERED)
         {
            if ((Default != -1) && (event == ENTERKEY))
            {
               answering = Default;
               result = winpars->controls[Default].OnEvent(
                                                  &winpars->controls[Default],
                                                  ENTERKEY);
            }
            else
               result = BroadCastEvent(winpars->controls, 
                                       winpars->AmofControls,
                                       event, &answering);
         }
         else
            answering = ActiveControl;
         
         do {
              again = FALSE;
              
              switch (result)
              {
           
                case LEAVE_WINDOW:
                     leave = 1;
                     break;

                case NEXT_CONTROL:
                     CurrentControl.OnLeave(&CurrentControl);
                     ActiveControl = NextControl(ActiveControl, 
                                                 winpars->controls,
                                                 winpars->AmofControls, 
                                                 FALSE, 1);

                     CurrentControl.OnEnter(&CurrentControl);
                     break;
                       
                case PREVIOUS_CONTROL:
                     CurrentControl.OnLeave(&CurrentControl);
                     ActiveControl = NextControl(ActiveControl, 
                                                 winpars->controls,
                                                 winpars->AmofControls, 
                                                 FALSE, -1);
                     CurrentControl.OnEnter(&CurrentControl);
                     break;
             
                case CONTROL_ACTIVATE:
                     CurrentControl.OnLeave(&CurrentControl);
                     ActiveControl = answering;
                     CurrentControl.OnEnter(&CurrentControl);
                     break;              
                
                case REQUEST_AGAIN:
                     CurrentControl.OnNormalDraw(&CurrentControl);

                     if (Default != -1)
                        winpars->controls[Default].OnUnDefault(
                                                &winpars->controls[Default]);

                     winpars->controls[answering].SecondPass = TRUE;
                     result = winpars->controls[answering].OnEvent(
                                                &winpars->controls[answering],
                                                event);                       
                     winpars->controls[answering].SecondPass = FALSE;
                     again = TRUE;
                     break;

                case EVENT_ANSWERED:
                     break;

                default:
                     CheckExternalEvent(event);
              }

              switch (result)
              {
                case NEXT_CONTROL:
                case PREVIOUS_CONTROL:
                case CONTROL_ACTIVATE:
                     AdjustDefaultControl(winpars, ActiveControl, Default, 
                                          DefaultActive);
              }

         } while (again);
     }

     CurrentControl.OnLeave(&CurrentControl);

     return answering;
}

void CloseWindow()
{
     HideDialog;
}
