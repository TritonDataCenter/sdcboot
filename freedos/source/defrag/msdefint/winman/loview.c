#include <stdlib.h>
#include <conio.h>

#include "control.h"
#include "vscrctrl.h"
#include "loview.h"
#include "controls.h"
#include "..\mouse\mouse.h"

#include "..\..\misc\bool.h"

static void FillViewer(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;

   struct LowView* view = (struct LowView*) VBox->ControlData;

   view->FillControl(control, 0);

   view->UpdateScrollBar(control);
}

static void OneDown(struct Control* control)
{
   struct VerticalScrollControl* VBox = 
                         (struct VerticalScrollControl*)control->ControlData;

   struct LowView* view = (struct LowView*) VBox->ControlData;

   if (!view->PastTop(control, view->top+1))
   {
      void *ScrollBuf = malloc((VBox->xlen-1) * (VBox->ylen-3) * 2);
      if (ScrollBuf)      /* Fast method */
      {
         int posy, ylen, top;
         
         LockMouse(control->posx, control->posy, 
                   control->posx+VBox->xlen, control->posy+VBox->ylen);
           
         gettext(control->posx+1, control->posy+2,
                 control->posx+VBox->xlen-1, control->posy+VBox->ylen-2,
                 ScrollBuf);

         puttext(control->posx+1, control->posy+1,
                 control->posx+VBox->xlen-1, control->posy+VBox->ylen-3,
                 ScrollBuf);

         UnLockMouse();
         
         free(ScrollBuf);

         posy = control->posy;
         ylen = VBox->ylen;
         top  = view->top;

         control->posy += VBox->ylen-3;
         VBox->ylen     = 3;
         view->top      = view->top+ylen-2;
         
         view->FillControl(control, view->top);

         control->posy = posy;
         VBox->ylen    = ylen;
         view->top     = top+1;

         view->UpdateScrollBar(control);
      }
      else                /* Slow method */
         view->FillControl(control, ++view->top);
   }
}

static void OneUp(struct Control* control)
{
   struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

   struct LowView* view = (struct LowView*) VBox->ControlData;

   if (view->top > 0)
   {
      void *ScrollBuf = malloc((VBox->xlen-2) * (VBox->ylen-3) * 2);
      
      if (ScrollBuf)      /* Fast method */
      {
         int ylen;
         
         LockMouse(control->posx, control->posy, 
                   control->posx+VBox->xlen, control->posy+VBox->ylen);
                   
         gettext(control->posx+1, control->posy+1,
                 control->posx+VBox->xlen-2, control->posy+VBox->ylen-3,
                 ScrollBuf);

         puttext(control->posx+1, control->posy+2,
                 control->posx+VBox->xlen-2, control->posy+VBox->ylen-2,
                 ScrollBuf);
                 
         UnLockMouse();       
                 
         free(ScrollBuf);

         ylen = VBox->ylen;
         view->top--;
         
         VBox->ylen = 3;

         view->FillControl(control, view->top);

         VBox->ylen = ylen;
         
         view->UpdateScrollBar(control);
      }
      else
         view->FillControl(control, --view->top);
   }
}

static void EntryChosen(struct Control* control, int index)
{
   control = control; index = index;
}

static void OnEntering(struct Control* control)
{
    control=control;
}

static void OnLeaving(struct Control* control)
{
    control=control;
}

struct Control CreateLowView(struct LowView* view,
                             struct VerticalScrollControl* VBox,
                             int posx, int posy,
                             int forcolor, int backcolor)
{
   struct Control result;

   result = CreateVScrollBox(VBox, posx, posy, forcolor, backcolor);

   VBox->ControlData = (void*) view;
   
   VBox->AnswersEvents = TRUE;

   VBox->OneUp       = OneUp;
   VBox->OneDown     = OneDown;
   VBox->EntryChosen = EntryChosen;
   VBox->DrawControl = FillViewer;
   VBox->OnEntering  = OnEntering;
   VBox->OnLeaving   = OnLeaving;

   view->top = 0;

   return result;
}
