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

#ifdef __PACIFIC__

static int findfile(struct find_t *ff, int mode, const char *name, int attr) {
	union REGS rx;
	struct SREGS rs;
	char far *curDTA = getdta();
	assert(curDTA);
	setdta((char far*)ff);
	if((rx.h.ah = mode) == 0x4E)
	{
		rs.ds = FP_SEG(name);
		rx.x.dx = FP_OFF(name);
		rx.x.cx = attr;
	}
	intdosx(&rx, &rx, &rs);
	setdta(curDTA);
	return (rx.x.cflag & 1)? (errno = rx.x.ax): 0;
}

int _dos_findfirst(const char *name, int attr, struct find_t *ff)
{	return findfile(ff, 0x4E, name, attr);
}

int _dos_findnext(struct find_t *ff)
{	return findfile(ff, 0x4F, '\0', '\0');
}

#endif
