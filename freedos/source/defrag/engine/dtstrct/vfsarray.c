
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "fte.h"

//static char buffer[1024];

BOOL CreateVFSArray(RDWRHandle handle, unsigned long size, unsigned unitSize, VFSArray* result)
{

 #if 0   
    unsigned long fileSize;

    // Calculate total number of bytes
    fileSize = size * unitSize;


    unsigned long i=0;

    
    /* Clear bits */
    memset(buffer, 0, 1024);

    // Calculate total number of bytes
    fileSize = size * unitSize;

    for (i = 0; i < fileSize / 1024; i++)
    {
if (!WriteVirtualFile(file, i*1024, buffer, 1024))
   {
       DeleteVirtualFile(file);
       return FALSE;
   }
    }

    if (fileSize % 1024)
    {
   if (!WriteVirtualFile(file, i*1024, buffer, fileSize % 1024))
   {
       DeleteVirtualFile(file);
       return FALSE;
   }
    }
#endif

    VirtualDirectoryEntry* file = CreateVirtualFile(handle);
    if (!file) return FALSE;

    result->entry = file;
    result->unitSize = unitSize;

    return PrepareVirtualFile(result->entry, size * unitSize);
}

void DestroyVFSArray(VFSArray* array)
{
    DeleteVirtualFile(array->entry);
}

BOOL SetVFSArray(VFSArray* array, unsigned long index, char* data)
{
    return (WriteVirtualFile(array->entry, index * array->unitSize, data, array->unitSize));
}

BOOL GetVFSArray(VFSArray* array, unsigned long index, char* data)
{
    return (ReadVirtualFile(array->entry, index * array->unitSize, data, array->unitSize));
}

