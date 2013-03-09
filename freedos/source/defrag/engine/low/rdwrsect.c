/*    
   rdwrsect.c - Abstractions for a common interface between absolute disk
                access and image files.
   
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

#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <dos.h>
#include <ctype.h>
#include <fcntl.h>
//#include <alloc.h>
#include <stdlib.h>
#include <string.h>

#include "../header/rdwrsect.h"
#include "../../misc/bool.h"
#include "../header/drive.h"
#include "../header/FTEerr.h"
#include "../disklib/disklib.h"
#include "../disklib/dosio.h"
#include "../header/fatconst.h"
#include "../header/lowfat32.h"
#include "../header/direct.h"
#include "../header/boot.h"
#include "../header/direct.h"
#include "../header/fsinfo.h"
#include "../header/FTEMem.h"
#include "../cache/WrtBack.h"

#ifdef USE_SECTOR_CACHE
#include "..\header\sctcache.h"
static int UniqueCounter=0;
#endif

#define INVALID_SECTOR 0xFFFFFFFFL

static char   LastReadBuffer[512];
unsigned long LastReadBufferSector  = INVALID_SECTOR;

#define UNUSED(x) if(x);

#define DEBUG_CACHE

/* Image file modifiers. */
static int ReadFromImageFile(int handle, int nsects, SECTOR lsect,
                             void* buffer)
{
  assert(buffer && nsects);  
    
  if (lseek(handle, lsect*BYTESPERSECTOR, SEEK_SET) == -1L)
  {
     SetFTEerror(FTE_READ_ERR);
     RETURN_FTEERROR(-1); 
  }

  if ((read(handle, buffer, nsects*BYTESPERSECTOR)) == -1)
  {
     SetFTEerror(FTE_READ_ERR);
     RETURN_FTEERROR(-1);
  }

  return 0;
}

static int WriteToImageFile(int handle, int nsects, SECTOR lsect,
                            void* buffer, unsigned area)
{
   UNUSED(area); 
   assert(buffer && nsects);

   if (lseek(handle, lsect*BYTESPERSECTOR, SEEK_SET) == -1L)
  {
     SetFTEerror(FTE_WRITE_ERR);
     RETURN_FTEERROR(-1); 
  }

  if (write(handle, buffer, nsects*BYTESPERSECTOR) == -1)
  {
     SetFTEerror(FTE_WRITE_ERR);
     RETURN_FTEERROR(-1); 
  }

  return 0;
}

/* Real disk sector modifiers. */
/*
   There are different read/write functions:
   - ReadFromRealDisk12, WriteToRealDisk12 are used for FAT12 and FAT16
     volumes <= 32 MB
   - ReadFromRealDisk16, WriteToRealDisk16 are used for FAT16
     volumes > 32 MB
   - ReadFromRealDisk32, WriteToRealDisk32 are used for FAT32 volumes

   Notice also that, despite the names absread/abswrite work independent
   of the file system used (i.e. the naming here is a litle unfortunate)
*/

#ifdef ALLOW_REALDISK


static int ReadFromRealDisk16(int handle, int nsects, SECTOR lsect,
                              void* buffer)
{
   assert(buffer && nsects);
    
   if (disk_read_ext(handle, lsect, buffer, nsects) != DISK_OK)
   {
     SetFTEerror(FTE_READ_ERR);
     RETURN_FTEERROR(-1);  
   }
   return 0;
}

static int WriteToRealDisk16(int handle, int nsects, SECTOR lsect,
                             void* buffer, unsigned area)
{
   UNUSED(area);
   assert(buffer && nsects);

   if (disk_write_ext(handle, lsect, buffer, nsects) != DISK_OK)
   {
     SetFTEerror(FTE_WRITE_ERR);
     RETURN_FTEERROR(-1);  
   }
   return 0;
}

static int ReadFromRealDisk12(int handle, int nsects, SECTOR lsect,
                              void* buffer)
{
  assert(buffer && nsects);
    
  if (absread(handle, nsects, (int) lsect, buffer) == -1)
  {
     if (ReadFromRealDisk16(handle, nsects, lsect, buffer))
     {      
    SetFTEerror(FTE_READ_ERR);
    RETURN_FTEERROR(-1);
     }    
  }

  return 0;
}

static int WriteToRealDisk12(int handle, int nsects, SECTOR lsect,
                             void* buffer, unsigned area)
{
  assert(buffer && nsects);
    
  if (abswrite(handle, nsects, (int) lsect, buffer) == -1)
  {
     if (WriteToRealDisk16(handle, nsects, lsect, buffer, area))
     {    
        SetFTEerror(FTE_WRITE_ERR);
   RETURN_FTEERROR(-1); 
     }    
  }

  return 0;
}

static int ReadFromRealDisk32(int handle, int nsects, SECTOR lsect,
                              void* buffer)
{
   return ExtendedAbsReadWrite(handle+1, nsects, lsect, buffer, 0);
}

static int WriteToRealDisk32(int handle, int nsects, SECTOR lsect,
                             void* buffer, unsigned area)
{
   assert(area);
    
   return ExtendedAbsReadWrite(handle+1, nsects, lsect, buffer, area);
}

#endif

/*
** Public read sector function.
*/
#ifdef USE_SECTOR_CACHE

int ReadSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer)
{
    SECTOR sector, j;
    SECTOR beginsector=0;
    BOOL LastReadFromDisk = FALSE;
    char* cbuffer = (char*) buffer, *cbuffer1;
    char secbuf[512];
    int i;
    
    assert(buffer);

    for (i=0, sector = lsect; i < nsects; sector++, i++)
    {
        if (RetreiveCachedSector(handle->UniqueNumber,
                                 sector, 
                                 secbuf))
   {
#ifndef NDEBUG
#ifdef  DEBUG_CACHE  
#ifndef WRITE_BACK_CACHE
       char sectbuf1[512];
       
       if (handle->ReadFunc(handle->handle, 1, sector, sectbuf1) == -1)
      assert(FALSE);
         
       if (memcmp(secbuf, sectbuf1, 512) != 0)
       {
      assert(FALSE);
       }
#endif       
#endif
#endif             
            if (LastReadFromDisk)
            {
               if (handle->ReadFunc(handle->handle, (int)(sector-beginsector),
                                    beginsector, cbuffer) == -1)
               {
                  RETURN_FTEERROR(-1);
          }

          /* Put the read sectors in the cache. */
          cbuffer1 = cbuffer;
          for (j = beginsector; j < sector; j++)
          {
         CacheSector(handle->UniqueNumber, j, cbuffer1, FALSE, 0);
         cbuffer1 += BYTESPERSECTOR;
          }

               cbuffer += BYTESPERSECTOR * (int)(sector-beginsector);
               LastReadFromDisk = FALSE;
           }
           memcpy(cbuffer, secbuf, BYTESPERSECTOR);
           cbuffer += BYTESPERSECTOR;
        }
        else
        {
           if (!LastReadFromDisk)
           {
              LastReadFromDisk = TRUE;
              beginsector = sector;     
           }
        }
    } 

    if (LastReadFromDisk)
    {
       if (handle->ReadFunc(handle->handle, (int)((lsect+nsects)-beginsector),
                            beginsector, cbuffer) == -1)
       {
     RETURN_FTEERROR(-1); 
       }

       /* Put the read sectors in the cache. */
       for (j = beginsector; j < (lsect+nsects); j++)
       {
    CacheSector(handle->UniqueNumber, j, cbuffer, FALSE, 0);
    cbuffer += BYTESPERSECTOR;
       }
    }
    
    return 0;
}
#else
int ReadSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer)
{   
    assert(buffer);
    
    return handle->ReadFunc(handle->handle, nsects, lsect, buffer);
}
#endif

char *ReadSectorsAddress(RDWRHandle handle, unsigned long position)
{
    if (position != LastReadBufferSector)
    {
   if (ReadSectors(handle, 1, position, LastReadBuffer))
      return NULL;
      
        LastReadBufferSector = position;
    } 

    return LastReadBuffer;
}


/*
** Public write sector function.
*/
#ifdef USE_SECTOR_CACHE
#ifdef WRITE_BACK_CACHE
int WriteSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer,
                 unsigned area)
{
    int i;
    SECTOR sector;
    char* cbuffer = (char*) buffer; 
    
    assert(buffer && nsects);

    if ((LastReadBufferSector >= lsect) && 
                                (LastReadBufferSector < (lsect + nsects)))
    {        
       LastReadBufferSector = INVALID_SECTOR;   
    }
    
    if (handle->ReadWriteMode == READING)
    { 
        SetFTEerror(FTE_WRITE_ON_READONLY);
   RETURN_FTEERROR(-1);
    }       
    
    for (i=0, sector = lsect; i < nsects; sector++, i++)
    {
        CacheSector(handle->UniqueNumber, sector, cbuffer, TRUE, area);
        cbuffer += BYTESPERSECTOR;
    }     

/*/---- To BE REMOVED    
    
    if (handle->WriteFunc(handle->handle, nsects, lsect, buffer, area) == -1)
    { 
       for (i=0, sector = lsect; i < nsects; sector++, i++)
           UncacheSector(handle->UniqueNumber, sector);
       RETURN_FTEERROR(-1);
    }
    
//---- To BE REMOVED    */
    
    return 0;
}                 
#else
int WriteSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer,
                 unsigned area)
{
    int i;
    SECTOR sector;
    char* cbuffer = (char*) buffer;
    
    assert(buffer && nsects);

    if ((LastReadBufferSector >= lsect) && 
                                (LastReadBufferSector < (lsect + nsects)))
    {        
       LastReadBufferSector = INVALID_SECTOR;       
    }
    
    if (handle->ReadWriteMode == READING)
    { 
        SetFTEerror(FTE_WRITE_ON_READONLY);
   RETURN_FTEERROR(-1);
    }   

    if (handle->WriteFunc(handle->handle, nsects, lsect, buffer, area) == -1)
    { 
       for (i=0, sector = lsect; i < nsects; sector++, i++)
           UncacheSector(handle->UniqueNumber, sector);
       RETURN_FTEERROR(-1);
    }
  
    for (i=0, sector = lsect; i < nsects; sector++, i++)
    {
        CacheSector(handle->UniqueNumber, sector, cbuffer, FALSE, 0);
        cbuffer += BYTESPERSECTOR;
    }
    return 0;   
}
#endif
#else
int WriteSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer,
                 unsigned area)
{
    assert(buffer && nsects);
    
    if ((LastReadBufferSector >= lsect) && 
                                (LastReadBufferSector < (lsect + nsects)))
    {        
       LastReadBufferSector = INVALID_SECTOR;       
    }
        
    if (handle->ReadWriteMode == READINGANDWRITING)
       return handle->WriteFunc(handle->handle, nsects, lsect, buffer, area);
    else
    {
        SetFTEerror(FTE_WRITE_ON_READONLY); 
   RETURN_FTEERROR(-1);
    }
}
#endif

/*
** Private function to initialise the reading and writing of sectors.
*/
static int PrivateInitReadWriteSectors(char* driveorfile, int modus,
                                       RDWRHandle* handle)
{
    unsigned sectorsize;
    
    assert(driveorfile && handle);
    assert((modus == O_RDWR) || (modus == O_RDONLY));

    *handle = (RDWRHandle) malloc(sizeof(struct RDWRHandleStruct));
    if (*handle == NULL) RETURN_FTEERR(FALSE);
    memset(*handle, 0, sizeof(struct RDWRHandleStruct));

#ifdef ALLOW_REALDISK
    if (HasAllFloppyForm(driveorfile))
    {
       (*handle)->handle = toupper(driveorfile[0]) - 'A';

       sectorsize = GetFAT32SectorSize((*handle)->handle);

       if (sectorsize)
       {
          (*handle)->ReadFunc  = ReadFromRealDisk32;
     (*handle)->WriteFunc = WriteToRealDisk32;
     (*handle)->BytesPerSector = sectorsize;
       }
       else
       {
     struct dfree diskfree;

     /* New trick: we start out asuming that we have a volume < 32MB. 
                   If it is not possible to use absread/abswrite on
                   this disk, we automatically switch over to 
              disk_read_ext/disk_write_ext            
      
        We can probably get away with this, because it is not a FreeDOS concern */    
     
     (*handle)->ReadFunc  = ReadFromRealDisk12;
          (*handle)->WriteFunc = WriteToRealDisk12;
     
     getdfree((*handle)->handle+1, &diskfree);
     
     if (diskfree.df_sclus == 0xffff)
     {
        free(*handle);
        *handle = NULL;
        SetFTEerror(FTE_READ_ERR);  
        RETURN_FTEERR(FALSE);
     }
     
     (*handle)->BytesPerSector = diskfree.df_bsec;
       }
    }
    else

#endif
    {
       (*handle)->ReadFunc    = ReadFromImageFile;
       (*handle)->WriteFunc   = WriteToImageFile;
       (*handle)->handle      = open (driveorfile, modus|O_BINARY);
       (*handle)->IsImageFile = TRUE;

       if ((*handle)->handle == -1)
       {
          free(*handle);
          *handle = NULL;
     SetFTEerror(FTE_READ_ERR); 
     RETURN_FTEERR(FALSE); 
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
       if (lseek((*handle)->handle, 11, SEEK_SET) == -1L)
       {
     free(*handle);
     *handle = NULL;
     SetFTEerror(FTE_READ_ERR);  
     RETURN_FTEERR(FALSE); 
       }

       if (read((*handle)->handle, &((*handle)->BytesPerSector),
               sizeof(unsigned short)) == -1)
       {
     free(*handle);
     *handle = NULL;
     SetFTEerror(FTE_READ_ERR);  
     RETURN_FTEERR(FALSE); 
       }

       /* No need to reset to the beginning of the file. */
    }

    if ((*handle)->BytesPerSector != 512)
    {
       SetFTEerror(FTE_INVALID_BYTESPERSECTOR);
       RETURN_FTEERR(FALSE);   
    }
    
#ifdef USE_SECTOR_CACHE

    (*handle)->UniqueNumber = UniqueCounter++;  
    
#ifdef WRITE_BACK_CACHE
    
    /* Install the write functions for the write back cache */
    InstallWriteFunction((*handle)->UniqueNumber, 
                         (*handle)->WriteFunc,
                         (*handle)->handle);
    
#endif    
#endif 
    
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
**  Init for writing only (reasonably stupid).
*
int InitWriteSectors(char* driveorfile, RDWRHandle* handle)
{
    return PrivateInitReadWriteSectors(driveorfile, O_WRONLY, handle);
}
*/

/*
**  Close the sector read/write system.
*/
void CloseReadWriteSectors(RDWRHandle* handle)
{
    if (*handle != NULL)
    {
#ifdef USE_SECTOR_CACHE
#ifdef WRITE_BACK_CACHE   
        UninstallWriteFunction((*handle)->UniqueNumber);
#endif        
#endif         
            
       if ((*handle)->IsImageFile) close((*handle)->handle);
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
