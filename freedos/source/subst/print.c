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
/* $RCSfile: PRINT.C $
   $Locker: ska $	$Name:  $	$State: Exp $

	print CDS structure.

*/
#pragma hdrfile "print.csm"

#include <stdio.h>
#include <string.h>
#include <process.h>
#include <dos.h>

#include <portable.h>
#include <suppl.h>
#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static const char rcsid[] =
	"$Id: PRINT.C 3.2 2002/11/28 06:20:26 ska Exp ska $";
#endif

const char attribMap[] = "_NPJS++++H+++++++_";
#define attribMapSkip 1

#pragma argsused
static char *printAttr(unsigned attr)
{	char *p;
	static char attrBuf[sizeof(attribMap)];

	memcpy(attrBuf, attribMap, sizeof(attribMap));
	for(p = attrBuf + attribMapSkip + 16; --p >= (attrBuf + attribMapSkip); attr >>= 1)
		if(!(attr & 1)) *p = '-';

	return attrBuf;
}

void printDosCDS(void)
{	int i, j;
	char hbuf[256];
	CDS FAR* dir;

    for (i = 0; i<lastdrv; i++)
        if((dir = cds(i)) == NULL)
            fatal(E_cds, i + 'A');
        else if(join && (dir->flags & JOIN) ||
           !join && (dir->flags & SUBST)) {
           	strcpy(hbuf, "A: => ");
           	hbuf[0] += i;
        	_fstrcpy(hbuf + 6, dir->current_path);
        	if(hbuf[(j = 6 + dir->backslash_offset) - 1] == ':') ++j;
        	hbuf[j] = 0;
        	puts(hbuf);
		}
	exit(0);
}

const char dotter[] = "...... ";
#define pr(a,b) pr2((a),(b),)
#define pr2(a,b,c) if(dir->flags & (a)) { fputs((b), stdout); \
							c; } else fputs(dotter + sizeof(dotter) - sizeof(b), stdout);
void printCDS(void)
{	int i, j;
	char hbuf[256];
	CDS FAR* dir;

    for (j = i = 0; i<lastdrv; i++)
        if((dir = cds(i)) == NULL)
            warning(E_cds, i + 'A');
        else if(prntAfter || dir->flags || dir->dpb || i == lastdrv - 1) {
        	_fstrcpy(hbuf, dir->current_path);
        	hbuf[dir->backslash_offset] = '"';
        	_fstrcpy(hbuf + 1 + dir->backslash_offset, dir->current_path + dir->backslash_offset);
        	if((dir->flags & (NETWORK | JOIN | SUBST | HIDDEN)) == 0
        	 && (*hbuf == 'A' || *hbuf == 'B')
        	 && dir->dpb->unit != dir->dpb->drive)
        		/* units swapped */
        		stpcat(hbuf, "  =>  A:")[-2] += dir->dpb->unit;
            printf("%c %04x:%04x ", 'A' + i, FP_SEG(dir->dpb), FP_OFF(dir->dpb));
            pr(NETWORK, "NET ")
            pr2(JOIN, "JOIN ", ++j)
            pr(PHYSICAL, "PHYS ")
            pr(SUBST, "SUBST ")
            pr(HIDDEN, "HIDDEN ")
            puts(hbuf);
            getDrvName(i);
            if(*DrvName)
            	printf("\t==>.%s.<==", DrvName);
            if(allAtt) {
            	putchar('\t');
            	puts(printAttr(dir->flags));
            }
            else if(*DrvName) putchar('\n');
        }
	if(cntJoin) {
		if(nrJoined == NULL)
			warning(E_nrJoin);
		else {
			message(stdout, M_CDS_1, j, *nrJoined);
			if(j == *nrJoined)
				message(stdout, M_CDS_2);
			else {
				message(stdout, M_CDS_3);
				if(cntJoin > 1) {
					message(stdout, M_CDS_4, j);
					*nrJoined = j;
				}
			}
		}
	}
}

