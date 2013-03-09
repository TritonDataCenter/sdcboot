/*
  PrintBuf.c - Buffers to contain the data to send to the user.
  Copyright (C) 2002 Imre Leber
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be  
*/

#include <stdio.h>
#include <string.h>

#include "fte.h"
#include "errmsgs.h"
#include "..\misc\useful.h"

static char FilePrintBuffer[255];
static char MessageBuffer[512];

char* GetFilePrintBuffer(void) 
{
    return FilePrintBuffer;
}

char* GetMessageBuffer(void)
{
    return MessageBuffer;
} 

void ShowDirectoryViolation(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            struct DirectoryEntry* entry,    /* Speed up */
                            char* message)
{
    char* p;
    
    if (!TraceFullPathName(handle, pos, FilePrintBuffer))
       ConvertEntryPath(FilePrintBuffer, entry->filename, entry->extension);
                   
    sprintf(MessageBuffer, message, FilePrintBuffer);
    ReportFileSysError(MessageBuffer, 0, &p, 0, FALSE);
}

void ShowClusterViolation(RDWRHandle handle, CLUSTER cluster, char* message)
{
    char* p;
    
    if (!TraceClusterPathName(handle, cluster, FilePrintBuffer))
       sprintf(FilePrintBuffer, "a file containing cluster %lu", cluster);
                       
    sprintf(MessageBuffer, message, FilePrintBuffer);
    ReportFileSysError(MessageBuffer, 0, &p, 0, FALSE);
}

void ShowFileNameViolation(RDWRHandle handle,
                           char* filename,
                           char* message)
{       
    char* p;        
    UNUSED(handle);
    
    sprintf(MessageBuffer, message, filename);
    ReportFileSysError(MessageBuffer, 0, &p, 0, FALSE);  
}

void ShowParentViolation(RDWRHandle handle,
                         char* filename,
                         char* message)
{       
    int i;
    
    strcpy(FilePrintBuffer, filename);  
    for (i = strlen(FilePrintBuffer)-1; i >= 0; i--)
    {
        if (FilePrintBuffer[i] == '\\')
        {
           FilePrintBuffer[i] = '\0';
           break;
        }
    }   

    if (i <= 0)
    {
       ShowFileNameViolation(handle, "The root directory", message); 
    }
    else
    {
       ShowFileNameViolation(handle, FilePrintBuffer, message);
    }         
}