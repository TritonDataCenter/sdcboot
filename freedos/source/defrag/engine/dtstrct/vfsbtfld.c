
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "fte.h"

VirtualDirectoryEntry* CreateVFSBitField(RDWRHandle handle, unsigned long size)
{
//    unsigned long i=0;

    VirtualDirectoryEntry* file = CreateVirtualFile(handle);
    if (!file) return NULL;
#if 0    
    /* Clear bits */
    memset(buffer, 0, 1024);

    // Calculate bits to bytes
    size = (size / 8) + (size % 8 > 0);

    for (i = 0; i < size / 1024; i++)
    {
   if (!WriteVirtualFile(file, i*1024, buffer, 1024))
   {
       DeleteVirtualFile(file);
       return NULL;
   }
    }

    if (size % 1024)
    {
   if (!WriteVirtualFile(file, i*1024, buffer, size % 1024))
   {
       DeleteVirtualFile(file);
       return NULL;
   }
    }
#endif

    // Calculate bits to bytes
    size = (size / 8) + (size % 8 > 0);
    
    if (!PrepareVirtualFile(file, size))
    {
        DeleteVirtualFile(file);
        return NULL;
    }

    return file;
}

void DestroyVFSBitfield(VirtualDirectoryEntry* entry)
{
    DeleteVirtualFile(entry);
}

BOOL SetVFSBitfieldBit(VirtualDirectoryEntry* file, unsigned long index)
{
    char temp;
    unsigned long byte;
    int bit;

    assert(file);
    
    byte = (unsigned long)(index / 8);
    bit  = (int)(index % 8);
    
    if (!ReadVirtualFile(file, byte, &temp, 1))
   return FALSE;

    temp |= 1 << bit;

    if (!WriteVirtualFile(file, byte, &temp, 1))
   return FALSE;

    return TRUE;
}

BOOL ClearVFSBitfieldBit(VirtualDirectoryEntry* file, unsigned long index)
{
    char temp;
    unsigned long byte;
    int bit;

    assert(file);
    
    byte = (unsigned long)(index / 8);
    bit  = (int)(index % 8);
    
    if (!ReadVirtualFile(file, byte, &temp, 1))
   return FALSE;

    temp &= ~(1 << bit);

    if (!WriteVirtualFile(file, byte, &temp, 1))
   return FALSE;

    return TRUE;
}

BOOL GetVFSBitfieldBit(VirtualDirectoryEntry* file, unsigned long index, int* value)
{
    char temp;
    unsigned long byte;
    int bit;
    
    assert(file);

    byte = (unsigned long)(index / 8);
    bit  = (int)(index % 8);

    if (!ReadVirtualFile(file, byte, &temp, 1))
   return FALSE;
    
    *value = (temp >> bit) & 1;

    return TRUE;
}

BOOL SwapVFSBitfieldBits(VirtualDirectoryEntry* bitfield, 
          unsigned long bitpos1,
          unsigned long bitpos2)
{
     int bit1, bit2;
 
     assert(bitfield);
    
     if (!GetVFSBitfieldBit(bitfield, bitpos1, &bit1))
    return FALSE;
     if (!GetVFSBitfieldBit(bitfield, bitpos2, &bit2))
    return FALSE;
     
     if (bit1)
        if (!SetVFSBitfieldBit(bitfield, bitpos2))
       return FALSE;
     else
        if (!ClearVFSBitfieldBit(bitfield, bitpos2))
       return FALSE;
        
     if (bit2)
        if (!SetVFSBitfieldBit(bitfield, bitpos1))
       return FALSE;
     else
        if (!ClearVFSBitfieldBit(bitfield, bitpos1))
       return FALSE;

     return TRUE;
}
