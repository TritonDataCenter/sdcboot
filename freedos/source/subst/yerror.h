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
/* $Id: YERROR.H 3.2 2002/11/28 06:20:28 ska Exp ska $
   $Locker: ska $	$Name:  $	$State: Exp $

   Template for local message declaration file.
   You need to update the names of the feature list file and the
   message declaration file you actually use.

*/

#ifndef __YERROR_H
#define __YERROR_H

/**********************
	Place the name of your feature list file here:				*/
#include "msgfeat.inc"


/**********************
	Place all defines that shall control special features/capabilties/
	bugs/etc. of the message library here:						*/


#include <portable.h>
#include <msglib.h>


/**************************
	Place the name of your message declaration file here:		*/
#include "msgdecl.inc"


void chkStruc(void far *poi);


/* no format string error reporting
extern MSGID E_hlpFmtStr;
#define Nr_E_hlpFmtStr 20
*/

#endif
