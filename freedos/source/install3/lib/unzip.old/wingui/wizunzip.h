#ifndef __wizunzip_h   /* prevent multiple inclusions */
#define __wizunzip_h

#include <windows.h>
#include <assert.h>    /* required for all Windows applications */
#include <commdlg.h>
#include <dlgs.h>

/* Main include file for  Windows Unzip: wizunzip.h
 * This include file is copied into all `C' source modules specific to 
 * Windows Info-ZIP Unzip, version 3.0.
 * Author: Robert A. Heath, 157 Chartwell Rd., Columbia, SC 29210
 * I, Robert A. Heath, place this module, wizunzip.h, in the public domain.
 *
 * Modifications: 1995 M. White
 */

/* Allow compilation under Borland C++ also */
#ifndef __based
#define __based(A)
#endif

/* Porting definations between Win 3.1x and Win32 */
#ifdef WIN32
#  define far
#  define _far
#  define __far
#  define near
#  define _near
#  define __near
#endif

/*
 * MW:
 * The following is to take care of some of the porting problems between
 * Win 3.1 and Win32 for WM_COMMAND notifications.
*/
#ifdef WIN32
#define GET_WM_COMMAND_CMD(wp, lp) HIWORD(wp)
#else
#define GET_WM_COMMAND_CMD(wp, lp) HIWORD(lp)
#endif

#define WIZUNZIP_MAX_PATH       128   /* max total file or directory name path   */
#define OPTIONS_BUFFER_LEN      256   /* buffer to hold .INI file options         */

/* These two are dependent on zip directory listing format string.
 * They help find the filename in the listbox entry.
 */
#define SHORT_FORM_FNAME_INX     27
#define LONG_FORM_FNAME_INX      58

#define MIN_SHORT_FORMAT_CHARS (SHORT_FORM_FNAME_INX+12)
#define MIN_LONG_FORMAT_CHARS (LONG_FORM_FNAME_INX+12)

/* Arbitrary Constants
 */
#define BG_SYS_COLOR COLOR_GRAYTEXT /* background color is a system color */

/* Main window menu item positions
 */
#define EDIT_MENUITEM_POS         1   /* edit menu position in main menu */
#define HELP_MENUITEM_POS         5   /* the Help menu                */

/* Main Window Message Codes
 */

#define IDM_OPEN                101
#define IDM_EXIT                102

#define IDM_SHORT               104
#define IDM_LONG                105


#define IDM_HELP                106
#define IDM_ABOUT               107

#define IDM_RECR_DIR_STRUCT     108
#define IDM_OVERWRITE           109
#define IDM_SAVE_UNZIP_FROM_DIR 110
#define IDM_SAVE_UNZIP_TO_DIR   111
#define IDM_EXTRACT_ONLY_NEWER  112
#define IDM_TRANSLATE           114
#define IDM_SPACE_TO_UNDERSCORE 115
#define IDM_UNZIP_TO_ZIP_DIR    116

#define IDM_LB_EXTRACT          117
#define IDM_LB_DISPLAY          118
#define IDM_LB_TEST             119


#define IDM_EDIT                120
#define IDM_PATH                121

#define IDM_UNZIP_FROM_DIR      122

#define IDM_COMMENT             123
#define IDM_SOUND_OPTIONS       124
#define IDM_COPY                125
#define IDM_SELECT_ALL          126

/* These six items are the tab-stop windows whose ID's must be kept
 * in order.
 */
#define IDM_LISTBOX             127
#define IDM_EXTRACT             128
#define IDM_DISPLAY             129
#define IDM_TEST                130
#define IDM_SHOW_COMMENT        131
#define IDM_STATUS              132

#define TABSTOP_ID_BASE IDM_LISTBOX


#define IDM_AUTOCLEAR_STATUS    133
#define IDM_SELECT_BY_PATTERN   134

/* Keep these 3 in order */
#define IDM_SPLIT               135
#define IDM_MAX_LISTBOX         136
#define IDM_MAX_STATUS          137

#define IDM_AUTOCLEAR_DISPLAY   138

/* Keep these 3 in order */

#define IDM_DESELECT_ALL        139
#define IDM_CLEAR_STATUS        140
#define IDM_HELP_KEYBOARD       141
#define IDM_HELP_HELP           142
#define IDM_CHDIR               143
#define IDM_SETFOCUS_ON_STATUS  144 /* internal: posted after extraction to Status window */


/* For the Copy, Move, Delete and Rename File functions */
#define IDM_COPY_ARCHIVE        145
#define IDM_MOVE_ARCHIVE        146
#define IDM_DELETE_ARCHIVE      147
#define IDM_RENAME_ARCHIVE      150

#define IDM_SAVE_AS_DEFAULT     161

#define IDM_MAKE_DIR            162
#define IDM_MAKEDIR_PATH        163
#define IDM_MAKEDIR_HELP        164
#define IDM_CURRENT_PATH        165
#define IDM_SHOW_BUBBLE_HELP    166
#define IDM_PROMPT_TO_OVERWRITE 167

/* Help Window Menu and Message ID's
 */
#define INDEX_MENU_ITEM_POS       0

#define IDM_FORWARD             100
#define IDM_BACKWARD            101

/*
 * About box identifiers used to display the current version number
 * information.
 */
#define IDM_ABOUT_VERSION_INFO  170
#define IDM_ABOUT_UNZIP_INFO    171

#define BTNWIDTH        3.0
#define MIN_BTN_WIDTH   1.5

extern int Width, Height;
extern int BtnSeparator;
extern float BtnMult;

#define NumOfBtns 21

/* Tab-stop table is used to sub-class those main window items to
 * which the tab and back-tab keys will tab and stop.
 */
typedef struct TabStop_tag {
FARPROC lpfnOldFunc;        /* original function                */
HWND hWnd ;
} TabStopEntry;

typedef TabStopEntry *PTABSTOPENTRY;
#define TABSTOP_TABLE_ENTRIES 26


#ifndef NDEBUG
#define WinAssert(exp) \
        {\
        if (!(exp))\
            {\
            char szBuffer[40];\
            sprintf(szBuffer, "File %s, Line %d",\
                    __FILE__, __LINE__) ;\
            if (IDABORT == MessageBox((HWND)NULL, szBuffer,\
                "Assertion Error",\
                MB_ABORTRETRYIGNORE|MB_ICONSTOP))\
                    FatalExit(-1);\
            }\
        }

#else

#define WinAssert(exp)

#endif


/* Unzip Flags */
typedef struct
{
unsigned int   fRecreateDirs : 1;
unsigned int   fShowBubbleHelp : 1;
unsigned int   fTranslate : 1;
unsigned int   fSaveUnZipToDir : 1;
unsigned int   fSaveUnZipFromDir : 1;
unsigned int   fFormatLong : 1;
unsigned int   fUnzipToZipDir : 1;
unsigned int   fBeepOnFinish : 1;
unsigned int   fDoAll : 1;
unsigned int   fIconSwitched : 1;
unsigned int   fHelp : 1;
unsigned int   fCanDragDrop : 1;
unsigned int   fAutoClearStatus : 1;
unsigned int   fAutoClearDisplay : 1;
unsigned int   fTrailingSlash : 1;
unsigned int   fPromptToOverwrite : 1;
unsigned int   fUnused : 5;
} UF, *PUF;

/* Unzip Miscellaneous Buffers */
typedef struct
{
char szFileName[WIZUNZIP_MAX_PATH]; /* fully-qualified archive file name in OEM char set */
char szDirName[WIZUNZIP_MAX_PATH];  /* directory of archive file in ANSI char set */
char szUnzipToDirName[WIZUNZIP_MAX_PATH];    /* extraction ("unzip to") directory name in ANSI */
char szUnzipToDirNameTmp[WIZUNZIP_MAX_PATH]; /* temp extraction ("unzip to") directory name in ANSI */
char szUnzipFromDirName[WIZUNZIP_MAX_PATH];  /* extraction ("unzip from") directory name in ANSI */
char szTotalsLine[80];              /* text for totals of zip archive */
char szBuffer[OPTIONS_BUFFER_LEN];  /* option strings from .INI, & gen'l scratch buf */
char szSoundName[WIZUNZIP_MAX_PATH];/* wave file name or sound from WIN.INI [sounds] in ANSI */
char szPassword[81];
LPSTR lpPassword;
OPENFILENAME ofn;
OPENFILENAME wofn;                  /* wave open file name struct */
MSG msg;
OFSTRUCT of;                        /* archive open file struct */
OFSTRUCT wof;                       /* wave open file struct   */
} UMB, __far *LPUMB;

#if defined (USEWIZUNZDLL) || defined (WIZUNZIPDLL)
typedef int (far *DLLPRNT) (FILE *, unsigned int, char *);
typedef void (far *DLLSND) (void);

typedef struct {
DLLPRNT print;
DLLSND sound;
FILE *Stdout;
LPUMB lpUMB;
HWND hWndList;
HWND hWndMain;
HWND hInst;
#else
typedef struct {
#endif
int ExtractOnlyNewer;
int Overwrite;
int SpaceToUnderscore;
int PromptToOverwrite;
int ncflag;
int ntflag;
int nvflag;
int nUflag;
int nzflag;
int ndflag;
int noflag;
int naflag;
int argc;
LPSTR lpszZipFN;
char **FNV;
} DCL, _far *LPDCL;

extern TabStopEntry TabStopTable[]; /* tab-stop control table           */

#ifndef WIN32
extern short dxChar, dyChar;    /* size of char in SYSTEM font in pixels    */
#else
extern long dxChar, dyChar;    /* size of char in SYSTEM font in pixels    */
#endif

extern HANDLE hFixedFont;

extern HWND hWndComment;        /* comment window                       */
extern HWND hWndList;       /* listbox handle                       */
extern HWND hWndButtonBar; /* Button bar handle */

extern HWND hWndMain;        /* the main window handle.         */

extern HWND hExtract;           /* extract button               */
extern HWND hDisplay;           /*display button                */
extern HWND hTest;              /* test button                  */
extern HWND hShowComment;       /* show comment button          */

extern HWND hExit;
extern HWND hMakeDir;
extern HWND hSelectAll;
extern HWND hDeselectAll;
extern HWND hSelectPattern;
extern HWND hClearStatus;
extern HWND hCopyStatus;
extern HWND hUnzipToDir;

extern HWND hHelp;
extern HWND hOpen;
extern HWND hCopyArchive;
extern HWND hMoveArchive;
extern HWND hRenameArchive;
extern HWND hDeleteArchive;
extern HWND hSplitButton;
extern HWND hStatusButton;
extern HWND hListBoxButton;

extern HWND hPatternSelectDlg; /* pattern select modeless dialog   */
extern HANDLE hInst;                       /* current instance */
extern HMENU  hMenu;                /* main menu handle         */
extern HANDLE hAccTable;

extern HANDLE hHourGlass;             /* handle to hourglass cursor      */
extern HANDLE hSaveCursor;            /* current cursor handle       */
extern HANDLE hHelpCursor;          /* help cursor              */
extern HANDLE hFixedFont;           /* handle to fixed font             */
extern HANDLE hOldFont;         /* handle to old font               */

extern int hFile;                 /* file handle             */
extern HWND hWndList;             /* list box handle        */
extern HWND hWndStatus;     /* status   */
extern BOOL bRealTimeMsgUpdate; /* update messages window in real-time */
extern BOOL gfCancelDisplay;   /* cancel ongoing display operation */
extern UF uf;

extern char szTargetDirName[];
extern LPSTR lpchLast;

extern WPARAM wLBSelection;   /* default listbox selection action */
extern WPARAM wWindowSelection; /* window selection: listbox, status, both   */

extern HBRUSH hBrush ;          /* brush for  standard window backgrounds  */

extern char __based(__segname("STRINGS_TEXT")) szCantChDir[];
extern char __based(__segname("STRINGS_TEXT")) szCantCopyFile[];
extern char __based(__segname("STRINGS_TEXT")) szAppName[];     /* application name             */
extern char __based(__segname("STRINGS_TEXT")) szDefaultUnzipToDir[]; /* default unzip to dir */
extern char __based(__segname("STRINGS_TEXT")) szDefaultUnzipFromDir[]; /* default unzip from dir */
extern char __based(__segname("STRINGS_TEXT")) szStatusClass[]; /* status class name                */
extern char __based(__segname("STRINGS_TEXT")) szFormatKey[];       /* Format .INI keyword       */
extern char __based(__segname("STRINGS_TEXT")) szOverwriteKey[];    /* Overwrite .INI keyword        */
extern char __based(__segname("STRINGS_TEXT")) szPromptOverwriteKey[];    /* Prompt to Overwrite .INI keyword        */
extern char __based(__segname("STRINGS_TEXT")) szExtractOnlyNewerKey[];    /* Extract only newer .INI keyword        */
extern char __based(__segname("STRINGS_TEXT")) szSaveUnZipToKey[]; /* SaveZipToDir keyword in WIN.INI     */
extern char __based(__segname("STRINGS_TEXT")) szSaveUnZipFromKey[]; /* SaveZipFromDir keyword in WIN.INI     */
extern char __based(__segname("STRINGS_TEXT")) szTranslateKey[];    /* Translate .INI keyword        */
extern char __based(__segname("STRINGS_TEXT")) szSpaceToUnderscoreKey[];    /* SpaceToUnderscore .INI keyword        */
extern char __based(__segname("STRINGS_TEXT")) szLBSelectionKey[];  /* LBSelection keyword in .INI */
extern char __based(__segname("STRINGS_TEXT")) szRecreateDirsKey[]; /* re-create directory structure .INI keyword */
extern char __based(__segname("STRINGS_TEXT")) szShowBubbleHelpKey[]; /* Show bubble help .INI keyword */
extern char __based(__segname("STRINGS_TEXT")) szUnzipToZipDirKey[];   /* unzip to .ZIP dir .INI keyword */
extern char __based(__segname("STRINGS_TEXT")) szAutoClearStatusKey[];   /* autoclear status .INI keyword */
extern char __based(__segname("STRINGS_TEXT")) szAutoClearDisplayKey[];   /* autoclear status .INI keyword */
extern char __based(__segname("STRINGS_TEXT")) szNoMemory[] ;       /* error message            */
extern char __based(__segname("STRINGS_TEXT")) szHelpFileName[];        /* help file name                       */
extern char __based(__segname("STRINGS_TEXT")) szWizUnzipIniFile[];   /* WizUnzip Private .INI file */
extern char __based(__segname("STRINGS_TEXT")) szYes[];
extern char __based(__segname("STRINGS_TEXT")) szNo[];

extern char * LBSelectionTable[];
extern char * Headers[][2] ;        /* headers to display           */
extern DWORD dwCommDlgHelpId; /* what to pass to WinHelp() */

extern WORD cchComment; /* length of comment in .ZIP file   */
extern LPUMB lpumb;
extern LPDCL lpDCL;

/* List box stuff
 */
extern WORD cZippedFiles;       /* total personal records in file   */
extern WORD cListBoxLines; /* max list box lines showing on screen */
extern WORD cLinesMessageWin; /* max visible lines on message window  */

/* Function Prototypes */

void SetCaption(HWND hWnd);

/* some global functions */
void Action(HWND hWnd, WPARAM wActionCode);
void CenterDialog(HWND hwndParent, HWND hwndDlg);
void CopyStatusToClipboard(HWND hWnd);
void DisplayComment(HWND hWnd);
int CLBItemsGet(HWND hListBox, int __far * __far *ppnSelItems, HANDLE *phnd);
void ReselectLB(HWND hListBox, int nSelCount, int __far *pnSelItems);
#ifndef USEWIZUNZDLL
BOOL FSetUpToProcessZipFile(LPDCL C);
void TakeDownFromProcessZipFile(void);
#endif
void InitSoundOptions(void); /* initialize sound options (sound.c)   */
void MigrateSoundOptions(void); /* translate beep into new option (sound.c) */
void SetStatusTopWndPos(void);
void SizeWindow(HWND hWnd, BOOL bOKtoMovehWnd);
void SoundAfter(void);
void SoundDuring(void);
BOOL StatusInWindow(void);
void UpdateButtons(HWND hWnd);
void UpdateListBox(HWND hWnd);
void UpdateMsgWndPos(void);
BOOL WizUnzipInit(HANDLE hInst);
void WriteBufferToMsgWin(LPSTR buffer, int nBufferLen, BOOL bUpdate);
void WriteStringToMsgWin(PSTR String, BOOL bUpdate);
int win_fprintf(FILE *file, unsigned int, char *);
void CopyArchive(HWND hWnd, BOOL move_flag, BOOL rename_flag);
void GetDirectory(LPSTR lpDir);
BOOL MakeDirectory(char *path, BOOL fileAttached);
LPSTR lstrrchr(LPSTR lpszSrc, char chFind);
void MoveButtons(void);
void CreateButtonBar(HWND);

/* Far Proc's */
#ifdef WIZUNZIPDLL
extern WINAPI DllProcessZipFiles(DCL far *);
extern WINAPI GetDllVersion(DWORD far *);
#endif

#ifdef USEWIZUNZDLL
extern FARPROC DllProcessZipFiles;
extern FARPROC GetDllVersion;
#else
BOOL WINAPI PasswordProc(HWND, WORD, WPARAM, LPARAM);
#endif
BOOL WINAPI AboutProc(HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI SelectDirProc(HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI CopyFileProc(HWND, WORD, WPARAM, LPARAM);
long WINAPI KbdProc(HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI PatternSelectProc(HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI ReplaceProc(HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI SoundProc(HWND, WORD, WPARAM, LPARAM);
long WINAPI StatusProc (HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI RenameProc (HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI MakeDirProc(HWND, WORD, WPARAM, LPARAM);
LONG WINAPI ButtonBarWndProc(HWND, UINT, WPARAM, LPARAM);
int GetReplaceDlgRetVal(void);
#endif /* __wizunzip_h */

