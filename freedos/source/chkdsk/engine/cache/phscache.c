/*    
   Physcache.c - sector cache for extended memory (XMS/EMS part).

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

#include <assert.h>

#include "fte.h"

#include "xms.h"
#include "ems.h"

#include "phscache.h"
#include "logcache.h"

#ifdef USE_EMS
static unsigned XMSStart;
#endif

static unsigned EMSStart;
static unsigned EMSEnd;

/* XMS data */
static unsigned int  XMSHandle;

/* EMS data */
#ifdef USE_EMS
static int           EMSHandle;  
#endif

extern struct LogicalBlockInfo LogicalBlocks[];

unsigned InitialisePhysicalCache(void)
{     
     unsigned long XMSsize;
    
#ifdef USE_EMS    
     int i;         
     unsigned EMSsize;
     unsigned int EMSBase;
#endif
     
     /* Allocate XMS */ 
     if (XMSinit())
     {
        XMSsize = XMScoreleft();
	if (XMSsize > 65536L)   /* Don't use XMS if not at least 64 Kb available */
        {
           /* Round down to the nearest multiple of 16Kb */
	   EMSStart = (unsigned)(XMSsize / CACHEBLOCKSIZE);

	   if (EMSStart > 4096) EMSStart = 4096;	/* Maximaly 32Mb */
	    
	   XMSsize =  (unsigned long)EMSStart * CACHEBLOCKSIZE;
           XMSHandle = XMSalloc(XMSsize);
           
           if (!XMSHandle)
           {
              EMSStart = 0;
           }
        }
        else
        {
           EMSStart = 0;
        }        
     }
     else
     {
        EMSStart = 0;
     }

#ifdef USE_EMS

     /* Allocate EMS */
     EMSBase = EMSbaseaddress();

     if (EMSBase && (EMSversion() != -1) && (EMSstatus() == 0))
     {
	/* Try allocating all available EMS memory. */
        EMSsize = EMSpages();
	if (EMSsize > 3)
        {
           EMSHandle = EMSalloc((int)EMSsize);
           if (EMSHandle != -1)
           {
              EMSEnd = EMSStart + EMSsize;
              
              /* Map the first EMS pages */
              for (i = 0; i < 4; i++)
                  if (EMSmap(i, EMSHandle, i) == -1)
                  {
                     EMSfree(EMSHandle);
		     EMSEnd = EMSStart;
		  }
	   }
	   else
	   {
	      EMSEnd = EMSStart;
	   }
	}
        else
        {
	   EMSEnd = EMSStart;
	}
     }
     else
     {
	EMSEnd = EMSStart;
     }
     
#else

     EMSEnd = EMSStart;
     
#endif

     return EMSEnd; /* Return number of physical blocks */
}

void ClosePhysicalCache(void)
{
     if (EMSStart > 0)
        XMSfree(XMSHandle);

#ifdef USE_EMS       
     if (EMSStart != EMSEnd)
        EMSfree(EMSHandle);
#endif
}

#ifdef USE_EMS

int GetPhysicalMemType(unsigned physblock)
{
     if ((physblock >= XMSStart) && (physblock < EMSStart))
     {
        return XMS;
     }
     if ((physblock >= EMSStart) && (physblock < EMSEnd))
     {
        return EMS;
     }
        
     assert(FALSE);
     return NOTEXT;
}

BOOL IsEMSCached(void)
{
    return EMSStart != EMSEnd;
}

#endif

static char* BlockAddress;
static unsigned CurrentPhysBlock;

BOOL RemapXMSBlock(int logblock, unsigned physblock)
{
     BlockAddress = LogicalBlocks[logblock].address;
     CurrentPhysBlock = LogicalBlocks[logblock].PhysicalBlock;
     
     assert((logblock >= 0) && (logblock < 4));
    
     if (DOStoXMSmove(XMSHandle, (unsigned long)CurrentPhysBlock * CACHEBLOCKSIZE,
                      BlockAddress, CACHEBLOCKSIZE) != CACHEBLOCKSIZE)
     {
        RETURN_FTEERROR(FALSE); 
     }

     if (XMStoDOSmove(BlockAddress, XMSHandle, (unsigned long)physblock * CACHEBLOCKSIZE,
                      CACHEBLOCKSIZE) != CACHEBLOCKSIZE)
     {
        RETURN_FTEERROR(FALSE);        
     }
     
     return TRUE;
}

#ifdef USE_EMS      
BOOL RemapEMSBlock(int logblock, unsigned physblock)
{
     assert((logblock >= 0) && (logblock < 4));
     assert(physblock < EMSEnd);
     assert(physblock >= EMSStart);
    
     return EMSmap(logblock, EMSHandle, physblock - EMSStart) == 0;
}
#endif    

#ifdef _WIN32

char XMSSpace[16*1024*1024];    // 16 Mb

int XMSinit(){return 1;}

 unsigned long XMScoreleft(void)
 {
    return 16*1024*1024;
 }

unsigned int XMSalloc(unsigned long size)
{
    return 1;
}

int  XMSfree(unsigned int handle)
{
    return 1;
}


unsigned DOStoXMSmove(unsigned int desthandle,                      
                      unsigned long destoff, const char *src,       
                      unsigned n)
{
    memcpy(&XMSSpace[destoff], src, n); 

    return n;
}

unsigned XMStoDOSmove(char *dest, unsigned int srchandle,          
                      unsigned long srcoff, unsigned n)
{
    memcpy(dest, &XMSSpace[srcoff], n);

    return n;
}

#endif

