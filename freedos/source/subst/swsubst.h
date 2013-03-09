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
/* $RCSfile: SWSUBST.H $
   $Locker: ska $	$Name:  $	$State: Exp $

	Global definitions for Switch Subst.

*/

#ifndef __SWSUBST_H
#define __SWSUBST_H


#include "cds.h"

#define maxPoi ((MCB FAR *)MK_FP(0xffff,0xffff))
#define toggle(a) ((a) = !(a))
#define NoDrive 127

extern char buf[128];
extern char dummyDrvA[4];	/* points to A:\\ */
extern char DrvName[41];	/* filled by getDriveName() */
extern int join;		/* do JOIN instead of SUBST */
extern int cntJoin;	/* 1: view JOIN stats; 2: update JOIN stat */
extern int prntAfter;	/* call printCDS() ‘fter command is done successfully */
extern int fllNam;		/* Use hexadecimal numbers within names */
extern int creatDir;	/* 1: create path; 2: check if path exist */
extern int allAtt;		/* view attributes as map, too */
extern int lastChar;	/* l‘st char‘cter of an option is '!' */
extern int criticErr;	/* errors encountered within Critical error handler */
extern int dosish;		/* DOS like acting? */
extern char *includeDrives;	/* drives to search while ::*: */

int Drive(char **str);
/* Check for drive spec: (in examples: A means any drive letter)
		a) normal A:.úúúú
		b) abbreviated A
		c) :driver name:
		d) ?th drive with name  :?:driver name:	
		e) ::label:

	return: NoDrive if none drive spec was found
			0 == A:
	*str points to the byte BEHIND the drive spec
*/

void getDrvName(int drvNr);
/* Fills DrvName with the name of the driver of drv # drvNr

   drvNr: 0 == A:
*/

int driveList(char *search, int wide);
/* Search/Display the device driver chain.

   If serach == NULL, the chain is displayed.
   Otherwise: The chain is searched for the name search. The name within
   	the chain must start with search und is matched
   	case-insensitive. If wide is TRUE, the driver's file name stored
   	within the MCB is search, too.

   Return:	0 if search successful or display chain
   			msgErrorNumber(E_searchList) if not found
*/

int mcbList(const char *search, const int wide);
/* Search/Display the MCB chain.

	If search == NULL, the chain is displayed.
	Otherwise the chain is searched for the name search.
*/

int mkDir(int dr, char *path, int chkOnly);
/*	Make the path on drive dr; path must not contain the drive spec!

	dr: 0 == A: ...
    path: to make
    chkOnly: don't make it, just test, if path exist
    return: == 0: not OK
            != 0: OK
*/


int mkPhysDir(int dr, char *path);
/* make physical path, that means, that for drive dr any SUBST/JOIN must
   be broken for mkdir
   dr: 0 == A:
   path: to make
    return: == 0: not OK
            != 0: OK
*/

void printDosCDS(void);
/* print CDS structure in DOS like manner:
	X: => Y:\PATH

   what means X: is SUBST'ed (or JOIN'ed) to Y:\PATH
*/

void printCDS(void);
/* print CDS structure fully:
   including DPB, drive flags, actual path, driver name
   optional: flag map, JOIN stat
*/

void removeAll(int all);
/* Perform a "SUBST X: -"	(break SUBST/JOIN, re-locate DPB)
   for either all SUBST'ed, or JOIN'ed  or all drives.

   all: == 0: process only SUBST'ed or JOIN'ed drives
        != 0: process all non-NETWORK'ed drives
*/

int drvSetting(char *p, int query);
/* Set/Query drive flag settings:
   p: drive flags
   	e.g. dr:{(+=-)(OFF|ON|PHYS|NET|SUBST|JOIN|HIDDEN|#}
   	+: set flag (query: is flag setted)
   	-: unset flag (query: is flag not setted)
   	=: set this flag, unset all others (query: not implemented)
   	OFF: performs a "SUBST dr -", then turn off PHYS flag;
   	     ignores (+=-); (query: not implemented)
   	ON: performs a "SUBST dr -"; ignores (+=-); (query: not implemented)
   	PHYS: physical flag
   	NET: networked flag
   	SUBST: SUBST'ed
   	JOIN: JOIN'ed
   	HIDDEN: hidden flag
   	#: decimal number of a flag, range: 0..15

   	all names (except ON) can be abbreviated down to a single letter.

   query: == 0: set flags
          != 0: query flags
   return: errorlevel
*/



void switchSubst(int dr, char *path, int bslashoff);
/* SUBST/JOIN path to drive dr; automatically breaks existing SUBST/JOIN

   if the drive of path is the same as dr, the SUBST/JOIN flag is not set.

   path: full-qualified path, e.g. Y:\PATH;; can contain '/' or trailing '/'
   dr: 0 == A:
   bslashoff: offset of the backslash:
   	0: determine fromout path
   	>length(path): determine fromout the path
   	else: overwrite the determined (only useful for physical paths, e.g.
		  by networked paths "\\H.\A.\INSTALL", it's impossible to
		  determine the correct offset. Here '7' would be good, the
		  CDS table would print the path like: \\H.\A."\INSTALL)

*/

void swap(char *d1, char *d2, int unitSwap);
/* swaps the CDS entries of the drive specs d1 and d2.

	if unitSwap is non-zero, not the entries are swapped, but
	the unit specifier if both are units of the same device
	driver. This enables the swapping of the floppies.
*/

void swapFD(void);
/* swaps the units of the floppy drives */

void flushDisks(void);
/* flush the buffers, reset disk caches */

#endif
