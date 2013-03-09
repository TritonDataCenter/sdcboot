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
/* $RCSfile: subst.c $
   $Locker: ska $	$Name:  $	$State: Exp $

	Subst/join.

*/
#pragma hdrfile "simple.csm"

#include <string.h>
#include <dos.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: subst.c 3.2 2002/11/28 06:20:27 ska Exp ska $";
#endif

static DPB FAR *lw2dpb(int dr)
/* locate the DPB for a drive;
   dr: 'A' == A:
   return NULL on error
*/
{	DPB FAR *dpb;

    dr -= 'A';
    if((dpb = firstDPB) != NULL) {
	    while(dr-- && FP_OFF(dpb) != 0xffff) 
	        if (_osmajor < 4) 
	            dpb = dpb->vers.dos3.next;
	        else
	            dpb = dpb->vers.dos4.next;
	    if(FP_OFF(dpb) == 0xffff) dpb = NULL;
	 }
	 return dpb;
}

void switchSubst(int dr, char *path, int bslashoff)
/* SUBST/JOIN physical path to drive dr;
   automatically breaks existing SUBST/JOIN
*/
{	int flags;
	int pathLen;
	int root;	/* SUBST/JOIN to root drive */
	int locDir;	/* path points to a local drive */
	CDS FAR *dir;
	char *p;

	/* flip '/' to '\\' */
	p = path;
	while((p = strchr(p, '/')) != NULL)
		*p++ = '\\';
	/* strip trailing '\\' */
	pathLen = strlen(path);
	if(path[pathLen - 1] == '\\' && path[pathLen - 2] != ':')
		path[--pathLen] = '\0';

	if(creatDir) /* creat path */
		if(!mkPhysDir(*path - 'A', path + 2))
	 		warning(E_mkDir, path);

	if(0 != (locDir = path[1] == ':' && path[2] == '\\')) {
		if(*path >= 'a' && *path <= 'z') *path += 'A' - 'a';
		else if(*path < 'A' || *path > 'Z') locDir = 0;
	}
	root = locDir && path[3] == NUL;	/* root directory */
	if(bslashoff && bslashoff > pathLen)
		bslashoff = 0;

	if(sizeof(dir->current_path) <= pathLen)
		error(E_pathLen, path);

	if((dir = cds(dr)) == NULL)
		fatal(E_cds, 'A' + dr);

   	if(join) {	/* force re-read of drive, so DOS detect JOIN */
   		CDS FAR *ddir;

		if(!locDir) error(E_joinLocal, dr + 'A', path);

        if((ddir = cds(*path - 'A')) == NULL)
            fatal(E_cds, *path);
        ddir->u.LOCAL.start_cluster = 0xffff;
	}

	/* update data area */
	_fmemcpy(dir->current_path, path, pathLen + 1);
	dir->backslash_offset = bslashoff? bslashoff: join? 2: root? pathLen - 1: pathLen;
	dir->u.LOCAL.start_cluster = 0xffff;
   	dir->dpb = lw2dpb(join? dr + 'A': *path);
   	flags = dir->flags;	/* keep old flags, to detect, if #Joined changed */
   	dir->flags &= ~(JOIN | SUBST);

	if(root && *path == dr + 'A') {
		/* SUBST/JOIN X: => X:  -> neither SUBST nor JOIN necessary, break it */
		if(!dir->dpb && !(dir->flags & NETWORK)) {
			/* no DPB => PHYSICAL this is possibly no vaild physical drive */
			dir->flags &= ~PHYSICAL;
		}
		else dir->flags |= PHYSICAL;	/* make it available, this is 
			necessary, if a former OFF setting must be reversed. */
	}
	else {
		/* perform SUBST/JOIN */
		dir->flags |= !locDir? PHYSICAL :join? JOIN | PHYSICAL: SUBST | PHYSICAL;
	}

	if((flags ^ dir->flags) & JOIN) { /* add|sub JOIN => upd #joined */
		if(nrJoined == NULL) 
			warning(E_nrJoin);
		else {
			if(flags & JOIN) --*nrJoined;
			else ++*nrJoined;
		}
	}
}
