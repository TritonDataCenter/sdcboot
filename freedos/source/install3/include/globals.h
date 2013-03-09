/* Copyright (C) 1998,1999,2000,2001 Jim Hall <jhall@freedos.org> */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* 
  This file contains global variables
  Currently they are used in:
  install.c, inst.c, pause.c, repaint.c
*/

#ifndef INSTALL_GLOBALS
#define INSTALL_GLOBALS


#include "catgets.h"			/* DOS catopen(), catgets() */


#ifdef __cplusplus
extern "C" {
#endif

/* Globals */
extern nl_catd cat;			/* language catalog */
extern char *yes;				/* localized versions of yes & no */
extern char *no;
extern int nopauseflag ;		/* 0=pause, 1=don't pause ie autoinstall */
extern int mono;

#ifdef __cplusplus
}
#endif

#endif /* INSTALL_GLOBALS */
