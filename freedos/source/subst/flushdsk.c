/*
    SWSUBST: Alternate CDS manipulator for MS-DOS
    Copyright (C) 1995-97,2002 Steffen Kaiser

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* $Id: FLUSHDSK.C 3.2 2002/11/28 06:20:23 ska Exp ska $

	Flush disks and reset disk caches.

	It would be good to place a "clear DOS internal BUFFER chain"
	procedure here, but I don't know nothing.

*/

#include <dos.h>
#include <io.h>
#include <fcntl.h>

#include <portable.h>
#include "swsubst.h"
#include "yerror.h"

enum CACHE { C_QUERY, C_NONE, C_SMARTDRV40, C_HYPERDISK,
			C_SMARTDRV3 }
	cache = C_QUERY;

union {
	unsigned _muxnr;
	int _fd;
} info;
#define muxnr info._muxnr
#define fd info._fd

#define dos(nr) _AX = nr; geninterrupt(0x21)
#define mux(nr) _AX = nr; geninterrupt(0x2f)

enum CACHE identifyCache(void)
{	USEREGS

	/* Let's identify SMARTDRV v4.0+ */
	_CX = 0xebab;
	_BX = 0;
	mux(0x4a10);
	if(_AX == 0xbabe) /* gotcha! */
		return C_SMARTDRV40;

	/* Let's identify HyperDisk */
	muxnr = 0xc000;	/* first free MUX number */
	do {
		_DX = _CX = 0;
		_BX = 'HD';		/* Disk cache, Hyperware */
		mux(muxnr);
		if(_AL == 0xff && _DX && _CX == 'HY')
			/* gotcha: This Hyperware product installed */
			return C_HYPERDISK;
	} while(muxnr += 0x100);

	/* Let's identify Smartdrv v3.x */
	if((fd = open("SMARTAAR", O_RDONLY)) >= 0) /* gotcha! */
		return C_SMARTDRV3;

	return C_NONE;
}

void flushDisks(void)
{	USEREGS

	asm push di;		/* MUX alters "fast" variable registers */
	asm push si;

	dos(0xd00);		/* very simple flush buffers */
	
retry:
	switch(cache) {
		case C_QUERY:
			cache = identifyCache();
			goto retry;

		case C_SMARTDRV40:
			_BX = 1;	/* flush cache */
			mux(0x4a10);

			_BX = 2;	/* reset cache */
			mux(0x4a10);

		case C_NONE:
			break;
		
		case C_HYPERDISK:
			_DX = 0xc;		/* Disable anything but Verify */
			_BX = 'HD';		/* Disk cache, Hyperware */
			mux(muxnr | 2);	/* disable the cache */
			_DX = _DH;		/* original setting */
			_BX = 'HD';		/* Disk cache, Hyperware */
			_AX = muxnr | 2;/* Re-set values */
			mux(muxnr | 2);	/* Re-set values */
			break;

		case C_SMARTDRV3:
			{	char c;

				c = 1;		/* flush & discard cache */
				_BX = fd;
				_CX = 1;
				_DX = FP_OFF(&c);
#if defined(__LARGE__) || defined(__HUGE__) || defined(__MEDIUM__)
#ifdef NOREGS
				asm push ds
#endif
				_DS = FP_SEG(&c);
#endif
				dos(0x4403);
#if defined(__LARGE__) || defined(__HUGE__) || defined(__MEDIUM__)
#ifdef NOREGS
				asm pop ds
#endif
#endif
			}
			break;

	}

	asm pop di;		/* MUX alters "fast" variable registers */
	asm pop si;
}

