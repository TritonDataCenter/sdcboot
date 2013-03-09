#ifndef DATA_STRUCT_H_
#define DATA_STRUCT_H_

/* Bitfield */

char* CreateBitField(unsigned long size);
void DestroyBitfield(char* bitfield);
void SetBitfieldBit(char* bitfield, unsigned long index);
void ClearBitfieldBit(char* bitfield, unsigned long index);
int GetBitfieldBit(char* bitfield, unsigned long index);
void SwapBitfieldBits(char* bitfield, unsigned long bitpos1,
                      unsigned long bitpos2);

#endif
