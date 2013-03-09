/*    
   Preread.c - preread expected clusters in the order that they
               are stored on the disk (as to avoid drive head movement).

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

#include <stdlib.h>

#include "fte.h"

#define MAX_ALLOCATING 16384

static BOOL PreReadLargeCluster(RDWRHandle, 
                                SECTOR start, unsigned char amofsectors,
                                char* buftouse, unsigned bufsize);
static BOOL ClusterPreReader(RDWRHandle handle, CLUSTER label,
                             SECTOR datasector, void** structure);    
static BOOL OnlyFilledRead(RDWRHandle handle, SECTOR lsect, 
                           unsigned numsectors, char* buf);
                                
static char DefaultReadBuf[BYTESPERSECTOR];

static char* AllocateReadBuf(unsigned long filelen,
                             unsigned* length)
{
    char* result = NULL;
    
    if (*length < BYTESPERSECTOR)
    {
       *length = BYTESPERSECTOR;
       return DefaultReadBuf;      
    }
    
    *length = (filelen < (unsigned long)MAX_ALLOCATING) ?
              (unsigned)filelen : MAX_ALLOCATING;
              
    *length = (*length / BYTESPERSECTOR) * BYTESPERSECTOR;
              
    while ((*length > BYTESPERSECTOR))
    {
          result = (char*) FTEAlloc(*length);
          if (result) break;
          *length = *length / 2;
    }
    
    if (*length <= BYTESPERSECTOR)
    {
       result = DefaultReadBuf; 
       *length = BYTESPERSECTOR;
    }
    
    return result;
}          

static void FreeReadBuf(char* ReadBuf)
{
    if (ReadBuf && (ReadBuf != DefaultReadBuf))
       FTEFree(ReadBuf);
}                   

BOOL PreReadClusterSequence(RDWRHandle handle, CLUSTER start,
                            unsigned long length)
{
    char* ReadBuf;   
    unsigned BufSize;
    unsigned long blocks, i, sectorstoread;
    unsigned rest;
    unsigned char sectorspercluster;
    SECTOR startsector;
    
    if (!CacheActive()) return TRUE;
    if (length == 0) return TRUE;
    
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERROR(FALSE); 
    
    sectorstoread = length * sectorspercluster; 
    startsector = ConvertToDataSector(handle, start);
    if (!startsector) RETURN_FTEERROR(FALSE); 
       
    ReadBuf = AllocateReadBuf(sectorstoread * BYTESPERSECTOR, &BufSize);
    BufSize /= BYTESPERSECTOR;
    
    blocks = (length * sectorspercluster) / BufSize;
    rest   = (unsigned)((length * sectorspercluster) % BufSize);
    
    for (i = 0; i < blocks; i++)
    {   
       if (!OnlyFilledRead(handle, startsector, BufSize, ReadBuf) == -1)
        {
           FreeReadBuf(ReadBuf);     
           RETURN_FTEERROR(FALSE); 
        }
        
        startsector += BufSize;
    }
    
    if (rest)
    {           
        if (!OnlyFilledRead(handle, startsector, rest, ReadBuf) == -1)
        {
           FreeReadBuf(ReadBuf);      
           RETURN_FTEERROR(FALSE); 
        }        
    }
    
    FreeReadBuf(ReadBuf); 
    return TRUE;
}

struct Pipe
{
    char* buf;
    unsigned bufsize;   /* In sectors */
    unsigned index; 
    unsigned maxindex;
    CLUSTER  prevcluster;
    CLUSTER  firstcluster;
};

BOOL PreReadClusterChain(RDWRHandle handle, CLUSTER start)
{
    SECTOR  firstsector;
    char* ReadBuf;   
    struct Pipe pipe, *ppipe = &pipe;
    unsigned char sectorspercluster; 
    unsigned BufSize;
    
    if (!CacheActive()) return TRUE;
        
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERROR(FALSE); 
    
    ReadBuf = AllocateReadBuf(MAX_ALLOCATING, &BufSize);
    BufSize /= BYTESPERSECTOR;    
        
    pipe.buf          = ReadBuf;
    pipe.bufsize      = BufSize;
    pipe.index        = 0;
    pipe.maxindex     = BufSize / sectorspercluster;
    pipe.prevcluster  = 0;
    pipe.firstcluster = start;
    
    if (!FileTraverseFat(handle, start, ClusterPreReader, (void**) &ppipe))
    {            
       FreeReadBuf(ReadBuf);  
       RETURN_FTEERROR(FALSE); 
    }
       
    if (pipe.index)
    {
       firstsector = ConvertToDataSector(handle, pipe.firstcluster);
       if (!firstsector)
       {        
          FreeReadBuf(ReadBuf);       
          RETURN_FTEERROR(FALSE); 
       }
       
       if (ReadSectors(handle, pipe.index*sectorspercluster, 
                       firstsector, pipe.buf) == -1)
       {
           FreeReadBuf(ReadBuf);  
           RETURN_FTEERROR(FALSE); 
       }    
    }
    
    FreeReadBuf(ReadBuf);  
    return TRUE;
}          

static BOOL ClusterPreReader(RDWRHandle handle, CLUSTER label,
                             SECTOR datasector, void** structure)
{
    SECTOR  firstsector;
    CLUSTER cluster;
    unsigned char sectorspercluster;
    struct Pipe* pipe = *((struct Pipe**) structure);
    
    if (label);
   
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FAIL;
    
    if (pipe->maxindex == 0) /* cluster > read buf */
    {
       if (!PreReadLargeCluster(handle, datasector, sectorspercluster, 
                                pipe->buf, pipe->bufsize))
          RETURN_FTEERROR(FAIL); 
    }
    else
    {
       cluster = DataSectorToCluster(handle, datasector);
       
       if ((pipe->index > 0) &&
           ((pipe->prevcluster != cluster-1) ||
            (pipe->index == pipe->maxindex)))
       {    
          firstsector = ConvertToDataSector(handle, pipe->firstcluster);
          if (!firstsector) RETURN_FTEERROR(FAIL); 
             
          if (ReadSectors(handle, pipe->index*sectorspercluster, 
                          firstsector, pipe->buf) == -1)
          {
              RETURN_FTEERROR(FAIL); 
          }
            
          pipe->index = 0;
          pipe->firstcluster = cluster;
       }
       else
       {
          pipe->index++;
       }                                      
    }
    
    pipe->prevcluster = cluster;
           
    return TRUE;    
}

static BOOL PreReadLargeCluster(RDWRHandle handle, 
                                SECTOR start, unsigned char amofsectors,
                                char* buftouse, unsigned bufsize)
{
    unsigned blocks = amofsectors / bufsize;
    unsigned rest  = amofsectors % bufsize;
    unsigned i;
    
    for (i = 0; i < blocks; i++)
    {
        if (ReadSectors(handle, bufsize, start, buftouse) == -1)
        {
           RETURN_FTEERROR(FALSE); 
        }  
        
        start += bufsize;
    }
    
    if (rest)
    {
        if (ReadSectors(handle, rest, start, buftouse) == -1)
        {
           RETURN_FTEERROR(FALSE); 
        }      
    }
  
    return TRUE;
}

static BOOL OnlyFilledRead(RDWRHandle handle, SECTOR lsect, 
                           unsigned numsectors, char* buf)
{
    unsigned i;
    SECTOR  start = lsect;
    CLUSTER cluster, label;
    
    for (i = 0; i < numsectors; i++)
    {
        cluster = DataSectorToCluster(handle, lsect+i);
        if (!cluster) RETURN_FTEERROR(FALSE); 
                
        if (!GetNthCluster(handle, cluster, &label))
        {
           RETURN_FTEERROR(FALSE); 
        }
        
        if (FAT_FREE(label) || FAT_BAD(label))
        {
           if (i)
           {        
              if (ReadSectors(handle, i, start, buf) == -1)
              {
                 RETURN_FTEERROR(FALSE); 
              }
           }
                  
           start = lsect+i+1;
        }
    }

    if (start < lsect+numsectors)
    {
       if (ReadSectors(handle, numsectors - (unsigned)(start - lsect), 
                       start, buf) == -1)
       {
          RETURN_FTEERROR(FALSE); 
       }       
    }
    
    return TRUE;
}                           
