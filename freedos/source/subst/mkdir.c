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
/* $RCSfile: mkdir.c $
   $Locker: ska $	$Name:  $	$State: Exp $

	make dir
	Analogous to Unix's "mkdir -p"

*/
#pragma hdrfile "mkdir.csm"

#include <dir.h>
#include <malloc.h>
#include <string.h>
#include <dos.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: mkdir.c 3.2 2002/11/28 06:20:23 ska Exp ska $";
#endif

int mkDir(int dr, char *path, int chkOnly)
/*	dr: 0 == A: ...
    path: to make
    chkOnly: don't make it, just test, if path exist
*/
{	struct ffblk ff;
	char *p, *h;
	int c;

	if(!*path) return 1;
	if((h = (char*)malloc((c = strlen(path) + 1) + 2)) == NULL)
		fatal(E_noMem);
	*h = dr + 'A';
	h[1] = ':';
	memcpy(p = h + 2, path, c);
	criticErr = 0;
	if(!chkOnly) {
		do {
			while(*++p && *p != '\\' && *p != '/');
			c = *p;
			*p = '\0';
			mkdir(h);
			*p = c;
		} while(*p);
		/* strip trailing '\\' */
		if(p[-1] == '\\' || p[-1] == '/')
			p[-1] = 0;
	}

	c = findfirst(h, &ff, FA_DIREC) == 0;

	free(h);
	return c && (ff.ff_attrib & FA_DIREC) && !criticErr;
}

int mkPhysDir(int dr, char *path)
/* make physical path, that means, that for drive dr any SUBST/JOIN must
   be broken for mkdir
   dr: 0 == A:
   path: to make
*/
{	CDS hdir, FAR*dir;
	int err, reSubst;
	unsigned joined;
	
	if((dir = cds(dr)) == NULL)
		fatal(E_cds, buf[0]);

	if(0 != (reSubst = (dir->flags & (SUBST | JOIN)))) {
		int oldCreatDir;
		char buf[sizeof(dummyDrvA)];

		*strcpy(buf, dummyDrvA) += dr;
		oldCreatDir = creatDir;
		creatDir = 0;	/* prevent from reEnter */
		chkStruc(nrJoined);
		joined = *nrJoined;

		_fmemcpy(&hdir, dir, sizeof(hdir));	/* keep old CDS entry */

		switchSubst(dr, buf, 0);	/* log -> phys Mapping */
		creatDir = oldCreatDir;
	}

	/* now: logical dr points to physical dr */
	err = mkDir(dr, path, creatDir == 2);

	if(reSubst) {
		*nrJoined = joined;	/* CDS restaurieren */
		_fmemcpy(dir, &hdir, sizeof(hdir));
	}
	return err;
}
