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
   email me at:  imre.leber@worldonline.be.

*/

#include <string.h>
#include <stdlib.h>

#include "os_id.h"
#include "dpmitst.h"
#include "checkos.h"
#include "..\misc\bool.h"

/************************************************************************/
/***                       CheckDefragEnvironment                     ***/
/************************************************************************/
/*** char* CheckDefragEnvironment(void);                              ***/
/***                                                                  ***/
/*** Check the operating system and wether defrag may become active.  ***/
/***                                                                  ***/
/*** Returns a pointer to a string to be displayed on the screen when ***/
/*** defrag cannot be loaded or NULL if there is no problem.          ***/
/************************************************************************/

static char message[80];

char* CheckDefragEnvironment(void)
{
    int os;

    os = get_os();
    if (!PLAIN_DOS(os))
    {
       if (IN_WINDOWS(os))
          strcpy(message, "I just simply refuse to run in windows!");
       else
       {
          strcpy(message, "This program cannot be run in ");
          strcat(message, id_os_name[get_os()]);
       }

       return message;
    }
    else if (DPMIinstalled())
    {
       strcpy(message, "This program cannot be run when a DPMI host is active!");
       return message;
    }
    else if ((os != FreeDOS) &&         /* Check if we are not running FAT32 FreeDOS */
	     (id_os_ver[DOS].maj >= 7)) /* Windows in DOS mode. */
    {
       strcpy(message, "I just simply refuse to run in windows!");
       return message;
    }

    return NULL;
}
