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
/* $RCSfile: swsubst.c $
   $Locker: ska $	$Name:  $	$State: Exp $

	Main shell for Switch Subst.
	Based on SwitchSubst SKAUS17B2.

*/
#pragma hdrfile "swsubst.csm"

#include <ctype.h>
#include <string.h>
#include <dos.h>
#include <dir.h>
#include <stdlib.h>

#include <portable.h>
#include <getopt.h>
#include <appname.h>
#include "swsubst.h"
#include "yerror.h"

#define toggle(a) ((a) = !(a))
#define CMD_SUBST 0
#define CMD_JOIN 1
#define CMD_MKDIR 2
#define CMD_CHDIR 3
#define CMD_QUERY 4
#define CMD_SWAP 5
#define CMD_WHICH 6
#define CMD_DRVLST 7
#define CMD_MCBLST 8
#define CMD_SWAPUNIT 9

typedef void interrupt 
#ifdef __cplusplus
	(*fintr)(...);
#else
	(*fintr)();
#endif

#ifndef lint
static char rcsid[] = "$Id: swsubst.c 3.2 2002/11/28 06:20:27 ska Exp ska $";
#endif
char buf[128];
char dummyDrvA[] = "A:\\";
int join = CMD_SUBST;		/* Join nicht Subst ausfhren */
#define command join
int cntJoin = 0;	/* 1: Join-Stat anzeigen; 2: Join-Stat berichtigen */
int prntAfter = 0;	/* nach getaner Arbeit printCDS() ausfhren */
int fllNam = 0;		/* Treibername vollst„ndig angeben (auch nicht printables) */
int creatDir = 0;	/* Verz erstellen, falls fehlt */
int allAtt = 0;		/* Attribute als Map ausgeben */
int lastChar = 0;	/* letztes Zeichen der Option */
int criticErr = 0;	/* errors encountered within Critical error handler */
int dosish = 0;		/* DOS like acting? */
int delete = 0;		/* "/d" specified */
char *includeDrives = NULL;	/* drives, which will be searched while ::*: */

int sk_mkdir(char **argv)
{	int phy, dr;
	char *p;

	join = 0;
	--argv;
	while((p = *++argv) != NULL) {
		if(0 != (phy = (*p == '-'))) ++p;
		if(p[1] == '\0' || (dr = Drive(&p)) == NoDrive)
			dr = getdisk();
		if(!(phy? mkPhysDir(dr, p): mkDir(dr, p, creatDir == 2)))
			error(E_mkDir, *argv);
	}
	return 0;
}

int sk_chdir(char **argv)
{	char *p;
	int dr;

	sk_mkdir(argv);
	if(*(p = strchr(argv[0], '\0') - 1) == '/' || *p == '\\')
		*p = 0;
	if(*(p = argv[0]) == '-')
		++p;
	if(p[1] && (dr = Drive(&p)) != NoDrive) {
		setdisk(dr);
		if(getdisk() != dr)
			error(E_setDisk, dr + 'A');
	}
	if(chdir(p))
		error(E_chDir, dr + 'A', p);
	return 0;
}

void processXclude(void)
/* Process the includeDrives string to hold non-excluded drives in
   alphabetical order. */
{	static char drives[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *p, *h;

	drives[lastdrv] = '\0';
	if(includeDrives) {	/* currently XCLUDED drives */
		int a, e, t;

		strupr(p = includeDrives);
		while(*p) 
			if((e = a = *p++) != '-') {
				if(*p == '-')
					if(0 != (e = *++p)) ++p;
					else e = a;
				if(a > e) {
					t = e;
					e = a;
					a = t;
				}
				if(a < 'A') a = 'A';
				if(e > 'Z') e = 'Z';
				do if(0 != (h = strchr(drives, e)))
					*h = h[1]? ' ': '\0';
				while(a <= --e);
			}
	}
	p = h = drives - 1;
	while(*++h) 
		if(*h != ' ') *++p = *h;
	p[1] = '\0';
	includeDrives = drives;
}

#pragma warn -par
#pragma warn -ofp
#pragma warn -rvl
void FAR interrupt new_int24(unsigned bp, unsigned di, unsigned si, 
	unsigned ds, unsigned es, unsigned dx, unsigned cx, unsigned bx,
	unsigned ax, unsigned ip, unsigned cs, unsigned flag)
{	++criticErr; ax = 3; /* fail */ }
#pragma warn +par
#pragma warn +ofp
#pragma warn +rvl

/* table of valid file names */
const char *N_fnames[] = {"JOIN", "SUBST", "MKDIR", "CHDIR", "-JOIN",
	"-MKDIR", "-CHDIR", "QUERY", "-QUERY", "SWAP", "-SWAP", "WHICH",
	"-WHICH", "DRVLIST", "MCBLIST", 
	NULL};
const int N_cmds[] = {CMD_JOIN, CMD_SUBST, CMD_MKDIR, CMD_CHDIR, CMD_JOIN,
	CMD_MKDIR, CMD_CHDIR, CMD_QUERY, CMD_QUERY, CMD_SWAP, CMD_SWAP, CMD_WHICH,
	CMD_WHICH, CMD_DRVLST, CMD_MCBLST,
	CMD_SUBST};		/* default */

_Cdecl main(int argc, char **argv)
{	int opt;
    char *p;
    int dr;
    CDS FAR* dir;
    int bslashoff;

	_dos_setvect(0x24, (fintr)new_int24);  
					// Critical Error Handler searchLabel

	msgInit();

#ifdef __TURBOC__
    lastdrv = setdisk(getdisk());
#else
    int currdrv, lastdrv;
    _dos_getdrive(&currdrv);
    _dos_setdrive(currdrv, &lastdrv);
#endif
	if(lastdrv > 26)		/* Novell Netware returns 6 additional */
		lastdrv = 26;		/* phantom drives, this check insists that
								the user has LASTDRIVE = Z, which is usually
								true in a Netware environment */

	(void)cds(lastdrv);	/* ensure, the listOfLists related variables are
						   initialized */

	/* transform file name into command */
	dr = -1;
	while(N_fnames[++dr] && strcmp(N_fnames[dr], appName()));
	command = N_cmds[dr];
	if(dr <= CMD_JOIN)
		creatDir = dosish = 1;	/* DOS-ish JOIN/SUBST */

	bslashoff = 0;
	if(dosish) {
		if(argc == 1) printDosCDS();
		else {
			if((delete = getopt(argc, argv, "D")) == EOF)
				delete = 0;
			else if(++optind == argc)
				hlpScreen();
			p = argv[optind++];
			goto directSubst;
		}
	}

	while((opt = getopt(argc, argv, "#!_?ACDFHJKLMNOQRSTUWO:X:")) != EOF)
		 switch(opt) {
			case '#': toggle(cntJoin); break;
			case '!': cntJoin = 2; break;
			case 'A': toggle(prntAfter); break;
			case 'F': toggle(fllNam); break;
			case 'M': command = CMD_MKDIR; break;
			case 'C': command = CMD_CHDIR; break;
			case 'J': command = CMD_JOIN; break;
			case 'U': command = CMD_SUBST; break;
			case 'S': command = CMD_SWAP; break;
			case 'N': command = CMD_SWAPUNIT; break;
			case 'Q': command = CMD_QUERY; break;
			case 'W': command = CMD_WHICH; break;
			case 'R': command = CMD_DRVLST; break;
			case 'L': command = CMD_MCBLST; break;
			case 'K': toggle(creatDir); break;
			case 'T': creatDir = 2; break;
			case '_': toggle(allAtt); break;
			case 'D': toggle(delete); break;
			case 'O': 
				if(*optarg) {	
					long l;

					if((l = strtol(optarg, &p, 0)) < 0 || p != NULL && *p
				 	 || l > 66)
				 		error(E_number, optarg, 66);
					bslashoff = (int)l;
				}
				else bslashoff = 0;
				break;
			case 'X':
				includeDrives = *optarg? optarg: NULL;
				break;
			case 'H': case '?': hlpScreen();
		}

	flushDisks();
	processXclude();

	p = argv[optind++];
#define args(a) if(optind + (a) - 1 != argc) hlpScreen();
	switch(command) {
		case CMD_MKDIR:
			sk_mkdir(argv + optind - 1);
			goto returnOK;
		case CMD_CHDIR:
			args(1)
			sk_chdir(argv + optind - 1);
			goto returnOK;
		case CMD_QUERY:
			args(1)
			return drvSetting(p, 1);
		case CMD_SWAP:
			args(2)
			swap(p, argv[optind], 0);
			goto returnOK;
		case CMD_SWAPUNIT:
			if(p == NULL) /* no argument => swap floppies */
				swapFD();
			else {
				args(2)
				swap(p, argv[optind], 1);
			}
			goto returnOK;
		case CMD_WHICH:
			args(1)
			if((opt = Drive(&p)) == NoDrive) return 1;
			return opt + 'A';
		case CMD_DRVLST:
			if(p == NULL) return driveList(NULL, 0);
			if(argv[optind]) hlpScreen();
			return    *p == '-'? driveList(p + 1, 0)
					: *p == '+'? driveList(p + 1, 1)
					: driveList(p, 0);
		case CMD_MCBLST:
			if(p == NULL) return mcbList(NULL, 0);
			if(argv[optind]) hlpScreen();
			return    *p == '-'? mcbList(p + 1, 0)
					: *p == '+'? mcbList(p + 1, 1)
					: mcbList(p, 0);
	}

	if(p == NULL) {
		printCDS();
		return 0;
	}

	if(argv[optind] == NULL)	/* a single argument */
		if(p[0] == '-' && (p[1] == 0 || p[1] == '-' && p[2] == 0)) {
			/* swsubst ? - for all SUBST'ed or JOIN'ed drives */
			removeAll(p[1]);
			goto returnOK;
		}
		else if(drvSetting(p, 0))	/* Maybe Drive flag settings */
			goto returnOK;

directSubst:
	if(delete) {
		args(1)
	}
	else {
		args(2)
		/* check if second parameter is delete flag => "-" or "--" or "/d" or "/D" */
		delete = argv[optind][0] == '-' && argv[optind][(argv[optind][1] == '-') + 1] == 0
		  || getopt(argc, argv, "D") != EOF;
	}

	if((dr = Drive(&p)) == NoDrive)
		error(E_drive, p);
	
        if((dir = cds(dr)) == NULL)
            fatal(E_cds, dr + 'A');
        else {
            if(!(dir->flags & NETWORK)) {
			  if(delete) {
			  	/* SUBST l"oschen */
			  	memcpy(buf, dummyDrvA, sizeof(dummyDrvA));
			  	*buf += dr;
			  	goto performSubst;
			  }

              p = argv[optind];

			  if(p[0] == '-') {
			  	if(dosish)
			  		error(E_path, p);

			  	/* bypass even fullpath, if path starts with "--" or "-\\\\" */
			  	if(p[1] == '-') strcpy(buf, p + 2);
			    else if(p[2] == '\\' && p[1] == '\\')
			  		strcpy(buf, p + 1);
			  	else {
			  		if(_fullpath(buf, p + 1, sizeof(buf)) == NULL) 
			  			fatal(E_noMem);
			  		strupr(buf);
			  	}
			  } 
			  else {
			  	union REGS regs;
			  	struct SREGS segregs;

				regs.h.ah = 0x60;
				regs.x.si = FP_OFF(p);
				segregs.ds = FP_SEG(p);
				regs.x.di = FP_OFF(buf);
				segregs.es = FP_SEG(buf);
			  	criticErr = 0;
			  	intdosx(&regs, &regs, &segregs);
				if(criticErr) error(E_criticPath, p);
				if(regs.x.cflag) error(E_path, p);
			  }
performSubst:
			  switchSubst(dr, buf, bslashoff);
			}
			else error(E_subst, 'A' + dr);
        }
returnOK:

#ifndef TEST
	if(prntAfter)
#endif
		printCDS();

	flushDisks();

    return 0;
}
