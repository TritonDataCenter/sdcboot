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
/* $RCSfile: SET.C $
   $Locker: ska $	$Name:  $	$State: Exp $

	Set/Query drive fl‘g settings.

*/
#pragma hdrfile "simple.csm"

#include <string.h>
#include <ctype.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: SET.C 3.2 2002/11/28 06:20:27 ska Exp ska $";
#endif

int drvSetting(char *p, int query)
/* query: == 0: set flags
          != 0: query flags
*/
{	char dr, c;
	unsigned pos;

	if((dr = Drive(&p)) != NoDrive && *p && strchr("=+-", *p)) {
		CDS FAR *dir;

		if((dir = cds(dr)) != NULL) {
			unsigned flags, mode, nowF;

			flags = dir->flags;
			while(*p && strchr("=+-", *p)) {
				mode = *p++;
				if(query)
					mode |= 0x80;
				if((pos = strcspn(p, "=+-")) != NULL) {
					if(!query && !strncmpi(p, "off", pos)) {
						if((dir->flags & NETWORK) == 0) {
							/* "swsubst X: -" simulieren */
							*(char*)_fmemcpy(dir->current_path, dummyDrvA, sizeof(dummyDrvA)) += dr;
							dir->backslash_offset = 2;
							dir->dpb = NULL;
							dir->ifsdrv = NULL;
							dir->u.LOCAL.start_cluster = 0xffff;
						}
						nowF = 0;
						flags = 0;
						mode = '=';
					}
					else if(!query && !strcmpi(p, "on")) {
						*(char*)memcpy(buf, dummyDrvA, sizeof(dummyDrvA)) += dr;
						join = 0;
						switchSubst(dr, buf, 0);
						nowF = PHYSICAL;
						flags = 0;
						mode = '=';
					}
					else if(!strncmpi(p, "PHYSICAL", pos)) nowF = PHYSICAL;
					else if(!strncmpi(p, "JOIN", pos)) nowF = JOIN;
					else if(!strncmpi(p, "SUBST", pos)) nowF = SUBST;
					else if(!strncmpi(p, "NETWORK", pos)) nowF = NETWORK;
					else if(!strncmpi(p, "HIDDEN", pos)) nowF = HIDDEN;
					else {
						char *h = p;

						nowF = 0;
						while(pos--)
							if(isdigit(*h))
								nowF = nowF * 10 + *h++ - '0';
							else break;	/* fail */
						if(nowF > 15) error(E_number, p, 15);
						nowF = 1 << nowF;
						pos = 0;
						p = h;
					}

					switch(mode) {
						case '-' : flags &= ~nowF; break;
						case '+' : flags |= nowF; break;
						case '=' : flags = (flags & ~ (PHYSICAL | JOIN | SUBST | NETWORK | HIDDEN)) | nowF; break;
                        case '-' | 0x80 : if(flags & nowF) return(44); break;
                        case '+' | 0x80 : if(!(flags & nowF)) return(44); break;
						default: pos = 0; /* erzeugt fehler */
					}
				}
				else break;
				p += pos;
			}
			if(*p)	error(E_drvSetting, p);
			if(!query) {
				if((dir->flags ^ flags) & JOIN) {
					/* add | sub JOIN => updated #Joined */
					if(nrJoined == NULL)
						warning(E_nrJoin);
					else {
						if(flags & JOIN) ++*nrJoined;
						else --*nrJoined;
					}
				}
				dir->flags = flags;
				return 1;
			}
		}
		else fatal(E_cds, dr + 'A');
	}
	return 0;
}

