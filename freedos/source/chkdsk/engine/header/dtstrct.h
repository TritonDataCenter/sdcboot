#ifndef DATA_STRUCT_H_
#define DATA_STRUCT_H_

/* Bitfield */

char* CreateBitField(unsigned long size);
void DestroyBitfield(char* bitfield);
void SwapBitfieldBits(char* bitfield, unsigned long bitpos1,
                      unsigned long bitpos2);


void SetBitfieldBit(char* bitfield, unsigned long index);
void ClearBitfieldBit(char* bitfield, unsigned long index);




#define GetBitfieldBit(bitfield, index) \
    ((((char*)(bitfield))[(unsigned)((index)>>3)] >> (index & 7)) & 1)
    

#endif
