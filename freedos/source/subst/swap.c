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
/* $RCSfile: SWAP.C $
   $Locker: ska $	$Name:  $	$State: Exp $

	Swap the CDS entries of two drives.

*/
#pragma hdrfile "simple.csm"

#include <string.h>
#include <dos.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: SWAP.C 3.2 2002/11/28 06:20:27 ska Exp ska $";
#endif


void swapDPB(DPB FAR *a, DPB FAR *b) /* swap units */
{	int unit;

	/* check if the drives can be swapped that way */
	if(_osmajor < 4? (a->vers.dos3.device_driver == a->vers.dos3.device_driver):
	  (a->vers.dos4.device_driver == a->vers.dos4.device_driver)) {
		/* OK: same device driver => swapping OK */

		unit = a->unit;
		a->unit = b->unit;
		b->unit = unit;
	}
	else error(E_swapDriverUnits);
}
#define stuff(a,act) \
	if(a == NULL) a = act;	\
	else error(E_identFloppies);

void swapFD(void) /* swap the floppies */
{	DPB FAR *a, FAR *b, FAR *act;

	/* search the DPB chain for the DPB's of A: and B: */
	a = b = NULL;
	chkStruc(act = firstDPB);
	do switch(act->drive) {
			case 0: stuff(a, act); break;
			case 1: stuff(b, act); break;
	} while(FP_OFF(act = _osmajor < 4? act->vers.dos3.next: act->vers.dos4.next) != 0xffff);

	if(a == NULL || b == NULL)
		error(E_identFloppies);

	swapDPB(a, b);
}

void swap(char *d1, char *d2, int unitSwap)
{	/* swap drive ‘ntries d1 and d2 */
	char dr1, dr2;

	if((dr1 = Drive(&d1)) != NoDrive && (dr2 = Drive(&d2)) != NoDrive) {
		CDS FAR *dir1, FAR *dir2;

		if((dir1 = cds(dr1)) != NULL && (dir2 = cds(dr2)) != NULL) {
			if(unitSwap) swapDPB(dir1->dpb, dir2->dpb);
			else {
				CDS dir;

				_fmemcpy(&dir, dir1, sizeof(dir));
				_fmemcpy(dir1, dir2, sizeof(dir));
				_fmemcpy(dir2, &dir, sizeof(dir));
			}
			if((dir1->flags & (NETWORK | HIDDEN)) == 0)
				dir1->u.LOCAL.start_cluster = 0xffff;
			if((dir2->flags & (NETWORK | HIDDEN)) == 0)
				dir2->u.LOCAL.start_cluster = 0xffff;
			return;	/* OK */
		}
	}
	error(E_swap, d1, d2);
}
