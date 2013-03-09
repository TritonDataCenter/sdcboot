#ifndef VFSARRAY_H_
#define VFSARRAY_H_

typedef struct VFSArray
{   
    VirtualDirectoryEntry* entry;
    unsigned unitSize;

} VFSArray;

BOOL CreateVFSArray(RDWRHandle handle, unsigned long size, unsigned unitSize, VFSArray* result);
void DestroyVFSArray(VFSArray* array);

BOOL SetVFSArray(VFSArray* array, unsigned long index, char* data);
BOOL GetVFSArray(VFSArray* array, unsigned long index, char* data);

#endif
