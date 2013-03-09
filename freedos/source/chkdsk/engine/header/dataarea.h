/*    
   Dataarea.c - routines to read/write the data area of a volume.

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
/*
   This file exists because of a requirement of the FAT32 api to
   give the place where you write as a parameter to the new absolete
   write function.

   These macro's works with absolute sectors start counting from the boot
   sector.
*/

#ifndef DATAAREA_H_
#define DATAAREA_H_


#define ReadDataSectors(handle, nsects, lsect, buffer) \
        (ReadSectors(handle, nsects, lsect, buffer) != -1)
        
#define WriteDataSectors(handle, nsects, lsect, buffer) \
        (WriteSectors(handle, nsects, lsect, buffer, WR_DATA) != -1)
        
#endif
