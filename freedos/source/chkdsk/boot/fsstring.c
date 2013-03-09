/*
   FSString.c - file system string checking.
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

#include <stdio.h>
#include <string.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\errmsgs\errmsgs.h"
#include "..\errmsgs\PrintBuf.h"

static char* GetCorrectFSString(RDWRHandle handle);

static char* fat12str = "FAT12   ";
static char* fat16str = "FAT16   ";
static char* fat32str = "FAT32   ";

/****************************** Checking *********************************/

/*************************************************************************
**                           CheckFSString
**************************************************************************
** Checks wether the file system string is correct. 
***************************************************************************/
 
RETVAL CheckFSString(RDWRHandle handle)
{
    char label[8];
    char* correct, *p;

    if (!GetBPBFileSystemString(handle, label))
       return ERROR;

    correct = GetCorrectFSString(handle);
    if (!correct) return ERROR;

    if (memcmp(correct, label, 8) != 0)
    {
       sprintf(GetMessageBuffer(), 
               "The file system string does not indicate %s\n", correct);
       strcat(GetMessageBuffer(), 
              "This is generally not an error,\nbut some low level disk "
	      "utilities may not handle this correctly\n");
              
       ReportFileSysWarning(GetMessageBuffer(), 0, &p, 0, FALSE);

       return FAILED;
    }
    else
       return SUCCESS;
}

/******************************* Fixing *********************************/

/*************************************************************************
**                           CorrectFSString
**************************************************************************
** Write the right file system string. 
***************************************************************************/

RETVAL CorrectFSString(RDWRHandle handle)
{
    char* correct;
    struct BootSectorStruct bootsect;

    switch (CheckFSString(handle))
    {
       case SUCCESS:
            break;
            
       case FAILED:
            correct = GetCorrectFSString(handle);
            if (!correct) return ERROR;

            if (!ReadBootSector(handle, &bootsect))
               return ERROR;

            correct = GetCorrectFSString(handle);

            WriteBPBFileSystemString(&bootsect, correct);

            if (!WriteBootSector(handle, &bootsect))
               return ERROR;
            break;
            
       case FAIL:
            return ERROR;
    }

    return SUCCESS;
}

/******************************* Common *********************************/

/*************************************************************************
**                           GetCorrectFSString
**************************************************************************
** Returns the string corresponding to the calculated FAT type. 
***************************************************************************/

static char* GetCorrectFSString(RDWRHandle handle)
{
    switch (GetFatLabelSize(handle))
    {
       case FAT12:
            return fat12str;
            
       case FAT16:
            return fat16str;
            
       case FAT32:
            return fat32str;
            
       default:
            return NULL;
    }
}