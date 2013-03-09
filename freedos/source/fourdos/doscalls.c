

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


// DOSCALLS.C - 4DOS-specific commands & support routines
//   Copyright 1993 - 2004 Rex C. Conn

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>

#include "4all.h"


#pragma alloc_text( _TEXT, BreakHandler, BreakOut )

// ^C / ^BREAK handler
void PASCAL BreakHandler( void )
{
	HoldSignals();			// turn off ^C during clean up
	cv.fException |= CTRLC;	// flag aliases/batch files to abort
	CriticalSection(0 );

	// reset IFF parsing flags if not in a batch file
	if ( cv.bn < 0 )
		cv.f.lFlags = 0L;
	BreakOut();
}


// jump back to location flagged by setjmp()
void BreakOut( void )
{
	extern int fNoComma;

	fNoComma = 0;
	SetCurSize( );			// reset normal cursor shape
	reset_disks();			// flush disk buffers (ignoring DiskReset value)

	longjmp( cv.env, -1 );		// jump to last setjmp()
}


#pragma alloc_text( _TEXT, installable_cmd )

// look for (& execute if found) a TSR installed via INT 2F function AEh
int _near installable_cmd( char *line )
{
	extern int _pascal noappend( void );
	extern char szAppendBlock[];

	static char DELIMS[] = "%9[^  \t;,.\"`\\+=]";
	struct {
		char max;
		char len;
		char args[128];
	} dosline;
	struct {
		char len;
		char name[16];
	} cmd;
	int rval, SwapFlag = 0;
	jmp_buf saved_env;

	if (( line[1] == ':' ) || (*line == '\\' ))
		return -1;

	DELIMS[4] = gpIniptr->SwChr;
	sscanf( line, DELIMS, cmd.name );
	cmd.len = (char)strlen( cmd.name );
	sprintf( cmd.name, FMT_LEFT_STR, 11, strupr( cmd.name ));

	sprintf( dosline.args, "%.*s\r", 124, line );
	dosline.max = (char)124;
	dosline.len = (char)strlen( dosline.args + 1 );

	// truncate line (may be > 128 chars!)
	sprintf( szAppendBlock, " %.126s\r", line + cmd.len );
	szAppendBlock[0] = (char)strlen( szAppendBlock + 2 );
	_fmemmove( MAKEP( _psp, 0x80 ), szAppendBlock, 128 );

	rval = ( dosline.len - cmd.len);

_asm {
	push	ds
	push	di
	push	si

	push	ds
	pop	es

	mov	ax,0AE00h		; test for existence of command
	lea	bx, word ptr dosline	; DS:BX points to command line
;In Win95 BX may need to point to dosline+2
	lea	si, word ptr cmd	; DS:SI points to command name
	mov	cx,rval
;In Win95 CH may need to be 0
	mov	ch,0FFh			; CX = 0FFh + length of command args
	mov	dx,0FFFFh		; DX always 0FFFFh for some reason
	int	02Fh

	pop	si
	pop	di
	pop	ds

	xor	ah,ah			; if command is installed, call
	mov	rval,ax			;   will return 0xFF in AL
}

	if ( rval != 0xFF )
		return -1;

	// save the old "env" (^C destination)
	memmove( saved_env, cv.env, sizeof(saved_env) );

	if ( setjmp( cv.env ) == -1 )
		rval = CTRLC;

	else {

		// turn off swapping & free low DOS memory
		SwapFlag = ServCtrl( SERV_SWAP, -1 );
		ServCtrl( SERV_SWAP, 0 );
		ServCtrl( SERV_TPA, 0 );

_asm {
		push	ds
		push	di
		push	si

		push	cs			; these 3 lines short-circuit
		pop	es			;   attempts by APPEND to run
		mov	di, offset cs:noappend	;   code inside COMMAND.COM

		mov	ax, 0AE01h		; execute the command
		lea	bx, word ptr dosline
		lea	si, word ptr cmd
		xor	ch, ch
		mov	cl, byte ptr cmd.len
		mov	dx, 0FFFFh
		int	02Fh

		pop	si
		pop	di
		pop	ds

		xor	ah, ah			; return AL value
		mov	rval, ax
}
		// rewrite line (installable command may have modified it)
		//   (if len == 0, installable command has executed the line)
		if ( cmd.len != 0 ) {
			sscanf( dosline.args, SCAN_NOCR, line );
			rval = -1;
		}
	}

	// restore the old "env"
	memmove( cv.env, saved_env, sizeof(saved_env) );

	// reenable swapping?
	if ( SwapFlag )
		ServCtrl( SERV_SWAP, 1 );

	return rval;
}


// read/write .INI files
char *IniReadWrite( int fWrite, char *pszIniFile, char *pszSection, char *pszName, char *pszValue )
{
	int fh;
	unsigned int uShare, uMode, uBlockSize, uBytesRead = 0, pMode = _S_IREAD | _S_IWRITE;
	long lReadOffset, lWriteOffset = 0L, lBlockSize;
	char *ptr, szBuf[512];
	char _far *fptr;

	if (( pszIniFile == NULL ) || ( pszSection == NULL ) || ( pszName == NULL ))
		return NULL;

	if (( *pszIniFile == '\0' ) || ( *pszSection == '\0' ) || ( *pszName == '\0' ))
		return NULL;

	if ( fWrite ) {
		uMode = _O_RDWR | _O_BINARY | _O_CREAT;
		uShare = _SH_DENYWR;
	} else {
		uMode = _O_RDONLY | _O_BINARY;
		uShare = _SH_DENYNO;
	}

	szBuf[0] = '\0';
	if (( fh = _sopen( pszIniFile, uMode, uShare, pMode )) >= 0 ) {

		HoldSignals();

		// read file for section names
		while ( getline( fh, szBuf, 511, 0 ) > 0 ) {

			trim( szBuf, WHITESPACE );

			if ( szBuf[0] != '[' )
				continue;

			strcpy( szBuf, szBuf + 1 );
			strip_trailing( szBuf, "]" );

			// check for section name
			if ( stricmp( szBuf, pszSection ) == 0 ) {

				for ( ; ; ) {

					lReadOffset = _lseek( fh, 0L, SEEK_CUR );
					if ( getline( fh, szBuf, 511, 0 ) <= 0 )
						break;

					trim( szBuf, WHITESPACE );

					if ( szBuf[0] == '[' )
						break;

					if (( ptr = strchr( szBuf, '=' )) != NULL ) {

						*ptr++ = '\0';
						strip_trailing( szBuf, " \t" );
						if ( stricmp( szBuf, pszName ) == 0 ) {

							// found entry
							if ( fWrite ) {
								lWriteOffset = _lseek( fh, 0L, SEEK_CUR );
								break;
							}

							_close( fh );
							EnableSignals();
							return ( strcpy( pszValue, ptr ));
						}
					}
				}

				// not found?  Write it!
				if ( fWrite ) {

					// if section not found, insert it here
					if ( lWriteOffset == 0L )
						lWriteOffset = lReadOffset;

					// get remaining size
					lBlockSize = ( QuerySeekSize( fh ) - lWriteOffset );

					// can't read/write more than 64K in DOS!
					if ( lWriteOffset >= 0xFFF0L )
			    			break;

					if ( lBlockSize > 0L ) {

						uBlockSize = (unsigned int)lBlockSize;

						if (( fptr = AllocMem( &uBlockSize )) == 0L )
						    goto EndIni;

						// read remainder of file
						_lseek( fh, lWriteOffset, SEEK_SET );
					    	FileRead( fh, fptr, uBlockSize, &uBytesRead );
					}

					_lseek( fh, lReadOffset, SEEK_SET );

					if (( pszValue ) && ( *pszValue ))
						qprintf( fh, "%s=%s\r\n", pszName, pszValue );

					// write remainder of file
					if ( lWriteOffset > 0L ) {
						FileWrite( fh, fptr, uBytesRead, &uBlockSize );
						FreeMem( fptr );
						_chsize( fh, _lseek( fh, 0L, SEEK_CUR ));
					}

					goto EndIni;
				}
			}
		}

		if (( fWrite ) && ( *pszName ) && ( *pszValue )) {
			// didn't find a matching section, so create it
			qprintf( fh, "\r\n[%s]\r\n%s=%s\r\n", pszSection, pszName, pszValue );
		}
EndIni:
		_close( fh );
		EnableSignals();
	}

	if ( fWrite )
		return (( fh < 0 ) ? NULL : (char *)-1 );

	return NULL;
}


#pragma alloc_text( _TEXT, process_rexx )

// check if a .BAT file is a REXX file; if yes, call the REXX interpreter
// (PC-DOS 7, or if RexxPath .INI directive is set)
int _near process_rexx( char *cmd_name, register char *line, int fRexx )
{
	char *arg;	// can't make register b/c of compiler bug w/_alloca()!
	int nFH;

	// open the .BAT file
	if ((( _osmajor == 7 ) || ( gpIniptr->RexxPath != INI_EMPTYSTR )) && (( nFH = _sopen( cmd_name, (_O_RDONLY | _O_BINARY), _SH_DENYWR )) > 0 )) {

		// save the passed line so we can use "gszCmdline" for scratch
		arg = _alloca( strlen( line ) + 1 );
		line = strcpy( arg, line );

		// read the first line of the file
		_read( nFH, gszCmdline, 8 );
		_close( nFH );

		// if it's a REXX comment (begins with "/*"), call the
		//   REXX interpreter
		if (( gszCmdline[0] == '/' ) && ( gszCmdline[1] == '*' )) {
			sprintf( gszCmdline, "%s %s%s", (( gpIniptr->RexxPath != INI_EMPTYSTR ) ? (char *)( gpIniptr->StrData + gpIniptr->RexxPath ) : "rexx.exe" ), cmd_name, line );
			return ( command( gszCmdline, 0 ));
		}
	}

	// call the batch file processor
	return ( ParseLine( cmd_name, line, NULL, (CMD_STRIP_QUOTES | CMD_ADD_NULLS | CMD_CLOSE_BATCH), 0 ));
}


#pragma alloc_text( _TEXT, External )

// Execute an external program (.COM or .EXE) via the DOS EXEC call
int _near External( char *pszCmd, char *pszCmdLine )
{
	char doscmdline[130], szShortName[MAXFILENAME], *pszComspec;
	PCH feptr;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	// check for a "4DOS /C 4DOS" & turn it into a "4DOS" (provided we're
	//   not in a batch file!)
	if (( cv.bn < 0 ) && ( gnTransient ) && ( stricmp( fname_part( pszCmd ), DOS_NAME) == 0 ) && (( *pszCmdLine != gpIniptr->SwChr ) || ( _ctoupper( pszCmdLine[1] ) != 'C' ))) {

		// turn off /C switch
		gnTransient = 0;

		// print the copyright that wasn't done the first time
		DisplayCopyright();

		return ( command( pszCmdLine, 0 ));
	}

	// tag the environment as belonging to 4DOS (for things like MAPMEM)
	// first, make sure we're really at the end of the environment (if the
	//   environment is empty, "end_of_env" returns a pointer to
	//   "environment"
	if (( feptr = end_of_env( glpEnvironment )) == glpEnvironment )
		feptr++;

	if ((unsigned int)(( glpEnvironment + gpIniptr->EnvSize ) - feptr ) > 8 )
		_fmemmove( feptr+1, "\001\000""4DOS\000", 7 );

	// Some programs don't like trailing spaces; others require leading
	//   spaces, so we have to get a bit tricky here

	// clear buffer (for Netware bug)
	memset( doscmdline, '\0', 130 );

	sprintf( doscmdline, " %.126s\r", pszCmdLine );

	// set length of command tail
	doscmdline[0] = (char)((( fWin95 ) && ( strlen( pszCmdLine ) > 126 )) ? 0x7F : strlen( pszCmdLine ));

	// if we're executing COMMAND.COM, change COMSPEC
	pszComspec = NULL;
	if ( stricmp( COMMAND_COM, fname_part( pszCmd )) == 0 ) {
		if (( feptr = get_variable( COMSPEC )) != 0L ) {
			pszComspec = _alloca( _fstrlen( feptr ) + 1 );
			_fstrcpy( pszComspec, feptr );
		}
		sprintf( gszCmdline, FMT_TWO_EQUAL_STR, COMSPEC, pszCmd );
		add_variable( gszCmdline );
	}

	strcpy( szShortName, pszCmd );
	GetShortName( szShortName );

	// call the EXEC function (DOS Int 21h 4Bh)
	_doserrno = 0;
	switch ( ServExec( szShortName, doscmdline, SELECTOROF( glpEnvironment ), (( gnTransient == 0 ) && ( gpIniptr->ChangeTitle ) && ( _osmajor >= 20 )), (int *)&_doserrno )) {
	case -1:			// couldn't execute
		gnErrorLevel = error( _doserrno & 0xFF, pszCmd );
		break;
	case -CTRLC:			// aborted with a ^C
		gnErrorLevel = cv.fException = CTRLC;
		break;
	default:			// no error, save external result
		gnErrorLevel = ( _doserrno & 0xFF );
	}

	gnHighErrorLevel = ( _doserrno >> 8 );

	// reset COMSPEC if necessary
	if ( pszComspec != NULL ) {
		sprintf( gszCmdline, FMT_TWO_EQUAL_STR, COMSPEC, pszComspec );
		add_variable( gszCmdline );
	}

	// reset alias & history pointers in case transient portion relocated
	glpAliasList = (PCH)(gpIniptr->AliasLoc);
	glpFunctionList = (PCH)(gpIniptr->FunctionLoc);
	glpHistoryList = (PCH)(gpIniptr->HistLoc);
	glpDirHistory = (PCH)(gpIniptr->DirHistLoc);

	return gnErrorLevel;
}


#pragma alloc_text( _TEXT, CheckFreeStack )

// check for a minimum amount of free stack
void CheckFreeStack( unsigned int nMinStack )
{
	if ( _stackavail() < nMinStack ) {

		error( ERROR_4DOS_STACK_OVERFLOW, NULL );

		// end command and snuff any batch files
		cv.fException |= BREAK_STACK_OVERFLOW;
		BreakOut();
	}
}


#pragma alloc_text( _TEXT, _help )

// call 4HELP.EXE
int _help( register char *arg, char *opts )
{
	register int rval = 0;
	char szHelpName[MAXFILENAME], argline[ARGMAX];

	// Search for 4HELP.EXE, complain and return if not found
	if (( FindInstalledFile( szHelpName, HELP_EXE )) != 0 ) {
		honk();
		return ERROR_EXIT;
	}

	if ( arg == NULL )
		arg = NULLSTR;
	else {
		// kludge to strip trailing slashes (for things like DIR/)
		strip_trailing( arg, SLASHES );

		// skip leading * and @
		while ( *arg == '*' )
			arg++;
	}

	if ( opts == NULL )
		opts = NULLSTR;

	sprintf( argline, " %.20s %s %.50s\r", arg, opts, (( gpIniptr->HOptions != INI_EMPTYSTR) ? (char *)( gpIniptr->StrData + gpIniptr->HOptions ) : NULLSTR ));
	argline[0] = strlen( argline + 2 );

	GetShortName( szHelpName );

	// call the EXEC function (DOS Int 21h 4Bh)
	// (Don't call external(), because it destroys gszCmdline, which
	//   screws up the batch debugger.)
	_doserrno = 0;
	switch ( ServExec( szHelpName, argline, SELECTOROF( glpEnvironment ), (( gnTransient == 0 ) && ( gpIniptr->ChangeTitle ) && ( _osmajor >= 20 )), (int *)&_doserrno )) {
	case -1:			// couldn't execute
		rval = error( _doserrno & 0xFF, szHelpName );
		break;
	case -CTRLC:			// aborted with a ^C
		rval = cv.fException = CTRLC;
		break;
	default:			// no error, save external result
		rval = ( _doserrno & 0xFF );
	}

	return rval;
}


// Set up output side of a pipe.
int open_pipe( register REDIR_IO *redirect )
{
	register int fd;
	char szPipeName[MAXFILENAME];
	PCH feptr;

	// look for TEMP disk area defined in environment, or default to boot
	feptr = GetTempDirectory( szPipeName );

	// if no temp directory, or temp directory is invalid, use root
	//   directory on boot drive
	if ( feptr == 0L )
		sprintf( szPipeName, FMT_ROOT, gpIniptr->BootDrive );

	// create unique names for pipes
	if (( UniqueFileName( szPipeName ) != 0 ) || (( fd = _sopen( szPipeName, (_O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY ), _SH_COMPAT, _S_IREAD | _S_IWRITE )) < 0 )) {
		remove( szPipeName );
		return ( error( _doserrno, szPipeName ));
	}

	redirect->pszOutPipe = strdup( szPipeName );

	// set the "pipe active" flags
	redirect->fPipeOpen = redirect->fIsPiped = 1;

	// save STDOUT & point it to the pipe
	redirect->naStd[STDOUT] = _dup( STDOUT );
	dup_handle( fd, STDOUT );

	return 0;
}


// Delete temporary pipe input file
void _fastcall killpipes( REDIR_IO *redirect )
{
	if ( redirect->fIsPiped ) {
		redirect->fIsPiped = 0;
		remove( redirect->pszInPipe );
		free( redirect->pszInPipe );
	}
}


// change size (truncate) the file
int _chsize( int fh, long lSize )
{
	unsigned int uBytesWritten;

	_lseek( fh, lSize, SEEK_SET );

	// MSC write() function won't truncate
	_dos_write( fh, NULLSTR, 0, &uBytesWritten );

	return 0;
}


// free DOS memory block
void _fastcall FreeMem( void _far *fptr )
{
	if ( fptr != 0L ) {
		ServCtrl( SERV_TPA, 0 );
		(void)_dos_freemem( SELECTOROF( fptr ));
		ServCtrl( SERV_TPA, 1 );
	}
}


// allocate DOS memory
void _far * PASCAL AllocMem( register unsigned int *size )
{
	register unsigned int uParagraphs;
	unsigned int uSegment;

	// get the number of paragraphs we want (rounded up)
	uParagraphs = (( *size + 0xF ) >> 4 );

	ServCtrl( SERV_TPA, 0 );

	// check that we don't overwrite the 4DOS transient portion
	if (( gpIniptr->SwapMeth != SWAP_NONE ) && ( _dos_allocmem( 0x7F, &uSegment ) == 0 )) {
		if ( gpIniptr->HighSeg < ( uSegment + uParagraphs ))
			uParagraphs = ( gpIniptr->HighSeg - uSegment ) - 1;
		(void)_dos_freemem( uSegment );
		if ( uSegment >= gpIniptr->HighSeg ) {
			ServCtrl( SERV_TPA, 1 );
			return 0L;
		}
	}

	if ( _dos_allocmem( uParagraphs, &uSegment ) != 0 ) {

		uParagraphs = uSegment;

		// if size >= 0xFFF0, return the biggest piece we can get
		if (( *size < 0xFFF0 ) || ( uParagraphs < 0x80 ) || ( _dos_allocmem( uParagraphs, &uSegment ) != 0 ))
			uSegment = 0;
	}

	// If we ended up reserving space in transient area that's an error
	if (( gpIniptr->SwapMeth != SWAP_NONE ) && (( uSegment >= gpIniptr->HighSeg ) || (( uSegment + uParagraphs ) > gpIniptr->HighSeg ))) {
		(void)_dos_freemem( uSegment );
		uSegment = 0;
	}

	*size = ( uParagraphs << 4 );
	ServCtrl( SERV_TPA, 1 );

	// clear it out (like Win32 does)
	_fmemset( (char _far *)MAKEP( uSegment, 0 ), 0, *size );

	return (char _far *)MAKEP( uSegment, 0 );
}


// grow or shrink DOS memory block
void _far * PASCAL ReallocMem( void _far *fptr, unsigned long ulSize )
{
	register int rval;
	unsigned int uParagraphs, uSegment, uMaxSize = 0, int_size;
	char _far *save_fptr;

	if ( fptr == 0L )
		fptr = AllocMem( (unsigned int *)&ulSize );

	else {

		// round paragraph size up
		uParagraphs = (unsigned int)(( ulSize + 0xF ) >> 4 );

		uSegment = SELECTOROF( fptr );

		ServCtrl( SERV_TPA, 0 );

		rval = _dos_setblock( uParagraphs, uSegment, &uMaxSize );

		ServCtrl( SERV_TPA, 1 );

		if ( rval != 0 ) {

			// realloc failed; try a new block in case memory
			//   is fragmented.  This only works if the original
			//   segment was < 64K
			if ( ulSize < 0xFFF0L ) {

				int_size = (unsigned int)ulSize;

				save_fptr = fptr;

				// copy old block to new; free old block
				if (( fptr = AllocMem( &int_size )) != 0L )
					_fmemmove( fptr, save_fptr, (unsigned int)ulSize );
				FreeMem( save_fptr );

			} else {
				FreeMem( fptr );
				fptr = 0L;
			}
		}

		// if we're swapping, make sure reallocation doesn't overwrite
		//   the transient portion of 4DOS
		if (( fptr != 0L ) && ( gpIniptr->SwapMeth != SWAP_NONE ) && ( gpIniptr->HighSeg < ( uSegment + uParagraphs))) {
			FreeMem( fptr );
			fptr = 0L;
		}
	}

	return fptr;
}


#pragma alloc_text( _TEXT, HoldSignals, EnableSignals )

// temporarily disable the ^C / ^BREAK / KILLPROCESS signals
void HoldSignals( void )
{
	ServCtrl( SERV_SIGNAL, SERV_SIG_DISABLE );
}


// enable the ^C / ^BREAK / KILLPROCESS signals
void EnableSignals( void )
{
	ServCtrl( SERV_SIGNAL, (int)BreakHandler );
	ServCtrl( SERV_SIGNAL, SERV_SIG_ENABLE );
}


// wait the specified number of seconds
void _fastcall SysWait( unsigned long ulSeconds, int fFlags )
{
	register int nInterval = 5;
	unsigned long i;

	// milliseconds (2) or seconds?
	if (( fFlags & 2 ) == 0 ) {
		// if not /B or /M, just wait
		if (( fFlags & 1 ) == 0 ) {
			DosBeep( 0, (unsigned int)(ulSeconds * 18) );
			return;
		}
		ulSeconds *= 4;
	}

	for ( i = 0; ( i < ulSeconds ); ) {

		if (( fFlags & 1 ) && ( _kbhit() ))
			break;

		if ( fFlags & 2 ) {
			// 18.2 ticks/second == roughly 50ms (with overhead)
			nInterval = 1;
			i += 50;
		} else {
			nInterval = (( nInterval == 5 ) ? 4 : 5 );
			i++;
		}

		// DosBeep with a frequency < 20 Hz just counts ticks & returns
		if ( DosBeep( 0, nInterval ) ) {
			// Got a ^C / ^Break; allow it to signal break handler
			GetKeystroke( EDIT_NO_ECHO );
		}
	}
}


// check if arg is a +, -, or 0-9
int PASCAL is_signed_digit( register int nDigit )
{
	return (( is_unsigned_digit( nDigit )) || ( nDigit == '+' ) || ( nDigit == '-' ));
}


// check if arg is a 0-9
int PASCAL is_unsigned_digit( register int digit )
{
	return (( digit >= '0' ) && ( digit <= '9' ));
}


// get the amount of installed RAM
int QuerySystemRAM( void )
{
_asm {
	int	12h		; get RAM size from BIOS
}
}


// Get the current date & time
void QueryDateTime( DATETIME *sysDateTime )
{
	_dos_gettime( (struct dostime_t *)sysDateTime );
	_dos_getdate( (struct dosdate_t *)&(sysDateTime->day) );
}


// Set the current date & time
int SetDateTime( DATETIME *sysDateTime )
{
	return (( _dos_settime( (struct dostime_t *)sysDateTime ) != 0 ) || ( _dos_setdate( (struct dosdate_t *)&(sysDateTime->day) ) != 0 ));
}


#ifdef VER2

// get the file's date & time
int QueryHandleDateTime( int fd, DATETIME *sysDateTime )
{
	int rval;
	union {
		unsigned int wr_time;
		struct {
			unsigned seconds : 5;
			unsigned minutes : 6;
			unsigned hours : 5;
		} fTime;
	} Time;
	union {
		unsigned int wr_date;
		struct {
			unsigned days : 5;
			unsigned months : 4;
			unsigned years : 7;
		} fDate;
	} Date;

	if (( rval = _dos_getftime( fd, &(Date.wr_date), &(Time.wr_time) )) == 0 ) {

		sysDateTime->day = Date.fDate.days;
		sysDateTime->month = Date.fDate.months;
		sysDateTime->year = Date.fDate.years + 1980;
		sysDateTime->seconds = Time.fTime.seconds;
		sysDateTime->minutes = Time.fTime.minutes;
		sysDateTime->hours = Time.fTime.hours;
	}

	return rval;
}

#endif


// set the file's date & time
int _fastcall SetFileDateTime( char *pszFilename, int fd, DATETIME *sysDateTime, int fField )
{
	int nFunction = 0x5701;
	unsigned int uTime, uDate;

	_doserrno = 0;
	if (( fd > 0 ) || (( fd = _sopen( pszFilename, (_O_WRONLY | _O_BINARY), _SH_DENYNO )) >= 0 )) {

		HoldSignals();

		uDate = sysDateTime->day + ( sysDateTime->month << 5 ) + (( sysDateTime->year - 1980 ) << 9 );
		uTime = ( sysDateTime->seconds / 2 ) + ( sysDateTime->minutes << 5 ) + ( sysDateTime->hours << 11 );

		if ( fWin95 ) {
		    if ( fField == 1 )
			nFunction = 0x5705;
		    else if ( fField == 2 )
			nFunction = 0x5707;
		}

_asm {
		mov	ax, nFunction
		mov	bx, fd
		mov	cx, uTime
		mov	dx, uDate
		push	si
		xor	si, si
		int	21h
		pop	si
		jnc	NoError
		mov	_doserrno, ax
NoError:
}
		if ( pszFilename != NULL )
			_close( fd );
		EnableSignals();
	}

	return _doserrno;
}


// Get the file size (-1 if file doesn't exist)
long QueryFileSize( char *fname, int fAllocated )
{
	int fval = FIND_FIRST;
	long lSize = 0L;
	FILESEARCH dir;
	QDISKINFO DiskInfo;
	char szFileName[MAXFILENAME];

	strcpy( szFileName, fname );
	if ( fAllocated )
		QueryDiskInfo( szFileName, &DiskInfo, 0 );

	for ( ; ( find_file( fval, szFileName, 0x07 | FIND_NO_ERRORS | FIND_NO_FILELISTS, &dir, NULL ) != NULL ); fval = FIND_NEXT ) {
		if ( fAllocated ) {
		    if ( dir.ulSize > 0L )
			lSize += (unsigned long)(( dir.ulSize + ( DiskInfo.ClusterSize - 1 )) / DiskInfo.ClusterSize ) * DiskInfo.ClusterSize;
		} else
		    lSize += dir.ulSize;
	}

	return (( fval == FIND_FIRST ) ? -1L : lSize );
}


// get the current code page
int QueryCodePage( void )
{
	unsigned int cp;

	if (gnOsVersion < 330 )
		cp = 0;
	else {

		// set bx & dx to 0xffff for DR-DOS bug
_asm {
		mov	ax,06601h
		mov	bx,0ffffh
		mov	dx,0ffffh
		int	21h
		jnc	cp_exit
		xor	bx,bx
cp_exit:
		mov	cp,bx
}
		// check for non-ASCII character sets (DOS/V)
		if ( cp == 932 ) {
			gchBlock = 0x14;
			gchDimBlock = 0x1A;
			gchVerticalBar = 0x1D;
			gchRightArrow = 0x1E;
			gchUpArrow = 0x1C;
			gchDownArrow = 0x07;
		} else {
			gchBlock = 'Û';
			gchDimBlock = '°';
			gchVerticalBar = '³';
			gchRightArrow = 0x10;
			gchUpArrow = 0x18;
			gchDownArrow = 0x19;
		}
	}

	return cp;
}


// call DOS to get the volume label
char * QueryVolumeInfo( register char *arg, register char *volume_name, unsigned long *ulSerial )
{
	char szVolName[] = "C:\\*.*";
	FILESEARCH dir;
	struct {
		int info_level;
		unsigned long disk_serial_number;
		char disk_label[11];
		char filesystem_type[8];
	} bpb;

	if (( arg = gcdir( arg,0 )) == NULL )
		return NULL;

	// get the drive spec
	szVolName[0] = (char)_ctoupper( *arg );

	// get the serial number (local drives, DOS 4+ & OS/2 2.0 VDMs only)
	if ( QueryDriveRemote( szVolName[0] - 64 ))
		goto dos_3;

_asm {
	cmp	_osmajor, 4
	jb	dos_3
	mov	ax, 06900h	; int 21h 440Dh 66 call doesn't work in OS/2!
	xor	bh, bh
	mov	bl, byte ptr szVolName[0]
	sub	bl, 64
	xor	cx, cx
	lea	dx, word ptr bpb
	int	21h		; return serial # & volume info
	jc	dos_3
}
	*ulSerial = bpb.disk_serial_number;
dos_3:

	// get disk label (don't use the one in the BPB because it may not
	//   be the same as the one in the root directory!)
	if ( find_file( FIND_FIRST, szVolName, 0x08 | FIND_CLOSE | FIND_NO_ERRORS, &dir, NULL ) != NULL ) {

		// save volume name & remove a period preceding the extension
		sscanf( dir.szFileName, "%12[^\001]", volume_name );
		if (( strlen( volume_name ) > 8 ) && ( volume_name[8] == '.' ))
			strcpy( volume_name+8, volume_name+9 );

	} else
		strcpy( volume_name, UNLABELED );	// disk isn't labeled

	return volume_name;
}


// Check for ANSI - return 1 if loaded, 0 if absent
int QueryIsANSI( void )
{
	char fAnsi;

	// Check for presence of ANSI.SYS
	// Alternately, the user can force ANSI by setting gpIniptr->Ansi
	if ( gpIniptr->ANSI )
		return ( gpIniptr->ANSI & 1 );

_asm {
	mov	ax, 1A00h	; DOS 4+ "ANSI is installed" call
	int	2Fh
	mov	fAnsi, al
}

	return ( fAnsi == 0xFF );
}


// return the file system type for the specified (or default) drive
//	0 - old FAT (8.3)
//	1 - LFN or other
int ifs_type( char *drive )
{
	static char szSystemName[32];
	char lszRoot[4];
	unsigned int uRoot, uType;
	int rval = 0;

	// we have to let other DOS versions try this; they may have a TSR
	//   loaded to see the LFNs.
	if ( fWin95LFN == 0 )
		return 0;

	if (( drive != NULL ) && ( *drive == '"' ))
		drive++;

	// get the drive name (if no drive specified, get the default)
	sprintf( lszRoot, FMT_ROOT, (gcdisk( drive ) + 64) );
	uRoot = (UINT)lszRoot;
	uType = (UINT)szSystemName;

_asm {
	mov	ax, 71A0h
	push	ds
	pop	es
	mov	di, uType
	mov	dx, uRoot
	xor	bx, bx
	mov	cx, 32
	int	21h
	and	bx, 4000h
	jz	NoLFN
	mov	rval, 1
NoLFN:
}

	return rval;
}


// check if "fname" is a character device
int PASCAL QueryIsDevice( register char *fname )
{
	register int rval = 0;
	int nFH;
	char *source;

	// First, check for a network device ("\\server\sharename...")
	if (( fname[0] == '\\' ) && ( fname[1] == '\\' ))
		return 0;

	// check for CLIP: pseudo-device
	if ( stricmp( fname, CLIP ) == 0 )
		return 1;

	// ignore drive names & strip path & trailing colon from device name
	source = _alloca( strlen( fname ) + 1 );
	strcpy( source, fname );

	// don't strip ":" on drive specs ("A:")
	if ( strlen( source ) > 2 ) {
		strip_trailing( source, ":" );
		strcpy( source, fname_part( source ));
	}

	// make sure there's a filename part!
	if ( *source != '\0' ) {

		// shortcut to test NUL
		if ( stricmp( source, "NUL" ) == 0 ) {
			strcpy( fname, source );
			return 1;
		}

		DosError( 2 );
		if (( nFH = _sopen( source, (_O_RDONLY | _O_BINARY), _SH_DENYNO )) >= 0 ) {
			rval = _isatty( nFH );
			_close( nFH );
	        }
		DosError( 1 );

		// if it's a device, remove path and trailing ":"
		if ( rval )
			strcpy( fname, source );
	}

	return rval;
}


// return the true name (sees through JOIN & SUBST)
char * true_name( char *source, char *target )
{
	char szFilename[MAXFILENAME+1];

	// if no arg, or only a disk spec, return TRUEPATH of current directory
	if ((( source == NULL ) || (( source[1] == ':' ) && ( source[2] == '\0' ))) && (( source = gcdir( source, 0 )) == NULL ))
		return NULL;

	// strip include lists
	sscanf( source, "%[^;]", szFilename );
	source = szFilename;
	StripQuotes( szFilename );

	GetShortName( source );

_asm {
	mov	si, source
	mov	di, target
	push	ds
	pop	es
	mov	dx, di
	mov	ax, 06000h
	int	21h		; call true name function
	jnc	true_bye
}
	strcpy( target, source );
true_bye:

	return ( filecase( target ));
}


int GetLongName( char *szPathName )
{
	char szTemp[MAXFILENAME+1];
	int uSourceOffset, uDestOffset;

	// only works with LFN drives
	if ( ifs_type( szPathName ) == 0 )
		return 0;

	strcpy( szTemp, szPathName );
	uSourceOffset = (unsigned int)szPathName;
	uDestOffset = (unsigned int)szTemp;

_asm {
	push	di
	push	si

	mov	ax, 7160h	  ; kludge for Win95 "feature" of returning
	mov	cx, 8002h	  ;   the shortname!
	push	ds
	pop	es
	mov	si, uSourceOffset  ; source path
	mov	di, uDestOffset    ; destination path
	int	21h

	pop	si
	pop	di
}

	strcpy( szPathName, szTemp );
	return 1;
}


int GetShortName( char *szPathName )
{
	char szTemp[MAXFILENAME+1];
	int uSourceOffset, uDestOffset;

	// only works in Win95 w/LFNs enabled
	if ( fWin95LFN == 0 )
		return 1;

	strcpy( szTemp, szPathName );
	mkfname( szTemp, 0 );
	uSourceOffset = (unsigned int)szPathName;
	uDestOffset = (unsigned int)szTemp;

_asm {
	push	di
	push	si

	mov	ax, 7160h	  ; kludge for Win95 "feature" of returning
	mov	cl, 1		  ;   the shortname!
	mov	ch, 080h
	push	ds
	pop	es
	mov	si, uSourceOffset  ; source path
	mov	di, uDestOffset    ; destination path
	int	21h

	pop	si
	pop	di
}

	strcpy( szPathName, szTemp );
	return 1;
}


// get the country code (for date formatting)
void QueryCountryInfo( void )
{
	if (gnOsVersion >= 330 ) {
_asm {
		mov	ax, 06501h		; get international info
		mov	bx, -1
		mov	cx, 029h		; sizeof(gaCountryInfo)
		mov	dx, -1			; default country
		push	ds
		pop	es
		lea	di, gaCountryInfo
		int	21h
		jc	old_func
}
	} else {
_asm {
old_func:
		mov	ax,03800h
		lea	dx,gaCountryInfo
		add	dx,7		; skip extended info
		int	21h
}
	}

	// set am/pm info (if TimeFmt==0, use default country info)
	if ( gpIniptr->TimeFmt == 1 )		// 12-hour format
		gaCountryInfo.fsTimeFmt = 0;
	else if ( gpIniptr->TimeFmt == 2 )	// 24-hour format
		gaCountryInfo.fsTimeFmt = 1;
	else
		gaCountryInfo.fsTimeFmt &= 1;

	if ( gaCountryInfo.szThousandsSeparator[0] == '\0' ) {
		gaCountryInfo.szThousandsSeparator[0] = ',';
		gaCountryInfo.szDateSeparator[0] = '-';
		gaCountryInfo.szTimeSeparator[0] = ':';
	}

	if ( gpIniptr->DecimalChar )
		gaCountryInfo.szDecimal[0] = (( gpIniptr->DecimalChar == 1 ) ? '.' : ',');
	if ( gpIniptr->ThousandsChar )
		gaCountryInfo.szThousandsSeparator[0] = (( gpIniptr->ThousandsChar == 1 ) ? '.' : ',');

	// make sure the OS version string has the right characters			
	SetOSVersion();
}


void SetOSVersion( void )
{
	// set gszOsVersion, gszMyVersion, PROGRAM
	sprintf( gszOsVersion, "%u%c%02u", _osmajor, gaCountryInfo.szDecimal[0],_osminor );
	sprintf( gszMyVersion, "%u%c%02u", VER_MAJOR, gaCountryInfo.szDecimal[0], VER_MINOR );
	sprintf( PROGRAM, SET_PROGRAM, VER_MAJOR, gaCountryInfo.szDecimal[0], VER_MINOR );
}


// map character to upper case (including foreign language chars)
int _fastcall _ctoupper( int c )
{
	if ( c < 'a' )
		return c;

	if (( c >= 'a' ) && ( c <= 'z' ))
		return ( c - 32 );

	if (( c >= 0x80 ) && ( c <= 0xFF )) {

_asm {
		mov	ax, c
		call	far ptr gaCountryInfo.case_map_func
		xor	ah, ah
		mov	c, ax
}
	}

	return c;
}


// get various disk info (free space, total space, cluster size)
int QueryDiskInfo( char *drive, QDISKINFO * DiskInfo, int fNoErrors )
{
	unsigned int SectorsPerCluster, AvailClusters;
	unsigned int BytesPerSector, TotalClusters;
	unsigned int nDisk;
	FAT32 Fat32;
	char szFAT32Drive[4];
	char szBuf[48];

	// get TRUENAME (to see through JOIN & SUBST)
	// but DON'T do a TRUENAME on a remote drive!!
	if ( is_net_drive( drive ))
		goto QDFailure;

	nDisk = gcdisk( drive );
	if ( QueryDriveExists( nDisk ) == 0 )
		goto QDFailure;

	if ( QueryDriveRemote( nDisk ) == 0 ) {
		if (( drive = true_name( drive, first_arg( drive ))) == NULL )
			return ERROR_EXIT;
		nDisk = gcdisk( drive );
	}

	// If FAT32 info is available, use it
	if ( gpIniptr->MSDOS7 ) {

		sprintf( szFAT32Drive, FMT_ROOT, (nDisk + 64) );
		if (( Win95GetFAT32Info( szFAT32Drive, &Fat32, sizeof(FAT32)) == 0 ) && ( Fat32.ExtFree_BytesPerSector != 0L )) {

			// get string equivalents (for > 4Gb drives)
			sprintf( szBuf, "(%lu*%lu)*%lu", Fat32.ExtFree_BytesPerSector, Fat32.ExtFree_SectorsPerCluster, Fat32.ExtFree_TotalClusters );
			evaluate( szBuf );
			AddCommas( szBuf );
			strcpy( DiskInfo->szBytesTotal, szBuf );

			sprintf( szBuf, "(%lu*%lu)*%lu", Fat32.ExtFree_BytesPerSector, Fat32.ExtFree_SectorsPerCluster, Fat32.ExtFree_AvailableClusters );
			evaluate( szBuf );
			AddCommas( szBuf );
			strcpy( DiskInfo->szBytesFree, szBuf );

			DiskInfo->ClusterSize = Fat32.ExtFree_BytesPerSector * Fat32.ExtFree_SectorsPerCluster;
			DiskInfo->BytesTotal = DiskInfo->ClusterSize * Fat32.ExtFree_TotalClusters;
			DiskInfo->BytesFree = DiskInfo->ClusterSize * Fat32.ExtFree_AvailableClusters;
			return 0;
		}
	}
_asm {
	mov	dx, nDisk
	mov	ah, 36h			; Get Disk Free Space
	int	21h
	mov	SectorsPerCluster, ax
	mov	AvailClusters, bx
	mov	BytesPerSector, cx
	mov	TotalClusters, dx
}

	if ( SectorsPerCluster == 0xFFFF ) {
QDFailure:
		return (( fNoErrors ) ? ERROR_INVALID_DRIVE : error( ERROR_INVALID_DRIVE, drive ));
	}

	DiskInfo->BytesTotal = (unsigned long)BytesPerSector;
	DiskInfo->BytesTotal = (unsigned long)(DiskInfo->BytesTotal * SectorsPerCluster) * (unsigned long)TotalClusters;
	DiskInfo->BytesFree = (unsigned long)BytesPerSector;
	DiskInfo->BytesFree = (unsigned long)(DiskInfo->BytesFree * SectorsPerCluster) * (unsigned long)AvailClusters;

	// kludge for DR-DOS bug
	// We can't just do a Bytes * Sectors because of a Win95 65K/track!
	DiskInfo->ClusterSize = BytesPerSector;
	if (( DiskInfo->ClusterSize *= SectorsPerCluster ) == 0 )
		DiskInfo->ClusterSize = -1L;

	// get string equivalents (for > 4Gb drives)
	sprintf( szBuf, "(%u*%u)*%u", BytesPerSector, SectorsPerCluster, TotalClusters );
	evaluate( szBuf );
	AddCommas( szBuf );
	strcpy( DiskInfo->szBytesTotal, szBuf );

	sprintf( szBuf, "(%u*%u)*%u", BytesPerSector, SectorsPerCluster, AvailClusters );
	evaluate( szBuf );
	AddCommas( szBuf );
	strcpy( DiskInfo->szBytesFree, szBuf );

	return 0;
}


// check to see if the specified name is a named pipe
int QueryIsPipeName( char *pszPipeName )
{
	// skip server name ("\\server\pipe\...")
	if (( pszPipeName[0] == '\\' ) && ( pszPipeName[1] == '\\' )) {
		if (( pszPipeName = strchr( pszPipeName+2, '\\' )) == NULL )
			pszPipeName = NULLSTR;
	}

	// if "\pipe\..." & no wildcards, assume named pipe exists
	return (( strnicmp( pszPipeName, "\\PIPE\\", 6 ) == 0 ) && ( strpbrk( pszPipeName, WILD_CHARS ) == NULL ));
}


// create a unique filename (based on system date & time)
int UniqueFileName( char *szFname )
{
	register int fd, i;
	char buffer[MAXFILENAME];

	copy_filename( buffer, szFname );
	mkdirname( buffer, NULLSTR );

	for ( i = 0; ( i <= 0xFFF ); i++ ) {

		// create a temporary filename using the Random routine
		sprintf( strend( buffer ), "%08lx.%03x", GetRandom( 0L, 0x7FFFFFFFL ), i);

		// try to create the temporary file
		errno = 0;
		if ((( fd = _sopen( buffer, (_O_BINARY | _O_CREAT | _O_EXCL | _O_WRONLY), _SH_COMPAT, (_S_IREAD | _S_IWRITE))) >= 0 ) || ((errno != EEXIST) && (errno != EACCES)))
			break;
	}

	if ( fd >= 0 ) {
		_close( fd );
		if (errno == 0 ) {
			strcpy( szFname, buffer );
			return 0;
		}
	}

	return _doserrno;
}



// convert Win32 FILETIME format to DOS file time format
int PASCAL FileTimeToDOSTime( PFILETIME pFT, USHORT *pusTime, USHORT *pusDate )
{
	UINT rval;

	rval = (UINT)pFT;

_asm {
	push	si
	mov	ax, 71A7h	; date & time format conversion
	mov	bx, 0
	mov	si, rval
	int	21h
	mov	rval, ax
	jc	FT_Error
	mov	bx, pusTime
	mov	[bx], cx
	mov	bx, pusDate
	mov	[bx], dx
	mov	rval, 0
FT_Error:
	pop	si
}

	return rval;
}


// Find the first or next matching file
//	fflag	Find First or Find Next flag
//	arg	filename to search for
//	attrib	search attribute to use - high byte has special purpose:
//		(FIND_NO_ERRORS) don't display any error messages
//		(FIND_ONLY_DIRS) only return directories
//		(FIND_BYATTS) check inclusive/exclusive match
//		(FIND_RANGE) check date / time range
//		(FIND_EXCLUDE) check exclusions
//              (FIND_CLOSE) close global search handle
//		(FIND_NO_CASE) don't do case conversions
//		(FIND_NO_DOTNAMES) don't return "." or ".."
//		(FIND_EXACTLY) no include lists or wildcards
//		(FIND_NO_FILELISTS) don't look for @lists
//	dir	pointer to directory structure
//    filename  pointer to place to return filename

char * PASCAL find_file( int fflag, register char *arg, unsigned long attrib, FILESEARCH *dir, char *filename )
{
	static char DELIMS[] = "%255[^;]%n";
	register char *ptr;
	char *pszFileName, *pszNext, *pszNamePart;
	int rval, fval = fflag, i, mode, nPathLength, fWildBrackets = 0;
	unsigned int uTime, uDate, fUnicode = 0, fWildcards = 1;
	unsigned long ulDTRange;
	WIN32_FIND_DATA *pWin95Dir;

	if ( fval == FIND_FIRST ) {

	    // check for @filename syntax first
	    dir->lFileOffset = -1L;
	    dir->pszFileList = NULL;

	    if ( strpbrk( arg, WILD_CHARS ) == NULL )
		fWildcards = 0;

	    // check for @filename syntax first
	    if ((( attrib & ( FIND_NO_FILELISTS | FIND_EXACTLY )) == 0 ) && ( filename != NULL ) && ( fWildcards == 0 )) {

		i = ((( ptr = path_part( arg )) != NULL ) ? strlen( ptr ) : 0 );

		// see if the file exists with the '@'
		if (( *arg == '@' ) || ( arg[ i ] == '@' )) {
	
			// if the file exists with the @ prefix, leave it alone!
			if ( _dos_getfileattr( arg, &i ) != 0 ) {

				// remove the '@' and look again
				if ( *arg == '@' )
					i = 0;
				dir->pszFileList = strdup( arg );

				strcpy( dir->pszFileList + i, dir->pszFileList + i + 1 );

				// if it's a device or a valid filename, set it up to read
				//   input from that device/file
				if ( dir->pszFileList[0] ) {
						
					if ( QueryIsDevice( dir->pszFileList ) != 0 ) {

						// need to allocate larger filename buffer because it might be @CLIP:
						ptr = malloc( MAXFILENAME );
						strcpy( ptr, dir->pszFileList );
						free( dir->pszFileList );
						dir->pszFileList = ptr;

						// read the first line
						dir->lFileOffset = 0L;
						goto ReturnAtFile;
							
					} else if ( _dos_getfileattr( dir->pszFileList, &i ) == 0 ) {
						// read the first line
						dir->lFileOffset = 0L;
						goto ReturnAtFile;
					}
				}					
			}
		}
	    }

	    if (( dir->fLFN = ifs_type( arg )) != 0 ) {
		// create directory search handle?
		dir->hdir = INVALID_HANDLE_VALUE;
	    }

	} else if ( dir->lFileOffset != -1L ) {
ReturnAtFile:
		// read next line from @filename
		GetFileLine( dir->pszFileList, &(dir->lFileOffset), filename );
		if ( *filename == '\0' ) {
			free( dir->pszFileList );
			return NULL;
		} else {
			StripQuotes( filename );
			return filename;
		}
	}

	if ( dir->fLFN )
		pWin95Dir = _alloca( sizeof(WIN32_FIND_DATA) );

	pszFileName = _alloca( strlen( arg ) + 1 );

next_list:
	// Check for include list by looking for ';' and extracting first name
	nPathLength = ((( ptr = path_part( arg )) != NULL ) ? strlen( ptr ) + (int)( *arg == '"' ) : 0 );
	pszNamePart = arg + nPathLength;

	if ( attrib & FIND_EXACTLY ) {
		copy_filename( pszFileName, arg );
		pszNext = NULLSTR;
	} else {

		if (( ptr = scan( arg+nPathLength, ";", "\"[" )) == BADQUOTES )
			return NULL;
		i = (int)( ptr - arg );
		sprintf( pszFileName, FMT_PREC_STR, i, arg );

		if ( arg[i] )		// skip the ';'
			i++;

		// point "pszNext" to next name in include list (or EOS)
		pszNext = arg + i;

		// This is yet another stupid kludge for illegal DOS syntax used
		//   by Netware ( [:, \:, ]:, _:, and ': )
		ptr = (( pszFileName[1] == ':' ) ? pszFileName + 2 : pszFileName );

		// We have to get tricky here because 4DOS supports extended
		//   wildcards (for example: "*m*s*.*d*").  MS-DOS & PC-DOS
		//   handle them all right (by ignoring them!), but DR-DOS doesn't.
		//   (MS-DOS 7 (in Win95) supports the extended wildcard syntax)
		if ( fWin95 == 0 ) {

		    for ( ; (( ptr = strchr( ptr, '*' )) != NULL ); ) {

			for ( ptr++; (( *ptr != '\0' ) && ( *ptr != '.' ) && ( *ptr != '[' ) && ( *ptr != gpIniptr->EscChr )); )
				strcpy( ptr, ptr+1 );
		    }
		}

		// force proper filename size (i.e., "123456789" -> "12345678")
		strcpy( pszFileName + nPathLength, fname_part( pszFileName ) );

		// if not a FIND_FIRST, set the flag to treat brackets as wildcards
		if (( strchr( pszFileName, '[' ) != NULL ) && ( fval == FIND_NEXT ))
			fWildBrackets = 1;
	}

	// check for LFNs delineated by double quotes
	//   and strip leading / trailing " (need to do it here as
	//   well as in mkfname() for things like IF EXIST "file 1")
	StripQuotes( pszFileName );

	// check for DR-DOS password
	if ( *pszNext == ';' ) {

		strcat( pszFileName, ";" );
		sscanf( ++pszNext, DELIMS, strend( pszFileName ), &i );

		// skip include list entry
		if ( pszNext[i] )
			i++;
		pszNext += i;
	}

	for ( mode = (int)( attrib & 0x7F ); ; ) {

RetryFindFirst:
		// search for the next matching file
		// if no wildcards in fname, don't bother with _dos_findnext()
		if (( dir->fLFN == 0 ) || ( mode & 0x08 )) {

		    if ( fval == FIND_FIRST )
			rval = _dos_findfirst( pszFileName, mode, (struct find_t *)dir);
		    else if ( strpbrk( pszFileName, WILD_CHARS ) == NULL )
			rval = ERROR_FILE_NOT_FOUND;
		    else
			rval = _dos_findnext( (struct find_t *)dir );

		    // kludge for Netware & NFS bug (directory names not always
		    //   terminated)
		    dir->szFileName[12] = '\0';
		    dir->szAlternateFileName[0] = '\0';

		} else {

		    // get Win95 LFNs
		    // search for the next matching file
		    if ( fval == FIND_FIRST ) {

			if ((( dir->hdir = FindFirstFile( pszFileName, mode, (LPWIN32_FIND_DATA)pWin95Dir, &fUnicode )) == INVALID_HANDLE_VALUE ) || ( dir->hdir == 0 )) {
				rval = GetLastError();
				dir->hdir = INVALID_HANDLE_VALUE;
			} else
				rval = 0;

		    } else if ( strpbrk( pszFileName, WILD_CHARS ) == NULL )
			rval = ERROR_FILE_NOT_FOUND;
		    else
			rval = (( FindNextFile( dir->hdir, (LPWIN32_FIND_DATA)pWin95Dir, &fUnicode ) == TRUE) ? 0 : GetLastError() );

		    // move data from Win32 structure to DOS structure
		    if ( rval == 0 ) {

			// check for untranslated Unicode characters
			strcpy( dir->szFileName, (( fUnicode == 1 ) ? pWin95Dir->szAlternateFileName : pWin95Dir->cFileName ));
			strcpy( dir->szAlternateFileName, pWin95Dir->szAlternateFileName );
			dir->attrib = (char)(pWin95Dir->dwFileAttributes);
			dir->ulSize = pWin95Dir->nFileSizeLow;

			// save FILETIME Creation & LastAccess fields
			dir->ftLastWriteTime.dwLowDateTime = pWin95Dir->ftLastWriteTime.dwLowDateTime;
			dir->ftLastWriteTime.dwHighDateTime = pWin95Dir->ftLastWriteTime.dwHighDateTime;
			dir->ftLastAccessTime.dwLowDateTime = pWin95Dir->ftLastAccessTime.dwLowDateTime;
			dir->ftLastAccessTime.dwHighDateTime = pWin95Dir->ftLastAccessTime.dwHighDateTime;
			dir->ftCreationTime.dwLowDateTime = pWin95Dir->ftCreationTime.dwLowDateTime;
			dir->ftCreationTime.dwHighDateTime = pWin95Dir->ftCreationTime.dwHighDateTime;

			// set DOS file time
			FileTimeToDOSTime( &(pWin95Dir->ftLastWriteTime ), &(dir->ft.wr_time), &(dir->fd.wr_date) );
		    }
		}

		if ( rval != 0 ) {

			// couldn't find file; if FIND_FIRST, display error
			if (( fval == FIND_FIRST ) && (( attrib & FIND_EXACTLY ) == 0 )) {

				// 4DOS handles UNIX-style "[a-c]" syntax which
				//   neither COMMAND.COM nor DOS recognize.
				//   But first, we have to try to match []'s!
				ptr = (( pszFileName[1] == ':' ) ? pszFileName + 2 : pszFileName );
				for ( i = 0; (( ptr = strchr( ptr, '[' )) != NULL ); ) {

					// replace [] with a single '?'
					*ptr++ = '?';
					while ( *ptr != '\0' ) {
						i = *ptr;
						strcpy( ptr, ptr+1 );
						if ( i == ']' )
							break;
					}
				}

				if ( i == ']' ) {
					fWildBrackets = 1;
					goto RetryFindFirst;
				}
			}

			// couldn't find file; if FIND_FIRST, display error
			if (( fflag == FIND_FIRST ) || (( rval != ERROR_FILE_NOT_FOUND ) && ( rval != ERROR_NO_MORE_FILES ))) {

				if (( attrib & FIND_NO_ERRORS ) == 0 ) {

					// kludge to rebuild original name;
					//   otherwise *WOK.WOK would display
					//   as *.WOK
					if (( i = (int)( scan( arg, ";", "\"[" ) - arg)) > MAXFILENAME - 1 )
						i = MAXFILENAME - 1;
					sprintf( pszFileName, FMT_PREC_STR, i, arg );
					error( rval, pszFileName );
				}
			}

			// close search handle
			HoldSignals();
			if ( dir->hdir != INVALID_HANDLE_VALUE ) {
				FindClose( dir->hdir );
				dir->hdir = INVALID_HANDLE_VALUE;
			}
			EnableSignals();
			
			// check include list for another argument
			if ( *pszNext ) {

				// remove the previous name & get next
				//   one from include list
				strcpy( pszNamePart, pszNext );

				fval = FIND_FIRST;
				goto next_list;
			}

			return NULL;
		}

		fval = FIND_NEXT;

		// skip "." and ".."?
		if (( attrib & FIND_NO_DOTNAMES ) && ( QueryIsDotName( dir->szFileName )))
			continue;

		// only retrieving directories? (ignore "." and "..")
		if (( attrib & FIND_ONLY_DIRS ) && ((( dir->attrib & _A_SUBDIR ) == 0 ) || ( QueryIsDotName( dir->szFileName ) != 0 )))
			continue;

		// check for inclusive / exclusive matches
		if ( attrib & FIND_BYATTS ) {

			if ( gnOrMode ) {
			
				// check for "this or that attribute"
				if (( gnOrMode & ((( dir->attrib & 0x3F ) == 0 ) ? 0x1000 : dir->attrib )) == 0 )
					continue;
			}

			// retrieve files with specified attributes?
			// or don't retrieve files with specified attributes?
			if ((( gnInclusiveMode & dir->attrib ) != gnInclusiveMode ) || (( gnExclusiveMode & dir->attrib ) != 0 ))
				continue;
		}

		// check for filename exclusions
		if ( attrib & FIND_EXCLUDE ) {
			if (( dir->aRanges.pszExclude != NULL ) && ( ExcludeFiles( dir->aRanges.pszExclude, dir->szFileName ) == 0 ))
				continue;
		}

		// check for date / time / size ranges
		if ( attrib & FIND_RANGE ) {

			if ( dir->aRanges.usDateType == 0 ) {
				uDate = dir->fd.wr_date;
				uTime = dir->ft.wr_time;
			} else if ( dir->aRanges.usDateType == 1 )
				FileTimeToDOSTime( &(dir->ftLastAccessTime), &uTime, &uDate );
			else
				FileTimeToDOSTime( &(dir->ftCreationTime), &uTime, &uDate );

			ulDTRange = (( long)uDate << 16 ) + uTime;
			if (( ulDTRange < dir->aRanges.DTRS.ulDTStart ) || ( ulDTRange > dir->aRanges.DTRE.ulDTEnd ))
				continue;

			if ( dir->aRanges.usTimeType == 0 )
				uTime = dir->ft.wr_time;
			else if ( dir->aRanges.usTimeType == 1 )
				FileTimeToDOSTime( &(dir->ftLastAccessTime), &uTime, &uDate );
			else
				FileTimeToDOSTime( &(dir->ftCreationTime), &uTime, &uDate );

			// strip seconds
			uTime &= 0xFFE0;

			// if start > end, we wrapped around midnight
			if ( dir->aRanges.usTimeEnd < dir->aRanges.usTimeStart ) {

				if (( uTime < dir->aRanges.usTimeStart ) && ( uTime > dir->aRanges.usTimeEnd ))
					continue;

			} else if (( uTime < dir->aRanges.usTimeStart ) || ( uTime > dir->aRanges.usTimeEnd ))
				continue;

			if (( dir->ulSize < dir->aRanges.ulSizeMin ) || ( dir->ulSize > dir->aRanges.ulSizeMax ))
				continue;

			// Check for /I (description match)
			if ( dir->aRanges.pszDescription ) {

				char szTempName[MAXFILENAME+1], szBuffer[512];

				szBuffer[0] = '\0';

				insert_path( szTempName, dir->szFileName, arg );
				process_descriptions( szTempName, szBuffer, DESCRIPTION_READ );
                	        if ( wild_cmp( dir->aRanges.pszDescription, szBuffer, FALSE, TRUE ) != 0 )
					continue;
			}
		}

		// check unusual wildcard matches
		if (( stricmp( pszNamePart, dir->szFileName ) == 0 ) || ( wild_cmp( (char _far *)pszNamePart, (char _far *)dir->szFileName, TRUE, fWildBrackets ) == 0 ) || ( attrib & _A_VOLID ))
			break;

		// Win95: try for a match on the SFN
		if (( fWin95SFN ) && ( dir->fLFN ) && ( wild_cmp( (char _far *)pszNamePart, (char _far *)dir->szAlternateFileName, TRUE, fWildBrackets ) == 0 ))
			break;
	}

	// close global search handle
	if ( attrib & FIND_CLOSE ) {
		FindClose( dir->hdir );
		dir->hdir = INVALID_HANDLE_VALUE;
	}

	// if no target filename requested, just return a non-NULL response
	if ( filename == NULL ) {

		// if no case conversion wanted, just return
		if (( attrib & FIND_NO_CASE ) == 0 ) {
		    if (( gpIniptr->Upper == 0 ) && ( dir->fLFN == 0 ) && (( attrib & _A_VOLID) == 0 ))
			strlwr( dir->szFileName );
		}
		return (char *)-1;
	}

	// copy the saved source path
	insert_path( filename, ((( attrib & FIND_SFN ) && ( dir->szAlternateFileName[0] )) ? dir->szAlternateFileName : dir->szFileName ), pszFileName );

	// put the DR-DOS password back
	if (( ptr = strchr( pszFileName, ';' )) != NULL )
		strcat( filename, ptr );

	// return matching filename shifted to upper or lower case
	return ( filecase( filename ));
}


// map string to upper case (including foreign language chars)
char * strupr( char *pszSource )
{
	register char *arg;

	for ( arg = pszSource; (*arg != '\0' ); arg++)
		*arg = (char)_ctoupper( *arg );

	return pszSource;
}


// Set Win95 LFN/SFN flags which may be reset by OPTION
void SetWin95Flags( void )
{
	fWin95LFN = fWin95SFN = 0;
	if ((( fWin95 ) && ( gpIniptr->Win95LFN == 2 )) || ( gpIniptr->Win95LFN == 1 ))
		fWin95LFN = 1;
	if ((( fWin95 ) && ( gpIniptr->Win95SFN == 2 )) || ( gpIniptr->Win95SFN == 1 ))
		fWin95SFN = 1;
}

