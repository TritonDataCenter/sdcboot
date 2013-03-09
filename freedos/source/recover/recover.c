/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  If you have any questions, comments, suggestions, or fixes please
 *  email me at:  imre.leber@worldonline.be   
 */

#include <stdio.h>

#include "misc\version.h"
#include "specific\recover.h"

int HasAllFloppyForm (char* drive);

void Usage(void)
{
   printf("RECOVER " VERSION "\n"  
          "Copyright 2003, Imre Leber\n"    
	  "\n"    
	  "Syntax:\n"
	  "\tRECOVER [d:][path]filename\n"
	  "\tRECOVER d:\n"
	  "\n"
	  "Purpose: Resolves sector problems on a file or a disk\n"  
	  "\n"
          "When entering a file this program will clean that file of any bad clusters.\n"
	  "\n"
          "When entering a disk all files on the disk will be cleared of bad clusters.\n"
	  "WARNING: when entering a disk all the file names on the disk will dissapear!\n"
	  "\n"
	  "Note:\n"
	  "\tYou should *NOT* be using this utility\n"
	  "\tThis utility only works on FAT12 and FAT16\n");        
}

int main(int argc, char** argv)
{
    char drive[3];
    
    if ((argc != 2) || (argv[1][0] == SwitchChar()) || (argv[1][0] == '/'))
    {
	Usage();
	return 0;
    }
    
    memcpy(drive, argv[1], 2); drive[2] = 0;
    
    if (HasAllFloppyForm(drive) && 
	((argv[1][2] == '\0') || 
	 ((argv[1][2] == '\\') && (argv[1][3] == '\0')))) /* Disk entered */
    {
	RecoverDisk(drive);
    }
    else
    {
	RecoverFile(argv[1]); 
    }
    
    printf("Done");
    return 0;
}
