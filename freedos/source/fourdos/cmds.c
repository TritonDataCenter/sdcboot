

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


// CMDS.C - Table of internal commands for 4dos
//   (c) 1988 - 2002 Rex C. Conn  All rights reserved

#include "product.h"

#include <stdio.h>
#include <string.h>

#include "4all.h"


// Structure for the internal commands
// The args are:
//	command name
//	pointer to function
//	enabled/disabled flag (see SETDOS /I)
//	parsing flag, where bit flags are:
//		CMD_EXPAND_VARS - expand variables
//		CMD_EXPAND_REDIR - do redirection
//		CMD_STRIP_QUOTES - strip quoting
//		CMD_ADD_NULLS - add terminators to each arg (batch)
//		CMD_GROUPS - check for command groups
//		CMD_CLOSE_BATCH - close batch file before executing command
//		CMD_ONLY_BATCH - command only allowed in a batch file
//		CMD_DISABLED - command disabled (SETDOS /I-)
//		CMD_RESET_DISKS - reset disks upon return (4DOS only)
//		CMD_SET_ERRORLEVEL - set ERRORLEVEL upon return (NT only)
//		CMD_BACKSLASH_OK - allow trailing backslash
//		CMD_DETACH_LINE - send entire line to DETACH command
//		CMD_UCASE_CMD
//		CMD_EXTERNAL
//		CMD_HELP - if != 0, check for /? anywhere on line
//		CMD_PERIOD_OK - accept trailing '.' as part of command

BUILTIN commands[] = {
	_TEXT("?"), Cmds_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES),
	_TEXT("ALIAS"), Alias_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH),
	_TEXT("ATTRIB"), Attrib_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("BEEP"), Beep_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("BREAK"), Break_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("CALL"), Call_Cmd, (CMD_CLOSE_BATCH | CMD_BACKSLASH_OK ),
	_TEXT("CANCEL"), Cancel_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP),
	_TEXT("CASE"),  Case_Cmd, CMD_ONLY_BATCH, 
	_TEXT("CD"), Cd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("CDD"), Cdd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("CHCP"), Chcp_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("CHDIR"), Cd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("CLS"), Cls_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("COLOR"), Color_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("COPY"), Copy_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	"CTTY", Ctty_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("DATE"), Date_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("DEFAULT"),  Case_Cmd, CMD_ONLY_BATCH, 
	_TEXT("DEL"), Del_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	_TEXT("DELAY"), Delay_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("DESCRIBE"), Describe_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS  | CMD_PERIOD_OK ),
	_TEXT("DIR"), Dir_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("DIRHISTORY"), DirHistory_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP),
	_TEXT("DIRS"), Dirs_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("DO"), Do_Cmd, (CMD_GROUPS | CMD_STRIP_QUOTES | CMD_ONLY_BATCH),
	_TEXT("DRAWBOX"), Drawbox_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("DRAWHLINE"), DrawHline_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("DRAWVLINE"), DrawVline_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("ECHO"), Echo_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK  | CMD_PERIOD_OK ),
	_TEXT("ECHOERR"), EchoErr_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK  | CMD_PERIOD_OK ),
	_TEXT("ECHOS"), Echos_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES  | CMD_PERIOD_OK ),
	_TEXT("ECHOSERR"), EchosErr_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES  | CMD_PERIOD_OK ),
	_TEXT("ENDLOCAL"), Endlocal_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH),
	_TEXT("ENDSWITCH"),  Remark_Cmd, CMD_ONLY_BATCH, 
	_TEXT("ERASE"), Del_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	_TEXT("ESET"), Eset_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("EXCEPT"), Except_Cmd, CMD_GROUPS | CMD_HELP,
	_TEXT("EXIT"), Exit_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("FFIND"), Ffind_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES),
	_TEXT("FOR"), For_Cmd, (CMD_GROUPS | CMD_CLOSE_BATCH),
	_TEXT("FREE"), Free_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("FUNCTION"), Function_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH),
	_TEXT("GLOBAL"), Global_Cmd,CMD_GROUPS | CMD_HELP,
	_TEXT("GOSUB"), Gosub_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP),
	_TEXT("GOTO"), Goto_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP),
	_TEXT("HEAD"), Head_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP),
	_TEXT("HELP"), Help_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("HISTORY"), History_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP),
	_TEXT("IF"), If_Cmd, CMD_GROUPS,
	_TEXT("IFF"), Iff_Cmd, CMD_GROUPS,
	_TEXT("INKEY"), Inkey_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES ),
	_TEXT("INPUT"), Input_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES ),
	_TEXT("KEYBD"), Keybd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("KEYSTACK"), Keystack_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH),
	"LFNFOR", LfnFor_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP ),
	"LH", Loadhigh_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("LIST"), List_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("LOADBTM"), Loadbtm_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_CLOSE_BATCH | CMD_HELP),
	"LOADHIGH", Loadhigh_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK | CMD_HELP),
	"LOCK", Lock_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("LOG"), Log_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES),
	_TEXT("MD"), Md_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("MEMORY"), Memory_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("MKDIR"), Md_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("MOVE"), Mv_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("ON"), On_Cmd, (CMD_ONLY_BATCH | CMD_GROUPS | CMD_HELP),
	_TEXT("OPTION"), Option_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES ),
	_TEXT("PATH"), Path_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("PAUSE"), Pause_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH),
	_TEXT("POPD"), Popd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("PROMPT"), Prompt_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK),
	_TEXT("PUSHD"), Pushd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	_TEXT("QUIT"), Quit_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("RD"), Rd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_RESET_DISKS | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("REBOOT"), Reboot_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("REM"), Remark_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK  | CMD_PERIOD_OK ),
	_TEXT("REN"), Ren_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	_TEXT("RENAME"), Ren_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP  | CMD_PERIOD_OK ),
	_TEXT("RETURN"), Ret_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP),
	_TEXT("RMDIR"), Rd_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("SCREEN"), Scr_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES),
	_TEXT("SCRPUT"), Scrput_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR),
	_TEXT("SELECT"), Select_Cmd,CMD_GROUPS | CMD_HELP,
	_TEXT("SET"), Set_Cmd, ( CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES ),
	_TEXT("SETDOS"), Setdos_Cmd, (CMD_EXPAND_VARS | CMD_STRIP_QUOTES | CMD_EXPAND_REDIR),
	_TEXT("SETLOCAL"), Setlocal_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("SHIFT"), Shift_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP),
	_TEXT("START"), Start_Cmd, (CMD_EXPAND_VARS | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_PERIOD_OK ),
	"SWAPPING", Swap_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("SWITCH"),  Switch_Cmd, (CMD_EXPAND_VARS | CMD_STRIP_QUOTES | CMD_ONLY_BATCH | CMD_HELP), 
	_TEXT("TAIL"), Tail_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP),
	_TEXT("TEE"), Tee_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP),
	_TEXT("TEXT"), Battext_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_ONLY_BATCH ),
	_TEXT("TIME"), Time_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("TIMER"), Timer_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("TOUCH"), Touch_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("TREE"), Tree_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP | CMD_PERIOD_OK ),
	_TEXT("TRUENAME"), Truename_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("TYPE"), Type_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_CLOSE_BATCH | CMD_BACKSLASH_OK | CMD_HELP),
	_TEXT("UNALIAS"), Unalias_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("UNFUNCTION"), Unfunction_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	"UNLOCK", Unlock_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("UNSET"), Unset_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("VER"), Ver_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("VERIFY"), Verify_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("VOL"), Volume_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_RESET_DISKS | CMD_SET_ERRORLEVEL | CMD_HELP),
	_TEXT("VSCRPUT"), VScrput_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR),
	_TEXT("WHICH"), Which_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_HELP),
	_TEXT("Y"), Y_Cmd, (CMD_EXPAND_VARS | CMD_EXPAND_REDIR | CMD_STRIP_QUOTES | CMD_CLOSE_BATCH | CMD_HELP)
};


// # of internal cmds
#define NUMCMDS (sizeof(commands)/sizeof(BUILTIN))


int QueryNumCmds( void )
{
	return NUMCMDS;
}


// display the enabled internal commands
int _near Cmds_Cmd( TCHAR *pszCmdLine )
{
	register int i, nColumn, nMaxColumns;

	nMaxColumns = GetScrCols();

	for ( i = 0, nColumn = 0; ( i < NUMCMDS ); i++ ) {

		// make sure command hasn't been disabled (SETDOS /I-cmd)
		if ((commands[i].fParse & CMD_DISABLED) == 0) {

			printf( FMT_STR, commands[i].szCommand );

			// get width of display
			if (( ++nColumn % ( nMaxColumns / 12 )) != 0 )
				printf( FMT_LEFT_STR, 12 - strlen( commands[i].szCommand ), NULLSTR );
			else {
				crlf();
			}
		}
	}

	if (( nColumn % ( nMaxColumns / 12 )) != 0 )
		crlf();

	return 0;
}


// do binary search to find command in internal command table & return index
int _fastcall findcmd( TCHAR *pszCommand, int fAlwaysFind )
{
	static TCHAR DELIMS[] = _TEXT("%10[^    \t;,.\"`\\+=<>|]");
	register int nLow, nHigh, nMid, nCondition;
	TCHAR szInternalName[12];

	// set the current compound command character & switch character
	DELIMS[5] = gpIniptr->CmdSep;
	DELIMS[6] = gpIniptr->SwChr;

	// extract the command name (first argument)
	//   (including nasty kludge for nasty people who do "echo:"
	//   and a minor kludge for "y:")
	DELIMS[7] = (( pszCommand[1] == _TEXT(':') ) ? _TEXT(' ') : _TEXT(':') );
	sscanf( pszCommand, DELIMS, szInternalName );

	// do a binary search for the command name
	for ( nLow = 0, nHigh = ( NUMCMDS - 1 ); ( nLow <= nHigh ); ) {

		nMid = ( nLow + nHigh ) / 2;

		if (( nCondition = _stricmp( szInternalName, commands[nMid].szCommand )) < 0 )
			nHigh = nMid - 1;
		else if ( nCondition > 0 )
			nLow = nMid + 1;
		else {
			// kludge for trailing '\' (i.e., "TEXT\")
			if ((( commands[nMid].fParse & CMD_BACKSLASH_OK ) == 0 ) && ( pszCommand[ strlen(szInternalName) ] == _TEXT('\\') ))
				return -1;
			if ((( commands[nMid].fParse & CMD_PERIOD_OK ) == 0 ) && ( pszCommand[ strlen(szInternalName) ] == _TEXT('.') ))
				return -1;
			return (((( commands[nMid].fParse & CMD_DISABLED) == 0 ) || fAlwaysFind ) ? nMid : -1 );
		}
	}

	return -1;
}
