#ifndef VFS_BITFIELD_H_
#define VFS_BITFIELD_H_

VirtualDirectoryEntry* CreateVFSBitField(RDWRHandle handle, unsigned long size);
void DestroyVFSBitfield(VirtualDirectoryEntry* entry);

BOOL SetVFSBitfieldBit(VirtualDirectoryEntry* entry, unsigned long index);
BOOL ClearVFSBitfieldBit(VirtualDirectoryEntry* entry, unsigned long index);

BOOL GetVFSBitfieldBit(VirtualDirectoryEntry* entry, unsigned long index, int* value);

BOOL SwapVFSBitfieldBits(VirtualDirectoryEntry* entry,
			 unsigned long bitpos1,
			 unsigned long bitpos2);

#endif
