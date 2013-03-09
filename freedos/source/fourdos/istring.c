

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


// ISTRING.C - INI file string insertion routine for 4dos
//   Copyright 1992 - 2002, JP Software Inc., All Rights Reserved

#include <string.h>
#include <stdio.h>

#include "product.h"

#if ((_DOS && !_WIN) && _OPTION)  // 4DOS OPTION
#include "resource.h"
#include "inistruc.h"
extern INIFILE gaInifile;
#include "iniistr.h"
#else
#include "4all.h"
#endif  // 4DOS OPTION

#include "inifile.h"

// store a string; remove any previous string for the same item
int ini_string(INIFILE *InitData, int *dataptr, LPTSTR string, int slen)
{
	register unsigned int i;
	int old_len, ptype, move_cnt;
	unsigned int offset;
	unsigned int *fixptr;
	LPTSTR old_string;

	// calculate length of previous string, change in string space
	old_len = ((offset = (unsigned int)*dataptr) == INI_EMPTYSTR) ? 0 : (( strlen(( old_string = InitData->StrData + offset)) + 1) * sizeof(TCHAR) );

	// holler if no room
	if (( InitData->StrUsed + slen + 1 - old_len) > InitData->StrMax )
		return 1;

	// if there is an old string in the middle of the string space, collapse
	// it out of the way and adjust all pointers
	if (offset != INI_EMPTYSTR) {

		if ((move_cnt = InitData->StrUsed - (offset + old_len)) > 0) {

			memmove(old_string, old_string + old_len, move_cnt * sizeof(TCHAR) );
			for (i = 0; (i < guINIItemCount); i++) {

				fixptr = (unsigned int *)((char *)InitData + ((char *)gaINIItemList[i].pItemData - (char *)&gaInifile));
				ptype = (int)(INI_PTMASK & gaINIItemList[i].cParseType);

				if (((ptype == INI_STR) || (ptype == INI_PATH)) && (*fixptr != INI_EMPTYSTR) && (*fixptr > offset))
					*fixptr -= old_len;
			}
		}

		InitData->StrUsed -= old_len;
	}

	// put new string in place and adjust space in use
	if (slen > 0) {
		memmove(InitData->StrData + InitData->StrUsed, string, ( slen + 1 ) * sizeof(TCHAR) );
		*dataptr = InitData->StrUsed;
		InitData->StrUsed += (slen + 1);
	} else
		*dataptr = INI_EMPTYSTR;

	return 0;
}

