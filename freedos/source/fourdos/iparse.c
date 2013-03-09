

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


// IPARSE.C - INI file parsing routines for 4xxx / TCMD
//   Copyright 1992 - 2002, JP Software Inc., All Rights Reserved

#include "product.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <string.h>

#if ((_DOS && !_WIN) && _OPTION)  // 4DOS OPTION
#include "general.h"
#include "iniutil.h"
#include "resource.h"
#include "inistruc.h"
extern INIFILE gaInifile;
#include "iniipar.h"
#include "iniistr.h"
#include "inikpar.h"
#include "argparse.h"
#else
#include "4all.h"
#endif  // 4DOS OPTION

#define INIPARSE 1
#include "inifile.h"

#if _WIN
extern TCHAR gszProgramName[];
#endif

static void _fastcall ClearKeyMap(INIFILE *);
//static void SwapScrollKeys(INIFILE *);
static int ini_byte(unsigned char *, unsigned char, int);
static int ini_int(int *, int, int *);
static int ini_uint(unsigned int *, unsigned int, unsigned int *);
static int _fastcall nonnum( LPTSTR, int );
static void ini_error( LPTSTR, INIFILE *, LPTSTR, unsigned int, LPTSTR );
int next_token(TCHAR **, int, int, LPTSTR, LPTSTR);

#if !((_DOS && !_WIN) && _OPTION)  // Not 4DOS OPTION
static int IniFProc(LPTSTR, INIFILE *, unsigned int);
static int nNestLevel;
#endif  // Not 4DOS OPTION

void _fastcall IniClear(INIFILE *InitData)
{
	register unsigned int i, defval;
	unsigned char *dataptr;

	// loop for all items in the structure
	for (i = 0; (i < guINIItemCount); i++) {

		dataptr = ((unsigned char *)InitData) + ((unsigned char *)gaINIItemList[i].pItemData - (unsigned char *)&gaInifile);
		defval = gaINIItemList[i].uDefValue;

		switch( gaINIItemList[i].cParseType & INI_PTMASK ) {

		// character
		case INI_CHAR:
			ini_byte( dataptr, (unsigned char)defval, (UINT_PTR)VNULL );
			break;

		// choice item or integer item
		case INI_CHOICE:
		case INI_INT:
			ini_int( (int *)dataptr, defval, (int *)VNULL );
			break;

		// string or path item, set default
		case INI_PATH:
		case INI_STR:

			defval = (unsigned int)INI_EMPTYSTR;

		// unsigned integer or color item
		case INI_UINT:
		case INI_COLOR:
			ini_uint((unsigned int *)dataptr, defval, (unsigned int *)VNULL);
		}
	}
}


#if !((_DOS && !_WIN) && _OPTION)  // Not 4DOS OPTION
// parse an INI file
int IniParse(LPTSTR fname, INIFILE *InitData, unsigned int fIgnoreSection)
{
	nNestLevel = 0;
	return (IniFProc(fname, InitData, fIgnoreSection));
}


// process a single INI file
static int IniFProc(LPTSTR fname, INIFILE *InitData, unsigned int fIgnoreSection)
{
	LPTSTR arg;
	unsigned int line_num, SecFound = 0, CurrentSec = 0;
	int fh, nReturn = 0;
	TCHAR szBuffer[2048];
	LPTSTR errmsg;

	if ( ++nNestLevel > MAX_INI_NEST ) {
		ini_error( E_NEST, InitData, fname, 1, NULLSTR );
		nReturn = -1;
		goto IFProcExit;
	}


	// force ignore of sections that aren't ours
	fIgnoreSection |= 0x100;

	// open the INI file
	if ((fh = _sopen(fname, (O_RDONLY | O_BINARY), SH_DENYWR)) < 0) {
		nReturn = -1;
		goto IFProcExit;
	}

	// loop to process each line
	for ( line_num = 1; ( getline( fh, szBuffer, 2047, 0 ) > 0 ); line_num++ ) {

		// skip line if empty or all comment
		arg = skipspace( szBuffer );
		if ((*arg == _TEXT('\0')) || (*arg == _TEXT(';')))
			continue;

		// query if it was requested
		if ( InitData->INIQuery != 0 ) {

			WriteTTY(szBuffer);
			WriteTTY(INI_QUERY);
query_prompt:
			switch (GetKeystroke(EDIT_BIOS_KEY | EDIT_ECHO | EDIT_ECHO_CRLF | EDIT_UC_SHIFT)) {
			case YES_CHAR:
				break;
			case NO_CHAR:
				continue;
			case INI_QUIT_CHAR:
				goto ini_done;
			case REST_CHAR:
				InitData->INIQuery = 0;
				break;
			case INI_EDIT_CHAR:
				egets( szBuffer, 2047, EDIT_BIOS_KEY | EDIT_ECHO );
				break;
			default:
				qputc( STDOUT, BS );
				honk();
				goto query_prompt;
			}

		}

		// process the line, holler if any error
		if ((nReturn = IniLine(szBuffer, InitData, (fIgnoreSection & CurrentSec), 0, &errmsg)) > 0)
			ini_error( errmsg, InitData, fname, line_num , szBuffer );

		// if no error, see if we have an included file to process
		else if (nReturn == -1) {

			// call ourselves recursively to process the included file
			if ( is_file( szBuffer ))
				(void)IniFProc( szBuffer, InitData, fIgnoreSection);
			else
				ini_error( E_INCL, InitData, fname, line_num, szBuffer );

		// if no error, see if we found a section name
		} else if (nReturn < 0) {

			CurrentSec = -nReturn;

			// save section bit if section is one of ours
			if (CurrentSec & 0xFF)
				SecFound |= CurrentSec;
		}
	}


ini_done:

	InitData->SecFlag = SecFound;    // save sections found
	_close(fh);

	nReturn = 0;

IFProcExit:
	--nNestLevel;
	return nReturn;
}

#endif  // Not 4DOS OPTION


// process a single INI directive
int IniLine(LPTSTR szBuffer, INIFILE *InitData, int fSectionOnly, int fLiveMod, TCHAR **errmsg)
{
	register int i, nTokenLength;
	unsigned int ptype, fPath;
	int fg, bg, bc;
	int j, toknum, tokdata, defval, context;
	int path_len, min_path_len;
	TCHAR szPathName[MAXFILENAME], szPathTest[MAXFILENAME];
	unsigned char *dataptr;
	void *vdata;
	LPTSTR pszToken, delims;

	// be sure the line is double-null terminated
	szBuffer[ strlen( szBuffer ) + 1 ] = '\0';

	// get first token, skip line if empty or all comment
	pszToken = skipspace( szBuffer );

	// handle section name 
	if (*pszToken == _TEXT('[')) {

		strip_trailing( ++pszToken, _TEXT(" \t]") );
		if ( toklist( pszToken, &SectionNames, &toknum ) != 1 )
			return -0x100;

		// legitimate section name, return corresponding bit
		return (-(0x80 >> toknum));
	}

	// not a section name, return if skipping other stuff
	if ( fSectionOnly )
		return 0;

	if (( nTokenLength = next_token( &pszToken, 0, 0, _TEXT(" \t"), _TEXT(" ;=\t") )) == 0 )
		return 0;

	// look up item name, holler if no good
	if ( toklist( pszToken, &INIItems, &toknum ) == 0 ) {
		*errmsg = E_BADNAM;
		return 1;
	}

	// Holler if we are in live modification mode (as opposed to startup) and
	// live mods are not allowed
	if (fLiveMod && (gaINIItemList[toknum].cParseType & INI_NOMOD)) {
		*errmsg = E_BADNAM;
		return 1;
	}

	// get data offset, parse type, and validation for the item
	dataptr = ((unsigned char *)InitData) + ((unsigned char *)gaINIItemList[toknum].pItemData - (unsigned char *)&gaInifile);
	ptype = (int)(gaINIItemList[toknum].cParseType & INI_PTMASK);
	vdata = gaINIItemList[toknum].pValidate;
	defval = gaINIItemList[toknum].uDefValue;

	// find item value
	if (ptype == INI_STR)
		delims = _TEXT("\t");
	else if (ptype == INI_COLOR)
		delims = _TEXT(";");
	else
		delims = _TEXT(" \t;");

	if ((ptype < INI_KEY_MAP) && ((nTokenLength = next_token(&pszToken, nTokenLength+1, 1, _TEXT(" =\t"), delims)) == 0)) {
		// if no value then return
		return 0;
	}

	// handle the item based on its type
	switch (ptype) {

	// single character -- but if user enters a longer string, see if
	// it's a key name, so mistakes like entering "Ctrl-X" instead of
	// the actual character don't cause trouble
	case INI_CHAR:

		*errmsg = E_BADCHR;
		if (nTokenLength > 1) {
			if ((tokdata = keyparse(pszToken, nTokenLength)) == -1)
				return 1;
		} else
			tokdata = (unsigned int)*pszToken;

		if (ini_byte(dataptr, (unsigned char)tokdata, (int)(INT_PTR)vdata))
			return 1;
		break;

	case INI_INT:		// signed integer

		sscanf(pszToken,FMT_INT,&j);
		if (nonnum(pszToken,nTokenLength) || ini_int((int *)dataptr, (int)j, (int *)vdata)) {
			*errmsg = E_BADNUM;
			return 1;
		}
		break;

	case INI_UINT:		// unsigned integer

		sscanf(pszToken,FMT_UINT,&j);
		if (nonnum(pszToken,nTokenLength) || ini_uint((unsigned int *)dataptr, (unsigned int)j, (unsigned int *)vdata)) {
			*errmsg = E_BADNUM;
			return 1;
		}
		break;

	case INI_CHOICE:	// choice, check against list

		if (toklist(pszToken, (TL_HEADER *)vdata, &toknum) != 1) {
			*errmsg = E_BADCHC;
			return 1;
		}
		ini_int((int *)dataptr, toknum, (int *)VNULL);
		break;

	case INI_COLOR:		// color spec

		fg = bg = -1;
		bc = 0;
		ParseColors(pszToken,&fg,&bg);

#if (_DOS && _OPTION)  // 4DOS OPTION
		for (i = 0; ((delims = NthArg(pszToken,i,NULL)) != NULL); i++) {
			if (_strnicmp(delims,BORDER,3) == 0) {
				bc = color_shade(NthArg(pszToken,++i,NULL));
				break;
			}
		}
#elif _DOS != 0
		for (i = 0; ((delims = ntharg(pszToken,i)) != NULL); i++) {
			if (_strnicmp(delims,BORDER,3) == 0) {
				bc = color_shade(ntharg(pszToken,++i));
				break;
			}
		}
#endif

		// we have a color, holler if it's invalid
		if ((fg == -1) || (bg == -1) || (fg == bg) || (bc > 7)) {
			*errmsg = E_BADCOL;
			return 1;
		}

		// store color
		ini_uint((unsigned int *)dataptr, fg + (bg << 4) + (bc << 8), (unsigned int *)VNULL);
		break;

	case INI_KEY:

		// see if it's a valid keystroke
		if ((tokdata = keyparse(pszToken, nTokenLength)) == -1) {
			*errmsg = E_BADKEY;
			return 1;
		}

		// see if there's already a mapping in the table
		context = CONTEXT_BITS(defval);

		for (i = 0, j = 0; (i < (int)InitData->KeyUsed); i++) {

			if ((tokdata == (int)InitData->Keys[i]) && (context == (int)(CONTEXT_BITS(InitData->Keys[InitData->KeyUsed+i])))) {
				// found old mapping with same context, override it
				InitData->Keys[InitData->KeyUsed+i] = (unsigned int)defval;
				j++;
				break;
			}
		}

		// if no previous mapping found create a new one
		if (!j) {

			// holler if no room
			if (InitData->KeyUsed >= InitData->KeyMax) {
				*errmsg = E_KEYFUL;
				return 1;
			}

			// slide old table up, then add new key
			if (InitData->KeyUsed > 0)
				memmove(&InitData->Keys[InitData->KeyUsed + 1], &InitData->Keys[InitData->KeyUsed], InitData->KeyUsed * sizeof(unsigned int));

			InitData->Keys[InitData->KeyUsed] = (unsigned int)tokdata;
			InitData->Keys[(2*InitData->KeyUsed++)+1] = (unsigned int)defval;
		}
		break;

	case INI_STR:		// string, just store it

		if (ini_string(InitData, (int *)dataptr, pszToken, nTokenLength)) {
			*errmsg = E_STROVR;
			return 1;
		}

		break;

	case INI_PATH:		// path, check for validity then store as string

		// pull path out of line buffer
		copy_filename( szPathName, pszToken );

		// check validity of path if validity check is not disabled
		if (((INT_PTR)vdata & 0x4000) == 0) {

			// if only looking for path, throw away file name
			fPath = (int)(INT_PTR)vdata & 0x8000;
			if ((fPath) && (path_part(szPathName) != NULL)) {
				strcpy(szPathTest, path_part(szPathName));
				strip_trailing(szPathTest, SLASHES);
			} else
				strcpy(szPathTest, szPathName);

			// check for bad name, or for file found when path required
			if (is_dir(szPathTest) == 0) {
				*errmsg = E_BADPTH;
				return 1;
			}
		}

		// pad length if required
		path_len = strlen(szPathName);
		if (((min_path_len = (int)((INT_PTR)vdata & 0xFFF)) > 0) && (min_path_len > path_len)) {
			wcmemset(szPathName+path_len, _TEXT('\0'), (min_path_len - path_len) );
			path_len = min_path_len;
		}

		// store the path

		if (ini_string(InitData, (int *)dataptr, szPathName, path_len)) {
			*errmsg = E_STROVR;
			return 1;
		}
	
		break;

	case INI_KEY_MAP:		// clear the key map
		ClearKeyMap(InitData);
		break;

	case INI_INCLUDE:		// process an included file
		if (next_token(&pszToken, nTokenLength+1, 1, _TEXT(" =\t"), _TEXT("\t")) > 0) {
			strcpy( szBuffer, pszToken );
			return -1;
		}
	}

	return 0;
}


// validate and store a byte value
static int ini_byte(unsigned char *dataptr, unsigned char data, int vrange)
{
	if ((vrange != (INT_PTR)VNULL) && ((data < (unsigned char)((vrange & 0xFF00) >> 8)) || (data > (unsigned char)(vrange & 0xFF))))
		return 1;

	*dataptr = data;

	return 0;
}


// validate and store a signed integer value
static int ini_int(int *dataptr, int data, int *vrange)
{
	if ((vrange != (INT_PTR *)VNULL) && ((data < vrange[0]) || (data > vrange[1])))
		return 1;

	*dataptr = data;

	return 0;
}


// validate and store an unsigned integer value
static int ini_uint(unsigned int *dataptr, unsigned int data, unsigned int *vrange)
{
	if ((vrange != (unsigned int *)VNULL) && ((data < vrange[0]) || (data > vrange[1])))
		return 1;

	*dataptr = data;

	return 0;
}



// clear the key map (procedure for ClearKeyMap directive)
static void _fastcall ClearKeyMap(INIFILE *InitData)
{
	InitData->KeyUsed = 0;
}


// check if a string is all numeric, return 0 if so, 1 if not
static int _fastcall nonnum( LPTSTR str, int nLength )
{
	register int i;

	for ( i = 0; ( i < nLength ); i++ ) {
		if (( isdigit( str[i] ) == 0 ) && ( str[i] != _TEXT('-') ))
			return 1;
	}

	return 0;
}


// move to next token, terminate it with '\0', and return its length
int next_token(TCHAR **tokptr, int prev_len, int fQuotes, LPTSTR pszSkipDelimiters, LPTSTR pszEndDelimiters)
{
	INT_PTR tend;
	LPTSTR pEnd;

	// Skip a specified number of spaces into the string
	*tokptr += prev_len;

	// Find beginning of data by skipping over a set of delimiters
	*tokptr += strspn(*tokptr, pszSkipDelimiters);

	// Handle quoted string (ignores end delimiters)
	if ((fQuotes) && (**tokptr == _TEXT('\"')) && ((pEnd = strrchr(*tokptr + 1, _TEXT('\"'))) != NULL)) {
		(*tokptr)++;
		tend = pEnd - *tokptr;
	} else 
		// If not quoted, find end of data based on delimiters
		tend = strcspn(*tokptr, pszEndDelimiters);

	// Terminate token
	*(*tokptr + tend) = '\0';

	return (int)tend;
}


#if !((_DOS && !_WIN) && _OPTION)  // Not 4DOS OPTION

static void ini_error( LPTSTR errmsg, INIFILE *InitData, LPTSTR fname, unsigned int line_num, LPTSTR szBuffer )
{

	qprintf( STDERR, INI_ERROR, line_num, fname, errmsg, szBuffer );
	if ( InitData->PauseErr ) {
		WriteTTY( PAUSE_PROMPT );
		(void)GetKeystroke( EDIT_BIOS_KEY | EDIT_NO_ECHO | EDIT_ECHO_CRLF );
	}

}

#endif  // Not 4DOS OPTION

