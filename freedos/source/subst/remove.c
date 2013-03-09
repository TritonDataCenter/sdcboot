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
/* $RCSfile: REMOVE.C $
   $Locker: ska $	$Name:  $	$State: Exp $

	perform a "SUBST X: -"	(break SUBST/JOIN, relocate DPB)
	for either all SUBST'ed, or JOIN'ed  or all drives.

*/
#pragma hdrfile "simple.csm"

#include <string.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: REMOVE.C 3.2 2002/11/28 06:20:26 ska Exp ska $";
#endif

void removeAll(int all)
/* all: == 0: process only SUBST'ed or JOIN'ed drives
        != 0: process all non-NETWORK'ed drives
*/
{	int dr;
    CDS FAR* dir;
    char buf[sizeof(dummyDrvA)];
	/* swsubst ? - for all SUBST'ed or JOIN'ed drives */

	strcpy(buf, dummyDrvA);
	for(dr = 0; dr < lastdrv; ++dr)
		if((dir = cds(dr)) == NULL) warning(E_cds, 'A' + dr);
		else if(all && (dir->flags & NETWORK) == 0 
		  || (dir->flags & (SUBST | JOIN))) {
			buf[0] = 'A' + dr;
			switchSubst(dr, buf, 0);
		}
}

