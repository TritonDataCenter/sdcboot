/*    
   Infofat.c - get information.

   Copyright (C) 2000, 2002 Imre Leber

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

#include "fte.h"
#include "expected.h"
#include "callback.h"
#include "infofat.h"
#include "..\dtstruct\vlhandle.h"
#include "..\..\modlgate\custerr.h"

int InfoFAT(void)
{
    RDWRHandle handle;
    int result=0, fatsize;

    /* Mention what comes next. */
    LargeMessage("Scanning file system...");
    LogMessage("Scanning file system...");

    /* Notice that we assume right input from the interface.*/
    handle = GetCurrentVolumeHandle();
    if (!handle) return 255;

    SmallMessage("Filling drive map.");
    if (!FillDriveMap(handle))
    {    
        SetCustomError(FILL_DRIVE_MAP_ERROR);
        return 255;
    }

    SmallMessage("Marking unmovable clusters.");
    if (!MarkUnmovables(handle)) 
    {    
        SetCustomError(GET_UNMOVABLES_FAILED);
        return 255;
    }

    /* Remember file count */
    fatsize = GetFatLabelSize(handle);
    if (!fatsize) return 255;

    if ((fatsize == FAT12) || (fatsize == FAT16))
    {
        if (!RememberFileCount(handle))
            return 255;
    }

    SmallMessage("Calculating defragmentation factor.");
    result = GetDefragFactor(handle);

    SmallMessage("");

    return result;
}

//#define DEBUG
#ifdef DEBUG

RDWRHandle theHandle;

static struct CallBackStruct CallBacks;

int main(int argc, char** argv)
{
    if (argc != 2)
       printf("InfoFAT <image file>\n");
    else
    {
       CMDEFINT_GetCallbacks(&CallBacks);
       SetCallBacks(&CallBacks);

       if (!InitReadWriteSectors(argv[1], &theHandle))
     RETURN_FTEERR(FALSE);

       if (!CreateFixedClusterMap(theHandle))
       {
     printf("Cannot create fixed cluster map.\n");
     return 1;
       }

       printf("%d\n", InfoFAT());
    }
}

int GiveFullOutput () {return 1;}

RDWRHandle GetCurrentVolumeHandle(void)
{
   return theHandle;
}

unsigned GetDrawDrvMapFactor(RDWRHandle handle){return 5;}
#endif
