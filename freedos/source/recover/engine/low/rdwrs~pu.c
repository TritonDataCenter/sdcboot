/*    
   rdwrsect.c - Abstractions for a common interface between absolute disk
                access and image files (linux version).
   
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

#include <ctype.h>
#include <fcntl.h>
#include <string.h>

#include <unistd.h>
#include <linux/unistd.h>
#include <sys/ioctl.h>
#include <linux/fd.h>
#include <stdlib.h>

#include "bool.h"
#include "../header/rdwrsect.h"
#include "../header/fatconst.h"
#include "../header/io.h"
#include "../header/fteerr.h"

#define UNUSED(x) if(x);

/*
	Private read/write functions (linux specific)

*/

static int PrivateReadSectors(int handle, int nsects, SECTOR lsect, void* buffer)
{
	return (fs_read(handle, lsect * BYTESPERSECTOR, nsects * BYTESPERSECTOR, buffer)) ? 
	       0 : -1;
}

static int PrivateWriteSectors(int handle, int nsects, SECTOR lsect, void* buffer,
						       unsigned area)
{
	UNUSED(area);
	
	return (fs_write(handle, lsect * BYTESPERSECTOR, nsects * BYTESPERSECTOR, buffer)) ?
		   0 : -1;	
}


/*
** Public read sector function.
*/

int ReadSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer)
{   
    return handle->ReadFunc(handle->handle, nsects, lsect, buffer);
}

/*
** Public write sector function.
*/

int WriteSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer,
                 unsigned area)
{
    if (handle->ReadWriteMode == READINGANDWRITING)
       return handle->WriteFunc(handle->handle, nsects, lsect, buffer, area);
    else
    {
        SetFTEerror(FTE_WRITE_ON_READONLY); 
        return -1;
    }
}


/*
** Private function to initialise the reading and writing of sectors.
*/
static int PrivateInitReadWriteSectors(char* driveorfile, int modus,
                                       RDWRHandle* handle)
{
    *handle = (RDWRHandle) malloc(sizeof(struct RDWRHandleStruct));
    if (*handle == NULL) return FALSE;
    memset(*handle, 0, sizeof(struct RDWRHandleStruct));

    (*handle)->ReadFunc    = PrivateReadSectors;
    (*handle)->WriteFunc   = PrivateWriteSectors;
    (*handle)->handle      = fs_open (driveorfile, modus);
    (*handle)->IsImageFile = TRUE;

	if ((*handle)->handle < 0)
	{
	   free(*handle);
	   *handle = NULL;
	   return FALSE;
    }

    /* Get number of bytes per sector from the boot sector, this
	   field is located at:
	   struct BootSectorStruct
	   {
	      char     Jump[3];
	      char     Identification[8];
	      unsigned short BytesPerSector;
	      ...
	   }
    */
	
	if (!fs_read((*handle)->handle, 11, sizeof(unsigned short), &((*handle)->BytesPerSector)))
	{
	   free(*handle);
	   *handle = NULL;
	   return FALSE;	   	
	}
	
    if ((*handle)->BytesPerSector != 512)
    {
       SetFTEerror(FTE_INVALID_BYTESPERSECTOR);
       return FALSE;
    }
      
    return TRUE;
}

/*
**  Init for reading and writing of sectors.
*/
int InitReadWriteSectors(char* driveorfile, RDWRHandle* handle)
{
    int result;        
        
    result = PrivateInitReadWriteSectors(driveorfile, O_RDWR, handle);
    if (result)
    {
       (*handle)->ReadWriteMode = READINGANDWRITING;
    }
    
    return result;
}

/*
**  Init for reading only.
*/
int InitReadSectors(char* driveorfile, RDWRHandle* handle)
{
    int result;
    
    result = PrivateInitReadWriteSectors(driveorfile, O_RDONLY, handle);
    if (result)
    {
       (*handle)->ReadWriteMode = READING;
    }
    
    return result;
}

/*
**  Close the sector read/write system.
*/
void CloseReadWriteSectors(RDWRHandle* handle)
{
    if (*handle != NULL)
    {     
       fs_close((*handle)->handle);  
       free(*handle);
       *handle = NULL;
    }
}

/*
** Return the read/write modus 
*/
int GetReadWriteModus(RDWRHandle handle)
{
    return handle->ReadWriteMode;    
}

/*
**  Return wether the handle works on an image file
*/

BOOL IsWorkingOnImageFile(RDWRHandle handle)
{
    return handle->IsImageFile;
}
