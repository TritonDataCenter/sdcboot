/*
   CHECKOS.C - checks wether defrag may become active.

   Copyright (C) 2000, Imre Leber.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have recieved a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be

*/

#include <string.h>
#include <stdio.h>

#include "..\..\environ\os_id.h"
#include "..\..\environ\dpmitst.h"
#include "..\..\misc\bool.h"
#include "checkos.h"

/************************************************************************/
/***                              CheckOS                             ***/
/************************************************************************/
/*** int CheckOS(void);                                               ***/
/***                                                                  ***/
/*** Check the operating system and if defrag may not become active   ***/
/*** inform the user.                                                 ***/
/************************************************************************/

static void ErrorMessage(char* message)
{
   printf("%s\n", message);
}

int CheckOS(void)
{
    int  Ok = TRUE;
    char message[80];

    int os = get_os();
    if (!PLAIN_DOS(os))
    {
       if (IN_WINDOWS(os))
          strcpy(message, "I just simply refuse to run in windows!");
       else
       {
          strcpy(message, "This program cannot be run in ");
          strcat(message, id_os_name[get_os()]);
       }

       Ok = FALSE;
    }
    else if (DPMIinstalled())
    {
       strcpy(message, "This program cannot be run when a DPMI host is active!");
       Ok = FALSE;
    }
    else if ((os != FreeDOS) &&
	     (id_os_ver[DOS].maj >= 7)) /* Windows in DOS mode. */
    {
       strcpy(message, "I just simply refuse to run in windows!");
       Ok = FALSE;
    }

    if (!Ok) ErrorMessage(message);

    return Ok;
}

