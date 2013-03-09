/*    
   Getdrvnms.c - routines to return the drive letters of the drives that
       can be defragmented.

   Copyright (C) 2000 Imre Leber

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

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/

#include <dos.h>
#include <bios.h>
#include <dir.h>

#include "misc.h"

/*
** Return how many floppy drives are detected by the bios. If there are
** more than two the function returns 2.
*/

static int AmofBiosReportedDrives (void)
{
     int configuration = biosequip();

     if (configuration & 1)               /* At least one floppy drive. */
   if (((configuration >> 6) & 3) > 0)
      return 2;
   else
      return 1;
     else
   return 0;
}

/*
** Returns the names of the optimizable drives in the string pointed to by 
** drives. Make sure that the string is minimal 27 bytes large.
*/

void GetDriveNames(char* drives)
{
     struct fatinfo info;
     int    amof, pos = 0;
     char   i;
     
     /* Look at how many floppy drives there are. */
     amof = AmofBiosReportedDrives();
     if (amof > 0) drives[pos++] = 'A';
     if (amof > 1) drives[pos++] = 'B';

     /* Look for any hard disks. */
     for (i = 2; i < (char) setdisk(getdisk()); i++)
     {
    if (!IsCDROM(i)) /* Still compatible with MSCDEX clones not 
              implementing version 2.0                */
    {
       getfat(i+1, &info);

       if ((!CriticalErrorOccured()) && 
      ((unsigned char) info.fi_fatid == 0xf8))
          drives[pos++] = 'A' + i;
    }
     }

     drives[pos] = '\0';
}

//#define DEBUG
#ifdef DEBUG

int main(int argc, char *argv[])
{
   char drives[27];

   GetDriveNames(drives);
   printf(drives);

   return 0;
}

#endif

