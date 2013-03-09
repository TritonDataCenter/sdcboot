/****************************************************************************

    PROGRAM: WizUnZip.c

    PURPOSE:  Windows Info-ZIP Unzip, an Unzipper for Windows
    FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        WizUnzipInit() - initializes window data and registers window
        WizUnzipWndProc() - processes messages
        About() - processes messages for "About" dialog box

    AUTHOR: Robert A. Heath,  157 Chartwell Rd. Columbia, SC 29210
    I place this source module, WizUnzip.c, in the public domain.  Use it as
    you will.

    Modifications: M. White - 1995, 1996
      - Ported to Borland C
      - Ported to WinNT and Win95
      - Added dll functionality
****************************************************************************/

#define UNZIP_INTERNAL
#include "unzip.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "wingui\wizunzip.h"
#include "wizdll\wizdll.h"
#ifdef WIN32
#include <winver.h>
#else
#include <ver.h>
#endif

#ifdef WIN32
#define UNZ_DLL_NAME "WIZUNZ32.DLL"
#else
#define UNZ_DLL_NAME "WIZUNZ16.DLL"
#endif

#ifndef WIZUNZIPDLL
static char __based(__segname("STRINGS_TEXT")) szFirstUse[] = "FirstUse"; /* first use keyword in WIN.INI */
char __based(__segname("STRINGS_TEXT")) szDefaultUnzipToDir[] = "DefaultUnzipToDir";
char __based(__segname("STRINGS_TEXT")) szDefaultUnzipFromDir[] = "DefaultUnzipFromDir";
char __based(__segname("STRINGS_TEXT")) szFormatKey[] = "Format";       /* Format keyword in WIN.INI        */
char __based(__segname("STRINGS_TEXT")) szOverwriteKey[] = "Overwrite"; /* Overwrite keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szPromptOverwriteKey[] = "PromptOverwrite"; /* Prompt to Overwrite keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szExtractOnlyNewerKey[] = "ExtractOnlyNewer"; /* ExtractOnlyNewer keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szNeverOverwriteKey[] = "NeverOverwrite"; /* Overwrite keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szSaveUnZipToKey[] = "SaveZipToDir"; /* SaveZipToDir keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szSaveUnZipFromKey[] = "SaveZipFromDir"; /* SaveZipFromDir keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szTranslateKey[] = "Translate"; /* Translate keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szSpaceToUnderscoreKey[] = "SpaceToUnderscore"; /* Space to underscore keyword in WIN.INI     */
char __based(__segname("STRINGS_TEXT")) szLBSelectionKey[] = "LBSelection"; /* LBSelection keyword in WIN.INI */
char __based(__segname("STRINGS_TEXT")) szRecreateDirsKey[] = "Re-createDirs"; /* re-create directory structure WIN.INI keyword             */
char __based(__segname("STRINGS_TEXT")) szShowBubbleHelpKey[] = "ShowBubbleHelp"; /* re-create directory structure WIN.INI keyword             */
char __based(__segname("STRINGS_TEXT")) szUnzipToZipDirKey[] = "UnzipToZipDir"; /* unzip to .ZIP dir WIN.INI keyword */
char __based(__segname("STRINGS_TEXT")) szHideStatus[] = "HideStatusWindow";
char __based(__segname("STRINGS_TEXT")) szAutoClearStatusKey[] = "AutoClearStatus";
char __based(__segname("STRINGS_TEXT")) szAutoClearDisplayKey[] = "AutoDisplayStatus";
char __based(__segname("STRINGS_TEXT")) szWizUnzipIniFile[] = "WIZUNZIP.INI";
char __based(__segname("STRINGS_TEXT")) szYes[] = "yes";
char __based(__segname("STRINGS_TEXT")) szNo[] = "no";

/* File and Path Name variables */
char __based(__segname("STRINGS_TEXT")) szAppName[] = "WizUnZip";       /* application title        */
char __based(__segname("STRINGS_TEXT")) szStatusClass[] = "MsgWndw";/* status window class  */

/* Values for listbox selection WIN.INI keyword
 */
char * LBSelectionTable[] =
{
"extract", "display", "test"
};
#define LBSELECTIONTABLE_ENTRIES (sizeof(LBSelectionTable)/sizeof(char *))
#endif /* WIZUNZIPDLL */

char __based(__segname("STRINGS_TEXT")) szHelpFileName[] = "WIZUNZIP.HLP";

HANDLE hInst;               /* current instance */
HMENU  hMenu;               /* main menu handle */
HANDLE hAccTable;

HANDLE hHourGlass;          /* handle to hourglass cursor        */
HANDLE hSaveCursor;         /* current cursor handle         */
HANDLE hHelpCursor;         /* help cursor                      */
HANDLE hFixedFont;          /* handle to fixed font             */
HANDLE hOldFont;            /* handle to old font               */

int hFile;                /* file handle             */
HWND hWndMain;        /* the main window handle.                */
HWND hWndList;            /* list box handle        */
HWND hWndStatus;        /* status   (a.k.a. Messages) window */
HWND hExtract;          /* extract button               */
HWND hDisplay;          /*display button                */
HWND hTest;             /* test button              */
HWND hShowComment;      /* show comment button          */
HWND hHelp;   /* help button */
HWND hOpen;   /* open button */
HWND hDeleteArchive; /* delete button */
HWND hCopyArchive;   /* copy archive button */
HWND hMoveArchive;   /* move archive button */
HWND hRenameArchive; /* rename archive button */
HWND hExit;
HWND hMakeDir;
HWND hSelectAll;
HWND hDeselectAll;
HWND hSelectPattern;
HWND hClearStatus;
HWND hCopyStatus;
HWND hUnzipToDir;
HWND hStatusButton;
HWND hListBoxButton;
HWND hSplitButton;

UF  uf;

#ifndef WIZUNZIPDLL
WPARAM wLBSelection = IDM_LB_DISPLAY; /* default listbox selection action */
WPARAM wWindowSelection = IDM_SPLIT; /* window selection: listbox, status, both */
HBRUSH hBrush ;         /* brush for  standard window backgrounds  */
LPUMB lpumb;
#endif /* !WIZUNZIPDLL */

LPDCL lpDCL;
HANDLE hDCL;
#ifdef USEWIZUNZDLL
HINSTANCE hWinDll;
FARPROC DllProcessZipFiles;
FARPROC GetDllVersion;
#endif

HANDLE  hStrings;

int ofretval;       /* return value from initial open if filename given */

WORD cZippedFiles;      /* total personal records in file   */
WORD cListBoxLines; /* max list box lines showing on screen */
WORD cLinesMessageWin; /* max visible lines on message window  */
WORD cchComment;            /* length of comment in .ZIP file   */

#ifndef WIZUNZIPDLL
/* Forward References */
int WINAPI WinMain(HANDLE, HANDLE, LPSTR, int);
long WINAPI WizUnzipWndProc(HWND, WORD, WPARAM, LPARAM);

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        This will initialize the window class if it is the first time this
        application is run.  It then creates the window, and processes the
        message loop until a WM_QUIT message is received.  It exits the
        application by returning the value passed by the PostQuitMessage.

****************************************************************************/
int WINAPI WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;         /* current instance             */
HANDLE hPrevInstance;     /* previous instance            */
LPSTR lpCmdLine;          /* command line                 */
int nCmdShow;             /* show-window type (open/icon) */
{
#ifdef USEWIZUNZDLL
DWORD dwVersion;  /* These variables are all used for version checking */
WORD dllMajor, dllMinor;
DWORD dwVerInfoSize;
DWORD dwVerHnd;
char szFullPath[_MAX_PATH], *ptr;
#endif
int i;
BOOL fFirstUse;           /* first use if TRUE         */

if (!hPrevInstance)       /* Has application been initialized? */
   if (!WizUnzipInit(hInstance))
      return 0;           /* Exits if unable to initialize     */


hStrings = GlobalAlloc( GPTR, (DWORD)sizeof(UMB));
if ( !hStrings )
   return 0;

lpumb = (LPUMB)GlobalLock( hStrings );
if ( !lpumb )
   {
   GlobalFree( hStrings );
   return 0;
   }
hDCL = GlobalAlloc( GPTR, (DWORD)sizeof(DCL));
if (!hDCL)
   {
   GlobalUnlock(hStrings);
   GlobalFree(hStrings);
   return 0;
   }
lpDCL = (LPDCL)GlobalLock(hDCL);
if (!lpDCL)
   {
   GlobalUnlock(hStrings);
   GlobalFree(hStrings);
   GlobalFree(hDCL);
   return 0;
   }
#ifndef USEWIZUNZDLL
CONSTRUCTGLOBALS();
#endif

uf.fCanDragDrop = FALSE;
#ifndef WIN32
if ((hHourGlass = GetModuleHandle("SHELL"))!=0)
   {
   if (GetProcAddress(hHourGlass, "DragAcceptFiles" ))
      uf.fCanDragDrop = TRUE;
   }
#else
uf.fCanDragDrop = TRUE;
#endif

#ifdef USEWIZUNZDLL
lpDCL->print = win_fprintf;
lpDCL->sound = SoundDuring;
lpDCL->Stdout = stdout;
lpDCL->lpUMB = lpumb;

/* Okay - we are now setting up to check the version stamp information
 * in the dll. We aren't doing anything with it yet, because there is only
 * one version of the dll, but this information will be useful for later
 * versions. Note that we are assuming that the dll is in the same directory
 * as the executable. Not a particularly valid assumption at this point, but
 * it will do until we actually need this information.
 */
GetModuleFileName(hInstance, szFullPath, sizeof(szFullPath));
ptr = strrchr(szFullPath, '\\') + 1;
*ptr = '\0';      /* We've now got the path to the executable (application) */
lstrcat(szFullPath, UNZ_DLL_NAME);
dwVerInfoSize =
    GetFileVersionInfoSize(szFullPath, &dwVerHnd);

if (dwVerInfoSize)
   {
   LPSTR   lpstrVffInfo; /* Pointer to block to hold info */
   HANDLE  hMem;         /* handle to mem alloc'ed */

   /* Get a block big enough to hold the version information */
   hMem          = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
   lpstrVffInfo  = GlobalLock(hMem);

   /* Get the version information */
   GetFileVersionInfo(szFullPath, 0L, dwVerInfoSize, lpstrVffInfo);

   /* free memory */
   GlobalUnlock(hMem);
   GlobalFree(hMem);
   }

hWinDll = LoadLibrary(UNZ_DLL_NAME);
#ifndef WIN32
if (hWinDll > HINSTANCE_ERROR)
#else
if (hWinDll != NULL)
#endif
   {
   DllProcessZipFiles = GetProcAddress(hWinDll, "DllProcessZipFiles");
   GetDllVersion = GetProcAddress(hWinDll, "GetDllVersion");
   }
else
   {
   char str[80];

   wsprintf (str, "Cannot find %s", UNZ_DLL_NAME);
   MessageBox (hWndMain, str, szAppName, MB_ICONSTOP | MB_OK);
   GlobalUnlock(hStrings);
   GlobalFree(hStrings);
   GlobalUnlock(hDCL);
   GlobalFree(hDCL);
   return 0;
   }

/* If, for some reason we got a dll with the same name, we don't want to
 * make a call out to never-never land. At some point we should put a message
 * up saying we can't load the proper dll.
 */

if (DllProcessZipFiles == NULL)
   { 
   FreeLibrary(hWinDll);
   GlobalUnlock(hStrings);
   GlobalFree(hStrings);
   GlobalUnlock(hDCL);
   GlobalFree(hDCL);
   return 0;
   }
/* We need a check here in case an old beta version of the dll is out there */
if (GetDllVersion != NULL)
   {
   (*GetDllVersion)(&dwVersion);
   dllMinor = LOWORD(dwVersion);
   dllMajor = HIWORD(dwVersion);
   /* This is just a place holder for now, but this is where we need to
    * do whatever it is we decide to do when the dll version doesn't match
    */
   if ((dllMajor != DLL_MAJOR) && (dllMinor != DLL_MINOR))
      {
      }
   }
#endif

#ifndef WIN32
if (_fstrlen(lpCmdLine))            /* if filename passed on start-up   */
#else
if (strlen(lpCmdLine))            /* if filename passed on start-up   */
#endif
   {
   if ((ofretval = OpenFile(lpCmdLine, &lpumb->of, OF_EXIST)) >= 0)
      lstrcpy(lpumb->szFileName, lpumb->of.szPathName); /* save file name */

   }

/* If first time using WizUnzip 3.0, migrate any of earlier WizUnZip options from WIN.INI
   to WIZUNZIP.INI.
*/
GetPrivateProfileString(szAppName, szFirstUse, szYes, lpumb->szBuffer, 256, szWizUnzipIniFile);
if ((fFirstUse = !lstrcmpi(lpumb->szBuffer, szYes))!=0) /* first time used as WizUnZip 3.0   */
   {
#ifndef WIZZIP
   GetProfileString(szAppName, szRecreateDirsKey, szYes, lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   WritePrivateProfileString(szAppName, szRecreateDirsKey, lpumb->szBuffer, szWizUnzipIniFile);

/* Don't propagate translate option. Its meaning has changed. Use default: No   */

   GetProfileString(szAppName, szOverwriteKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   WritePrivateProfileString(szAppName, szOverwriteKey, lpumb->szBuffer, szWizUnzipIniFile);

   GetProfileString(szAppName, szFormatKey, "long", lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   WritePrivateProfileString(szAppName, szFormatKey, lpumb->szBuffer, szWizUnzipIniFile);

   GetProfileString(szAppName, szUnzipToZipDirKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   WritePrivateProfileString(szAppName, szUnzipToZipDirKey, lpumb->szBuffer, szWizUnzipIniFile);

   GetProfileString(szAppName, szLBSelectionKey, "display", lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   WritePrivateProfileString(szAppName, szLBSelectionKey, lpumb->szBuffer, szWizUnzipIniFile);

   MigrateSoundOptions();   /* Translate former beep option to new sound option   */

   WriteProfileString(szAppName, NULL, NULL); /* delete [wizunzip] section of WIN.INI file */
#endif /* !WizZip */
/* Flag that this is no longer the first time.                              */
   WritePrivateProfileString(szAppName, szFirstUse, szNo, szWizUnzipIniFile);
/* After first use, all options come out of WIZUNZIP.INI file                  */
   }

/* Get initial Re-create dirs flag */
GetPrivateProfileString(szAppName, szRecreateDirsKey, szYes, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fRecreateDirs = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get show toolbar help flag */
GetPrivateProfileString(szAppName, szShowBubbleHelpKey, szYes, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fShowBubbleHelp = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get translate flag */
GetPrivateProfileString(szAppName, szTranslateKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fTranslate = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get initial display format: short or long */
GetPrivateProfileString(szAppName, szFormatKey, "long", lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fFormatLong = (WORD)(!lstrcmpi(lpumb->szBuffer, "long") ? 1 : 0);

/* Get overwrite option */
GetPrivateProfileString(szAppName, szOverwriteKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
lpDCL->Overwrite = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get prompt to overwrite option */
GetPrivateProfileString(szAppName, szPromptOverwriteKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
lpDCL->PromptToOverwrite = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get always save zip-to dir option */
GetPrivateProfileString(szAppName, szSaveUnZipToKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fSaveUnZipToDir = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get always save zip-from dir option */
GetPrivateProfileString(szAppName, szSaveUnZipFromKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fSaveUnZipFromDir = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get Unzip to .ZIP dir option: yes or no  */
GetPrivateProfileString(szAppName, szUnzipToZipDirKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fUnzipToZipDir = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get default "unzip-to" directory */
GetPrivateProfileString(szAppName, szDefaultUnzipToDir, "", lpumb->szUnzipToDirName, WIZUNZIP_MAX_PATH, szWizUnzipIniFile);

/* Get Automatically Clear Status Window option */
GetPrivateProfileString(szAppName, szAutoClearStatusKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fAutoClearStatus = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get Automatically Clear Display Window option */
GetPrivateProfileString(szAppName, szAutoClearDisplayKey, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);
uf.fAutoClearDisplay = (BOOL)(!lstrcmpi(lpumb->szBuffer, szYes));

/* Get default "unzip-from" directory */
GetPrivateProfileString(szAppName, szDefaultUnzipFromDir, "", lpumb->szUnzipFromDirName, WIZUNZIP_MAX_PATH, szWizUnzipIniFile);

/* Get default listbox selection operation */
GetPrivateProfileString(szAppName, szLBSelectionKey, "display", lpumb->szBuffer, OPTIONS_BUFFER_LEN, szWizUnzipIniFile);

for (i = 0; i < LBSELECTIONTABLE_ENTRIES &&
   lstrcmpi(LBSelectionTable[i], lpumb->szBuffer) ; i++);
InitSoundOptions();               /* initialize sound options         */
wLBSelection = IDM_LB_DISPLAY;      /* assume default is to display     */
if (i < LBSELECTIONTABLE_ENTRIES)
   wLBSelection = (WORD) (IDM_LB_EXTRACT + i);

hWndMain = CreateWindow(szAppName,  /* window class     */
        szAppName,                      /* window name      */
        WS_OVERLAPPEDWINDOW,            /* window style     */
        0,                              /* x position       */
        0,                              /* y position       */
        CW_USEDEFAULT,                  /* width            */
        0,                              /* height           */
        (HWND)0,                        /* parent handle    */
        (HWND)0,                        /* menu or child ID */
        hInstance,                      /* instance         */
        NULL);                          /* additional info  */

if ( !hWndMain )
   return 0;

CreateButtonBar(hWndMain);
#ifdef USEWIZUNZDLL
lpDCL->hWndMain = hWndMain;
lpDCL->hWndList = hWndList;
lpDCL->hInst = hInst;
#endif

UpdateButtons(hWndMain);
SizeWindow(hWndMain, TRUE);
/* Enable/Disable buttons */
WinAssert(hSelectAll);
EnableWindow( hSelectAll, FALSE);
WinAssert(hDeselectAll);
EnableWindow( hDeselectAll, FALSE);
WinAssert(hSelectPattern);
EnableWindow( hSelectPattern, FALSE);

/* On first use, throw up About box, saying what WizUnZip is, etc. */
if (fFirstUse)
   {
   PostMessage(hWndMain, WM_COMMAND, IDM_ABOUT, 0L);
   }
hHelpCursor = LoadCursor(hInstance, "HelpCursor");

WinAssert(hWndMain);
ShowWindow(hWndMain, nCmdShow);
UpdateWindow(hWndMain);

while ( GetMessage(&lpumb->msg, 0, 0, 0) )
   {
   if (hPatternSelectDlg == 0 || /* Pattern select dialog is non-modal */
      !IsDialogMessage(hPatternSelectDlg, &lpumb->msg))
      {
      if ( !TranslateAccelerator(hWndMain, hAccTable, &lpumb->msg) )
         {
         TranslateMessage(&lpumb->msg);
         DispatchMessage(&lpumb->msg);
         }
      }
   }
/* Don't turn on compiler aliasing or C7 will move */
/* the following assignment after the GlobalFree() */
/* which contains the memory for pumb! */
i = (int)lpumb->msg.wParam;

GlobalUnlock( hStrings );
GlobalFree( hStrings );
GlobalUnlock(hDCL);
GlobalFree(hDCL);
#ifdef USEWIZUNZDLL
FreeLibrary(hWinDll);
#else
DESTROYGLOBALS();
#endif
return i;
}
#endif /* WIZUNZIPDLL */

