#include "control.h"
#include "vscrctrl.h"
#include "loview.h"
#include "controls.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\helpsys\hlpfncs.h"
#include "..\helpsys\hlpsysid.h"

static void FillControl(struct Control* control, int top);

static int PastTop(struct Control* control, int top)
{
    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    return PastEndOfHelp(top, VBox->ylen-2);
}

static void UpdateScrollBar(struct Control* control)
{
    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    struct LowView* LView = (struct LowView*) VBox->ControlData;

    VBox->AdjustScrollBar(control, LView->top,
                          GetLastHelpTop(VBox->ylen-2)+1);
}

static int OnClick(struct Control* control)
{
    int index;
    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    struct LowView* LView = (struct LowView*) VBox->ControlData;

    if ((index = CheckHelpClick(control->posx+1, control->posy+1,
                                VBox->xlen-2, VBox->ylen-2,
                                GetPressedX(), GetPressedY(),
                                LView->top)) != -1)
    {
       SelectHelpPage(index);
       FillControl(control, 0);
       LView->top = 0;
       return EVENT_ANSWERED;
    }
    else
       return NOT_ANSWERED;
}

static int HandleEvent(struct Control* control, int event)
{
    int index;

    static int PreviousHelpLine = -1;

    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    struct LowView* LView = (struct LowView*) VBox->ControlData;

    if (control->active)
       switch (event)
       {
              case PAGEDOWN:
                   {
                     int ltop = GetLastHelpTop(VBox->ylen-2);

                     if (ltop != LView->top)
                     {
                        if (PastEndOfHelp(LView->top + VBox->ylen - 2,
                                          VBox->ylen-2))
                           LView->top = ltop;
                         else
                            LView->top += VBox->ylen - 2;
                         FillControl(control, LView->top);
                     }
                   }
                   break;

              case PAGEUP:
                   if (LView->top != 0)
                   {
                      if (LView->top > VBox->ylen - 2)
                         LView->top -= (VBox->ylen - 2);
                      else
                         LView->top = 0;
                      FillControl(control, LView->top);
                   }
                   break;

              case HOME:
                   if (LView->top != 0)
                   {
                      LView->top = 0;
                      FillControl(control, LView->top);
                   }
                   break;

              case END:
                   {
                     int ltop = GetLastHelpTop(VBox->ylen-2);

                     if (LView->top != ltop)
                     {
                        LView->top = ltop;
                        FillControl(control, LView->top);
                     }
                   }
                   break;

	      case TABKEY:
	           {
                      int line;

                      line = CheckHelpTab(LView->top, VBox->ylen-2);

                      if (line != -1)
                      	 DrawHelpLine(line, control->posx+1,
                                      control->posy+1+(line-LView->top),
                                      VBox->xlen-1);

                      if ((PreviousHelpLine != -1)         &&
                      	  (PreviousHelpLine != line)       &&
                          (PreviousHelpLine >= LView->top) &&
                          (PreviousHelpLine <  LView->top+VBox->ylen-2))
                         DrawHelpLine(PreviousHelpLine, control->posx+1,
                                      control->posy+1+(PreviousHelpLine-LView->top),
                                      VBox->xlen-2);

                      PreviousHelpLine = line;
                   }
                   break;

              case ENTERKEY:
                   if ((index = CheckHelpEnter(LView->top, VBox->ylen-2))
                       != -1)
                   {
                      SelectHelpPage(index);
                      FillControl(control, 0);
                      LView->top = 0;
                   }
                   break;

              case ALT_F1:
                   {
                      int previous;
                      
                      index    = GetHelpIndex();
                      previous = PreviousHelpIndex();

                      if (index != previous)
                      {
                         SelectHelpPage(previous);
                         FillControl(control, 0);
                         LView->top = 0;
                      }
                   }

              default:
                   return NOT_ANSWERED;
       }

    VBox->AdjustScrollBar(control, LView->top, GetLastHelpTop(VBox->ylen-2)+1);

    return EVENT_ANSWERED;
}

static void FillControl(struct Control* control, int top)
{
    int i;
    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    for (i = 0; i < VBox->ylen-2; i++)
        DrawHelpLine(top+i, control->posx+1, control->posy+1+i,
                     VBox->xlen-1);
}

static void EntryChosen(struct Control* control, int index)
{
    struct VerticalScrollControl* VBox =
                         (struct VerticalScrollControl*)control->ControlData;

    struct LowView* LView = (struct LowView*) VBox->ControlData;
  
    int count = GetLastHelpTop(0), line, middle;
    if (count < 2) return;
    
    line = (((index * 100) / (VBox->BarY2 - VBox->BarY1)) * 
               count) / 100;
               
    // See if the line can be put in the middle of the control 
    middle = (VBox->ylen-2)/2;
    if (line < middle)
    {
        // The line is above the middle of the first screen full    
        LView->top = 0;
    }
    else if (PastEndOfHelp(line+1, middle))
    {
        // If not show the last entries with the cursor on the indicated line.
        LView->top = GetLastHelpTop(VBox->ylen-2);
    }
    else 
    {  
        // Put the line in the middle of the control    
        LView->top = line - middle; 
    }
            
    FillControl(control, LView->top);
    VBox->AdjustScrollBar(control, LView->top, GetLastHelpTop(VBox->ylen-2)+1);
}

struct Control CreateHelpView(struct LowView* LView,
                              struct VerticalScrollControl* VBox,
                              int posx, int posy,
                              int forcolor, int backcolor)
{
     struct Control result;

     VBox->OnClick     = OnClick;
     VBox->HandleEvent = HandleEvent;
     result = CreateLowView(LView, VBox, posx, posy, forcolor, backcolor);
     
     VBox->EntryChosen = EntryChosen; /* Overwrite basic behaviour */
     
     LView->FillControl = FillControl;
     LView->PastTop     = PastTop;
     LView->UpdateScrollBar = UpdateScrollBar;
     return result;
}
