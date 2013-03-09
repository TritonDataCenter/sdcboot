

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


// DOSINIT.C - DOS initialization for 4DOS
//   (c) 1993 - 2004  Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"

int fWin98 = 0;
int fWinME = 0;

void _near InitOS( int argc, char **argv )
{
	register char *arg;
	int fMSDOS7 = (int)(gpIniptr->MSDOS7);
	char szMSDOS[16];
	char szPassedLine[CMDBUFSIZ+1];

	Win95DisableClose();

	// reduce malloc() block size
	_amblksiz = 64;

	// initialize the critical variables
	memset( &cv, '\0', sizeof(cv) );
	cv.bn = -1;
	cv.fLfnFor = 1;

	_setmode( STDIN, _O_BINARY );
	_setmode( STDOUT, _O_BINARY );
	_setmode( STDERR, _O_BINARY );

	// flush the disk buffers (required for MSCDEX bug)
	if ( gpIniptr->DiskReset )
		reset_disks();

	// initialize the server
	ServInit( &(gszCmdline[CMDBUFSIZ]), gpIniptr );

	// get default switch character
	gpIniptr->SwChr = QuerySwitchChar();

	GetDOSVersion();
	gnOsVersion = ( _osmajor * 100 ) + _osminor;

	glpEnvironment = MAKEP( gpIniptr->EnvSeg, 0 );

	// point to the master environment (created in ServInit)
	if (( glpMasterEnvironment = MAKEP( gpIniptr->MastSeg, 0 )) == 0L )
		glpMasterEnvironment = glpEnvironment;

	// Set Win95 flags
	fWin95 = ( gpIniptr->WinMode >= 40 );

	// Set Win98 flag
	if ( fWin95 ) {
_asm {
		mov	bx, 0
		mov	ax, 0160Ah
		int	2Fh
		mov	fWin98, bx
		mov	fWinME, bx
}
		fWinME = ( fWinME >= 0x45A );
		fWin98 = (( fWinME == 0 ) && ( fWin98 >= 0x40A ));
	}

	// Reset internal LFN / SFN flags
	SetWin95Flags();

	// disable START if not OS/2 2+
	if ( _osmajor < 20 )
		commands[ findcmd( "START", 1 )].fParse |= CMD_DISABLED;

	// (Undocumented: Win95's COMMAND.COM looks for WINBOOT.INI first,
	//   then for MSDOS.SYS)
	if ( fMSDOS7 ) {

		sprintf( szMSDOS, "%c:\\WINBOOT.INI", gpIniptr->BootDrive );
		DosError( 2 );
		if ( is_file( szMSDOS ) == 0 )
			sprintf( szMSDOS, "%c:\\MSDOS.SYS", gpIniptr->BootDrive );
		DosError( 1 );

	} else {
		// disable LOCK & UNLOCK if not MS-DOS 7
		commands[ findcmd( "LOCK", 1 ) ].fParse |= CMD_DISABLED;
		commands[ findcmd( "UNLOCK", 1 ) ].fParse |= CMD_DISABLED;
	}

	// set the current drive
	if (( gnCurrentDisk = _getdrive()) < 0 )
		gnCurrentDisk = 0;

	SetCurSize( );		// set the default cursor shape

	// get the international format chars (for PROMPT)
	QueryCountryInfo();

	// The COMSPEC directory should be passed to us by 4DLINIT in
	// gszFindDesc -- convert it to the full COMSPEC
	if ( gszFindDesc[0] )
		mkdirname( gszFindDesc, DOS_NAME );
	else
		sprintf( gszFindDesc, FMT_PATH, gpIniptr->BootDrive, DOS_NAME );

	// save _pgmptr
	_pgmptr = (char _far *)strdup( gszFindDesc );

	if ( fWin95 || ( gpIniptr->ShellLevel == 0 )) {
		// set COMSPEC
		sprintf( gszFindDesc, COMSPEC_DOS, COMSPEC, _pgmptr );
		add_variable( gszFindDesc );
	}

	glpAliasList = (PCH)(gpIniptr->AliasLoc);
	glpFunctionList = (PCH)(gpIniptr->FunctionLoc);
	glpHistoryList = (PCH)(gpIniptr->HistLoc);
	glpDirHistory = (PCH)(gpIniptr->DirHistLoc);

	// The command line is passed to us by 4DLINIT in gszCmdline, but we have
	// to copy it to local storage as others (e.g. find_4files) use gszCmdline
	pszCmdLineOpts = strcpy( szPassedLine, gszCmdline );

	// check command line for switches, INI file, etc.
	for ( argc = 0; (( arg = ntharg( pszCmdLineOpts, argc )) != NULL ); argc++ ) {

next_start_arg:
		if (( *arg == gpIniptr->SwChr ) || ( *arg == '-' )) {

			switch ( _ctoupper( arg[1] )) {
			case 'C':	// transient load

				gnTransient = 1;

				// kludge for people who do "/Ccommand"
				if ( arg[2] ) {
					arg += 2;
					gpNthptr += 2;
					goto next_start_arg;
				}

				// check for a "4DOS.COM /C 4DOS.COM" & turn it
				//   into a "4DOS.COM"

				if (( arg = ntharg( pszCmdLineOpts, argc+1 )) != NULL )
					arg = fname_part( arg );

				if (( arg != NULL ) && (cv.bn < 0 ) && ( stricmp( arg, DOS_NAME ) == 0 )) {
					gnTransient = 0;
					argc++;
				}

				break;

			case 'P':	// permanent load in 4DOS

				gpIniptr->ShellLevel = 0;

				// kludge for people who do "/Pcommand"
				if (( arg[2] ) && ( isdelim( arg[2] ) == 0 )) {
					arg += 2;
					gpNthptr += 2;
					goto next_start_arg;
				}

				break;

			default:
				// kludge for "4DOS -c ..."
				if ( *arg == '-' )
					goto args_done;
				error( ERROR_INVALID_PARAMETER, arg );
			}
		} else
			break;
	}
args_done:

	pszCmdLineOpts = gpNthptr;

	// set the default cursor shape (must be after INI file is processed)
	gnEditMode = gpIniptr->EditMode & 1;
	SetCurSize( );

	// enable ^C and ^BREAK handling
	ServCtrl( SERV_SIGNAL, (unsigned int)BreakHandler );
	ServCtrl( SERV_SIGNAL, SERV_SIG_ENABLE );

	// if non-transient, display signon message & test user brand
	if ( gnTransient == 0 )
		DisplayCopyright();

	// set LogFileName
	arg = (char *)(gpIniptr->StrData + gpIniptr->LogName);
	if (*arg == '\0')
		sprintf( arg, FMT_PATH, gpIniptr->BootDrive, LOG_FILENAME );
 
	// set HistoryFileName
	arg = (char *)(gpIniptr->StrData + gpIniptr->HistLogName);
	if (*arg == '\0')
		sprintf( arg, FMT_PATH, gpIniptr->BootDrive, HLOG_FILENAME );

	// set DescriptionName
	if ( gpIniptr->DescriptName != INI_EMPTYSTR )
		sprintf( DESCRIPTION_FILE, "%.12s", gpIniptr->StrData + gpIniptr->DescriptName );

	// kludge for Microsoft using 5D09 in COMMAND.COM to signal that
	//   the DOS box has been completely loaded
	if ( fMSDOS7 ) {
_asm {
		mov	ax, 05D09h
		int	21h
}
	}

	// turn off single-step unless we're in a nested batch file
	if ( cv.bn < 0 )
		gpIniptr->SingleStep = 0;

	// execute 4START.BTM / 4START.BAT / 4START.CMD
	find_4files( AUTOSTART );

	// execute AUTOEXEC.BAT if we're in the root shell
	if (( gpIniptr->ShellLevel == 0 ) && ( gpIniptr->AEPath != INI_EMPTYSTR )) {

		strcpy( AUTOEXEC, ( gpIniptr->StrData + gpIniptr->AEPath ));

		if ( is_file( AUTOEXEC )) {

			// add any parameters to the end of the filename
			gszCmdline[0] = '\0';
			if ( gpIniptr->AEParms != INI_EMPTYSTR )
				strcpy( gszCmdline, (gpIniptr->StrData + gpIniptr->AEParms));

			// don't call "command()" because people want to call
			//   things like AUTOEXEC.FOO!
			gpBatchName = AUTOEXEC;
			ParseLine( AUTOEXEC, gszCmdline, NULL, (CMD_STRIP_QUOTES | CMD_ADD_NULLS), 0 );
			crlf();
		}
	}

	// execute remainder of command line
	if (( pszCmdLineOpts != NULL ) && ( *pszCmdLineOpts )) {

		strcpy( gszCmdline, pszCmdLineOpts );

		// stupid kludge for Win95 bug
		if ( fMSDOS7 && ( stricmp( gszCmdline, "autoexec" ) == 0 ) && ( gpIniptr->AEParms != INI_EMPTYSTR )) {
			strcat( gszCmdline, " " );
			strcat( gszCmdline, (gpIniptr->StrData + gpIniptr->AEParms));
		}

		command( gszCmdline, ( gnTransient == 0 ));
	}

	// check for BootGUI=1 in MSDOS.SYS (or WINBOOT.INI) in Win95
	if (( gpIniptr->ShellLevel == 0 ) && fMSDOS7 && ( gpIniptr->NoWin95GUI == 0 )) {

	    int fd;

	    DosError( 2 );
	    fd = ( is_file( szMSDOS ));
	    DosError( 1 );

	    if ( fd ) {

		if (( fd = _sopen( szMSDOS, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) > 0 ) {

			while ( getline( fd, gszCmdline, CMDBUFSIZ-1, EDIT_COMMAND ) > 0 ) {

				arg = skipspace( gszCmdline );
				if ( strnicmp( arg, "BootGUI", 7 ) == 0 ) {

					_close( fd );
					for ( arg += 8; (( *arg == '=' ) || ( iswhite( *arg ))); arg++ )
						;
					if ( *arg == '1' )
						command( "win", 0 );
					return;
				}
			}

			_close( fd );
		}
	    }
	}
}


// display copyright / beta test message & test brand
void DisplayCopyright( void )
{
	char MsgBuf[256];
	char *pszOS;

	if ( gnOSFlags & DOS_IS_DR ) {
		pszOS = (( gchMajor >= 7 ) ? NOVVER : DRVER );
	} else if ( gnOSFlags & DOS_IS_OS2 )
		pszOS = OS2VER;
	else if ( fWinME )
		pszOS = MSMEVER;
	else if ( fWin98 )
		pszOS = MS98VER;
	else if ( fWin95 )
		pszOS = MS95VER;
	else
		pszOS = MSVER;

	printf( DOS_VERSION, PROGRAM, pszOS, gchMajor, gaCountryInfo.szDecimal[0], gchMinor );

	printf( DecodeMsg( SEC_COPYRIGHT, MsgBuf ));		// copyright
	crlf();
}

