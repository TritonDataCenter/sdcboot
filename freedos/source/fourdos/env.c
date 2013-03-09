

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


// ENV.C - Environment routines (including aliases) for 4xxx / TCMD family
//   (c) 1988 - 2002  Rex C. Conn   All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"


#define SET_EXPRESSION 1
#define SET_MASTER 2
#define SET_PAUSE 4
#define SET_READ 8
#define SET_DEFAULT 0x10
#define SET_SYSTEM 0x20
#define SET_USER 0x40
#define SET_VOLATILE 0x80

#define SET_REGDISPLAY 0x100
#define SET_REGREAD 0x200
#define SET_REGWRITE 0x400
#define SET_REGEDIT 0x800
#define SET_REGDELETE 0x1000


static int SetFromFile( LPTSTR, TCHAR _far *, long );
static int _fastcall __Set( LPTSTR, TCHAR _far * );
int _fastcall __Unset( LPTSTR, TCHAR _far * );


#pragma alloc_text( MISC_TEXT, SetFromFile, __Set, __Unset, add_list )



static int SetFromFile( LPTSTR pszLine, TCHAR _far *pchList, long fFlags )
{
	static TCHAR szDelimiters[] = _TEXT("%[^= \t]");
	register LPTSTR pszArg;
	TCHAR szBuffer[CMDBUFSIZ];
	int i, n, nError = 0, nEditFlags = EDIT_DATA, nFH = -1;


	if (( *pszLine == _TEXT('\0') ) && ( QueryIsConsole( STDIN ) == 0 )) {
		pszLine = CONSOLE;
		nFH = STDIN;
	}

	for ( i = 0; (( nError == 0 ) && (( pszArg = ntharg( pszLine, i )) != NULL )); i++ ) {

		if ( nFH != STDIN ) {
			mkfname( pszArg, 0 );
			if (( nFH = _sopen( pszArg, _O_RDONLY | _O_BINARY, _SH_DENYWR )) < 0 )
				return ( error( _doserrno, pszArg ));
		}

		if ( setjmp( cv.env ) == -1 ) {
			if ( nFH > 0 ) {
				_close( nFH );
				nFH = -1;
			}
            return CTRLC;
		}

		for ( pszArg = szBuffer; (( nError == 0 ) && ( getline( nFH, pszArg, CMDBUFSIZ - (UINT)(( pszArg - szBuffer ) + 1 ), nEditFlags ) > 0 )); ) {

			// remove leading white space
			strip_leading( pszArg, WHITESPACE );

			// if last char is escape character, append the next line
			n = strlen( pszArg ) - 1;
			if (( pszArg[0] ) && ( pszArg[n] == gpIniptr->EscChr )) {
				pszArg += n;
				continue;
			}

			// skip blank lines & comments
			pszArg = szBuffer;
			if (( *pszArg ) && ( *pszArg != _TEXT(':') )) {

				// delete variable
				if ( fFlags & 1 ) {
					// an UNSET requires a '=' delimiter; it's optional elsewhere
					szDelimiters[4] = (( pchList == glpEnvironment ) ? _TEXT('\0') : _TEXT(' '));
					sscanf( szBuffer, szDelimiters, szBuffer );
				}

				// create/modify a variable or alias
				nError = add_list( pszArg, pchList );
			}
		}

		if ( nFH != STDIN ) {
			 _close( nFH );
			 nFH = -1;
		}
	}

	return nError;
}


// create or display aliases
int _near Alias_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Set( pszCmdLine, glpAliasList );
	if ( nReturn == USAGE_ERR )
		return  ( Usage( ALIAS_USAGE ));
	if ( nReturn == ERROR_NOT_IN_LIST )
		return ( error( ERROR_4DOS_NOT_ALIAS, pszCmdLine ));
	if ( nReturn == ERROR_LIST_EMPTY )
		return  ( error( ERROR_4DOS_NO_ALIASES, NULL ));

	return nReturn;
}


// create or display user functions
int _near Function_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Set( pszCmdLine, glpFunctionList );
	if ( nReturn == USAGE_ERR )
		return ( Usage( FUNCTION_USAGE ));
	if ( nReturn == ERROR_NOT_IN_LIST )
		return ( error( ERROR_4DOS_NOT_FUNCTION, pszCmdLine ));
	if ( nReturn == ERROR_LIST_EMPTY )
		return ( error( ERROR_4DOS_NO_FUNCTIONS, NULL ));

	return nReturn;
}


// create or display environment variables
int _near Set_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Set( pszCmdLine, glpEnvironment );
	if ( nReturn == USAGE_ERR )
		return ( Usage( SET_USAGE ));
	if ( nReturn == ERROR_NOT_IN_LIST )
		return ( error( ERROR_4DOS_NOT_IN_ENVIRONMENT, pszCmdLine ));

	// not an error to have an empty environment (but unlikely!!)

	return nReturn;
}



// create or display environment variables or aliases
static int _fastcall __Set( LPTSTR pszCmdLine, TCHAR _far * pchList )
{
	LPTSTR pszArg;
	long fSet = 0L;
	TCHAR _far *lpszVars;

	init_page_size();

	// set the pointer to the environment, alias, or function list

	// strip leading switches
	if (( pszCmdLine != NULL ) && ( *pszCmdLine == gpIniptr->SwChr )) {
		if ( GetSwitches( pszCmdLine, "AMPR", &fSet, 1 ) != 0 )
			return USAGE_ERR;
	}

	// check for master environment set
	if (( pchList == glpEnvironment ) && ( fSet & SET_MASTER ))
		pchList = glpMasterEnvironment;

	// read environment or alias file(s)
	if ( fSet & SET_READ )
		return ( SetFromFile( pszCmdLine, pchList, fSet & ( SET_DEFAULT | SET_SYSTEM | SET_USER | SET_VOLATILE )));

	if ( setjmp( cv.env ) == -1 )
		return CTRLC;

	// pause after each page
	if ( fSet & SET_PAUSE ) {
		gnPageLength = GetScrRows();
	}

	if (( pszCmdLine == NULL ) || ( *(pszCmdLine = skipspace( pszCmdLine )) == _TEXT('\0'))) {

		// print all the variables or aliases
		for ( lpszVars = pchList; ( *lpszVars != _TEXT('\0') ); lpszVars = next_env( lpszVars ) ) {

			more_page( lpszVars, 0 );
		}

		// return an error if no entries exist
		return (( lpszVars == pchList ) ? ERROR_LIST_EMPTY : 0 );
	}

	if ( fSet & SET_EXPRESSION ) {

		if (( pszArg = strchr( pszCmdLine, _TEXT('=') )) != NULL ) {

			if (( pszArg > pszCmdLine ) && ( strchr( _TEXT("+-*/%&^|><"), pszArg[-1] ) != NULL )) {

				TCHAR szBuf[256];

				// it's an assignment operator ("set /a test+=2")
				sscanf( pszCmdLine, _TEXT(" %[^ +-*/%&^|><=]"), szBuf );
				strcpy( pszArg, pszArg+1 );

				strins( pszCmdLine, _TEXT("=") );
				strins( pszCmdLine, szBuf );
				pszArg = pszCmdLine + strlen( szBuf ) + 1;

			} else
				pszArg = skipspace( pszArg+1 );

		} else
			pszArg = pszCmdLine;
		StripQuotes( pszArg );

		evaluate( pszArg );
		if ( cv.bn < 0 ) {
			qputs( pszArg );
			crlf();
		}

		// create/modify/delete a variable
		return (( pszArg == pszCmdLine ) ? 0 : add_list( pszCmdLine, pchList ));
	}

	// display the current variable or alias argument?
	// (setting environment vars requires a '='; it's optional with aliases)
	if ((( pszArg = strchr( pszCmdLine, _TEXT('=') )) == NULL ) && (( pchList == 0L ) || ( ntharg( pszCmdLine, 0x8001 ) == NULL ))) {

		if (( lpszVars = get_list( pszCmdLine, pchList )) == 0L ) {

			return ERROR_NOT_IN_LIST;
		}

		printf( FMT_FAR_STR_CRLF, lpszVars );
		return 0;
	}

	// create/modify/delete a variable or alias
	return ( add_list( pszCmdLine, pchList ));
}


#define UNSET_MASTER 1
#define UNSET_QUIET  2
#define UNSET_READ   4
#define UNSET_DEFAULT 0x10
#define UNSET_SYSTEM 0x20
#define UNSET_USER 0x40
#define UNSET_VOLATILE 0x80

// delete aliases
int _near Unalias_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Unset( pszCmdLine, glpAliasList );
	if ( nReturn == USAGE_ERR )
		return ( Usage( UNALIAS_USAGE ));
	if ( nReturn == ERROR_NOT_IN_LIST )
		nReturn = error( ERROR_4DOS_NOT_ALIAS, pszCmdLine );

	return nReturn;
}


// create or display user functions
int _near Unfunction_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Unset( pszCmdLine, glpFunctionList );
	if ( nReturn == USAGE_ERR )
		return ( Usage( UNFUNCTION_USAGE ));

	if ( nReturn == ERROR_NOT_IN_LIST )
		nReturn = error( ERROR_4DOS_NOT_FUNCTION, pszCmdLine );
	
	return nReturn;
}


// delete environment variables
int _near Unset_Cmd( LPTSTR pszCmdLine )
{
	int nReturn;

	nReturn = __Unset( pszCmdLine, glpEnvironment );
	if ( nReturn == USAGE_ERR )
		return ( Usage( UNSET_USAGE ));
	if ( nReturn == ERROR_NOT_IN_LIST )
		nReturn = error( ERROR_4DOS_NOT_IN_ENVIRONMENT, pszCmdLine );

	return nReturn;
}


// remove environment variable(s) or aliases
int _fastcall __Unset( LPTSTR pszCmdLine, char _far *pchList )
{
	register LPTSTR pszArg;
	register int i, nReturn = 0;
	long fUnset;
	TCHAR _far *lpszVars;

	// strip leading switches
	if (( GetSwitches( pszCmdLine, "MQR", &fUnset, 1 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return USAGE_ERR;

	// check for master environment set
	if (( pchList == glpEnvironment ) && ( fUnset & UNSET_MASTER ))
		pchList = glpMasterEnvironment;

	// read environment or alias file(s)
	if ( fUnset & UNSET_READ )
		return ( SetFromFile( pszCmdLine, pchList, ( fUnset & ( UNSET_DEFAULT | UNSET_SYSTEM | UNSET_USER | UNSET_VOLATILE )) | 1 ));

	for ( i = 0; (( pszArg = ntharg( pszCmdLine, i )) != NULL ); i++ ) {


		if ( _stricmp( pszArg, _TEXT("*") ) == 0 ) {

			// wildcard kill - null the environment or alias list
			pchList[0] = _TEXT('\0');
			pchList[1] = _TEXT('\0');
			break;

		} else if ( strpbrk( pszArg, _TEXT("*?[") ) != NULL ) {

			// wildcard delete!
			lpszVars = pchList;

			while ( *lpszVars ) {

				TCHAR szVarName[128];

				sscanf_far( lpszVars, _TEXT("%127[^=]"), szVarName );

				// treat it like a "unset varname*"
				if (( szVarName[0] ) && ( wild_cmp( pszArg, szVarName, FALSE, TRUE ) == 0 )) {

					// got a match -- remove it
					if ( add_list( szVarName, pchList ) != 0 ) {
						// check for "quiet" switch
						nReturn = (( fUnset & UNSET_QUIET ) ? ERROR_EXIT : ERROR_NOT_IN_LIST );
					}

				} else
					lpszVars = next_env( lpszVars );
			}

		} else {

			// kill the variable or alias
			if ( get_list( pszArg, pchList ) == 0L ) {

				// check for "quiet" switch
				if ( fUnset & UNSET_QUIET )
					nReturn = ERROR_EXIT;
				else
					nReturn = ERROR_NOT_IN_LIST;

			} else if ( add_list( pszArg, pchList ) != 0 )
				nReturn = ERROR_EXIT;
		}

		// remove argument so it won't appear in error list
		if ( nReturn == 0 ) {
			strcpy( gpNthptr, skipspace( gpNthptr + strlen( pszArg )));
			i--;
		}
	}

	return nReturn;
}


#define ESET_ALIAS 1
#define ESET_FUNCTION 2
#define ESET_MASTER 4
#define ESET_DEFAULT 0x10
#define ESET_SYSTEM 0x20
#define ESET_USER 0x40
#define ESET_VOLATILE 0x80

// edit an existing environment variable or alias
int _near Eset_Cmd( LPTSTR pszCmdLine )
{
	LPTSTR pszArg;
	register int nLength;
	int i, nReturn = 0;
	long fEset;
	TCHAR _far *lpszVarName, _far *lpszVars, _far *pchList;
	TCHAR szBuffer[CMDBUFSIZ];

	// check for alias or master environment switches
	if (( GetSwitches( pszCmdLine, _TEXT("AFM"), &fEset, 1 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( ESET_USAGE ));

	for ( i = 0; (( pszArg = ntharg( pszCmdLine, i )) != NULL ); i++ ) {

		pchList = (( fEset & ESET_MASTER ) ? glpMasterEnvironment : glpEnvironment );

		// function edit request?
		if ( fEset & ESET_FUNCTION ) {

			pchList = glpFunctionList;
			if (( lpszVars = get_list( pszArg, pchList )) == 0L ) {
				nReturn = error( ERROR_4DOS_NOT_FUNCTION, pszArg );
				continue;
			}

		} else {

			// try environment variables first, then aliases
			if (( fEset & ESET_ALIAS ) || (( lpszVars = get_list( pszArg, pchList )) == 0L )) {

				// check for alias editing
				if (( lpszVars = get_list( pszArg, glpAliasList )) == 0L ) {
					nReturn = error((( fEset & ESET_ALIAS ) ? ERROR_4DOS_NOT_ALIAS : ERROR_4DOS_NOT_IN_ENVIRONMENT ), pszArg );
					continue;
				}

				pchList = glpAliasList;
			}

		}

		// get the start of the alias / function / variable name
		for ( lpszVarName = lpszVars; (( lpszVarName > pchList ) && ( lpszVarName[-1] != _TEXT('\0') )); lpszVarName-- )
			;

		// length of alias/function/variable name
		nLength = (int)( lpszVars - lpszVarName );

		sprintf( szBuffer, _TEXT("%.*Fs%.*Fs"), nLength, lpszVarName, (( CMDBUFSIZ - 1 ) - nLength), lpszVars );

		// echo & edit the argument
		printf( FMT_FAR_PREC_STR, nLength, lpszVarName );
		egets( szBuffer + nLength, (( CMDBUFSIZ - 1 ) - nLength ), EDIT_ECHO | EDIT_SCROLL_CONSOLE );

		if ( add_list( szBuffer, pchList ) != 0 )
			nReturn = ERROR_EXIT;
	}

	return nReturn;
}


// get environment variable
TCHAR _far * _fastcall get_variable( LPTSTR pszVariable )
{
	return ( get_list( pszVariable, glpEnvironment ));
}


// get alias (a near pointer in the current data segment)
TCHAR _far * _fastcall get_alias( LPTSTR pszAlias )
{
	return ( get_list( pszAlias, glpAliasList ));
}


// retrieve an environment or alias list entry
TCHAR _far * PASCAL get_list( LPTSTR pszName, TCHAR _far *pchList )
{
	LPTSTR pszArg;
	register int fWild;
	TCHAR _far *pchEnv;
	TCHAR _far *pchStart;

	if ( pchList == 0L )
		pchList = glpEnvironment;

	for ( pchEnv = pchList; *pchEnv; ) {

		// aliases allow "wher*eis" ; so collapse the '*', and 
		//   only match to the length of the varname
		pszArg = pszName;
		fWild = 0;
		pchStart = pchEnv;

		do {

			if (( pchList == glpAliasList ) && ( *pchEnv == _TEXT('*') )) {
				pchEnv++;
				fWild++;

				// allow entry of "ab*cd=def"
				if ( *pszArg == _TEXT('*') )
					pszArg++;
			}

			if ((( *pszArg == _TEXT('\0') ) || ( *pszArg == _TEXT('=') )) && ((( *pchEnv == _TEXT('=') ) && ( pchEnv != pchStart )) || ( fWild ))) {

				for ( ; ( *pchEnv ); pchEnv++ ) {
					if ( *pchEnv == _TEXT('=') )
						return ++pchEnv;
				}

				return 0L;
			}

		} while ( _ctoupper( *pchEnv++ ) == _ctoupper( *pszArg++ ));

		while ( *pchEnv++ != _TEXT('\0') )
			;
	}

	return 0L;
}


// add or delete environment var
int _fastcall add_variable( LPTSTR pszVariable )
{
	return ( add_list( pszVariable, glpEnvironment ));
}


// add a variable to the environment or alias to the alias list
int PASCAL add_list( LPTSTR pszVariable, TCHAR _far *pchList )
{
	LPTSTR pszLine;
	TCHAR _far *pszVarName, _far *pszArgument, _far *pszEndOfList, _far *pszLastVariable;
	unsigned int uLength;
	int nError = 0;

	if ( pchList == 0L )
		pchList = glpEnvironment;

	pszLine = pszVariable;
	if ( *pszLine == _TEXT('=') ) {
		return ( error( ERROR_4DOS_BAD_SYNTAX, pszVariable ));
	}

	for ( ; (( *pszLine ) && ( *pszLine != _TEXT('=') )); pszLine++ ) {

		if ( pchList != glpEnvironment ) {

			if ( iswhite( *pszLine )) {
				strcpy( pszLine, skipspace( pszLine ) );
				break;
			}
		}
		else	    // ensure environment entry is in upper case
			*pszLine = (unsigned char)_ctoupper( *pszLine );
	}

	// stupid kludge to strip quotes from PATH for compatibility with
	//   COMMAND.COM
	if (( fWin95 ) && ( pchList == glpEnvironment )) {

		char szVarName[8];

		if (( uLength = ( pszLine - pszVariable )) > 7 )
			uLength = 7;

		sprintf( szVarName, "%.*s", uLength, pszVariable );
		if ( stricmp( szVarName, PATH_VAR ) == 0 )
			StripQuotes( pszLine );
	}

	if ( *pszLine == _TEXT('=') ) {

		// point to the first char of the argument
		pszLine++;

		// collapse whitespace around '=' in aliases, but not in env
		//   variables, for COMMAND.COM compatibility (set abc def= ghi)
		if ( pchList != glpEnvironment )
			strcpy( pszLine, skipspace( pszLine ));

	} else if ( *pszLine ) {
		// add the missing '='
		strins( pszLine, _TEXT("=") );
		pszLine++;
	}

	// removing single back quotes at the beginning and end of an alias
	//   argument (they're illegal there; the user is probably making a
	//   mistake with ALIAS /R)
	if (( *pszLine == SINGLE_QUOTE ) && ( pchList != glpEnvironment )) {

		// remove leading single quote
		strcpy( pszLine, pszLine + 1 );

		// remove trailing single quote
		if ((( uLength = strlen( pszLine )) != 0 ) && ( pszLine[--uLength] == SINGLE_QUOTE ))
			pszLine[uLength] = _TEXT('\0');
	}

	// block other processes & threads while updating list
	if ( pchList != glpEnvironment ) {
		// disable task switches under Windows and DESQview
		CriticalSection( 1 );
	}

	// get pointers to beginning & end of list space
	pszEndOfList = pchList + ((( pchList == glpAliasList ) ? gpIniptr->AliasSize : gpIniptr->EnvSize ) - 4 );

	// get pointer to end of environment or alias variables
	pszLastVariable = end_of_env( pchList );

	uLength = strlen( pszVariable ) + 1;

	// check for modification or deletion of existing entry
	if (( pszArgument = get_list( pszVariable, pchList )) != 0L ) {

		// get the start of the alias or variable name
		for ( pszVarName = pszArgument; (( pszVarName > pchList ) && ( pszVarName[-1] != _TEXT('\0') )); pszVarName-- )
			;

		if ( *pszLine == _TEXT('\0') ) {
			// delete an alias or environment variable
			_fmemmove( pszVarName, next_env( pszVarName ), (unsigned int)( (ULONG_PTR)pszLastVariable - (ULONG_PTR)next_env(pszVarName)) + sizeof(TCHAR));
		} else {
			// get the relative length (vs. the old variable)
			uLength = strlen( pszLine ) - _fstrlen( pszArgument );
		}
	}

	if ( *pszLine != _TEXT('\0') ) {

		// check for out of space
		if (( pszLastVariable + ( uLength * sizeof(TCHAR) )) >= pszEndOfList ) {

			if ( pchList == glpAliasList )
				nError = error( ERROR_4DOS_OUT_OF_ALIAS, NULL );
			else if ( pchList == glpEnvironment )
				nError = error( ERROR_4DOS_OUT_OF_ENVIRONMENT, NULL);
			else
				nError = error( ERROR_NOT_ENOUGH_MEMORY, NULL );
			goto add_bye;
		}

		if ( pszArgument != 0L ) {

			// modify an existing value
			//   adjust the space & insert new value
			pszVarName = next_env( pszVarName );
			_fmemmove(( pszVarName + uLength ), pszVarName, (unsigned int)(( (ULONG_PTR)pszLastVariable - (ULONG_PTR)pszVarName ) + sizeof(TCHAR) ));
			_fstrcpy( pszArgument, pszLine );

		} else {
			// put it at the end & add an extra null
			_fstrcpy( pszLastVariable, pszVariable );
			pszLastVariable[uLength] = _TEXT('\0');
		}
	}

add_bye:
	if ( pchList != glpEnvironment ) {
		// re-enable task switches under Windows and DESQview
		CriticalSection( 0 );
	}

	return nError;
}


// return a pointer to the next entry in list
TCHAR _far * PASCAL next_env( TCHAR _far *pchEnv )
{
	register int i;

	if (( i = _fstrlen( pchEnv )) > 0 )
		i++;

	return ( pchEnv + i );
}


// get pointer to end of list
TCHAR _far * PASCAL end_of_env( TCHAR _far *pchList )
{
	for ( ; ( *pchList != _TEXT('\0') ); pchList = next_env( pchList ))
		;

	return pchList;
}
