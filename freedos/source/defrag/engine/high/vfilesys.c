#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "fte.h"

/* Coreleft() says about 329Kb free when starting defragmentation under FreeDOS, normal drivers installed */
#define MAX_ENTRIES 10
#define CACHE_SIZE  17408

static BOOL Ensurelength(VirtualDirectoryEntry* entry, RDWRHandle handle, CLUSTER start, unsigned long newlength);
static CLUSTER GetNthRealCluster(VirtualDirectoryEntry* entry, CLUSTER start, unsigned long n);
static BOOL ReadVirtualCluster(RDWRHandle handle, CLUSTER currentcluster, 
                               unsigned long start, unsigned long length, char* data);
static BOOL WriteVirtualCluster(RDWRHandle handle, CLUSTER currentcluster, 
                                unsigned long start, unsigned long length, char* data);

static BOOL RealyWriteVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length);
static BOOL RealyReadVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length);

static BOOL SetNextCluster(RDWRHandle handle, CLUSTER current, CLUSTER next);
static BOOL ZeroCluster(RDWRHandle handle, CLUSTER cluster);

static char SectorBuffer[BYTESPERSECTOR];

VirtualDirectoryEntry VirtualDirectory[MAX_ENTRIES]; 

VirtualDirectoryEntry* CreateVirtualFile(RDWRHandle handle)
{
    int i;

    /* Find a free spot in the directory */
    for (i = 0; i < MAX_ENTRIES; i++)
        if (!VirtualDirectory[i].used)
        {
            /* Try allocating memory */
            VirtualDirectory[i].cache = FTEAlloc(CACHE_SIZE);
            VirtualDirectory[i].hasCache = VirtualDirectory[i].cache != 0;

            VirtualDirectory[i].used = TRUE;
            VirtualDirectory[i].start = 0;
            VirtualDirectory[i].length = 0;
            VirtualDirectory[i].handle = handle;
            VirtualDirectory[i].cachedBlock = 0;
            VirtualDirectory[i].lowestClusterInvolved = ULONG_MAX;   

            return &VirtualDirectory[i];
        }

        return NULL;
}

void DeleteVirtualFile(VirtualDirectoryEntry* entry)
{
    if (entry->used)
    {
        if (entry->cache) FTEFree(entry->cache);    
        entry->used = FALSE;
    }
}

BOOL GetVirtualFileSpaceUsed(RDWRHandle handle, unsigned long* spaceUsed) /* In clusters ! */
{
    int i;
    unsigned long sectorspercluster;
    unsigned long bytespercluster;

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;

    bytespercluster = (sectorspercluster * BYTESPERSECTOR)-sizeof(CLUSTER);

    *spaceUsed = 0;

    for (i=0; i < MAX_ENTRIES; i++)
    {
        if ((VirtualDirectory[i].used) &&
            (VirtualDirectory[i].handle == handle))
        {
            *spaceUsed += (VirtualDirectory[i].length / bytespercluster) + 
                (VirtualDirectory[i].length % bytespercluster > 0);
        }
    }

    return TRUE;
}

BOOL WriteVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length)
{
    unsigned long block, rest;

    /* First see wether we actually have a cache */
    if (!entry->hasCache)
        return RealyWriteVirtualFile(entry, pos, data, length);

    /* See wether the right block is in the cache */
    block = pos / CACHE_SIZE;

    if (entry->cachedBlock == block)
    {
        /* We are writing to this cache block */
        entry->dirty = TRUE;

        /* If it is, write what we can to it */
        rest = CACHE_SIZE - (pos % CACHE_SIZE);

        if (length <= rest)
        {
            memcpy(&entry->cache[(int)(pos % CACHE_SIZE)], data, (size_t)length);
            return TRUE;
        }

        /* Write the first part to the cache and the rest to the virtual file */
        memcpy(&entry->cache[(int)(pos % CACHE_SIZE)], data, (size_t)rest);

        return RealyWriteVirtualFile(entry, pos+rest, data+(unsigned)rest, length-rest);
    }
    else
    {
        /* See wether the block is dirty and write it back if it is */
        if (entry->dirty)
        {
            if (!RealyWriteVirtualFile(entry, entry->cachedBlock*CACHE_SIZE, entry->cache, CACHE_SIZE))
                return FALSE;

            entry->dirty = FALSE;
        }

        /* Read the block containing the data in memory */
   if (entry->length >= block*CACHE_SIZE+CACHE_SIZE)
        {
            if (!RealyReadVirtualFile(entry, block*CACHE_SIZE, entry->cache, CACHE_SIZE))
                return FALSE;
   }
   else
   {
       memset(entry->cache, 0, CACHE_SIZE);
   }

        entry->cachedBlock = block;

        /* Write the real data */
        return WriteVirtualFile(entry, pos, data, length);
    }
}

BOOL ReadVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length)
{
    unsigned long block, rest;

    /* See wether we actually have a cache */
    if (!entry->hasCache)
        return RealyReadVirtualFile(entry, pos, data, length);

    /* See wether the right block is in the cache */
    block = pos / CACHE_SIZE;

    if (entry->cachedBlock == block)
    {
        /* If it is, read what we can from it */
        rest = CACHE_SIZE - (pos % CACHE_SIZE);

        if (length < rest)
        {
            memcpy(data, &entry->cache[(int)(pos % CACHE_SIZE)], (int)length);
            return TRUE;
        }

        /* Read the first part from the cache and the rest from the virtual file */
        memcpy(data, &entry->cache[(int)(pos % CACHE_SIZE)], (int)rest);

        return RealyReadVirtualFile(entry, pos+rest, data+(unsigned)rest, length-rest);
    }
    else
    {
        /* See wether the block is dirty and write it back if it is */
        if (entry->dirty)
        {
            if (!RealyWriteVirtualFile(entry, entry->cachedBlock*CACHE_SIZE, entry->cache, CACHE_SIZE))
                return FALSE;

            entry->dirty = FALSE;
        }

        /* Read the block containing the data in memory */
        if (entry->length >= block*CACHE_SIZE+CACHE_SIZE)
        {
            if (!RealyReadVirtualFile(entry, block*CACHE_SIZE, entry->cache, CACHE_SIZE))
                return FALSE;
   }
   else
   {
       memset(entry->cache, 0, CACHE_SIZE);
   }

        entry->cachedBlock = block;

        /* Return the real data */
        return ReadVirtualFile(entry, pos, data, length);
    }
}

BOOL IsClusterUsed(RDWRHandle handle, CLUSTER cluster)
{
    int i;
    CLUSTER current;
    SECTOR datasector;

    assert(cluster);

    for (i=0; i < MAX_ENTRIES; i++)
    {
        if (VirtualDirectory[i].used)
        {   
            if (VirtualDirectory[i].lowestClusterInvolved == cluster)
            {
                return TRUE;   
            }
            else if (VirtualDirectory[i].lowestClusterInvolved < cluster)
            {
                /* Go through the chain to see wether it contains the cluster */
                current = VirtualDirectory[i].start;
                while (current && current != cluster)
                {
                    datasector = ConvertToDataSector(handle, current);
                    if (!datasector)
                        return FAIL;

                    if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
                        return FAIL;

                    current = ((VirtualFileSpace*) SectorBuffer)->next;
                }

                if (current == cluster)
                    return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL EnsureClusterFree(RDWRHandle handle, CLUSTER cluster)
{
    return EnsureClustersFree(handle, cluster, 1);
}

BOOL EnsureClustersFree(RDWRHandle handle, CLUSTER cluster, unsigned long n)
{
    int i;
    CLUSTER last;

    /* Ensure all free in this range! */
    unsigned long ni;
    SECTOR datasector, origBase, targetBase;
    CLUSTER current, chaser = 0, origCluster=cluster;
    unsigned long sectorspercluster;

    /* Go through the directory and see wether any might have the given cluster in it */    
    for (ni=0; ni < n; ni++)
    {
        for (i=0; i < MAX_ENTRIES; i++)
        {
            if ((VirtualDirectory[i].used) &&
                (VirtualDirectory[i].lowestClusterInvolved <= cluster) &&
                (VirtualDirectory[i].start))
            {
                /* Go through the chain to see wether it contains the cluster */
                chaser=0;
                current = VirtualDirectory[i].start;
                while (current && (current != cluster))
                {
                    datasector = ConvertToDataSector(handle, current);
                    if (!datasector)
                        return FALSE;

                    if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
                        return FALSE;

                    chaser  = current;
                    current = ((VirtualFileSpace*) SectorBuffer)->next;
                }

                /* If found, relocate it */
                if (current == cluster)
                {
                    BOOL retVal;

                    VirtualDirectory[i].isFragmented = TRUE;
                    
                    /* Find the last free cluster on the disk */
                    if (!FindLastFreeCluster(handle, &last) == FAIL)
                        return FALSE;

                    while (((retVal = (IsClusterUsed(handle, last))) == TRUE) ||
                           ((last >= origCluster) && (last < origCluster+n)))
                    {
                        if (!GetPreviousFreeCluster(handle, last, &last))
                            return FALSE;

                        if (last == 0)     /* Out of disk space! */
                            return FALSE;
                    }

                    if (retVal == FAIL) return FALSE;

                    if (chaser == 0) /* First cluster */
                    {
                        VirtualDirectory[i].start = last;
                    }   
                    else
                    {
                        datasector = ConvertToDataSector(handle, chaser);
                        if (!datasector)
                            return FALSE;

                        if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
                            return FALSE;   

                        ((VirtualFileSpace*) SectorBuffer)->next = last;

                        if (!WriteDataSectors(handle, 1, datasector, SectorBuffer))
                            return FALSE;  
                    }

                    // Copy the data from original cluster to  new cluster
                    sectorspercluster = GetSectorsPerCluster(handle);
                    if (!sectorspercluster) return FALSE;

                    origBase = ConvertToDataSector(handle, current);
                    if (!origBase) return FALSE;
                    targetBase = ConvertToDataSector(handle, last);
                    if (!targetBase) return FALSE;

                    if (!CopySectors(handle, origBase, targetBase, (unsigned int)sectorspercluster))
                        return FALSE;

                    if (last < VirtualDirectory[i].lowestClusterInvolved)
                        VirtualDirectory[i].lowestClusterInvolved = last;

                    // continue with next cluster
                    break;
                }
            }
        }

        cluster++;
    }
    return TRUE;
}

static BOOL RealyWriteVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length)
{
    unsigned long bytespercluster, rest, i, sectorspercluster;
    CLUSTER currentcluster, startcluster;  

    if (pos + length > entry->length)
    {
        if (!Ensurelength(entry, entry->handle, entry->start, pos + length))
            return FALSE;

        entry->length = pos + length;
    }

    sectorspercluster = GetSectorsPerCluster(entry->handle);
    if (!sectorspercluster) 
        return FALSE;

    bytespercluster = (sectorspercluster * BYTESPERSECTOR)-sizeof(CLUSTER);

    /* Search for the cluster that has the start byte */
    startcluster = pos / bytespercluster;

    /* Write the first part out to this cluster */
    rest = bytespercluster - (pos % bytespercluster);

    currentcluster = GetNthRealCluster(entry, entry->start, startcluster);
    if (!currentcluster) 
        return FALSE;

    if (length <= rest)
    {
        return WriteVirtualCluster(entry->handle, currentcluster, pos % bytespercluster + sizeof(CLUSTER), length, data); 
    }

    if (!WriteVirtualCluster(entry->handle, currentcluster, pos % bytespercluster + sizeof(CLUSTER), rest, data))
        return FALSE;

    data   += (unsigned)rest;
    length -= rest;

    for (i = 0; i < length / bytespercluster; i++)
    {
        currentcluster = GetNthRealCluster(entry, currentcluster, 1);

        if (!WriteVirtualCluster(entry->handle, currentcluster, sizeof(CLUSTER), bytespercluster, data))
            return FALSE;

        data += (unsigned)bytespercluster;
    }

    if (length % bytespercluster)
    {
        currentcluster = GetNthRealCluster(entry, currentcluster, 1);

        if (!WriteVirtualCluster(entry->handle, currentcluster, sizeof(CLUSTER), length%bytespercluster, data))
            return FALSE; 
    }

    return TRUE;
}

static BOOL RealyReadVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length)
{
    unsigned long sectorspercluster;
    unsigned long bytespercluster, rest, i;
    CLUSTER currentcluster, startcluster;

    /* First check bounderies */
    if (pos + length > entry->length) 
        return FALSE;

    sectorspercluster = GetSectorsPerCluster(entry->handle);
    if (!sectorspercluster) 
        return FALSE;

    bytespercluster = (sectorspercluster * BYTESPERSECTOR)-sizeof(CLUSTER);

    /* Search for the cluster that has the start byte */
    startcluster = pos / bytespercluster;

    /* Read the first part out of this cluster */
    rest = bytespercluster - (pos % bytespercluster);

    currentcluster = GetNthRealCluster(entry, entry->start, startcluster);
    if (!currentcluster) 
        return FALSE;

    if (length < rest)
    {
        return ReadVirtualCluster(entry->handle, currentcluster, pos % bytespercluster + sizeof(CLUSTER), length, data); 
    }

    if (!ReadVirtualCluster(entry->handle, currentcluster, pos % bytespercluster + sizeof(CLUSTER), rest, data))
        return FALSE;

    data   += (unsigned)rest;
    length -= rest;

    for (i = 0; i < length / bytespercluster; i++)
    {
        currentcluster = GetNthRealCluster(entry, currentcluster, 1);

        if (!ReadVirtualCluster(entry->handle, currentcluster, sizeof(CLUSTER), bytespercluster, data))
            return FALSE;

        data += (unsigned)bytespercluster;
    }

    if (length % bytespercluster)
    {
        currentcluster = GetNthRealCluster(entry, currentcluster, 1);

        if (!ReadVirtualCluster(entry->handle, currentcluster, sizeof(CLUSTER), length%bytespercluster, data))
            return FALSE; 
    }

    return TRUE;
}

static BOOL Ensurelength(VirtualDirectoryEntry* entry, RDWRHandle handle, CLUSTER start, unsigned long newlength)
{
    unsigned long having, needed;
    unsigned long bytespercluster; 
    unsigned long sectorspercluster, i;
    CLUSTER last;
    BOOL retVal;

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;

    bytespercluster = (sectorspercluster * BYTESPERSECTOR)-sizeof(CLUSTER);

    /* Calculate how many clusters we already have, and how many we need to add */
    having = (entry->length / bytespercluster) + (entry->length % bytespercluster > 0);
    needed = (newlength / bytespercluster) + (newlength % bytespercluster > 0) - having;

    for (i = 0; i < needed; i++)
    {
        /* Find the last free cluster on the disk, make sure it is not already used */
        if (!FindLastFreeCluster(handle, &last) == FAIL)
            return FALSE;

        if (last == 0) 
            return FALSE;

        while ((retVal = (IsClusterUsed(handle, last))) == TRUE)
        {
            if (!GetPreviousFreeCluster(handle, last, &last)) 
                return FALSE;

            if (last == 0)       /* Out of disk space! */
                return FALSE;
        }

        if (retVal == FAIL) return FALSE;

        if (!SetNextCluster(handle, last, 0))
            return FALSE;

        /* Add this one to the end of the list */
        if (!start)
        {
            entry->start = last;
        }
        else
        {
            /* Move to the end of the list */
            CLUSTER previous = GetNthRealCluster(entry, start, having-1);

            if (!SetNextCluster(handle, previous, last))
                return FALSE;
        }

        start = last;
        having=1;

        /* See wether this cluster is the lowest */
        if (entry->lowestClusterInvolved > last)
            entry->lowestClusterInvolved = last;
    }

    return TRUE;
}

static BOOL SetNextCluster(RDWRHandle handle, CLUSTER current, CLUSTER next)
{
    SECTOR datasector;

    datasector = ConvertToDataSector(handle, current);
    if (!datasector) return FALSE;

    if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
        return FALSE;

    ((VirtualFileSpace*) SectorBuffer)->next = next;

    if (!WriteDataSectors(handle, 1, datasector, SectorBuffer))
        return FALSE;

    return TRUE;
}

static CLUSTER GetNthRealCluster(VirtualDirectoryEntry* entry, CLUSTER start, unsigned long n)
{
    unsigned long i;
    SECTOR datasector;
    CLUSTER current = start;

    if (!entry->isFragmented)
        return start + n;

    for (i=0; i < n; i++)
    {
        datasector = ConvertToDataSector(entry->handle, current);
        if (!datasector)
            return FALSE;

        if (!ReadDataSectors(entry->handle, 1, datasector, SectorBuffer))
            return FALSE;

        current = ((VirtualFileSpace*) SectorBuffer)->next;
    }

    return current;
}

static BOOL ReadVirtualCluster(RDWRHandle handle, CLUSTER currentcluster, 
                               unsigned long start, unsigned long length, char* data)
{
    SECTOR datasector;
    unsigned long sectorspercluster, i;
    unsigned short rest;

    /* Read length bytes from real currentcluster in data buffer, starting at real start */
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;

    datasector = ConvertToDataSector(handle, currentcluster);
    if (!datasector) return FALSE;

    datasector = datasector + start / BYTESPERSECTOR;
    if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
        return FALSE;

    rest = (unsigned short)(BYTESPERSECTOR - start % BYTESPERSECTOR);

    if (length < rest)
    {
        memcpy(data, &SectorBuffer[(int)(start % BYTESPERSECTOR)], (size_t)length);
        return TRUE;
    }

    memcpy(data, &SectorBuffer[(int)(start % BYTESPERSECTOR)], (size_t)rest);

    data += rest;
    length -= rest;

    for (i = 0; i < length / BYTESPERSECTOR; i++)
    {
        datasector++;
        if (!ReadDataSectors(handle, 1, datasector, data))
            return FALSE;

        data += BYTESPERSECTOR;
    }

    if (length % BYTESPERSECTOR)
    {
        datasector++;
        if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
            return FALSE;

        memcpy(data, SectorBuffer, (size_t)(length % BYTESPERSECTOR));   
    }

    return TRUE;
}

static BOOL WriteVirtualCluster(RDWRHandle handle, CLUSTER currentcluster, 
                                unsigned long start, unsigned long length, char* data)
{
    /* Write length bytes to real currentcluster from data buffer, starting at real start */
    SECTOR datasector;
    unsigned long sectorspercluster, i;
    unsigned short rest;

    /* Read length bytes from real currentcluster in data buffer, starting at real start */
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;

    datasector = ConvertToDataSector(handle, currentcluster);

    datasector = datasector + start / BYTESPERSECTOR;
    if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
        return FALSE;

    rest = (unsigned short)(BYTESPERSECTOR - start % BYTESPERSECTOR);

    if (length < rest)
    {
        memcpy(&SectorBuffer[(int)(start % BYTESPERSECTOR)], data, (size_t)length);

        if (!WriteDataSectors(handle, 1, datasector, SectorBuffer))
            return FALSE;

        return TRUE;
    }

    memcpy(&SectorBuffer[(int)(start % BYTESPERSECTOR)], data, (size_t)rest);
    if (!WriteDataSectors(handle, 1, datasector, SectorBuffer))
        return FALSE;

    data += rest;
    length -= rest;

    for (i = 0; i < length / BYTESPERSECTOR; i++)
    {
        datasector++;
        if (!WriteDataSectors(handle, 1, datasector, data))
            return FALSE;

        data += BYTESPERSECTOR;
    }

    if (length % BYTESPERSECTOR)
    {
        datasector++;
        if (!ReadDataSectors(handle, 1, datasector, SectorBuffer))
            return FALSE;

        memcpy(SectorBuffer, data, (size_t)(length % BYTESPERSECTOR));   

        if (!WriteDataSectors(handle, 1, datasector, SectorBuffer))
            return FALSE;
    }

    return TRUE;
}

BOOL PrepareVirtualFile(VirtualDirectoryEntry* entry, unsigned long length) 
{ 
    int j=0;
    unsigned long count=0, i=0, nrClusters; 
    CLUSTER prevCluster = 0, label, last = ULONG_MAX;
    unsigned long sectorspercluster, bytespercluster;
    BOOL isFragmented = FALSE;

    sectorspercluster = GetSectorsPerCluster(entry->handle);
    if (!sectorspercluster) return FALSE;

    bytespercluster = (sectorspercluster * BYTESPERSECTOR)-sizeof(CLUSTER);
    nrClusters = (length / bytespercluster) + (length % bytespercluster > 0);

    for (j=0; j < MAX_ENTRIES; j++)
    {
        if ((VirtualDirectory[j].used) &&
            (VirtualDirectory[j].lowestClusterInvolved < last))
        {
            last = VirtualDirectory[j].lowestClusterInvolved;
        }
    }

    if (last == ULONG_MAX)
    {
        last = GetLabelsInFat(entry->handle);
        if (!last) return FALSE;
    }

    for (i = last-1; i >= 2; i--) 
    { 
        if (!GetNthCluster(entry->handle, i, &label)) 
            return FALSE; 

        if (FAT_FREE(label)) 
        { 
            count++; 
            if (count == nrClusters) 
            { 
                break; 
            } 
        } 
    } 

    if (i == 1)
        return FALSE;   // Insufficient free disk space

    entry->start = i; 
    entry->lowestClusterInvolved = i;

    for (; i < last; i++) 
    { 
        if (!GetNthCluster(entry->handle, i, &label)) 
            return FALSE; 

        if (FAT_FREE(label)) 
        { 
            ZeroCluster(entry->handle, i); 

            if (prevCluster) 
                if (!SetNextCluster(entry->handle, prevCluster, i)) 
                    return FALSE; 

            prevCluster = i; 
        } 
        else
            isFragmented = TRUE;
    } 

    if (prevCluster) 
        if (!SetNextCluster(entry->handle, prevCluster, 0)) 
            return FALSE;

    if (entry->hasCache)
        memset(entry->cache, 0, CACHE_SIZE);

    entry->length = length;
    entry->isFragmented = isFragmented;
   
    return TRUE; 
} 

/* Clears a cluster, ie. writes all 0 to it. */
static BOOL ZeroCluster(RDWRHandle handle, CLUSTER cluster) 
{ 
    unsigned long sectorsPerCluster, i;
    SECTOR sector;

    sectorsPerCluster = GetSectorsPerCluster(handle); 
    if (!sectorsPerCluster) return FALSE; 

    memset(SectorBuffer, 0, BYTESPERSECTOR); 

    sector = ConvertToDataSector(handle, cluster);
    if (!sector) return FALSE;

    for (i=0; i < sectorsPerCluster; i++) 
    { 
        if (!WriteDataSectors(handle, 1, sector+i, SectorBuffer)) 
            return FALSE; 
    } 

    return TRUE; 
} 

BOOL GetMaxRelocatableSize(RDWRHandle handle, unsigned long* size)
{
    unsigned long totalfree, fileSysUsed;

    if (handle->hasMaxRelocSize)
    {
        *size = handle->MaxRelocSize;
        return TRUE;
    }

    /* Calculate how much space we maximally can relocate */
    if (!GetFreeDiskSpace(handle, &totalfree))
        return FALSE;

    if (!GetVirtualFileSpaceUsed(handle, &fileSysUsed))
        return FALSE;

    if (totalfree == fileSysUsed)
        // todo: set custom error (not enough free disk space)
        return FALSE;

    *size = totalfree - fileSysUsed-1;
    handle->MaxRelocSize = *size;
    handle->hasMaxRelocSize = TRUE;
    
    return TRUE;
}

