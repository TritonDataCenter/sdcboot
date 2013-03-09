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

#ifndef __TURBOC__

char far *getdta(void) {
#ifdef USE_C
	union REGS reg;
	struct SREGS sreg;
        reg.h.ah = 0x2F;
        intdosx(&reg, &reg, &sreg);
	return MK_FP(sreg.es, reg.x.bx);
#else
	asm {
		mov ah, 0x2F
		int 0x21
		mov ax, bx
		ret
	}
	return 0;
#endif
}

#ifdef USE_C
void setdta(char far *dta) {
	union REGS reg;
	struct SREGS sreg;
	reg.h.ah = 0x1A;
	sreg.ds = FP_SEG(dta);
	reg.x.dx = FP_OFF(dta);
	intdosx(&reg, &reg, &sreg);
}
#endif

#endif
