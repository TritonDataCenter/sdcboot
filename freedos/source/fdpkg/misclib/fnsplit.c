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

int fnsplit(const char *path, char *drv, char *dir, char *name, char *ext) {
	const char* end;
	const char* p;
	const char* s;
	int ret = 0;

	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
			ret += DRIVE;
		}
	} else if (drv) *drv = '\0';

	for(end=path; *end && *end!=':'; ) end++;

	for(p=end; p>path && *--p!='\\' && *p!='/'; )
		if (*p == '.') {
			end = p;
			break;
		}

	if (ext) for(s=end; (*ext=*s++); ) ext++;
	if (ext) ret += EXTENSION;

	for(p=end; p>path; )
		if (*--p=='\\' || *p=='/') {
			p++;
			break;
		}

	if (name) {
		for(s=p; s<end; ) *name++ = *s++;
		*name = '\0';
		ret += FILENAME;
	}

	if (dir) {
		for(s=path; s<p; ) *dir++ = *s++;
		*dir = '\0';
		ret += DIRECTORY;
	}

	if ( strchr(path, '*') != NULL || strchr(path, '?') != NULL) ret += WILDCARDS;
	return ret;
}

#endif
#else

int fnsplit(const char *path, char *drive, char *dir, char *name, char *ext) {
	int a = 0;
	char rname[_MAX_FNAME], rext[_MAX_EXT];
	_splitpath(path, drive, dir, rname, rext);
	if (strlen(rext) > 0) a+=EXTENSION;
	if (strlen(rname) > 0) a+=FILENAME;
	if (strchr(path, '\\') != NULL) a+=DIRECTORY;
	if (strchr(path, ':') != NULL) a+=DRIVE;
	if (strchr(path, '*') != NULL || strchr(path, '?') != NULL) a+=WILDCARDS;
	strcpy(name, rname);
	strcpy(ext, rext);
	return a;
}

#endif
