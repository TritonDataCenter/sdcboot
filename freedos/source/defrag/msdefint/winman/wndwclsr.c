
#include "control.h"

#include "..\event\event.h"
#include "..\mouse\mouse.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawWindowCloser(struct Control* control)
{
    DrawText(control->posx, control->posy, "[þ]",
        control->forcolor, control->backcolor);
}

static int LookAtEvent(struct Control* control, int event)
{
    switch (event)
    {
   case MSLEFT:
   case MSRIGHT:
   case MSMIDDLE:
        if (PressedInRange(control->posx, control->posy,
            control->posx+2, control->posy))
      return LEAVE_WINDOW;
    }

    return NOT_ANSWERED;
}

struct Control CreateWindowCloser(int forcolor, int backcolor,
              int posx, int posy)
{
   struct Control result;

   InitializeControlValues(&result);

   result.forcolor      = forcolor;
   result.backcolor     = backcolor;
   result.posx         = posx;
   result.posy          = posy;

   result.HandlesEvents = TRUE;

   result.OnEvent      = LookAtEvent;
   result.OnRedraw     = DrawWindowCloser;
   result.OnNormalDraw = DrawWindowCloser;

   return result;
}
