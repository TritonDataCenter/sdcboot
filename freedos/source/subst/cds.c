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
/* $RCSfile: CDS.C $
   $Locker: ska $	$Name:  $	$State: Exp $

   CDS.C -- uses undocumented DOS to return pointer to
   current directory structure for a given drive

*/

#include <stdlib.h>
#include <dos.h>

#include "cds.h"

#ifndef lint
static const char rcsid[] =
"$Id: CDS.C 3.2 2002/11/28 06:20:22 ska Exp ska $";
#endif

typedef enum { UNKNOWN=-1, FALSE=0, TRUE=1 } OK;
byte FAR *nrJoined = NULL;
DPB FAR *firstDPB = NULL;
DRV FAR *firstDRV = NULL;
MCB FAR *firstMCB = NULL;
MCB FAR *firstMCBinUMB = NULL;

byte lastdrv;
CDS FAR *cds(unsigned drive)
{
    /* statics to preserve state: only do init once */
    static byte FAR *dir = (byte FAR *) 0;
    static OK ok = UNKNOWN;
    static unsigned currdir_size;

    if (ok == UNKNOWN)  /* only do init once */
    {
        unsigned drv_ofs, nulldrv_ofs;

        /* curr dir struct not available in DOS 1.x or 2.x */
        if ((ok = (_osmajor < 3) ? FALSE : TRUE) == FALSE)
            return (CDS FAR *) 0;

        /* compute offset of curr dir struct and LASTDRIVE in DOS
           list of lists, depending on DOS version */
        drv_ofs = (_osmajor == 3 && _osminor == 0)? 0x17: 0x16;
        nulldrv_ofs = (_osmajor == 3 && _osminor == 0)? 0x28: 0x22;
        #define nrJoined_ofs 0x34
        #define firstDPB_ofs 0

#if defined(__TURBOC__) || (defined(_MSC_VER) && (_MSC_VER >= 600))       
#ifdef __TURBOC__
#define _asm    asm
#endif

        _asm    push si    /* needs to be preserved */

        /* get DOS list of lists into ES:BX */
        _asm    mov ah, 52h
        _asm    int 21h

        /* get current directory structure */
        _asm	push es
        _asm    mov si, drv_ofs
        _asm    les ax, es:[bx+si]
        _asm    mov word ptr dir+2, es
        _asm    mov word ptr dir, ax
        _asm	pop es

        /* get address of "number of JOIN'ed drives */
        _asm	mov word ptr nrJoined+2, es
        _asm	mov word ptr nrJoined, bx
        _asm	add word ptr nrJoined, nrJoined_ofs

        /* get address of NUL device */
        _asm	mov word ptr firstDRV+2, es
        _asm	mov ax, bx
        _asm	add ax, word ptr nulldrv_ofs
        _asm	mov word ptr firstDRV, ax

		/* get address of first DPB */
		_asm	push es
		_asm	les ax, es:[bx+firstDPB_ofs]
        _asm	mov word ptr firstDPB+2, es
        _asm	mov word ptr firstDPB, ax
        _asm	pop es

        /* get address of first MCB in UMB */
        _asm	push es
        _asm	push bx
        _asm	les bx, es:[bx+0x12]	/* get addr of disk buffer info */
        _asm	mov ax, es:[bx+0x1f]
        _asm	pop bx
        _asm	pop es
        _asm	mov word ptr firstMCBinUMB+2, ax
        _asm	mov word ptr firstMCBinUMB, 0

        /* get address of first MCB */
        _asm	cmp bx, 2
        _asm	jl skipDown
        _asm	mov ax, word ptr es:[bx - 2]
        _asm	jmp short movIt
        skipDown:
        _asm	push es
        _asm	push bx
        _asm	mov ax, es
        _asm	dec ax
        _asm	mov es, ax
        _asm	add bx, 16	/* hop one segment */
        _asm	mov ax, word ptr es:[bx - 2]
        _asm	pop bx
        _asm	pop es
        movIt:
        _asm	mov word ptr firstMCB+2, ax
        _asm	mov word ptr firstMCB, 0

        _asm    pop si

#else           
{       /* Microsoft C 5.1 -- no inline assembler available */
        union REGS r;
        struct SREGS s;
        byte FAR *doslist;
        segread(&s);
        r.h.ah = 0x52;
        intdosx(&r, &r, &s);
        FP_SEG(doslist) = s.es;
        FP_OFF(doslist) = r.x.bx;
        lastdrv = doslist[lastdrv_ofs];
        dir = *((byte FAR * FAR *) (&doslist[drv_ofs]));
        firstDPB = *((DPB FAR * FAR *) (&doslist[firstDPB_ofs]));
        nrJoined = &doslist[nrJoined_ofs];


        /* must come last */
        if(r.x.bx < 2) { /* ein Segment runter */
        	FP_SEG(doslist) = s.es - 1;
        	FP_OFF(doslist) = r.x.bx + 16;
        	FP_SEG(firstMCB) = *(WORD FAR*)(&doslist[-2]);
        	FP_OFF(firstMCB) = 0;
		}
}
#endif

		if(FP_SEG(firstMCBinUMB) == 0xffff || _osmajor < 5)
			firstMCBinUMB = NULL;

        /* in OS/2 DOS box 0xffff:0xffff */
        if(dir == MK_FP(0xffff, 0xffff)) {
        	ok = FALSE;
			firstMCBinUMB = NULL;
    		nrJoined = NULL;
    		firstDPB = NULL;
    	}
#define chkit(a) if(a == MK_FP(0xffff, 0xffff)) a = NULL;
		chkit(nrJoined);
		chkit(firstDPB);
#undef chkit

        /* compute curr directory structure size */
        currdir_size = (_osmajor >= 4) ? 0x58 : 0x51;

    }

    if(ok == FALSE || drive >= lastdrv) /* is the drive < LASTDRIVE? */
        return (CDS FAR *) 0;

    /* return array entry corresponding to drive */
    return (CDS FAR*)(dir + (drive * currdir_size));
}
