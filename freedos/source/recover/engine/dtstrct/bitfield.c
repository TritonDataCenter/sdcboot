/*
   Bitfield.c - bit field data structure.
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
#include <string.h>

char* CreateBitField(unsigned long size)
{
    unsigned len;
    char * result;

    len = (unsigned)((size / 8) + ((size % 8) > 0));

    result = (char*) malloc(len);

    if (result) memset(result, 0, len);

    return result;
}

void DestroyBitfield(char* bitfield)
{
    if (bitfield) free(bitfield);
}

void SetBitfieldBit(char* bitfield, unsigned long index)
{
    char* temp;
    unsigned byte;
    int bit;

    byte = (unsigned)(index / 8);
    bit  = (unsigned)(index % 8);

    temp   = bitfield+byte;
    *temp |= 1 << bit;
}

void ClearBitfieldBit(char* bitfield, unsigned long index)
{
    char* temp;
    unsigned byte;
    int bit;

    byte = (unsigned)(index / 8);
    bit  = (unsigned)(index % 8);

    temp   = bitfield+byte;
    *temp &= ~(1 << bit);
}

int GetBitfieldBit(char* bitfield, unsigned long index)
{
    char temp;
    unsigned byte;
    int bit;

    byte = (unsigned)(index / 8);
    bit  = (unsigned)(index % 8);

    temp   = bitfield[byte];
    return (temp >> bit) & 1;
}

void SwapBitfieldBits(char* bitfield,
                      unsigned long bitpos1,
                      unsigned long bitpos2)
{
     int bit1, bit2;
 
     bit1 = GetBitfieldBit(bitfield, bitpos1);
     bit2 = GetBitfieldBit(bitfield, bitpos2);
     
     if (bit1)
        SetBitfieldBit(bitfield, bitpos2);
     else
        ClearBitfieldBit(bitfield, bitpos2);
        
     if (bit2)
        SetBitfieldBit(bitfield, bitpos1);
     else
        ClearBitfieldBit(bitfield, bitpos1);        
}
