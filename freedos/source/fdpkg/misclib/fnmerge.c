/***************************************************************************
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License version 2 as          *
 * published by the Free Software Foundation.                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                *
 ***************************************************************************/

#define __INCLUDE_FILES_H 1

#include "misc.h"

#ifndef __WATCOMC__
#ifndef __TURBOC__

/*
 * Unfortunately, this one (by me) doesn't always work, so I'm replacing it
 */
/*void _makepath(char*path,const char *drive,const char *dir,const char *fname,const char *ext) {
	sprintf(path, "%s%s%s%s%s%s%s",
			(drive[0]!='\0')?drive:"",
			(drive[0]!='\0'&&drive[1]!=':')?":":"",
			(dir[0]!='\0')?dir:"",
			(dir[0]!='\0'&&dir[strlen(dir)-1]!='\\')?"\\":"",
			(fname[0]!='\0')?fname:"",
			(ext[0]!='\0'&&ext[0]!='.')?".":"",
			(ext[0]!='\0')?ext:"");
}*/

void _makepath(char *path,const char *drive,const char *dir,const char *fname,const char *ext) {
	int dir_len;

	if ((drive != NULL) && (*drive)) {
		strcpy(path, drive);
		strcat(path, ":");
	} else (*path)=0;

	if (dir != NULL) {
		strcat(path, dir);
		dir_len = strlen(dir);
		if (dir_len && *(dir + dir_len - 1) != '\\') strcat(path, "\\");
	}


	if (fname != NULL) {
		strcat(path, fname);
		if (ext != NULL) {
			if (*ext != '.') strcat(path, ".");
			strcat(path, ext);
		}
	}
}

#endif
#endif
