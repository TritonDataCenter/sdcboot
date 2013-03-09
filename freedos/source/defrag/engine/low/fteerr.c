/*    
   fteerr.c - engine error handling.
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

#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "../header/fteerr.h"

static int FTEerror = FTE_OK;

/************************************************************
**                        GetFTEerror
*************************************************************
** Returns the FTE error
*************************************************************/

int GetFTEerror (void)
{
    return FTEerror;
}

/************************************************************
**                        SetFTEerror
*************************************************************
** Changes the FTE error to the given value
*************************************************************/

void SetFTEerror (int error)
{
	if (FTEerror != FTE_READ_ERR)
	{
        FTEerror = error;
	}
}

/************************************************************
**                        ClearFTEerror
*************************************************************
** Changes the FTE error to FTE_OK
*************************************************************/

void ClearFTEerror (void)
{
    FTEerror = FTE_OK;
}

#ifndef NDEBUG

struct FTETrack
{
    char* file;
    int line;
    
    struct FTETrack* next;
};

static struct FTETrack* FTEHead = NULL;
static struct FTETrack* FTEPtr;

/************************************************************
**                        TrackFTEError
*************************************************************
** Sets the FTE error
*************************************************************/

void TrackFTEError(int line, char* file)
{
    struct FTETrack* track;
	
    track = (struct FTETrack*)malloc(sizeof(struct FTETrack));    
    if (!track) return;
	
    track->line = line;
    track->file = (char*) malloc(strlen(file)+1);    
        
    if (!track->file)
    {
	free(track);
	return;
    }
    
    strcpy(track->file, file);
    
    track->next = FTEHead;
    
    FTEHead = track;
}

/************************************************************
**                        GetFirstFTEError
*************************************************************
** Returns the first FTE error
*************************************************************/

BOOL GetFirstFTEError(int* line, char** file)
{
    FTEPtr = FTEHead;
    return GetNextFTEError(line, file);
}

/************************************************************
**                        GetNextFTEError
*************************************************************
** Returns the next FTE error
*************************************************************/

BOOL GetNextFTEError(int* line, char** file)
{
    if (!FTEPtr) return FALSE;
    
    *line = FTEPtr->line;
    *file = FTEPtr->file;
    
    FTEPtr = FTEPtr->next;
    
    return TRUE;    
}

/************************************************************
**                        UntrackFTEErrors
*************************************************************
** Releases the memory of the FTE errors track
*************************************************************/

void UntrackFTEErrors()
{
    struct FTETrack* ptr = FTEHead, *ptr1;
	
    while (ptr)
    {
	ptr1 = ptr->next;
	
	free(ptr->file);
	free(ptr);

	ptr = ptr1;	
    }    
}


#endif

#ifdef DEBUG

int main(int argc, char** argv)
{
    int line; char* file;
    int retVal;
    
    func1();
    func2();
    func3();
    func4();
    func5();
    
    
    retVal = GetFirstFTEError(&line, &file);
    while (retVal)
    {
	printf("Error at %s (%d)\n", file, line);		
	retVal = GetNextFTEError(&line, &file);
    }
    
    UntrackFTEErrors();
}


int func1()
{
    RETURN_FTEERR(1);    
}

int func2()
{
    RETURN_FTEERR(2);    
}

int func3()
{
    RETURN_FTEERR(3);    
}

int func4()
{
    RETURN_FTEERR(4);    
}

int func5()
{
    RETURN_FTEERR(5);    
}


#endif
