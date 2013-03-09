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

#ifdef __WATCOMC__

char *getenv(const char *name) {
	char **envp, *ep;
	const char *np;
	char ec, nc;
	for (envp = environ; (ep = *envp) != NULL; envp++) {
		np = name;
		do {
			ec = *ep++;
			nc = *np++;
			if (nc == 0) {
				if (ec == '=') return ep;
				break;
			}
		} while (ec == nc);
	}
	return NULL;
}

#endif
