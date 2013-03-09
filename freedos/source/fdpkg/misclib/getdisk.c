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

int _getdrive(void)
{
	union REGS rx;
	rx.h.ah = 0x19;
	intdos(&rx, &rx);
	return rx.h.al + 1;
}

void _dos_getdrive(unsigned *drive)
{
	union REGS rx;
	rx.h.ah = 0x19;
	intdos(&rx, &rx);
	drive = (unsigned *)&rx.h.al + 1;
}

void _dos_setdrive(unsigned a, unsigned *total)
{
	union REGS rx;
	rx.h.ah = 0x0E;
	rx.h.dl = a - 1;
	intdos(&rx, &rx);
	total = (unsigned *)rx.h.al;
}


#endif

#ifdef __WATCOMC__

int setdisk(unsigned char drive) {
#ifdef USE_C
	union REGS r;
	r.h.ah = 0x0E;
	r.h.dl = drive;
	intdos(&r, &r);
	return r.h.al;
#else
	unsigned char tmp;
	asm {
		mov ah, 0x0E
		mov dl, drive
		int 0x21
		mov tmp, al
	}
	return (int)tmp;
#endif
}

#ifndef USE_C
int getdisk(void) {
	unsigned char tmp;
	asm {
		mov ah, 0x19
		int 0x21
		mov tmp, al
	}
	return (int)tmp;
}
#endif

#endif
