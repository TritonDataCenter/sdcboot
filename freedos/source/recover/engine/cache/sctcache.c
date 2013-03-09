/*    
   Sctcache.c - sector cache interface routines.

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

#include "blkcache.h"

static int CacheLocked = FALSE;
static int CacheStarted = FALSE;
static int CacheInitialised = FALSE;
static int CacheCorrupt = FALSE;

void InitSectorCache(void)
{
#ifdef USE_SECTOR_CACHE        
   CacheInitialised = InitialisePerBlockCaching();
#endif   
}

void CloseSectorCache(void)
{
   if (CacheInitialised)
      ClosePerBlockCaching();
}

void LockSectorCache()
{
  CacheLocked = TRUE;   
}

void UnlockSectorCache()
{
  CacheLocked = FALSE;      
}

void StartSectorCache()
{
  CacheStarted = TRUE;  
}

void StopSectorCache()
{
  CacheStarted = FALSE;      
}

void CacheIsCorrupt()
{
  CacheCorrupt = TRUE;
  CloseSectorCache();
}

int CacheSector(unsigned devid, SECTOR sector, char* buffer, BOOL dirty,
                unsigned area)
{
   if (!CacheActive()) return TRUE;
   
   return CacheBlockSector(devid, sector, buffer, dirty, area);        
}

int RetreiveCachedSector(unsigned devid, SECTOR sector, char* buffer)
{
   if (!CacheActive())
      return FALSE;
      
   return RetreiveBlockSector(devid, sector, buffer);
}

void UncacheSector(unsigned devid, SECTOR sector)
{
   if (!CacheActive()) return;
   
   UncacheBlockSector(devid, sector);
}

void InvalidateCache()
{
   if (!CacheActive()) return;
   
   InvalidateBlockCache();
}

BOOL CacheActive()
{
    return CacheInitialised && CacheStarted && (!CacheLocked) && 
           (!CacheCorrupt);
}

int CommitCache(void)
{
#ifdef USE_SECTOR_CACHE
    return WriteBackCache();
#endif
}