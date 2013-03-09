/*
   Drive.c - contains a routine to check wether a string has the drive
	     form.

   Copyright (C) 1999, 2000, 2002 Imre Leber.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have recieved a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be

*/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>

#include "../../misc/bool.h"
#include "../header/drive.h"

/**************************************************************
**                    HasAllFloppyForm
***************************************************************
** Returns TRUE if the given string is of the form [A-Z]:
***************************************************************/

int HasAllFloppyForm (char* drive)
{
  int drv;
    
  assert(drive);

  if (strlen(drive) != 2)
     return FALSE;

  drv = toupper(drive[0]);
  if ((drv >= 'A') && (drv <= 'Z') && (drive[1] == ':'))
     return TRUE;

  return FALSE;
}

/**************************************************************
**                    HasFloppyForm
***************************************************************
** Returns TRUE if the given string is of the form [A-B]:
***************************************************************/

int HasFloppyForm (char* drive)
{
  int drv;

  if (HasAllFloppyForm(drive))
  {
     drv = toupper(drive[0]);
     if ((drv >= 'A') && (drv <= 'B'))
	return TRUE;
  }
  
  return 0;
}


