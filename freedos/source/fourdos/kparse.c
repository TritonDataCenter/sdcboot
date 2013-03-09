

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


// KPARSE.C - Keystroke parsing routines for everything except 4DOS
//   Copyright 1992 - 2002, JP Software Inc., All Rights Reserved

#include "product.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if ((_DOS && !_WIN) && _OPTION)  // 4DOS OPTION
#include "general.h"
#include "resource.h"
#include "inistruc.h"
extern INIFILE gaInifile;
#include "iniutil.h"
#include "inikpar.h"
#else
#include "4all.h"
#endif  // 4DOS OPTION

#define KEYPARSE 1
#include "inifile.h"

static int FindNext( LPTSTR pszOutput, LPTSTR pszInput, LPTSTR pszSkipDelims, LPTSTR pszEndDelims, int *nStart );


// Parse value for key directive
int _fastcall keyparse( LPTSTR pszKey, int nLength )
{
	register int nKey;
	TCHAR pszToken[11], szTokenBuf[11];
	int nKeyNumber, nPrefix = -1, tl, nTStart;

	sprintf( pszToken, FMT_FAR_PREC_STR, (( nLength > 10 ) ? 10 : nLength ), pszKey );

	// Get token, quit if nothing there
	if ( FindNext( szTokenBuf, pszToken, _TEXT(" -\t"), _TEXT("^ -;\t"), &nTStart ) == 0)
		return -1;

	// handle "@nn" -- return -nn for extended keystroke
	if ( szTokenBuf[0] == _TEXT('@') )
		return ( EXTENDED_KEY + atoi( &szTokenBuf[1] ));

	// handle numeric without '@'
	if ( isdigit( szTokenBuf[0] ))
		return atoi( szTokenBuf );

	// look for Alt / Ctrl / Shift prefix (must be more than 1 char, or
	// 1 char with a '-' after it)
	if ((( tl = strlen( szTokenBuf )) > 1 ) || ( pszToken[nTStart+1] == _TEXT('-') )) {

		// check prefix; complain if no match & single character; if no
		// match and multiple characters set prefix to 0
		if ( toklist(szTokenBuf, &KeyPrefixes, &nPrefix) != 1) {
			if (tl == 1)
				return -1;
			nPrefix = 0;
		}

		// if it matches look for something else, complain if not there
		else if ( FindNext( szTokenBuf, pszToken + nTStart + strlen( szTokenBuf ), _TEXT(" -\t"), _TEXT("^ -;\t"), NULL ) == 0 )
			return -1;

		else
			tl = strlen( szTokenBuf );
	}

	// If single character return ASCII
	if (tl == 1) {

		nKey = (int)_ctoupper( szTokenBuf[0] );

		if ( isalpha( nKey )) {

			switch ( nPrefix ) {
			case 1:     // Alt
				nKey = EXTENDED_KEY + ((int)ALT_ALPHA_KEYS[nKey - _TEXT('A')]);
				break;
			case 2:     // Ctrl -- adjust
				nKey -= 0x40;
				break;
			case 3:     // Shift -- error
				nKey = -1;
			}

		} else if ( isdigit( nKey )) {

			switch ( nPrefix ) {
			case 0:     // No prefix -- numeric key value
				nKey = atoi( szTokenBuf );
				break;
			case 1:     // Alt -- do table lookup
				nKey = EXTENDED_KEY + ((int)ALT_DIGIT_KEYS[nKey - '0']);
				break;
			case 2:     // Ctrl -- error
			case 3:     // Shift -- error
				nKey = -1;
			}
		}

		return nKey;
	}

	// We must have a non-printing key name, see if it's a function key
	// (no other key names start with 'F')
	if (_ctoupper(szTokenBuf[0]) == _TEXT('F')) 
		return (((( nKey = atoi(&szTokenBuf[1]) - 1) < 0) || ( nKey > 12 ) || ( nPrefix < 0 )) ? -1 : EXTENDED_KEY + (((nKey <= 9) ? (int)KeyPrefixList[nPrefix].F1Pref : (int)KeyPrefixList[nPrefix].F11Pref ) + nKey ));

	// Check for a valid non-printing key name
	if (toklist(szTokenBuf, &KeyNames, &nKeyNumber) != 1)
		return -1;

	// Get standard value for this key
	nKey = KeyNameList[nKeyNumber].NPStd;

	// handle prefix (special handling for some keys)
	if (nPrefix) {

		// don't allow Alt-Tab, handle Ctrl-Tab as special case
		// (Shift-Tab drops through to load NPSecond value)
		if (nKey == K_Tab) {
			if (nPrefix == 1)
				return -1;
         else if (nPrefix == 2)
            return K_CtlTab;

		// don't allow Shift-Backspace, handle Alt-Backspace as special case
		// (Ctrl-Backspace drops through to load NPSecond value)
		} else if (nKey == K_Bksp) {
			if (nPrefix == 3)
				return -1;
			else if (nPrefix == 1)
				return K_AltBS;

		// For all other keys the only valid prefix is Ctrl
		} else if (nPrefix != 2)
			return -1;

		// It is a real prefixed key, get value with prefix
		nKey = KeyNameList[nKeyNumber].NPSecond;
	}

	return nKey;
}


int toklist( LPTSTR token, TL_HEADER *tlist, int *pnIndex )
{
	register int i;
	union {
		TCHAR **pptr;
		char *cptr;
	} eptrs;

	eptrs.pptr = tlist->elist;

	for ( i = 0; ( i < tlist->num_entries ); i++, eptrs.cptr += tlist->entry_size ) {

		if ( _stricmp( token, *eptrs.pptr ) == 0 ) {
			*pnIndex = i;
			return 1;
		}
	}

	return 0;
}


// move to next token, copy into buffer, and return its length
static int FindNext( LPTSTR pszOutput, LPTSTR pszInput, LPTSTR pszSkipDelims, LPTSTR pszEndDelims, int *pnStart)
{
	int nEnd;
	int nHoldStart;

	// Find beginning of token by skipping over a set of delimiters
	nHoldStart = (int)strspn( pszInput, pszSkipDelims );
	pszInput += nHoldStart;
	if ( pnStart != NULL )
		*pnStart = nHoldStart;

	// Find the end of token
	nEnd = (int)strcspn( pszInput, pszEndDelims );

	// Copy token into buffer
	sprintf( pszOutput, FMT_PREC_STR, nEnd, pszInput );

	return nEnd;
}  // End FindNext

