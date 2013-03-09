/*    
   Timlabel.c - label with changing caption.
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

#include "control.h"
#include "timlabel.h"

#include "..\ovlhost\lowtime.h"
#include "..\screen\screen.h"
#include "..\..\misc\bool.h"

static void DrawTimeLabel(struct Control* control)
{
    struct TimeLabel* label = (struct TimeLabel*) control->ControlData;

   DrawSequence(control->posx, control->posy, label->ClearLength,
                ' ', control->forcolor, control->backcolor);

    DrawText(control->posx, control->posy, label->strings[label->current],
             control->forcolor, control->backcolor);
}

static int OnEvent(struct Control* control, int event)
{
    int dummy, sec;
    struct TimeLabel* label = (struct TimeLabel*) control->ControlData;
    
    if (event) return NOT_ANSWERED; 

    GetTime(&dummy, &dummy, &sec);
    if (label->lastsec != sec)
    {
       label->current = (label->current + 1) % label->AmofStrings;
       label->lastsec = sec;
       DrawTimeLabel(control);
    }

    return NOT_ANSWERED;
}

struct Control CreateTimeLabel(struct TimeLabel* timelabel, 
                               int forcolor, int backcolor,
                               int posx, int posy)
{
    struct Control result;

    InitializeControlValues(&result);

    result.forcolor  = forcolor;
    result.backcolor = backcolor;
    result.posx      = posx;
    result.posy      = posy;

    result.HandlesEvents = TRUE;
    result.OnRedraw      = DrawTimeLabel;
    result.OnEvent       = OnEvent;
    
    result.ControlData   = (void*) timelabel;
    
    return result;
}
