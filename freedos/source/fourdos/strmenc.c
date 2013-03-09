

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


// STRMENC.C - 4xxx / TCMD stream encryption / decryption
// Copyright 1998, JP Software Inc., All Rights Reserved

//**********************************************************
// Based on "Applying Stream Encryption" by Warren Ward.
// C/C++ Users Journal, September, 1998, p. 23.
//
// Portions Copyright (c) 1998 by Warren Ward
// Permission is granted to use this source 
// code as long as this copyright notice appears
// in all source files that now include it.
//**********************************************************

#include "product.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "4all.h"

// The linear feedback shift registers
static unsigned long ulShiftRegA;
static unsigned long ulShiftRegB;
static unsigned long ulShiftRegC;

// Global encrypt / decrypt byte counter
static unsigned long ulByteCount;

// Initialize the masks to magic numbers.
//
// These values are primitive polynomials mod 2, described in Applied
// Cryptography, second edition, by Bruce Schneier (New York:  John Wiley
// and Sons, 1994).  See Chapter 15:  Random Sequence Generators and Stream
// Ciphers, particularly thediscussion on Linear Feedback Shift Registers.
//
// The primitive polynomials used here are:
// Register A:	( 32, 7, 6, 2, 0 )
// Register B:	( 31, 6, 0 )
// Register C:	( 29, 2, 0 )
//
// The bits that must be set to "1" in the 
// XOR masks are:
// Register A:	( 31, 6, 5, 1 )
// Register B:	( 30, 5 )
// Register C:	( 28, 1 )
//
// Developer's Note
//
// DO NOT CHANGE THESE NUMBERS WITHOUT REFERRING TO THE DISCUSSION IN
// SCHNEIER'S BOOK.  They are some of very few near-32-bit values that will
// act as maximal-length random generators.
//
static unsigned long ulMaskA = 0x80000062;
static unsigned long ulMaskB = 0x40000020;
static unsigned long ulMaskC = 0x10000002;

// Set up LFSR "rotate" masks.
//
// These masks limit the number of bits used in the shift registers.  Each
// one provides the most-significant bit (MSB) when performing a "rotate"
// operation.  Here are the shift register sizes and the byte mask needed
// to place a "1" bit in the MSB for Rotate-1, and a zero in the MSB for
// Rotate-0.  All the shift registers are stored in an unsigned 32-bit
// integer, but these masks effectively make the registers 32 bits (A), 31
// bits (B), and 29 bits (C).
//
//	Bit	  |  3            2             1            0
//	Pos'n | 1098 7654  3210 9876  5432 1098  7654 3210
//	===== | ==========================================
//	Value | 8421-8421  8421-8421  8421-8421  8421-8421
//	===== | ==========================================
//		  | 
// A-Rot0 | 0111 1111  1111 1111  1111 1111  1111 1111  
// A-Rot1 | 1000 0000  0000 0000  0000 0000  0000 0000 
//		  | 
// B-Rot0 | 0011 1111  1111 1111  1111 1111  1111 1111  
// B-Rot1 | 1100 0000  0000 0000  0000 0000  0000 0000  
//		  | 
// C-Rot0 | 0000 1111  1111 1111  1111 1111  1111 1111  
// C-Rot1 | 1111 0000  0000 0000  0000 0000  0000 0000  
//
//	
// Reg Size	MSB Position	Rotate-0 Mask	Rotate-1 Mask
//	A	32		31			0x7FFFFFFF		0x80000000
//	B	31		30			0x3FFFFFFF		0xC0000000
//	C	29		28			0x0FFFFFFF		0xF0000000
//
static unsigned long ulRotate0A = 0x7FFFFFFF;
static unsigned long ulRotate0B = 0x3FFFFFFF;
static unsigned long ulRotate0C = 0x0FFFFFFF;
static unsigned long ulRotate1A = 0x80000000;
static unsigned long ulRotate1B = 0xC0000000;
static unsigned long ulRotate1C = 0xF0000000;


// Transform a sequence of characters.  If it is plaintext, it will be
// encrypted.  If it is encrypted, and if the LFSRs are in the same state
// as when it was encrypted (that is, the same keys loaded into them and
// the same number of calls to Transform_Char after the keys were loaded),
// the character will be decrypted to its original value.
void EncryptDecrypt( unsigned long _far *pulShiftRegs, char _far *pKey, char _far *pTarget, int nTargetLen )
{
	register int nBitCount;
	int i;
	register unsigned char Crypto;
	unsigned char ucOutB, ucOutC;

	// Get the shift register set if requested
	if (pulShiftRegs != NULL) {
		ulShiftRegA = pulShiftRegs[0];
		ulShiftRegB = pulShiftRegs[1];
		ulShiftRegC = pulShiftRegs[2];
	}

	// Set the key if requested
	if (pKey != NULL) {

		// LFSR A, B, and C get the first, second, and
		// third four bytes of the seed, respectively.
		for ( i = 0; i < 4; i++ ) {
			ulShiftRegA = (( ulShiftRegA <<= 8 ) | (( unsigned long ) pKey[i] ));
			ulShiftRegB = (( ulShiftRegB <<= 8 ) | (( unsigned long ) pKey[i + 4] ));
			ulShiftRegC = (( ulShiftRegC <<= 8 ) | (( unsigned long ) pKey[i + 8] ));
		}

		// If any LFSR contains 0x00000000, load a non-zero default value instead.
	
		if ( ulShiftRegA == 0 )
			ulShiftRegA = 0xAFCE1246;
	
		if ( ulShiftRegB == 0 )
			ulShiftRegB = 0x990FD227;
	
		if ( ulShiftRegC == 0 )
			ulShiftRegC = 0xF39BCD44;

		// clear the byte count
		ulByteCount = 0;
	}

	// Move through each character of the string
	for ( ; nTargetLen > 0; pTarget++, nTargetLen--, ulByteCount++ ) {

		// Cycle the LFSRs eight times to get eight 
		// pseudo-random bits. Assemble these into 
		// a single random character (Crypto).
		for ( nBitCount = 0, Crypto = 0, ucOutB = (unsigned char)(ulShiftRegB & 1), 
				ucOutC = (unsigned char)(ulShiftRegC & 1); nBitCount < 8; nBitCount++ ) {

			if ( ulShiftRegA & 1 ) {
				// The least-significant bit of LFSR A is "1". XOR LFSR A with
				// its feedback mask.
				ulShiftRegA = ((( ulShiftRegA ^ ulMaskA ) >> 1 ) | ulRotate1A );
				
				// Clock shift register B once.
				if (( ucOutB = (unsigned char)(ulShiftRegB & 1 )) == 1 )
					// The LSB of LFSR B is "1". XOR LFSR B with its feedback mask.
					ulShiftRegB = ((( ulShiftRegB ^ ulMaskB ) >> 1 ) | ulRotate1B );
				else
					// The LSB of LFSR B is "0". Rotate the LFSR contents once.
					ulShiftRegB = (( ulShiftRegB >> 1) & ulRotate0B );

			} else {
				// The LSB of LFSR A is "0".  Rotate the LFSR contents once.
				ulShiftRegA = (( ulShiftRegA >> 1 ) & ulRotate0A );
	
				// Clock shift register C once.
				if (( ucOutC = (unsigned char)(ulShiftRegC & 1 )) == 1 )
					// The LSB of LFSR C is "1".  XOR LFSR C with its feedback mask.
					ulShiftRegC = ((( ulShiftRegC ^ ulMaskC ) >> 1 ) | ulRotate1C );
				else
					// The LSB of LFSR C is "0". Rotate the LFSR contents once.
					ulShiftRegC = (( ulShiftRegC >> 1 ) & ulRotate0C );
				
			}
	
			// XOR the output from LFSRs B and C and 
			// rotate it into the right bit of Crypto.
			Crypto = (( Crypto << 1 ) | ( ucOutB ^ ucOutC ));
		}
	
		// XOR the resulting character with the input character to
		// encrypt/decrypt it [Note (A | B) & (~(A & B)) is an XOR].
		*pTarget = (*pTarget | Crypto) & (~(*pTarget & Crypto));
	}

	// Save the shift register set if requested
	if (pulShiftRegs != NULL) {
		pulShiftRegs[0] = ulShiftRegA;
		pulShiftRegs[1] = ulShiftRegB;
		pulShiftRegs[2] = ulShiftRegC;
	}
}

