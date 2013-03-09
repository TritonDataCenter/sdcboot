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
/* $RCSfile: yerror.c $
   $Locker: ska $	$Name:  $	$State: Exp $

   Fehlerbehandlungsroutinen

*/

#pragma hdrfile "error.csm"
#include <dos.h>
#include <process.h>
#include <appname.h>
#include <msglib.h>
#include "yerror.h"
#pragma hdrstop

#ifndef lint
static char const rcsid[] = 
	"$Id: yerror.c 3.2 2002/11/28 06:20:27 ska Exp ska $";
#endif

extern int join;			/* do this process a JOIN? */
extern int dosish;			/* compatibly mode? */

#undef hlpScreen
void hlpScreen(void)
{	if(dosish)
		message(stdout, join? E_JOIN_E: E_SUBST_E);
	else 
		message(stdout, E_hlpScreen, appName(), join? "/j": "/u");
	exit(msgErrorNumber(E_hlpScreen));
}

void chkStruc(void far *poi)
{	if(!poi)	fatal(E_struc);
}
