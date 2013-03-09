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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "fte.h"

char* CreateBitField(unsigned long size)
{
    unsigned len;
    char * result;
    
    assert(size);

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

    assert(bitfield);
    
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
    
    assert(bitfield);

    byte = (unsigned)(index / 8);
    bit  = (unsigned)(index % 8);

    temp   = bitfield+byte;
    *temp &= ~(1 << bit);
}

/*
int GetBitfieldBit(char* bitfield, unsigned long index)
{
    char temp;
    unsigned byte;
    int bit;
    
    assert(bitfield);

    byte = (unsigned)(index / 8);
    bit  = (unsigned)(index % 8);

    temp   = bitfield[byte];
    return (temp >> bit) & 1;
}
*/

void SwapBitfieldBits(char* bitfield,
                      unsigned long bitpos1,
                      unsigned long bitpos2)
{
     int bit1, bit2;
 
      assert(bitfield);
    
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

#ifdef DEBUG

int main(int argc, char** argv)
{
    unsigned long bit, i;
    char* bitfield = CreateBitField(100);
    
    randomize();
    
    for (i=0; i<20; i++)
    {
	do
	{
	    bit = random(100);	    
	    
	} while (GetBitfieldBit(bitfield, bit));
	
	SetBitfieldBit(bitfield, bit);
	printf("%d\n", bit);		
    }
    
    printf("De getallen gesorteerd:\n");
    
    for (i=0; i<100; i++) 
    {
	if (GetBitfieldBit(bitfield, i))
	    printf("%d\n", i);	
    }
    
    DestroyBitfield(bitfield);
    
    printf("---------------------------------\n");
    
    
    bitfield = CreateBitField(100);
    
   
    for (i=0; i<20; i++)
    {
	do
	{
	    bit = random(50);	    
	    
	} while (GetBitfieldBit(bitfield, bit));
	
	SetBitfieldBit(bitfield, bit);
	printf("%d\n", bit+50);		
    }
	
     //printf("/////////////////////////////////\n");
    
    for (i=0; i<50; i++) 
    {
	if (GetBitfieldBit(bitfield, i))
	{
	//    printf("%lu::%lu\n", i, (i+50));
	    SwapBitfieldBits(bitfield, i, (i+50));
	}
    }
    
     //printf("/////////////////////////////////\n");
    

    printf("De getallen gesorteerd:\n");
      
    for (i=0; i<100; i++) 
    {
	if (GetBitfieldBit(bitfield, i))
	    printf("%d\n", i);	
    }

    
    DestroyBitfield(bitfield);    
}


#endif
