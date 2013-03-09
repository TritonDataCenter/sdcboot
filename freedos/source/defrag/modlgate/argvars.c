/*    
   Argvars.c - keep track of the parameters passed to defrag.

   Copyright (C) 2002 Imre Leber

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

/*
   Remember:

            defrag [<drive>:] [{/F|/U}] [/Sorder[-]] [/B] [/X]

            with:
                 order = {N|       # name
                          D|       # date & time
                          E|       # extension
                          S}       # size
                 
                 - suffix to sort in descending order

                 F: full optimization.
                 U: unfragment files only.

                 B: reboot after optimization.
                 X: automatically exit.

            No-Ops: 
            
                 /SKIPHIGH 
                 /LCD      
                 /BW       
                 /G0       
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "..\misc\bool.h"

static char ParsedDrive     = 0;
static int  RebootRequested = FALSE;
static int  AutoExit        = FALSE;
static int  MethodEntered   = FALSE;
static int  AudibleWarning  = FALSE;
static int  FullOutput      = FALSE;

void MethodIsEntered(void)
{
    MethodEntered = TRUE;
}

int IsMethodEntered(void)
{
    return MethodEntered;
}

void SetParsedDrive(char drive)
{
     ParsedDrive = drive;
}

char GetParsedDrive(void)
{
     return ParsedDrive;
}

void RequestReboot(void)
{
     RebootRequested = TRUE;
}

int IsRebootRequested(void)
{
    return RebootRequested;
}

void AutomaticallyExit(void)
{
     AutoExit = TRUE;
}

int MustAutomaticallyExit(void)
{
    return AutoExit;
}

void SetFullOutput(void)
{
    FullOutput = TRUE;
}

int  GiveFullOutput(void)
{   
    return FullOutput;
}

void SetAudibleWarning(void)
{
    AudibleWarning = TRUE;
}

int  GiveAudibleWarning(void)
{
    return AudibleWarning;
}

