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

int memicmp(const void *s1, const void *s2, size_t n) {
	if (n != 0)
	{
		const unsigned char *p1 = (const unsigned char *) s1;
		const unsigned char *p2 = (const unsigned char *) s2;
		do {
			if (*p1 != *p2)
			{
				int c = toupper(*p1) - toupper(*p2);
				if (c) return c;
			}
			p1++; p2++;
		} while (--n != 0);
	}
	return 0;
}

#endif
