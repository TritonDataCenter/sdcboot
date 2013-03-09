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

#include "fte.h"

#include "xms.h"
#include "ems.h"

#include "phscache.h"
#include "logcache.h"

static unsigned long XMSStart;
static unsigned long EMSStart;
static unsigned long EMSEnd;

/* XMS data */
static unsigned int  XMSHandle;

/* EMS data */
static int           EMSHandle;  

unsigned long InitialisePhysicalCache(void)
{
     int i;
     unsigned EMSsize;
     unsigned long XMSsize;
     unsigned int  EMSBase;
     
     /* Allocate XMS */ 
     if (XMSinit())
     {
        XMSsize = XMScoreleft();
	if (XMSsize > 65536L)   /* Don't use XMS if not at least 64 Kb available */
        {
           /* Round down to the nearest multiple of 16Kb */
	   EMSStart = XMSsize / CACHEBLOCKSIZE;
           XMSsize =  EMSStart * CACHEBLOCKSIZE;
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

int GetPhysicalMemType(unsigned long physblock)
{
     if ((physblock >= XMSStart) && (physblock < EMSStart))
     {
        return XMS;
     }
     if ((physblock >= EMSStart) && (physblock < EMSEnd))
     {
        return EMS;
     }
        
     return NOTEXT;
}

BOOL IsEMSCached(void)
{
    return EMSStart != EMSEnd;
}

BOOL RemapXMSBlock(int logblock, unsigned long physblock)
{
     char* block = GetLogicalBlockAddress(logblock);
     unsigned long CurrentPhysBlock = GetMappedPhysicalBlock(logblock);
     
     if (DOStoXMSmove(XMSHandle, CurrentPhysBlock * CACHEBLOCKSIZE,
                      block, CACHEBLOCKSIZE) != CACHEBLOCKSIZE)
     {
        return FALSE; 
     }

     if (XMStoDOSmove(block, XMSHandle, physblock * CACHEBLOCKSIZE,
                      CACHEBLOCKSIZE) != CACHEBLOCKSIZE)
     {
        return FALSE;       
     }
     
     return TRUE;
}

BOOL RemapEMSBlock(int logblock, unsigned long physblock)
{
     return EMSmap(logblock, EMSHandle,
                   (unsigned) (physblock - EMSStart)) == 0;
}