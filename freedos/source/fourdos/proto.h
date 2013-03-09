

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  (1) The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  (2) The Software, or any portion of it, may not be compiled for use on any
//  operating system OTHER than FreeDOS without written permission from Rex Conn
//  <rconn@jpsoft.com>
//
//  (3) The Software, or any portion of it, may not be used in any commercial
//  product without written permission from Rex Conn <rconn@jpsoft.com>
//
//  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.


// Function prototypes for 4xxx family
//   Copyright 1993 - 2003 Rex C. Conn

// Prototypes for internal commands

int _near Alias_Cmd( LPTSTR );
int _near Attrib_Cmd( LPTSTR );
int _near Batch( int, TCHAR ** );
int _near Battext_Cmd( LPTSTR );
int _near Beep_Cmd( LPTSTR );
int _near Break_Cmd( LPTSTR );
int _near Call_Cmd( LPTSTR );
int _near Cancel_Cmd( LPTSTR );
int _near Case_Cmd( LPTSTR );
int _near Cd_Cmd( LPTSTR );
int _near Cdd_Cmd( LPTSTR );

int _near Chcp_Cmd( LPTSTR );

int _near Cls_Cmd( LPTSTR );
int _near Cmds_Cmd( LPTSTR );
int _near Color_Cmd( LPTSTR );
int _near Copy_Cmd( LPTSTR );

int _near Ctty_Cmd( LPTSTR );

int _near Date_Cmd( LPTSTR );

int _near Del_Cmd( LPTSTR );
int _near Delay_Cmd( LPTSTR );
int _near Describe_Cmd( LPTSTR );

int _near Dir_Cmd( LPTSTR );
int _near DirHistory_Cmd( LPTSTR );
int _near Dirs_Cmd( LPTSTR );
int _near Do_Cmd( LPTSTR );
int _near Drawbox_Cmd( LPTSTR );
int _near DrawHline_Cmd( LPTSTR );
int _near DrawVline_Cmd( LPTSTR );
int _near Echo_Cmd( LPTSTR );
int _near EchoErr_Cmd( LPTSTR );
int _near Echos_Cmd( LPTSTR );
int _near EchosErr_Cmd( LPTSTR );
int _near Endlocal_Cmd( LPTSTR );
int _near Eset_Cmd( LPTSTR );
int _near Eventlog_Cmd( LPTSTR );
int _near Except_Cmd( LPTSTR );
int _near Exit_Cmd( LPTSTR );
int _near External( LPTSTR, LPTSTR );
int _near Ffind_Cmd( LPTSTR );
int _near For_Cmd( LPTSTR );
int _near Free_Cmd( LPTSTR );

int _near Function_Cmd( LPTSTR );
int _near Global_Cmd( LPTSTR );
int _near Gosub_Cmd( LPTSTR );
int _near Goto_Cmd( LPTSTR );
int _near Head_Cmd( LPTSTR );
int _near Help_Cmd( LPTSTR );
int _near History_Cmd( LPTSTR );
int _near If_Cmd( LPTSTR );
int _near Iff_Cmd( LPTSTR );

int _near Inkey_Cmd( LPTSTR );
int _near Input_Cmd( LPTSTR );
int _near Keybd_Cmd( LPTSTR );

int _near Keystack_Cmd( LPTSTR );
int _near List_Cmd( LPTSTR );
int _near Loadbtm_Cmd( LPTSTR );

int _near LfnFor_Cmd( LPTSTR );
int _near Loadhigh_Cmd( LPTSTR );
int _near Lock_Cmd( LPTSTR );

int _near Log_Cmd( LPTSTR );
int _near Md_Cmd( LPTSTR );
int _near Memory_Cmd( LPTSTR );

int _near Mv_Cmd( LPTSTR );
int _near On_Cmd( LPTSTR );
int _near Option_Cmd( LPTSTR );
int _near Path_Cmd( LPTSTR );
int _near Pause_Cmd( LPTSTR );

int _near Popd_Cmd( LPTSTR );

int _near Prompt_Cmd( LPTSTR );
int _near Pushd_Cmd( LPTSTR );

int _near Quit_Cmd( LPTSTR );
int _near Rd_Cmd( LPTSTR );
int _near Reboot_Cmd( LPTSTR );
int _near Recycle_Cmd( LPTSTR );
int _near Remark_Cmd( LPTSTR );
int _near Ren_Cmd( LPTSTR );
int _near Ret_Cmd( LPTSTR );

int _near Scr_Cmd( LPTSTR );
int _near Scrput_Cmd( LPTSTR );
int _near Set_Cmd( LPTSTR );
int _near Select_Cmd( LPTSTR );

int _near Setdos_Cmd( LPTSTR );
int _near Setlocal_Cmd( LPTSTR );
int _near Shift_Cmd( LPTSTR );

int _near Start_Cmd( LPTSTR );

int _near Swap_Cmd( LPTSTR );

int _near Switch_Cmd( LPTSTR );
int _near Tail_Cmd( LPTSTR );

int _near Tee_Cmd( LPTSTR );
int _near Time_Cmd( LPTSTR );
int _near Timer_Cmd( LPTSTR );

int _near Touch_Cmd( LPTSTR );
int _near Tree_Cmd( LPTSTR );
int _near Truename_Cmd( LPTSTR );
int _near Type_Cmd( LPTSTR );

int _near Unlock_Cmd( LPTSTR );

int _near Unalias_Cmd( LPTSTR );
int _near Unfunction_Cmd( LPTSTR );
int _near Unset_Cmd( LPTSTR );
int _near Ver_Cmd( LPTSTR );
int _near Verify_Cmd( LPTSTR );
int _near Volume_Cmd( LPTSTR );
int _near VScrput_Cmd( LPTSTR );
int _near Which_Cmd( LPTSTR );

int _near Y_Cmd( LPTSTR );


// Support routines in all versions

// BATCH.C
int _near ExitBatchFile( void );
int  _fastcall do_parsing( LPTSTR );
int  _fastcall iff_parsing( LPTSTR, LPTSTR );
int _fastcall TestCondition( LPTSTR, int );


// CMDS.C
int _fastcall findcmd( LPTSTR, int );
int QueryNumCmds( void );


// DIRCMDS.C
void init_dir( void );
void init_page_size( void );
LPTSTR _fastcall GetSearchAttributes( LPTSTR );
LPTSTR  _fastcall dir_sort_order( LPTSTR );
void _fastcall dir_free( DIR_ENTRY _huge * );
void _page_break( void );
void ColorizeDirectory( DIR_ENTRY _huge *, unsigned int, int );
int PASCAL fstrcmp( TCHAR _far *, TCHAR _far *, int );
int PASCAL SearchDirectory( long, LPTSTR, DIR_ENTRY _huge **, unsigned int *, RANGES *, int );


// ENV.C
TCHAR _far *  _fastcall get_variable( LPTSTR );
TCHAR _far *  _fastcall get_alias( LPTSTR );
TCHAR _far * PASCAL get_list( LPTSTR, TCHAR _far * );
int  _fastcall add_variable( LPTSTR );
int PASCAL add_list( LPTSTR, TCHAR _far * );
TCHAR _far * PASCAL next_env( TCHAR _far * );
TCHAR _far * PASCAL end_of_env( TCHAR _far * );


// ERROR.C
int  _fastcall Usage( LPTSTR );
int _fastcall error( int, LPTSTR );
int _fastcall ErrorMsgBox( unsigned int, LPTSTR );
void BatchErrorMsg( void );
void PASCAL CheckOnError( void );
void CheckOnErrorMsg( void );


// EVAL.C
void SetEvalPrecision( LPTSTR, unsigned int *, unsigned int * );
int _fastcall evaluate( LPTSTR );

// EXETYPE.C
INT _fastcall GetExeType( LPTSTR );


// EXPAND.C
void _fastcall dup_handle( unsigned int, unsigned int );
void _fastcall RedirToClip( LPTSTR, int );
int _fastcall CopyToClipboard( int );
int _fastcall CopyTextToClipboard( LPTSTR, int );
int _fastcall CopyFromClipboard( LPTSTR );
int _fastcall GetClipboardLine( int, LPTSTR, int );
int _fastcall redir( LPTSTR, REDIR_IO *);
void _fastcall unredir(REDIR_IO *, int *);
void _fastcall ClosePipe( LPTSTR );
int _fastcall alias_expand( LPTSTR );
int _fastcall var_expand( LPTSTR, int );
void  _fastcall EscapeLine( LPTSTR );
void  _fastcall escape( LPTSTR );
TCHAR  _fastcall escape_char( TCHAR );
void _fastcall addhist( LPTSTR );
TCHAR _far * _fastcall prev_hist( TCHAR _far * );
TCHAR _far * _fastcall next_hist( TCHAR _far * );


// FILECMDS.C
void _fastcall FilesProcessed( LPTSTR, long );
LPTSTR _fastcall show_atts( int );


// IOFMT.C
void  _fastcall IntToAscii( INT_PTR, LPTSTR );
int _cdecl sscanf_far(const char _far *, LPCTSTR, ...);
int _cdecl sprintf_far( char _far *, LPCTSTR, ...);

int _cdecl qprintf( int, LPCTSTR, ...);
int _cdecl color_printf( int, LPCTSTR, ...);

int  _fastcall qputs( LPTSTR );
void crlf( void );
void  _fastcall qputc( int, TCHAR );

// LINES.C
void PASCAL _box( int, int, int, int, int, int, int, int, int );
int  _fastcall verify_row_col( unsigned int, unsigned int );

// MD5.C
extern void MD5Hash(char *, unsigned int, char *);

// MISC.C
unsigned long _fastcall CRC32( LPTSTR );
int PASCAL GetFileLine( LPTSTR, long *, LPTSTR );

long _fastcall QuerySeekSize( int );
long _fastcall RewindFile( int );

int _fastcall cvtkey( unsigned int, unsigned int );
int  _fastcall iswhite( TCHAR );
int  _fastcall isdelim( TCHAR );
int  _fastcall QueryIsNumeric( LPTSTR);
int  _fastcall QueryIsCON( LPTSTR);
LPTSTR  _fastcall skipspace( LPTSTR);
LPTSTR GetToken( LPTSTR, LPTSTR, int, int );
LPTSTR  _fastcall first_arg( LPTSTR);
LPTSTR  _fastcall next_arg( LPTSTR, int );
LPTSTR  _fastcall last_arg( LPTSTR, int * );
LPTSTR _fastcall ntharg( LPTSTR, int );
LPTSTR scan( LPTSTR, LPTSTR, LPTSTR );
int PASCAL QueryIsSwitch( LPTSTR, TCHAR, int );
int PASCAL GetMultiCharSwitch( LPTSTR, LPTSTR, LPTSTR, int );
int PASCAL GetSwitches( LPTSTR, LPTSTR, long *, int );
long _fastcall switch_arg( LPTSTR, LPTSTR );
int PASCAL GetRange( LPTSTR, RANGES *, int );
int GetStrDate( LPTSTR, unsigned int *, unsigned int *, unsigned int * );
int _fastcall MakeDaysFromDate(long *, LPTSTR );
int MakeDateFromDays(long, unsigned int *, unsigned int *, unsigned int * );
void  _fastcall collapse_whitespace( LPTSTR, LPTSTR );
void  _fastcall strip_leading( LPTSTR, LPTSTR );
void  _fastcall strip_trailing( LPTSTR, LPTSTR );
void  _fastcall trim( LPTSTR, LPTSTR );
TCHAR _far * _fastcall GetTempDirectory( LPTSTR );
LPTSTR  _fastcall filecase( LPTSTR);
void _fastcall SetDriveString( LPTSTR );
LPTSTR _fastcall gcdir( LPTSTR, int );
int  _fastcall gcdisk( LPTSTR);
int _fastcall QueryIsDotName( LPTSTR );
LPTSTR _fastcall path_part( LPTSTR );
LPTSTR _fastcall fname_part( LPTSTR );
LPTSTR _fastcall ext_part( LPTSTR );
void  _fastcall copy_filename( LPTSTR, LPTSTR);
void _fastcall AddCommas( LPTSTR );
void _fastcall StripQuotes( LPTSTR );
int _fastcall AddQuotes( LPTSTR );
int _fastcall mkdirname( LPTSTR, LPTSTR );
LPTSTR _fastcall mkfname( LPTSTR, int );
void PASCAL insert_path( LPTSTR, LPTSTR, LPTSTR );
int _fastcall is_file( LPTSTR );
int _fastcall is_file_or_dir( LPTSTR );
int _fastcall is_dir( LPTSTR );
int _fastcall AddWildcardIfDirectory( LPTSTR );
int  _fastcall is_net_drive( LPTSTR );
TCHAR _far * _fastcall executable_ext( LPTSTR );
int _fastcall ExcludeFiles( LPTSTR, LPTSTR );
int wild_cmp( TCHAR _far *, TCHAR _far *, int, int );
int wild_brackets( TCHAR _far *, TCHAR, int );
LPTSTR FormatDate( int, int, int, int );
void honk( void );
int _fastcall QueryInputChar( LPTSTR, LPTSTR );
LPTSTR _fastcall stristr( LPTSTR, LPTSTR );
LPTSTR _fastcall strins( LPTSTR, LPTSTR );
LPTSTR _fastcall strend( LPTSTR );
LPTSTR _fastcall strlast( LPTSTR );
void _fastcall more_page( TCHAR _far *, int );
void _fastcall incr_column( TCHAR, int * );
long _fastcall GetRandom( long, long );
int _fastcall OffOn( LPTSTR );
int PASCAL GetCursorRange( LPTSTR, int *, int *);
int _fastcall GetColors( LPTSTR, int );
void _fastcall set_colors( int );
LPTSTR _fastcall ParseColors( LPTSTR, int *, int *);
int _fastcall color_shade( LPTSTR );
int PASCAL process_descriptions( LPTSTR, LPTSTR, int );
int _fastcall FindInstalledFile( LPTSTR, LPTSTR);
void _fastcall TestBrand( int );


// MAIN.C
int _cdecl main( int, TCHAR ** );


// PARSER.C
int PASCAL DoINT2E( TCHAR _far * );
void _near _fastcall find_4files( LPTSTR );
int _near BatchCLI( void );
int open_batch_file( void );
void close_batch_file( void );

int PASCAL getline( int, LPTSTR, int, int );
int _near _fastcall command( LPTSTR, int );
extern int _near _fastcall ContinueLine( LPTSTR );
int _near ParseLine( LPTSTR, LPTSTR, int (_near *)(LPTSTR), unsigned int, int );
LPTSTR PASCAL searchpaths( LPTSTR, LPTSTR, int, int * );
void ShowPrompt( void );


// SCREENIO.C
int PASCAL egets( LPTSTR, int, int );


// STRMENC.C
void EncryptDecrypt( unsigned long _far *, char _far *, char _far *, int );


// SYSCMDS.C
int _fastcall __cd( LPTSTR, int );
void _fastcall SaveDirectory( TCHAR _far *, LPTSTR);
int _fastcall MakeDirectory( LPTSTR, int );
int _fastcall DestroyDirectory( LPTSTR );
int _fastcall getlabel( LPTSTR);
LPTSTR _fastcall GetLogName( int );
int  _fastcall _log_entry( LPTSTR, int );
LPTSTR _fastcall gdate( int );
LPTSTR _fastcall gtime( int );
void _fastcall _timer( int, LPTSTR );


// WINDOW.C
POPWINDOWPTR wOpen( int, int, int, int, int, LPTSTR, LPTSTR );
void _fastcall wRemove(POPWINDOWPTR);
void wClear( void );
void  _fastcall wSetCurPos( int, int );
void wWriteStrAtt( int, int, int, TCHAR _far *);
TCHAR _far * wPopSelect( int, int, int, int, TCHAR _far * _far *, int, int, LPTSTR, LPTSTR, LPTSTR, int );

// Environment-specific modules

// WIN95.ASM - support routines for Win95 support
int _cdecl _dos_getcwd( int, LPTSTR );
int _cdecl Win95EnableClose( void );
int _cdecl Win95DisableClose( void );
int _cdecl Win95AckClose( void );
int _cdecl Win95QueryClose( void );

BOOL WINAPI FindClose( INT );
INT  WINAPI FindFirstFile( PCH, WORD, LPWIN32_FIND_DATA, unsigned int * );
BOOL WINAPI FindNextFile( INT, LPWIN32_FIND_DATA, unsigned int * );
BOOL WINAPI Win95GetTitle( LPTSTR );
BOOL WINAPI Win95SetTitle( LPTSTR );
INT WINAPI GetLastError( void );
INT WINAPI Win95GetFAT32Info(LPTSTR, FAT32 *, int);


// SERVER.ASM, 4DOSERRS.ASM - prototypes for miscellaneous 
//   .ASM support routines

// SERVER.ASM
void _pascal ServInit( LPTSTR, INIFILE *);
int _pascal ServExec( LPTSTR, LPTSTR, unsigned int, int, int *);
int _pascal ServCtrl( unsigned int, int );
void _pascal ServTtl( char _far * );

// ServCtrl function codes
#define SERV_QUIT 0
#define SERV_SWAP 1
#define SERV_AF 2
#define SERV_TPA 3
#define SERV_CTTY 4
#define SERV_AVAIL 5
#define SERV_UREG 6
#define SERV_SIGNAL 7
#define SERV_SIG_DISABLE 0xFFFE	// disable signal code
#define SERV_SIG_ENABLE 0xFFFD	// enable signal code


// DOSCALLS.C
void PASCAL BreakHandler( void );
void BreakOut( void );
int _near installable_cmd( LPTSTR);
int _near process_rexx( LPTSTR, LPTSTR, int );
void CheckFreeStack( unsigned int );
int _help( LPTSTR, LPTSTR );
int open_pipe( REDIR_IO * );
void  _fastcall killpipes( REDIR_IO * );
void  _fastcall FreeMem( void _far * );
void _far * PASCAL AllocMem( unsigned int *);
void _far * PASCAL ReallocMem( void _far *, unsigned long );
extern char _far * PASCAL AllocHigh( char _far *, unsigned long );
void HoldSignals( void );
void EnableSignals( void );
void  _fastcall SysWait( unsigned long, int );
int PASCAL is_signed_digit( int );
int PASCAL is_unsigned_digit( int );
int QuerySystemRAM( void );
int ifs_type( LPTSTR );
void  QueryDateTime( DATETIME * );
int  SetDateTime( DATETIME * );
#ifdef VER2
int QueryHandleDateTime( int, DATETIME * );
#endif
int  _fastcall SetFileDateTime( LPTSTR, int, DATETIME *, int );
long QueryFileSize( LPTSTR, int );
int QueryCodePage( void );
LPTSTR QueryVolumeInfo( LPTSTR, LPTSTR, unsigned long *);
int QueryIsANSI( void );
int PASCAL QueryIsDevice( LPTSTR);
LPTSTR true_name( LPTSTR, LPTSTR);
int GetLongName( LPTSTR);
int GetShortName( LPTSTR);
void QueryCountryInfo( void );
void SetOSVersion( void );
int  _fastcall _ctoupper( int );
int QueryDiskInfo( LPTSTR, QDISKINFO *, int );
int QueryIsPipeName( LPTSTR );
int UniqueFileName( LPTSTR );
LPTSTR PASCAL find_file( int, LPTSTR, unsigned long, FILESEARCH *, LPTSTR);
int PASCAL FileTimeToDOSTime( PFILETIME, USHORT *, USHORT * );
void SetWin95Flags(void);
#if _DOS
void AccessSharewareData( SHAREWARE_DATA *, int );
void GetMachineName( LPTSTR );
#endif
LPTSTR IniReadWrite( int, LPTSTR, LPTSTR, LPTSTR, LPTSTR );
int MouseReset( void );


// DOSINIT.C
void _near InitOS( int, LPTSTR*);
void DisplayCopyright( void );


// DOSTTY.C
unsigned int PASCAL GetKeystroke( unsigned int );
unsigned int GetScrRows( void );
unsigned int GetScrCols( void );
void PASCAL SetCurSize( void );
void SetBrightBG( void );
int PASCAL keyparse( char _far *, int );
void MouseCursorOn( void );
void MouseCursorOff( void );
void GetMousePosition( int *, int *, int * );
void SetMousePosition( int, int );
void MouseReleased( int *, int *, int );


// UMBREG.ASM
extern int _pascal FindUReg( int, UMBREGINFO _far *);
extern int _pascal SetUReg( int, int, int, int, UMBREGINFO _far *);
extern void _pascal FreeUReg( int, int, UMBREGINFO _far *);


// DOSUTIL.ASM -- combined ASM support for DOS and TCMD/16
char _pascal QuerySwitchChar( void );
int _pascal QueryVerifyWrite( void );
void _pascal SetVerifyWrite( int );
void _pascal CheckForBreak( void );
int _pascal SetCodePage( int );
int _pascal DosError( int );
LPTSTR _pascal GetError( int, LPTSTR );
void _pascal GetDOSVersion( void );
void _pascal reset_disks( void );
void _pascal SDFlush( void );
int _pascal ForceDelete( LPTSTR );
void _pascal SetIOMode( int, int );
int _pascal _dos_createEA( LPTSTR, int, LPTSTR, int *, unsigned int, unsigned int );
int _pascal QueryIsConsole( int );
int QueryMouseReady( void );
int _pascal QueryDriveRemovable( int );
int _pascal QueryDriveReady( int );
int _pascal QueryDriveExists( int );
int _pascal QueryDriveRemote( int );
int _pascal QueryIsCDROM( LPTSTR );
int _pascal QueryPrinterReady( int );
unsigned int get_cpu( void );
unsigned int get_ndp( void );
unsigned int _pascal get_expanded( unsigned int *);
unsigned int get_extended( void );
unsigned int _pascal get_xms( unsigned int *);
int bios_key( void );
int QueryMSDOS7( void );
unsigned int _pascal DosBeep( unsigned int, unsigned int );
void _pascal GetCurPos(int *, int * );
void _pascal SetCurPos( int, int );
unsigned int _pascal GetVideoMode( void );
unsigned int _pascal GetCellSize( int );
void _pascal GetAtt( unsigned int *, unsigned int *);
void _pascal Scroll( int, int, int, int, int, int );
void _pascal SetBorderColor( int );
void _pascal SetLineColor( int, int, int, int );
void _pascal SetScrColor( int, int, int );
void _pascal ReadCellStr( char _far *, int, int, int );
void _pascal WriteCellStr( char _far *, int, int, int );
void _pascal WriteChrAtt( int, int, int, int );
void _pascal WriteStrAtt( int, int, int, LPTSTR);
void _pascal WriteCharStrAtt( int, int, int, int, LPTSTR);
void _pascal WriteVStrAtt( int, int, int, LPTSTR);
void _pascal WriteTTY( LPTSTR );
void _pascal CriticalSection( int );
char * _pascal DecodeMsg( int, char * );
void _cdecl BatDComp( char _far *, char _far * );
void _pascal safe_append( void );
int _pascal QueryKSTACK( void );
int _pascal QueryClipAvailable( void );
int _pascal OpenClipboard( void );
int _pascal CloseClipboard( void );
long _pascal QueryClipSize( void );
void _pascal ReadClipData( char _far * );
void _pascal SetClipData( char _far *, long );

