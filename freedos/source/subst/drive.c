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
/* $RCSfile: drive.c $
   $Locker: ska $	$Name:  $	$State: Exp $

	Check for drive spec: (in examples: A means any drive letter)
		a) normal A:.תתתת
		b) abbreviated A
		c) :driver name:
		d) ?th drive with name  :?:driver name:	
		e) ::label:
		f) 

	Display/Search in the MCB and device driver chain.

*/
#pragma hdrfile "drive.csm"

#include <string.h>
#include <dos.h>
#include <dir.h>
#include <stdlib.h>
#include <ctype.h>
#include <portable.h>
#include <msglib.h>
#include <suppl.h>

#include "swsubst.h"
#include "yerror.h"

#ifndef lint
static char rcsid[] = "$Id: drive.c 3.2 2002/11/28 06:20:23 ska Exp ska $";
#endif


char DrvName[41];
#define cpyName(a,b,c) _cpyName(a, sizeof(a), b, c)
#define chkNamSize(a) if((namSize -= a) <= 0) return;
#define tohex(nr) if((*name++ = '0' + ((nr) & 0xf)) > '9') name[-1] += 'a' - 10;
static void _cpyName(char *name, int namSize, char FAR* from, int frmSize)
{	memset(name, 0, namSize);
	while(frmSize--) if(*from >= ' ' && *from <= 0x7e) {
		chkNamSize(1);
		*name++ = *from++;
	} else if(fllNam) {
		chkNamSize(4);
		*name++ = '\\';
		*name++ = 'x';
		tohex(*from >> 8)
		tohex(*from)
		from += 4;
	} else return;
}

static void drv2name(DRV FAR *p)
{	*DrvName = 0;
	if(p == NULL) return;
	if(p->DAttr & 0x8000) /* char */
		cpyName(DrvName, p->drvr.chr, 8);
	else /* block */
		cpyName(DrvName, p->drvr.block.dummy, 7);
}

void getDrvName(int drvNr)
{	DRV FAR *p;
	DPB FAR *dp;
	CDS FAR *dir;

	p = NULL;
	if((dir = cds(drvNr)) == NULL)
		goto returnName;

	if((dp = dir->dpb) == NULL) { /* maybe CD-ROM */
		char *mscd = "MSCD";

		if(!(dir->flags & NETWORK)) goto returnName;
		chkStruc(p = firstDRV);
		do if(p->drv == drvNr+1 && !_fmemcmp(&(p->version), mscd, 4))
			goto returnName;
		while((p = p->nxt) != NULL && (FP_OFF(p) != 0xffff));
		p = NULL;
	} else p = _osmajor >= 4? 	(DRV FAR*)dp->vers.dos4.device_driver: 
		 						(DRV FAR*)dp->vers.dos3.device_driver;
returnName:
	drv2name(p);
}

static int name2drv(char *name, int nr)
/* transform a drive name into a drive spec */
{	int i;
	int len;
	CDS FAR* dir;

	if(name == NULL || *name == '\0') return NoDrive;
	if((len = strlen(name)) > 8)
		len = 8;
    for(i=0; i<lastdrv; i++) {
			getDrvName(i);
			if(!strncmpi(DrvName, name, len) && nr-- == 0)
				return i;
        }
   return NoDrive;
}

static int searchLabel(char *lab, int phys)
{	CDS hdir, FAR*dir;
	int err, dr, reSubst, oldCreatDir, le;
	struct ffblk ff;
	char hbuf[sizeof(dummyDrvA)], nam[sizeof(dummyDrvA)+3];
	unsigned joined;
	char *p;

	stpcpy(hbuf, dummyDrvA);
	stpcpy(stpcpy(nam, dummyDrvA), "*.*");
	chkStruc(nrJoined);
	joined = *nrJoined;
	oldCreatDir = creatDir;
	le = strlen(lab);
	criticErr = creatDir = 0;
	err = NoDrive;
	p = includeDrives - 1;

	while(err == NoDrive && *++p)
		if((dir = cds(dr = *p - 'A')) == NULL)
			warning(E_cds, hbuf[0]);
		else {
			nam[0] = hbuf[0] = *p;

			reSubst = (dir->flags & (SUBST | JOIN)) && phys;
			if(reSubst) {
				_fmemcpy(&hdir, dir, sizeof(hdir));	/* keep old CDS entry */
				switchSubst(dr, hbuf, 0);	/* log -> phys Mapping */
			}

			if(criticErr || findfirst(nam, &ff, FA_LABEL) || criticErr)
				criticErr = 0;	/* not found */
		 	else if(memicmp(ff.ff_name, lab, le) == 0) /* found */
					err = dr;

			if(reSubst) {
				*nrJoined = joined;	/* restore CDS entry */
				_fmemcpy(dir, &hdir, sizeof(hdir));
			}
		}

	creatDir = oldCreatDir;
	return err;
}

int Drive(char **str)
{	char d;
	char *p, *h, *hh;

	p = *str;
	if(dosish) {	/* allow dosish style X:... only */
		if(p[1] != ':') return NoDrive;
		goto letter;
	}

	d = (p[1] == '-') + 1;
	if(*p == ':' && p[d] == ':' && (h = strchr(p + d + 1, ':')) != NULL) { /* Label search */
		*h = 0;
		if((d = searchLabel(p + d + 1, d == 2)) != NoDrive)
			*str = h + 1;
		*h = ':';
		return d;
	}

	if(*p == ':' && p[1] != ':' && (h = strchr(p + 1, ':')) != NULL) { /* Drive Name */
		unsigned long l;
		char *endp;

		if((hh = strchr(h + 1, ':')) != NULL) { /* test auf :#:drv: */
			if((l = strtoul(p + 1, &endp, 0)) < 255 && endp == NULL) {
				p = h;
				h = hh;
				goto driveName;
			}
		}
		l = 0;

driveName:
		*h = '\0';
		if((d = name2drv(p + 1, (int)l)) != NoDrive)
			*str = h + 1;
		*h = ':';
		return d;
	}
letter:
	if((d = toupper(*p++)) < 'A' || d >= 'A' + lastdrv) return NoDrive;
	if(*p == ':') {	/* OK */
		*str = p + 1;
		return d - 'A';
	}
	if(!*p) {	/* OK */
		*str = p;
		return d - 'A';
	}
	return NoDrive;
}

int driveList(char *search, int wide)
{	DRV far *p;
	int l;
	MCB far *mcb;

	if(search) l = strlen(search);
	else l = 0;

	chkStruc(p = firstDRV);
	wide = wide && (_osmajor >= 4);

	do {
		drv2name(p);
		if(l) {
			if(memicmp(DrvName, search, l) == 0)
				return 0;
			if(wide
			 && (mcb = (MCB far *)MK_FP(FP_SEG(p) - 1, 0))->type >= ' '
			 && mcb->type <= 0x7e) {
				cpyName(DrvName, mcb->name, 8);
				if(*DrvName && memicmp(DrvName, search, l) == 0)
					return 0;
			}
		}
		else if(*DrvName) {
			if(_osmajor >= 4) {
				fputs(DrvName, stdout);
				mcb = (MCB far*)MK_FP(FP_SEG(p) - 1, 0);
				if(mcb->type >= ' ' && mcb->type <= 0x7e) {
					cpyName(DrvName, mcb->name, 8);
					if(*DrvName) printf(" <<%c>> %s", mcb->type, DrvName);
				}
				putchar('\n');
			} else puts(DrvName);
		}
	} while((p = p->nxt) != NULL && (FP_OFF(p) != 0xffff));

	return l? msgErrorNumber(E_searchList): 0;
}

static void systemMessage(MSGID msg)
{	message(stdout, M_systemMCB, msgRetrieve(msg));	}

void dumpName(const char *type, char far*name, const unsigned segm)
/* Dump & transform a name

	type: Type of the name
	name: address of the source
			if NULL, no characters will be found
	segm: of no characters found at the source, dump this adress
			if NULL, this feature is turned off
*/
{	char *p;

/* get name */
	if(name) {
		if(name != DrvName)
			cpyName(DrvName, name, 8);
	}
	else *DrvName = NUL;

/* strip trailing spaces */
	p = strchr(DrvName, NUL);
	while(--p > DrvName && *p == ' ');
	p[1] = NUL;

/* dump the name or the PSP address */
	if(*DrvName)
		printf(" %s=%s", type, DrvName);
	else if(segm)	/* no name => print PSP */
			printf(" %s#0x%04x", type, segm);
}

void handleMCB(const char *arg2, const int len2, MCB far*mcb,
	const int puf, const MCB far* maxMCB, const int wide)
/* Search/Display a MCB chain and a chain within a MCB

   arg2: name to search if len2 != 0
   len2: length of name to search
   mcb : first MCB to check
   puf : >0 => test for a chain within a puf-th sub-chain
   maxMCB: address, which limits the current MCB chain 
*/
{	MCB far* tmcb;	/* temporary MCB pointer */
	DRV far* drv;	/* temporary driver pointer */
	int skipRec;	/* do not look for a sub-chain in this MCB */
	int freeMCB;	/* this MCB is marked as free */
	char *p;

	do {
		skipRec = 0;
		if(/* mcb->owner == 0*/			/* something's wrong */
		 /*||*/ mcb->size + FP_SEG(mcb) - 3 > FP_SEG(maxMCB)) continue;

		if(puf) {	/* in a sub-chain */
			if(FP_SEG(mcb) >= FP_SEG(maxMCB))
				return;	/* sub-chain finished with a quirk */
			fputmc(' ', puf, stdout);
		}

	/* dump information of this MCB */
		p = strchr(DrvName, NUL) - 1;
		while(p > DrvName && *p == ' ') --p;	/* strip trailing spaces */
		p[1] = 0;
		message(stdout, wide? M_mcbChainWide: M_mcbChain,
			FP_SEG(mcb), mcb->type, mcb->size);
		if((freeMCB = mcb->owner) == 0)
			skipRec = 1;	/* no sub-chain */
		else {
			dumpName("nam", mcb->name, NULL);

			if(len2 && memicmp(DrvName, arg2, len2) == 0
			 && (!wide || len2 == strlen(DrvName))) /* wide <=> exact match */
				skipRec = 2;	/* MCB's name matchs */
		}

	/* decode MCB's usage */
#define puts(a) fputs(a, stdout)
		if(mcb->owner == 8 || mcb->owner == 10) {	/* used by system */
			switch(mcb->name[1]) {
				case 'C': systemMessage(M_codeMCB); break;
				case 'D': systemMessage(M_dataMCB); break;
				case 'M': systemMessage(M_memoryMCB); break;
				default: systemMessage(M_noneMCB); break;
			}
		}
		else if(freeMCB) systemMessage(M_freeMCB);
		else {
			tmcb = (MCB far*)MK_FP(mcb->owner - 1, 0);
			if(tmcb == mcb)	;	/* the owner of the MCB itself */
			else if(FP_SEG(mcb) + 1 == *((word far*)((byte far*)tmcb + 16 + 0x2c))) {
				/* Env */
				dumpName("env", tmcb->name, FP_SEG(tmcb));
				skipRec |= 1;
			}
			else
				dumpName("own"
						 , tmcb->owner == mcb->owner? tmcb->name : NULL
					     , FP_SEG(tmcb));
		}
		if(skipRec == 2)
			exit(0);	/* found */

	/* mayhap, it's a driver? */
		chkStruc(drv = firstDRV);
		do if(FP_SEG(drv) == FP_SEG(mcb) + 1) {
			drv2name(drv);
			dumpName("drv", DrvName, FP_SEG(drv) - 1);
			break;
		} while((drv = drv->nxt) != NULL && (FP_OFF(drv) != 0xffff));

	/* usage by MCB type */
		switch(mcb->type) {
			case 'D': systemMessage(M_devMCB); break;
			case 'E': systemMessage(M_extraDevMCB); break;
			case 'I': systemMessage(M_ifsMCB); break;
			case 'F': systemMessage(M_filesMCB); break;
			case 'X': systemMessage(M_fcbMCB); break;
			case 'C': systemMessage(M_emsBufMCB); break;
			case 'B': systemMessage(M_bufMCB); break;
			case 'L': systemMessage(M_lstDrvMCB); break;
			case 'S': systemMessage(M_stckMCB); break;
			case 'T': systemMessage(M_instMCB); break;
			case 'Z': systemMessage(M_eocMCB); break;
		}
		putchar('\n');
#undef puts

	/* Is there a sub-chain? */
		tmcb = (MCB far*)MK_FP(FP_SEG(mcb) + 1, 0);
		if(!skipRec && FP_SEG(tmcb) < FP_SEG(maxMCB) && 
		   tmcb->type >= 'A' && tmcb->type <= 'Z')
			handleMCB(arg2, len2, tmcb, puf + 4, (MCB far*)MK_FP(FP_SEG(mcb) + mcb->size + 1, 0), wide);

	} while(mcb->type != 'Z' && (mcb = (MCB far*)MK_FP(FP_SEG(mcb) + 1 + mcb->size, 0)) != NULL
		&& (puf == 0 || mcb->type >= 'A' && mcb->type <= 'Z'));
}

int mcbList(const char *search, const int wide)
{	int l;

	if(search) l = strlen(search);
	else l = 0;
	chkStruc(firstMCB);
	if(l)	/* no display */
		freopen("nul", "w", stdout);
	handleMCB(search, l, firstMCB, 0, maxPoi, wide);
	if(firstMCBinUMB)
		handleMCB(search, l, firstMCBinUMB, 0, maxPoi, wide);
	return l? msgErrorNumber(E_searchList): 0;	/* nothing found */
}
