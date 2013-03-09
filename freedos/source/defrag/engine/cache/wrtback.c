/*    
   WrtBack.c - ADT to contain the functions to write the data back to disk.

   Copyright (C) 2003 Imre Leber

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

#include <assert.h>
#include <string.h>

#include "fte.h"

#define AMOF_WRITEFUNCS 1

struct WriteFuncStruct
{
    unsigned devid;
    int (*write)(int handle, int nsects, SECTOR lsect,
                 void* buffer, unsigned area);
    int handle;
};

struct WriteFuncStruct WriteFuncs[AMOF_WRITEFUNCS];

static int WriteFuncPointer = 0;

void InstallWriteFunction(unsigned devid, 
                          int (*write)(int handle, int nsects, SECTOR lsect,
                                       void* buffer, unsigned area),
                          int handle)
{
    assert(WriteFuncPointer < AMOF_WRITEFUNCS);
    assert(write);
    
#ifndef NDEBUG
{
    int i;
    for (i=0; i < WriteFuncPointer; i++)
	if (WriteFuncs[WriteFuncPointer].devid == devid)
	   assert(FALSE);
}
#endif
    
    WriteFuncs[WriteFuncPointer].devid  = devid;
    WriteFuncs[WriteFuncPointer].write  = write;
    WriteFuncs[WriteFuncPointer].handle = handle;
   
    WriteFuncPointer++;
}                                       

void UninstallWriteFunction(unsigned devid)
{
    int i, j;
    
    assert(WriteFuncPointer <= AMOF_WRITEFUNCS);
    
    for (i = 0; i < WriteFuncPointer; i++)
    {
        if (WriteFuncs[i].devid == devid)
        {
           for (j = i; j < WriteFuncPointer-1; j++)
           {
               memcpy(&WriteFuncs[j], &WriteFuncs[j+1], 
                      sizeof(WriteFuncs[j]));
           }
           WriteFuncPointer--;
           return;
        }
    }    
    
    assert(FALSE);
}

static int GetWriteFunction(unsigned devid)
{
    int i;
    
    for (i = 0; i < WriteFuncPointer; i++)
    {
        if (WriteFuncs[i].devid == devid)
        {
           return i;
        }
    }
    
    assert(FALSE);
    return -1;
}

int WriteBackSectors(unsigned devid, int nsects, SECTOR lsect, 
                     void* buffer, unsigned area)
{
    int retVal;
    int element = GetWriteFunction(devid);
    
    assert((element < WriteFuncPointer) && (WriteFuncs[element].write));

    if (element >= 0)
    {        
	retVal = WriteFuncs[element].write(WriteFuncs[element].handle,
                                           nsects, lsect,
                                           buffer, area) != -1; 
	RETURN_FTEERR(retVal);
    }
    
    assert(FALSE);
    return FALSE;
}
