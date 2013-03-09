

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


// DOSCMDS.C - DOS-specific commands for 4DOS
//   Copyright 1993-2002 Rex C. Conn

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"

static int _near SetKeystack( int, char * );
static int _near _lock( int );
static int _near _unlock( int );

static char KstackScanCodes[128] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	14,		// backspace
	15,		// tab
	0,		// 10
	0,
	0,
	28,		// CR
	0,
	0,
	0,
	0,
	0,
	0,
	0,		// 20
	0,
	0,
	0,
	0,
	0,
	0,
	1,		// Esc
	0,
	0,
	0,		// 30
	0,
	57,		// space
	2,		// !
	40,		// "
	4,		// #
	5,		// $
	6,		// %
	38,		// &
	40,		// '
	10,		// [40] (
	11,		// )
	9,		// *
	13,		// +
	51,		// ,
	12,		// -
	52,		// .
	53,		// /
	11,		// 0
	2,		// 1
	3,		// [50] 2
	4,		// 3
	5,		// 4
	6,		// 5
	7,		// 6
	8,		// 7
	9,		// 8
	10,		// 9
	39,		// :
	39,		// ;
	51,		// [60] <
	13,		// =
	52,		// >
	53,		// ?
	3,		// @
	30,		// A
	48,		// B
	46,		// C
	32,		// D
	18,		// E
	33,		// [70] F
	34,		// G
	35,		// H
	23,		// I
	36,		// J
	37,		// K
	38,		// L
	50,		// M
	49,		// N
	24,		// O
	25,		// [80] P
	16,		// Q
	19,		// R
	31,		// S
	20,		// T
	22,		// U
	47,		// V
	17,		// W
	45,		// X
	21,		// Y
	44,		// [90] Z
	26,		// [
	43,		// backslash
	27,		// ]
	7,		// ^
	12,		// _
	41,		// `
	30,		// a
	48,		// b
	46,		// c
	32,		// [100] d
	18,		// e
	33,		// f
	34,		// g
	35,		// h
	23,		// i
	36,		// j
	37,		// k
	38,		// l
	50,		// m
	49,		// [110] n
	24,		// o
	25,		// p
	16,		// q
	19,		// r
	31,		// s
	20,		// t
	22,		// u
	47,		// v
	17,		// w
	45,		// [120] x
	21,		// y
	44,		// z
	26,		// {
	43,		// |
	27,		// }
	41,		// ~
	0		// 127
};

char szAppendBlock[130] = "";


// enable/disable ^C trapping
int _near Break_Cmd( register char *pszCmdLine )
{
	int argc;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) {

		// inquiring about break status

_asm {
		mov	ax,03300h
		int	21h
		xor	dh,dh
		mov	argc,dx
}
		printf( BREAK_IS, (( argc ) ? ON : OFF ));

	} else {		// setting new break status

		if ( *pszCmdLine == '=' )
			pszCmdLine++;

		if (( argc = OffOn( pszCmdLine )) == -1 )
			return ( Usage( BREAK_USAGE ));
_asm {
		mov	ax,03301h
		mov	dx,argc
		int	21h
}
	}

	return 0;
}


// clear the screen (using ANSI or BIOS calls), with optional color set
int _near Cls_Cmd( char *pszCmdLine )
{
	register int nAttrib = -1;

	// check if colors were requested
	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if (( nAttrib = GetColors( pszCmdLine, 1 )) == -1 )
			return ( Usage( COLOR_USAGE ));

	} else if ( gpIniptr->StdColor != 0 ) {

		// check for default border (high byte)
		if (( nAttrib = ( gpIniptr->StdColor >> 8 )) != 0xFF )
			SetBorderColor( nAttrib );

		nAttrib = ( gpIniptr->StdColor & 0xFF );
	}

	// check for presence of ANSI
	if ( _isatty(STDOUT) && QueryIsANSI() ) {

		// output ANSI color sequence
		if ( nAttrib != -1 )
			set_colors( nAttrib );

		// output ANSI clear screen sequence
   		printf( "\033[2J" );

	} else {

		// default to white on black
		if ( nAttrib == -1 )
			nAttrib = 7;

		// use the BIOS to clear the screen
		Scroll( 0, 0, GetScrRows(), GetScrCols()-1, 0, nAttrib );

		// move cursor to upper left corner
		SetCurPos( 0, 0 );

	}

	return 0;
}


// Change stdin, stdout, stderr to a new device
int _near Ctty_Cmd( register char *pszCmdLine )
{
	register int fd;

	// only allowed one argument, and it must be a device
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ) || ( QueryIsDevice( pszCmdLine ) == 0 ))
		return ( Usage( CTTY_USAGE ));
	
	if (( fd = _sopen( pszCmdLine, (_O_RDWR | _O_BINARY), _SH_DENYNO )) < 0 )
		return ( error( _doserrno, pszCmdLine ));

	// if not console, set gfCTTY so we don't try to use egets()
	gfCTTY = stricmp( fname_part( pszCmdLine ), CONSOLE );

	// close original & duplicate file handle into stdin, stdout, & stderr
	dup_handle( fd, STDIN );
	_dup2( STDIN, STDOUT );
	_dup2( STDOUT, STDERR );

_asm {
	mov	ax,04401h
	mov	bx,0
	mov	dx,10000011b
	int	21h
	mov	ax,04401h
	mov	bx,1
	mov	dx,10000011b
	int	21h
	mov	ax,04401h
	mov	bx,2
	mov	dx,10000011b
	int	21h
}

	// Switch INT 24 I/O handle
	ServCtrl( SERV_CTTY, 0 );

	return 0;
}


// exit the current shell
int _near Exit_Cmd( char *pszCmdLine )
{
	static int fExit = 0;
	register int nRet;

	// can't exit root shell in DOS (except OS/2 2+ MVDM & Win95)
	if (( gpIniptr->ShellLevel != 0 ) || ( _osmajor >= 20 ) || ( fWin95 )) {

		nRet = ((( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) ? gnErrorLevel : atoi( pszCmdLine ));

		// clean up any open batch files
		while ( ExitBatchFile() == 0 )
			;

		// watch out for EXITs inside 4EXIT!
		if ( fExit++ == 0 )
			find_4files( AUTOEXIT );

		// ensure shareware data is written
		if ( gfShareware && gfSharewareDataRead ) {
			AccessSharewareData( &SharewareData, 1 );
			AccessSharewareDup( &SharewareData, 1 );
		}

		ServCtrl( SERV_QUIT, nRet );
	}

	return ERROR_EXIT;
}


// toggle the keyboard (caps lock, numlock, scroll lock)
int _near Keybd_Cmd( char *pszCmdLine )
{
	register char *arg;
	register unsigned int uToggle;
	int i, nKeybState, fFlag;
	long lKeybdFlags;

_asm {
	xor	ax, ax
	mov	es, ax
	mov	ax, es:[417h]
	mov	nKeybState, ax
}

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) {

		// display the current state
		printf( KBD_CAPS_LOCK, (( nKeybState & 0x40 ) ? ON : OFF ));
		printf( KBD_NUM_LOCK, (( nKeybState & 0x20 ) ? ON : OFF ));
		printf( KBD_SCROLL_LOCK, (( nKeybState & 0x10 ) ? ON : OFF ));

	} else {

		for ( i = 0; (( arg = ntharg( pszCmdLine, i )) != NULL ); i++ ) {

			lKeybdFlags = switch_arg( arg, "CNS" );
			if ( lKeybdFlags <= 0 )
				return ( Usage( KEYBD_USAGE ));

			fFlag = (( arg[2] == '0' ) ? 0 : 1 );
			if ( lKeybdFlags & 1 )		// Caps Lock
				uToggle = 0x40;
			else if ( lKeybdFlags & 2 )	// Numlock
				uToggle = 0x20;
			else				// Scroll lock
				uToggle = 0x10;

			if ( fFlag )
				nKeybState |= uToggle;
			else
				nKeybState &= ~uToggle;
		}

		// reset the keyboard

_asm {
		xor	ax, ax
		mov	es, ax
		mov	ax, nKeybState
		mov	es:[417h], ax
}
	}

	return 0;
}


// transfer the string to KSTACK.COM through an INT 2Fh

static int _near SetKeystack( int nLength, char *buf )
{
_asm {
	mov	ax, 0D44Fh		; call KSTACK & load the KSTACK buffer
	mov	bx, 1
	mov	cx, nLength
	mov	dx, [buf]
	int	2Fh
}
}


// pump characters from a keyboard buffer into a program
int _near Keystack_Cmd( char *pszCmdLine )
{
	register char *arg;
	register unsigned int *kptr;
	int fd;
	unsigned int argc, uCount, uVal;

	// check for /R(ead file) switch
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ) || (( arg = first_arg( pszCmdLine )) == NULL )) {
Keystack_Usage:
		return ( Usage( KEYSTACK_USAGE ));
	}

	if (( *arg == gpIniptr->SwChr ) && ( _ctoupper( arg[1] ) == 'R' )) {

		if (( arg = first_arg( pszCmdLine+2 )) == NULL )
			goto Keystack_Usage;

		// strip quotes and resolve relative paths
		mkfname( arg, MKFNAME_NOERROR );
		if (( fd = _sopen( arg, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 )
			return ( error( _doserrno, arg ));

		// read a line from the file
		getline( fd, pszCmdLine, MAXLINESIZ-1, EDIT_DATA );
		_close( fd );
	}

	// save command line so we can overwrite it with the converted string
	arg = _alloca( strlen( pszCmdLine ) + 1 );
	strcpy( arg, pszCmdLine );

	for ( kptr = (int *)gszCmdline; ( *arg != '\0' ); ) {

		argc = 1;

		if ( *arg == '"' ) {

			// string value ("abc")
			for ( ; (( *( ++arg ) != '"' ) && ( *arg )); kptr++ ) {
				if ( *arg < 128 )
					*kptr = *arg + ( KstackScanCodes[*arg] << 8 );
				else
					*kptr = *arg;
			}

			// skip the trailing double quote
			if ( *arg != '"' )
				argc = 0;

		} else if ( *arg == '!' ) {

			// flush keyboard buffer
			while ( _kbhit() )
				GetKeystroke( EDIT_NO_ECHO );

		} else if ( *arg == gpIniptr->SwChr ) {

			if ( _ctoupper( arg[1] ) == 'W' ) {

				// time delay in ticks (/Wnn)
				arg += 2;
				*kptr++ = 0xFFFF;
				sscanf( arg, FMT_UINT_LEN, kptr++, &argc );
			} else
				argc = 0;

		} else if ( *arg == '[' ) {

			// get repeat count
			uCount = 0;
			sscanf( arg, "[%d]%n", &uCount, &argc );
			if (( argc == 0 ) || ( uCount > 255 ) || ( kptr <= (unsigned int *)gszCmdline ))
				return ( error( ERROR_4DOS_BAD_SYNTAX, arg ));
			uVal = kptr[-1];
			// we already have the first one, so decrement before
			//   duplicating it
			while ( --uCount > 0 )
				*kptr++ = uVal;

		} else if ( isdelim( *arg ) == 0 ) {

			// it's either a numeric value ("keystack 13"); an
			//   extended keycode ("keystack @59"); or a
			//   key name ("keystack Alt-F1")

			sscanf( arg, "%*[^ ,\t!/\"]%n", &argc );

			if ( isdigit( *arg )) {

				sscanf( arg, FMT_UINT, &uVal );
				if ( uVal < 128 )
					*kptr++ = uVal + ( KstackScanCodes[uVal] << 8 );
				else
					*kptr++ = uVal;

			} else if (( *kptr = keyparse( arg, argc )) != (unsigned int)-1 ) {

				// convert extended scan code to KSTACK format
				if ( *kptr > 0xFF )
					*kptr <<= 8;
				else
					*kptr += ( KstackScanCodes[*kptr] << 8 );
				kptr++;
			}

		}

		if ( argc == 0 )
			return ( error( ERROR_4DOS_BAD_SYNTAX, arg ));

		arg += argc;
	}

	// send the buffer to KSTACK
	if ( SetKeystack( (int)( (char *)kptr - gszCmdline ) / 2, gszCmdline ) != 0 )
		return ( error( ERROR_4DOS_NO_KEYSTACK, NULL ));

	return 0;
}


// load programs in high memory
//   (LOADHIGH only supported in MS-DOS 5.0+, DR-DOS 5.0+ and OS/2 2.0 MVDM's)
int _near Loadhigh_Cmd( char *pszCmdLine )
{
	UMBREGINFO _far *pURInfo;
	int URCount;

	register char *arg;
	register int i;
	int rval = 0, shrink = 0, regreq = -1, link = 1, regnum;
	unsigned long regmin;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ))
		return ( Usage( LOADHIGH_USAGE ));

	URCount = ServCtrl(SERV_UREG, (int)(&pURInfo));
	
	// default to all regions locked out
	for ( i = 0; ( i < URCount ); i++ )
		pURInfo[i].MinFree = 0xFFFF;

	// check for command line arguments
	for ( arg = pszCmdLine; ( arg != NULL ); ) {

		// if not a switch it must be the command
		arg = skipspace( arg );
		if (( *arg != gpIniptr->SwChr ) || ( *arg == '\0' ))
			break;

		// process switches
		if ( URCount <= 0 ) {

			while ( *( ++arg ) && ( iswhite( *arg ) == 0 ))
				;
			if ( rval == 0 ) {
				qprintf( STDERR, WARNING_NO_REGIONS );
				rval++;
			}

		} else {

			switch ( _ctoupper( arg[1] )) {
			case 'S':
				// shrink region(s) to minimum size required
				shrink = 1;
				arg += 2;
				break;

			case 'L':
				// skip past delimiters, check for empty spec
				arg += 2;
				arg += strspn( arg, ":= \t" );
				if ( *arg == '\0' )
					return ( error( ERROR_4DOS_REGION_SYNTAX, NULL ));

				// loop through regions and minimum sizes
				while (( *arg ) && ( iswhite( *arg ) == 0 )) {

					regmin = 0;
					regnum = -1;

					if (( strchr( ";0123456789", *arg ) == NULL) || (sscanf(arg,"%*[;]%u%n,%lu%n;%n",&regnum,&rval,&regmin,&rval,&rval) < 1))
						return ( error( ERROR_4DOS_REGION_SYNTAX, first_arg( arg ) ));

					if (( regnum < 0 ) || ( regnum > URCount ))
						qprintf( STDERR, WARNING_INVALID_REGION, first_arg( arg ) );

					else if ( regnum == 0 ) {

						// region 0, disable UMB links, ignore any minimum size
						link = 0;

					} else {

						// region 1 or above, set minimum size
						pURInfo[regnum-1].MinFree = (unsigned int)((regmin + 0xF) >> 4);
						regreq = 0;
					}

					arg += rval;
				}
				break;

			default:
				return (error(ERROR_INVALID_PARAMETER,arg));
			}
		}
	}

	// link in UMBs; check for no UMBs, DOS=,umb not loaded, or MCB's trashed
	if ( SetUReg( regreq, shrink, link, URCount, pURInfo ) == 0 )
		error( ERROR_4DOS_NO_UMBS, arg );

	// load the (TSR?) in high (or perhaps low?) DOS memory
	rval = command( arg, 0 );

	// clean up and unlink UMBs
	FreeUReg( regreq, URCount, pURInfo );

	return rval;
}


static int _near _lock( int nDisk )
{
_asm {
	mov	ax, 0440Dh
	mov	bx, nDisk
	mov	cx, 084Ah
	mov	dx, 0
	int	21h
	jc	LockError
	xor	ax,ax
LockError:
	mov	nDisk, ax
}
	return nDisk;
}


int _near Lock_Cmd( char *pszCmdLine )
{
	register int nDisk, argc;
	int rval = 0;
	char *arg, szDisk[4];

	qputs( LOCK_WARNING );

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) {

		// if no arguments, lock everything
		for ( nDisk = 1; ( nDisk <= 26 ); nDisk++ ) {
			sprintf( szDisk, "%c:", nDisk+64 );
			if (( QueryDriveExists( nDisk ) == 0 ) || ( QueryDriveRemote( nDisk ) == 1 ) || ( QueryIsCDROM( szDisk ) == 1 ))
				continue;
			if (( argc = _lock( nDisk )) != 0 )
				rval = error( argc, szDisk );
		}

	} else {

		// lock only the specified drives
		for ( argc = 0; (( arg = ntharg( pszCmdLine, argc )) != NULL ); argc++ ) {

			if (( nDisk = _lock( gcdisk( arg ))) != 0 )
				rval = error( nDisk, arg );
		}
	}

	return rval;
}


// display RAM statistics
int _near Memory_Cmd( char *pszCmdLine )
{
	unsigned int nMem;
	unsigned long ram;

	ram = (unsigned long)QuerySystemRAM();
	printf( TOTAL_DOS_RAM, ( ram << 10 ));

	// get and display free memory
	printf( LBYTES_FREE, ((long)(unsigned int)ServCtrl(SERV_AVAIL,0 ) << 4 ));

	// display expanded memory (if any)
	if (( ram = (long)get_expanded( &nMem )) > 0L ) {
		printf( TOTAL_EMS, ( ram << 14 ));
		printf( LBYTES_FREE, ((long)nMem << 14 ));
	}

	// display extended memory (if any) for 286's and 386's
	if ((ram = get_extended()) > 0 )
		printf( TOTAL_EXTENDED,(ram << 10 ));

	// display XMS memory (if any) for 286's and 386's
	nMem = 0;
	if ((( ram = get_xms( &nMem )) > 0 ) || ( nMem )) {

		printf(XMS_FREE,(ram << 10 ));

		if ( nMem & 0x100 )
			printf( HMA_FREE );
		else if ( nMem == 0x91 )
			printf( HMA_USED );

		crlf();
	}

	// display environment stats
	printf( TOTAL_ENVIRONMENT, (long)gpIniptr->EnvSize );
	printf( LBYTES_FREE, (long)((glpEnvironment + gpIniptr->EnvSize ) - (end_of_env(glpEnvironment) + 1)));

	// display alias stats
	printf( TOTAL_ALIAS, (long)gpIniptr->AliasSize);
	printf( LBYTES_FREE, (long)((glpAliasList + gpIniptr->AliasSize) - (end_of_env(glpAliasList) + 1)));

	// display function stats
	printf( TOTAL_FUNCTION, (long)gpIniptr->FunctionSize);
	printf( LBYTES_FREE, (long)((glpFunctionList + gpIniptr->FunctionSize) - (end_of_env(glpFunctionList) + 1)));

	// display history stats
	printf( TOTAL_HISTORY, (long)gpIniptr->HistorySize );

	return 0;
}


// Set an INI file option
int _near Option_Cmd( char *pszCmdLine )
{
	static char szINI[] = "4DOS.INI";
	int rval, nFH;
	unsigned int uOptSize;
	char szOptName[MAXFILENAME], argline[ARGMAX];
	char _far *pIniData, _far *pIniStrings, _far *pIniKeys, _far *lpszComspec;
	char *pSaveStrData;
	unsigned int *pSaveKeys;

	// check for .INI file existence
	if ( gpIniptr->PrimaryININame == INI_EMPTYSTR ) {

		// create the file?

		_fstrcpy( szOptName, _pgmptr );
		if (( path_part( szOptName ) != NULL ) || (( lpszComspec = get_variable( COMSPEC )) == 0L ))
			lpszComspec = _pgmptr;

		// remove "4dos.com" & replace w/ 4DOS.INI
		sprintf( szOptName, FMT_FAR_PREC_STR, MAXFILENAME-1, lpszComspec );
		insert_path( szOptName, szINI, szOptName );
		
		if (( nFH = _sopen( szOptName, _O_RDWR | _O_BINARY | O_CREAT, _SH_DENYNO, _S_IREAD | _S_IWRITE )) >= 0 ) {
			_close( nFH );
			strcpy( gpIniptr->StrData + gpIniptr->StrUsed, szOptName );
			gpIniptr->PrimaryININame = gpIniptr->StrUsed;
			gpIniptr->StrUsed += strlen( szOptName ) + 1;
		}
	}

	// Search for OPTION.EXE, complain and return if not found
	if (( rval = FindInstalledFile( szOptName, OPTION_EXE )) != 0 )
		return ( error( rval, OPTION_EXE ));

	// Reserve space for INI stuff
	uOptSize = INI_TOTAL_BYTES;
	if (( pIniData = AllocMem( &uOptSize )) == NULL )
		return error ( ERROR_NOT_ENOUGH_MEMORY, NULL );

	pIniStrings = pIniData + sizeof(INIFILE);
	pIniKeys = pIniStrings + INI_STRMAX;

	// Save string and keys pointers
	pSaveStrData = gpIniptr->StrData;
	pSaveKeys = gpIniptr->Keys;

	// Copy INI data to low memory
	_fmemmove( pIniData, (char _far *)gpIniptr, sizeof(INIFILE));
	_fmemmove( pIniStrings, (char _far *)(gpIniptr->StrData), INI_STRMAX);
	_fmemmove( pIniKeys, (char _far *)(gpIniptr->Keys), INI_KEYS_SIZE);

	// Put low-memory address in hex, plus any arguments, into argv[1]
	sprintf( argline, "%x %s", FP_SEG(pIniData), (( pszCmdLine == NULL ) ? NULLSTR : pszCmdLine ));

	// Run OPTION.EXE
	if (( rval = External( szOptName, argline )) == 0 ) {

		// Successful -- retrieve modified data
		_fmemmove( (char _far *)gpIniptr, pIniData, sizeof(INIFILE));
		_fmemmove( (char _far *)(gpIniptr->StrData), pIniStrings, INI_STRMAX);
		_fmemmove( (char _far *)(gpIniptr->Keys), pIniKeys, INI_KEYS_SIZE);

		// Restore string and keys pointers
		gpIniptr->StrData = pSaveStrData;
		gpIniptr->Keys = pSaveKeys;

		// Reset internal LFN / SFN flags
		SetWin95Flags();
	}

	// Free the low-memory space
	FreeMem( pIniData );

	QueryCountryInfo();
	SetOSVersion();

	return rval;
}


// reboot the system (warm or cold boot)
int _near Reboot_Cmd( char *pszCmdLine )
{
	int nBootType;
	long fReboot;
	char _far *reboot_offset = (char _far *)(0xF000FFF0L);

	// check for /C(old boot) or /V(erify) switches
	if ( GetSwitches( pszCmdLine, "CV", &fReboot, 0 ) != 0 )
		return ( Usage( REBOOT_USAGE ));

	if (( fReboot & 2 ) && ( QueryInputChar( REBOOT_IT, YES_NO ) != YES_CHAR ))
		return 0;

	// run 4EXIT
	find_4files( AUTOEXIT );

	// flush the disk buffers & reset disks for nitwits like FASTOPEN
	reset_disks();

	// wait 2 seconds for caches to flush
	SysWait( 2L, 0 );

	nBootType = (int)fReboot;

_asm {
	and	nBootType, 1		; check for cold boot
	jz	warm_boot		;   do a warm boot

	call	get_cpu			; get CPU type - XTs have different
					;   keyboard controller
	xor	dx,dx			; write a 0 to 0000:0472
	cmp	ax,286
	jb	not_AT
	cli
	xor	al,al			; clear DMA page port
	out	080h, al		; clear mfg port
	mov	al,0FEh			; cold boot (through keyboard reset)
	out	064h,al			;   (because QEMM & MAX intercept boot)
	nop
        jmp     short not_AT

warm_boot:
	mov	dx,01234h
not_AT:
	xor	ax,ax
	mov	es,ax
	mov	word ptr es:[0472h],dx
	jmp	[reboot_offset]		; reboot
}

	return ERROR_EXIT;
}


#define NOTSPECIFIED 0			// START session type flags
#define NOTWINDOWCOMPAT 1
#define WINDOWCOMPAT 2
#define WINDOWAPI 3
#define DOSVDM_FS 4
#define DOSVDM_WIN 7

#define START_NO_4OS2 1
#define START_TRANSIENT 2
#define START_FG 4
#define START_BG 8
#define START_LOCAL_LISTS 0x10
#define START_LOCAL_ALIAS 0x20
#define START_LOCAL_DIRECTORY 0x40
#define START_LOCAL_FUNCTION 0x80
#define START_LOCAL_HISTORY 0x100
#define START_AND_WAIT 0x200
#define START_STD_WIN3 0x400


typedef struct _STARTDATA {
      USHORT  Length;
      USHORT  Related;
      USHORT  FgBg;
      USHORT  TraceOpt;
      PSZ     PgmTitle;
      PSZ     PgmName;
      PBYTE   PgmInputs;
      PBYTE   TermQ;
      PBYTE   Environment;
      USHORT  InheritOpt;
      USHORT  SessionType;
      PSZ     IconFile;
      ULONG   PgmHandle;
      USHORT  PgmControl;
      UINT    InitXPos;
      UINT    InitYPos;
      UINT    InitXSize;
      UINT    InitYSize;
      UINT    Reserved;
      PSZ     ObjectBuffer;
      ULONG   ObjectBuffLen;
} STARTDATA;

#define SSF_FGBG_FORE           0
#define SSF_FGBG_BACK           1

#define SSF_TYPE_DEFAULT        0
#define SSF_TYPE_FULLSCREEN     1
#define SSF_TYPE_WINDOWABLEVIO  2
#define SSF_TYPE_PM             3
#define SSF_TYPE_VDM            4
#define SSF_TYPE_GROUP          5
#define SSF_TYPE_DLL            6
#define SSF_TYPE_WINDOWEDVDM    7
#define SSF_TYPE_PDD            8
#define SSF_TYPE_VDD            9


// Start another job in a different OS/2 2.1 session.  If no arguments, just
//   start another command processor
int _near Start_Cmd( char *pszCmdLine )
{
	register char *arg;
	register int start_flag = 0;
	char *cmd_name = NULLSTR, *line, session_name[62];
	unsigned int i, dos_vdm = 0, size;
	int rval = 0, nFH;
	STARTDATA stdata;

	// save beginning of command line
	line = pszCmdLine;

	memset(&stdata,'\0',sizeof(STARTDATA));
	stdata.Length = sizeof(STARTDATA);

	if ( setjmp( cv.env ) == -1 )
	    rval = CTRLC;

	else {

	    // check for command line arguments
	    for ( i = 0; (( arg = ntharg( pszCmdLine, i )) != NULL ); i++ ) {

		// get the session title, if any (enclosed in double quotes)
		if ( *arg == DOUBLE_QUOTE ) {

			// if we already have a title, it must be program name
			if ( stdata.PgmTitle != 0L )
				break;

			if ( sscanf(arg+1,"%60[^\042]",session_name) > 0 )
				stdata.PgmTitle = session_name;
			continue;

		} else if ( *arg != gpIniptr->SwChr )
			break;

		// must be a startup switch
		if ( strnicmp( ++arg, START_BG_STR, 1 ) == 0 ) {

			// start as background session
			stdata.FgBg = START_BG;

		} else if ( stricmp( arg, START_TRANSIENT_STR ) == 0 ) {

			// make transient load
			start_flag |= START_TRANSIENT;

		} else if ( strnicmp( arg, START_DOS_STR, 3 ) == 0 ) {

			// start a foreground DOS VDM
			dos_vdm = 1;
			stdata.FgBg |= START_FG;

			// check if passing DOS strings
			if (arg[3] == '=') {

				char buffer[256];
set_dos_environment:
				// next arg is the filename
				arg += 4;
				if ((mkfname( arg, 0 ) == NULL) || (( nFH = _sopen( arg, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) < 0 ))
					return (error( _doserrno, arg ));

				HoldSignals();
				size = 4096;
				stdata.Environment = AllocMem(&size);
				stdata.Environment[0] = stdata.Environment[1] = '\0';

				while ((rval == 0 ) && (getline(nFH,buffer,255,EDIT_DATA) > 0 )) {

					// skip blank lines, leading whitespace, & comments
					arg = skipspace( buffer );
					if ((*arg) && (*arg != ':'))
						rval = add_list( arg, stdata.Environment );
				}

				_close( nFH );
				EnableSignals();
			}

		} else if ( stricmp( arg, START_FS_STR ) == 0 ) {

			// start as full-screen foreground session
			stdata.SessionType = NOTWINDOWCOMPAT;
			stdata.FgBg |= START_FG;

		} else if ( strnicmp( arg, START_FG_STR, 1 ) == 0 ) {

			// make program foreground session
			stdata.FgBg = START_FG;

		} else if ( strnicmp( arg, START_ICON_STR, 5 ) == 0 ) {

			// use specified icon file
			stdata.IconFile = _alloca( strlen( arg + 4 ));
			_fstrcpy( stdata.IconFile, arg+5 );

		} else if ( stricmp( arg, START_INV_STR ) == 0 ) {

			// start up invisible
			stdata.PgmControl |= 1;

		} else if ( stricmp( arg, START_KEEP_STR ) == 0 ) {

			// start with 4OS2; keep session when program finishes
			start_flag &= ~START_TRANSIENT;

		} else if ( stricmp( arg, START_LOCAL_STR ) == 0 ) {

			// start session with local alias & history lists
			start_flag |= START_LOCAL_LISTS;

		} else if ( stricmp( arg, START_LA_STR ) == 0 ) {

			// start session with local alias list
			start_flag |= START_LOCAL_ALIAS;

		} else if ( stricmp( arg, START_LD_STR ) == 0 ) {

			// start session with local directory list
			start_flag |= START_LOCAL_DIRECTORY;

		} else if ( stricmp( arg, START_LF_STR ) == 0 ) {

			// start session with local function list
			start_flag |= START_LOCAL_FUNCTION;

		} else if ( stricmp( arg, START_LH_STR ) == 0 ) {

			// start session with local history list
			start_flag |= START_LOCAL_HISTORY;

		} else if ( stricmp( arg, START_MAX_STR ) == 0 ) {

			// start maximized
			stdata.PgmControl |= 2;

		} else if ( stricmp( arg, START_MIN_STR ) == 0 ) {

			// start minimized
			stdata.PgmControl |= 4;

		} else if ( stricmp( arg, START_NO_STR ) == 0 ) {

			// start session without invoking 4OS2
			start_flag |= START_NO_4OS2;

		} else if ( stricmp( arg, START_PGM_STR ) == 0 ) {

			// next arg is program name, not session name
			//   no more switches allowed after program name
			arg = ntharg( pszCmdLine, ++i );
			break;

		} else if ( stricmp(arg,START_PM_STR) == 0 ) {

			// start as foreground PM app
			stdata.SessionType = WINDOWAPI;
			stdata.FgBg |= START_FG;

		} else if ( strnicmp(arg,START_POS_STR,4) == 0 ) {

			// start window at coordinates x,y with size x1, y1
			if (sscanf(gpNthptr+5,"%u,%u,%u,%u",&(stdata.InitXPos),&(stdata.InitYPos),&(stdata.InitXSize),&(stdata.InitYSize)) != 4)
				goto bad_parm;
			i += 3;
			stdata.PgmControl |= 0x8000;

		} else if ( stricmp(arg,START_WIN_STR) == 0 ) {

			// start as foreground window app within PM
			stdata.SessionType = WINDOWCOMPAT;
			stdata.FgBg |= START_FG;

		} else if ( strnicmp(arg,START_WIN3_STR,4) == 0 ) {

			// start as Win 3.1 app
			dos_vdm |= 2;
			stdata.FgBg |= START_FG;
			stdata.SessionType = NOTWINDOWCOMPAT;

			// standard mode session requested?
			if (_ctoupper(arg[4]) == 'S') {
				start_flag |= START_STD_WIN3;
				arg++;
			}

			// check if passing DOS strings
			if (arg[4] == '=') {
				arg++;
				goto set_dos_environment;
			}

		} else {
bad_parm:
			return (error(ERROR_INVALID_PARAMETER,arg));
		}
	    }

	    // set foreground/background flag
	    stdata.FgBg = ((stdata.FgBg & START_BG) || ((stdata.FgBg & START_FG) == 0 ));

	    // save the command line arguments
	    strcpy( line, (( gpNthptr != NULL ) ? gpNthptr : NULLSTR ));

	    // check STARTed command
	    // (if it's an internal command or alias, use 4DOS/4OS2 to run it)
	    if (( arg != NULL ) && ( findcmd( arg, 0 ) < 0 ) && ( get_alias( arg ) == 0L )) {

		// remove the external command argument
		strcpy(line,line+strlen(arg));

		// if an external command, get the full pathname
		if ((cmd_name = searchpaths( arg, NULL, TRUE, NULL )) == NULL) {
			if (*arg != '"')
				return (error(ERROR_4DOS_UNKNOWN_COMMAND,arg));
			cmd_name = arg;
		} else
			strupr(mkfname(cmd_name,0 ));

		// BAT files default to 4DOS sessions
		arg = ext_part( cmd_name );
		if ( stricmp( arg, BAT ) == 0 )
			dos_vdm |= 1;
	    }

	    // DOS VDM start requested?
	    if (dos_vdm) {

		// VDM has command in PgmInputs, not PgmName
		strins(line,cmd_name);

		// is it a Win 3 app?
		if (dos_vdm & 2) {
			strins(line,((start_flag & START_STD_WIN3) ? " /s " : " /3 "));
			strins(line,((arg = searchpaths( WIN_OS2, NULL, TRUE, NULL )) != NULL) ? arg : WIN_OS2);
			start_flag |= START_TRANSIENT;
		}

		// set 4DOS/4OS2 command line arguments (/C or /K)
		strins(line,((start_flag & START_TRANSIENT) ? "/C " : "/K "));

		if (stdata.SessionType & NOTWINDOWCOMPAT)
			stdata.SessionType = DOSVDM_FS;
		else
			stdata.SessionType = DOSVDM_WIN;

	    } else {

		// if it's a PM app, start it without 4OS2
		// if not specified, default to a 4OS2 window
		if ( stdata.SessionType == WINDOWAPI ) {

			// if no program name, it can't be a PM app!
			if ( *cmd_name == '\0' ) {
				stdata.SessionType = WINDOWCOMPAT;
				start_flag &= ~START_NO_4OS2;
			} else
				start_flag |= START_NO_4OS2;

		} else if ( stdata.SessionType == 0 )
			stdata.SessionType = WINDOWCOMPAT;

		// if NO_4OS2, don't invoke 4OS2.EXE for new session
		//   change program name from 4OS2 to user-specified name
		if (start_flag & START_NO_4OS2)
			stdata.PgmName = cmd_name;

		else {

			// replace original command name w/fully-qualified name
			strins( line, cmd_name );

			// check for local alias / history lists
			if ( start_flag & START_LOCAL_LISTS )
				strins( line, "/L " );
			else {
				if ( start_flag & START_LOCAL_ALIAS )
					strins( line, "/LA " );
				if ( start_flag & START_LOCAL_DIRECTORY )
					strins( line, "/LD " );
				if ( start_flag & START_LOCAL_FUNCTION )
					strins( line, "/LF " );
				if ( start_flag & START_LOCAL_HISTORY )
					strins( line, "/LH " );
			}

			// set command line arguments (/C or /K)
			strins( line, (( start_flag & START_TRANSIENT ) ? "/C " : "/K " ));
		}
	    }

	    stdata.PgmInputs = (PBYTE)line;

_asm {
	    push   di
	    push   si
	    mov    ax, 06400h
	    mov    bx, 025h		; Dos32StartSession ordinal
	    mov    cx, 0636Ch
	    lea    si, stdata
	    int    21h
	    mov    rval, ax
	    pop    si
	    pop    di
}
	}

	if ( stdata.Environment != 0L )
		FreeMem( stdata.Environment );

	return rval;
}


// Turn 4DOS disk, EMS, or XMS swapping on or off
int _near Swap_Cmd( char *pszCmdLine )
{
	register int nSwapState;

	if (gpIniptr->SwapMeth == SWAP_NONE)	// check if resident mode
		return (error(ERROR_4DOS_NOT_SWAPPING_MODE,NULL));

	// inquiring about swap status
	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ))
		printf( SWAPPING_IS, gpInternalName, swap_mode[gpIniptr->SwapMeth],((ServCtrl(SERV_SWAP,-1)) ? ON : OFF));

	else {			// set new swap status

		if (( nSwapState = OffOn( pszCmdLine )) == -1 )
			return ( Usage( SWAPPING_USAGE ));

		// reset the 4DOS swap server
		ServCtrl( SERV_SWAP, nSwapState );
	}

	return 0;
}


// display the true path name for the specified file
int _near Truename_Cmd( char *pszCmdLine )
{
	char name[MAXFILENAME];

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ))
		return ( Usage( TRUENAME_USAGE ));

	if ( true_name( pszCmdLine, name ) == NULL )
		return ERROR_EXIT;

	printf( FMT_STR_CRLF, name );

	return 0;
}


static int _near _unlock( int nDisk )
{
_asm {
	mov	ax, 0440Dh
	mov	bx, nDisk
	mov	cx, 086Ah
	int	21h
	jc	UnLockError
	xor	ax,ax
UnLockError:
	mov	nDisk, ax
}
	return nDisk;
}


int _near Unlock_Cmd( char *pszCmdLine )
{
	register int nDisk, argc;
	int rval = 0;
	char *arg, szDisk[4];

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) {

		// if no arguments, unlock everything
		for ( nDisk = 1; ( nDisk <= 26 ); nDisk++ ) {
			sprintf( szDisk, "%c:", nDisk+64 );
			if (( QueryDriveExists( nDisk ) == 0 ) || ( QueryDriveRemote(nDisk) == 1) || (QueryIsCDROM( szDisk ) == 1))
				continue;
			if (( argc = _unlock( nDisk )) != 0 )
				rval = error( argc, szDisk );
		}

	} else {

		// unlock the specified drives
		for ( argc = 0; (( arg = ntharg( pszCmdLine, argc )) != NULL ); argc++ ) {

			if (( nDisk = _unlock( gcdisk( arg ))) != 0 )
				rval = error( nDisk, arg );
		}
	}

	return rval;
}


// print version numbers of 4DOS & DOS
int _near Ver_Cmd( char *pszCmdLine )
{
	extern int fWin98;
	extern int fWinME;
	long lVerFlags;
	char *pszOS;

	if (( lVerFlags = switch_arg( pszCmdLine, "R" )) < 0 )
		return ( Usage( VER_USAGE ));

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

	if (( gnOSFlags & DOS_IS_OS2 ) && ( _osmajor == 20 ) && ( _osminor >= 30 )) {
		char chOS2Major = (( _osminor >= 40 ) ? 4 : 3 );
		printf( DOS_VERSION, PROGRAM, WARPVER, chOS2Major, gaCountryInfo.szDecimal[0], ( _osminor - ( 10 * chOS2Major )));
	} else
		printf( DOS_VERSION, PROGRAM, pszOS, gchMajor, gaCountryInfo.szDecimal[0], gchMinor );

	// display version info (Rev level, & whether in HMA or ROM)
	if ( lVerFlags == 1 ) {

		printf( FOURDOS_REV, VER_BUILD );

		// DR-DOS doesn't have an internal revision number
		if (( gnOSFlags & DOS_IS_DR ) == 0 ) {

			if ( gnOSFlags & DOS_IS_OS2 )
				printf( OS2_REVISION, ( gnOSFlags & 0x7FF ));
			else
				printf( DOS_REVISION, ( gnOSFlags & 0x7 ) + 'A' );

			printf( DOS_LOCATION, (( gnOSFlags & DOS_IN_HMA ) ? DOS_HMA : (( gnOSFlags & DOS_IN_ROM ) ? DOS_ROM : DOS_LOW )));
		}

		crlf();

		// display serial number (brand) info
		TestBrand( 0 );
	}

	return 0;
}

