/* Windows Info-ZIP Unzip Window Procedure, wndproc.c.
 * Author: Robert A. Heath, 157 Chartwell Rd., Columbia, SC 29210
 * I, Robert Heath, place this source code module in the public domain.
 *
 * Modifications: 1995 M. White
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "wingui\wizunzip.h"
#include "wingui\helpids.h"
#include "wingui\password.h"

#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include <shellapi.h>
#include <lzexpand.h>

#define MAKE_TABSTOP_TABLE_ENTRY(WNDHANDLE, ID) \
    { \
    TabStopTable[ID - TABSTOP_ID_BASE].lpfnOldFunc = \
       (FARPROC)GetWindowLong(WNDHANDLE, GWL_WNDPROC); \
    SetWindowLong(WNDHANDLE, GWL_WNDPROC, (LONG)lpfnKbd); \
    TabStopTable[ID - TABSTOP_ID_BASE].hWnd = WNDHANDLE; \
    }

#ifdef USEWIZUNZDLL
char Far HeadersS[]  = " Length    Date    Time    Name";
char Far HeadersS1[] = " ------    ----    ----    ----";
char Far HeadersL[]  =
  " Length  Method   Size  Ratio   Date    Time   CRC-32     Name";
char Far HeadersL1[] =
  " ------  ------   ----  -----   ----    ----   ------     ----";
char Far *Headers[][2] = { {HeadersS, HeadersS1}, {HeadersL, HeadersL1} };
#endif

/* Forward Refs
 */
static void GetArchiveDir(LPSTR lpszDestDir);
void GetDirectory(LPSTR lpDir);

HWND hPatternSelectDlg; /* pattern select modeless dialog   */
static UINT uCommDlgHelpMsg;   /* common dialog help message ID */
static DWORD dwCommDlgHelpId = HELPID_HELP; /* what to pass to WinHelp() */
static char szFormatKeyword[2][6] = { "short", "long" };
static BOOL move_flag = FALSE;
static BOOL rename_flag = FALSE;

LPSTR lpchLast;

/* Trailers are the lines just above the totals */
static char * __based(__segname("STRINGS_TEXT")) szTrailers[2] = {
" ------                    -------",
" ------          ------  ---                              -------"
} ;
static char __based(__segname("STRINGS_TEXT")) szCantChDir[] =
   "Internal error: Cannot change directory. Common dialog error code is 0x%lX.";

/* size of char in SYSTEM font in pixels */
#ifndef WIN32
short dxChar, dyChar;
#else
long dxChar, dyChar;
#endif

/* button control table -- one entry for
 * each of 4 entries. Indexed by the window ID relative to
 * the first tabstop (TABSTOP_ID_BASE).
 */
TabStopEntry TabStopTable[TABSTOP_TABLE_ENTRIES];

LPSTR lstrrchr(LPSTR lpszSrc, char chFind)
{
LPSTR   lpszFound = (LPSTR)0;
LPSTR   lpszT;

if ( lpszSrc )
   {
   for (lpszT = lpszSrc; *lpszT; ++lpszT)
       {
       if ((*lpszT) == chFind)
          lpszFound = lpszT;
       }
   }
return lpszFound;
}

/* Copy only the path portion of current file name into
 * given buffer, lpszDestDir, translate into ANSI.
 */
static void GetArchiveDir(LPSTR lpszDestDir)
{
LPSTR lpchLast;

/* strip off filename to make directory name    */
OemToAnsi(lpumb->szFileName, lpszDestDir);
if ((lpchLast = lstrrchr(lpszDestDir, '\\'))!=0)
   *lpchLast = '\0';
else if ((lpchLast = lstrrchr(lpszDestDir, ':'))!=0)
   *(++lpchLast) = '\0'; /* clobber char AFTER the colon! */
}

void GetDirectory(LPSTR lpDir)
{
LPSTR lpchLast;
/* If no '\\' then set directory name to "" */
if ((lpchLast = lstrrchr(lpDir, '\\')) == 0)
   {
   lpDir[0] = '\0';
   return;
   }
/* strip off filename to make directory name    */
if ((lpchLast = lstrrchr(lpDir, '\\'))!=0)
   *lpchLast = '\0';
else if ((lpchLast = lstrrchr(lpDir, ':'))!=0)
   *(++lpchLast) = '\0'; /* clobber char AFTER the colon! */
}

/*
 * FUNCTION: SetCaption(HWND hWnd)
 * PURPOSE: Set new caption for main window
 */
void
SetCaption(HWND hWnd)
{
#define SIMPLE_NAME_LEN 15
static BOOL FirstTime = TRUE;
WORD wMenuState;
char szSimpleFileName[SIMPLE_NAME_LEN+1];  /* just the 8.3 part in ANSI char set */
LPSTR lpszFileNameT;        /* pointer to simple filename               */
BOOL    fIconic = IsIconic(hWnd);   /* is window iconic ?   */
BOOL fWndEnabled; /* Is button to be enabled? */

/* point to simple filename in OEM char set */
if ((((lpszFileNameT = lstrrchr(lpumb->szFileName, '\\'))!=0) ||
   ((lpszFileNameT = lstrrchr(lpumb->szFileName, ':')))!=0))
   lpszFileNameT++;
else
   lpszFileNameT = lpumb->szFileName;

#ifndef WIN32
_fstrncpy(szSimpleFileName, lpszFileNameT, SIMPLE_NAME_LEN);
#else
strncpy(szSimpleFileName, lpszFileNameT, SIMPLE_NAME_LEN);
#endif
szSimpleFileName[SIMPLE_NAME_LEN] = '\0'; /* force termination */

wMenuState = (WORD)(szSimpleFileName[0] ? MF_ENABLED : MF_GRAYED);
fWndEnabled = (BOOL) (szSimpleFileName[0] ? TRUE : FALSE);
/* Enable/Disable menu items */
EnableMenuItem(hMenu, IDM_SELECT_ALL, wMenuState|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_DESELECT_ALL, wMenuState|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_SELECT_BY_PATTERN, wMenuState|MF_BYCOMMAND);

/* Enable/Disable buttons */
if (!FirstTime)
   {
   WinAssert(hSelectAll);
   EnableWindow( hSelectAll, fWndEnabled);
   WinAssert(hDeselectAll);
   EnableWindow( hDeselectAll, fWndEnabled);
   WinAssert(hSelectPattern);
   EnableWindow( hSelectPattern, fWndEnabled);
   }

if (!szSimpleFileName[0])
   lstrcpy(szSimpleFileName, "No Zip File");
OemToAnsi(szSimpleFileName, szSimpleFileName);
wsprintf(lpumb->szBuffer, "%s - %s %s %s",
               (LPSTR)szAppName,
               (LPSTR)(szSimpleFileName),
               (LPSTR)(!fIconic && lpumb->szUnzipToDirName[0] ? " - " : ""),
               (LPSTR)(!fIconic ? lpumb->szUnzipToDirName : ""));
SetWindowText(hWnd, lpumb->szBuffer);
FirstTime = FALSE;
}

static void ManageStatusWnd(WPARAM wParam);
static void ManageStatusWnd(WPARAM wParam)
{
int nWndState;  /* ShowWindow state     */
BOOL fWndEnabled;   /* Enable Window state */

if (LOWORD(wParam) == IDM_SPLIT)
   {
   WinAssert(hWndStatus);
   ShowWindow(hWndStatus, SW_RESTORE);
   UpdateWindow(hWndStatus);
   fWndEnabled = TRUE;
   nWndState = SW_SHOWNORMAL;
   }
else    /* Message window goes to maximum state     */
   {
   nWndState = SW_HIDE;    /* assume max state     */
   fWndEnabled = FALSE;
   }

wWindowSelection = LOWORD(wParam); /* window selection: listbox, status, both */
WinAssert(hWndList);
EnableWindow( hWndList, fWndEnabled);
UpdateWindow( hWndList);
ShowWindow( hWndList, nWndState);

if (LOWORD(wParam) == IDM_SPLIT) /* uncover buttons    */
   {
   UpdateButtons(hWndMain);    /* restore to proper state  */
   }
else
   {   /* Disable Extract, Display, Test, and Comment buttons */
   WinAssert(hExtract);
   EnableWindow( hExtract, fWndEnabled);
   WinAssert(hTest);
   EnableWindow( hTest, fWndEnabled);
   WinAssert(hDisplay);
   EnableWindow( hDisplay, fWndEnabled);
   WinAssert(hShowComment);
   EnableWindow( hShowComment, fWndEnabled);
   }

UpdateWindow(hExtract);
UpdateWindow(hTest);
UpdateWindow(hDisplay);
UpdateWindow(hShowComment);

if (LOWORD(wParam) == IDM_MAX_STATUS)   /* message box max'd out */
   {
   WinAssert(hWndStatus);
   ShowWindow(hWndStatus, SW_SHOWMAXIMIZED);
   }
SetFocus(hWndStatus);
SizeWindow(hWndMain, FALSE);
}

/*
 * FUNCTION: WizunzipWndProc(HWND, unsigned, WORD, LONG)
 *
 * PURPOSE:  Processes messages
 *
 * MESSAGES:
 *
 * WM_DESTROY      - destroy window
 * WM_SIZE         - window size has changed
 * WM_QUERYENDSESSION - willing to end session?
 * WM_ENDSESSION   - end Windows session
 * WM_CLOSE        - close the window
 * WM_SIZE         - window resized
 * WM_PAINT        - windows needs to be painted
 * WM_DROPFILES    - open a dropped file
 * COMMENTS:
 * WM_COMMAND processing:
 *    IDM_OPEN -  open a new file.
 *    IDM_EXIT -  exit.
 *    IDM_ABOUT - display "About" box.
 */
long WINAPI WizUnzipWndProc(HWND hWnd, WORD wMessage, WPARAM wParam, LPARAM lParam)
{

HDC hDC;                /* device context       */
TEXTMETRIC    tm;           /* text metric structure    */
POINT point;
char drive;
#ifndef WIN32
FARPROC lpfnAbout, lpfnSelectDir;
#endif
FARPROC lpfnKbd;
char *ptr;

switch (wMessage)
    {
    case WM_CREATE: /* create  window       */
      hInst = ((LPCREATESTRUCT)lParam)->hInstance;
      lpfnKbd = MakeProcInstance((FARPROC)KbdProc, hInst);
      hAccTable = LoadAccelerators(hInst, "WizunzipAccels");
      hBrush = CreateSolidBrush(GetSysColor(BG_SYS_COLOR)); /* background */

      hMenu = GetMenu(hWnd);
      /* Check Menu items to reflect .INI settings */
      CheckMenuItem(hMenu, IDM_RECR_DIR_STRUCT, MF_BYCOMMAND |
                            (uf.fRecreateDirs ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, IDM_SHOW_BUBBLE_HELP, MF_BYCOMMAND |
                            (uf.fShowBubbleHelp ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, (IDM_SHORT+uf.fFormatLong), MF_BYCOMMAND | MF_CHECKED);
      CheckMenuItem(hMenu, IDM_PROMPT_TO_OVERWRITE, MF_BYCOMMAND |
                            (lpDCL->PromptToOverwrite ? MF_CHECKED : MF_UNCHECKED));
      if (!lpDCL->PromptToOverwrite)
         {
         ModifyMenu(hMenu, IDM_OVERWRITE, MF_BYCOMMAND |
                 MF_ENABLED, IDM_OVERWRITE,
                 (lpDCL->Overwrite ? "Always Overwrite Existing Files":
                 "Never Overwrite Existing Files"));
         CheckMenuItem(hMenu, IDM_OVERWRITE, MF_BYCOMMAND |MF_CHECKED);
         CheckMenuItem(hMenu, IDM_EXTRACT_ONLY_NEWER, MF_BYCOMMAND |
                            (lpDCL->ExtractOnlyNewer ? MF_CHECKED : MF_UNCHECKED));
         }
      else
         {
         EnableMenuItem(hMenu, IDM_OVERWRITE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
         EnableMenuItem(hMenu, IDM_EXTRACT_ONLY_NEWER, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
         }
      CheckMenuItem(hMenu, IDM_TRANSLATE, MF_BYCOMMAND |
                            (uf.fTranslate ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, IDM_SPACE_TO_UNDERSCORE, MF_BYCOMMAND |
                            (lpDCL->SpaceToUnderscore ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, IDM_SAVE_UNZIP_TO_DIR, MF_BYCOMMAND |
                            (uf.fSaveUnZipToDir ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, IDM_SAVE_UNZIP_FROM_DIR, MF_BYCOMMAND |
                            (uf.fSaveUnZipFromDir ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, wLBSelection, MF_BYCOMMAND | MF_CHECKED);
      CheckMenuItem(hMenu, IDM_UNZIP_TO_ZIP_DIR, MF_BYCOMMAND |
                            (uf.fUnzipToZipDir ? MF_CHECKED : MF_UNCHECKED));
      EnableMenuItem(hMenu, IDM_CHDIR, MF_BYCOMMAND |
                            (uf.fUnzipToZipDir ? MF_GRAYED : MF_ENABLED));
      CheckMenuItem(hMenu, IDM_AUTOCLEAR_STATUS, MF_BYCOMMAND |
                            (uf.fAutoClearStatus ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(hMenu, IDM_AUTOCLEAR_DISPLAY, MF_BYCOMMAND |
                            (uf.fAutoClearDisplay ? MF_CHECKED : MF_UNCHECKED));
      EnableMenuItem(hMenu, IDM_COPY, MF_GRAYED);

      /* Get an hourglass cursor to use during file transfers */
      hHourGlass = LoadCursor(0, IDC_WAIT);

      hFixedFont = GetStockObject(SYSTEM_FIXED_FONT);
      hDC = GetDC(hWnd);  /* get device context */
      hOldFont = SelectObject(hDC, hFixedFont);
      GetTextMetrics(hDC, &tm);
      ReleaseDC(hWnd, hDC);
      dxChar = tm.tmAveCharWidth;
      dyChar = tm.tmHeight + tm.tmExternalLeading;

      WinAssert(hWnd);
#ifndef WIN32
      hWndList = CreateWindow("listbox", NULL,
                        WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOTIFY|
                        LBS_EXTENDEDSEL,
                        0, 0,
                        0, 0,
                        hWnd, (HMENU)IDM_LISTBOX,
                        GetWindowWord (hWnd, GWW_HINSTANCE), NULL);
#else
      hWndList = CreateWindow("listbox", NULL,
                        WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOTIFY|
                        LBS_EXTENDEDSEL,
                        0, 0,
                        0, 0,
                        hWnd, (HMENU)IDM_LISTBOX,
                        (HANDLE)GetWindowLong (hWnd, GWL_HINSTANCE), NULL);
#endif
      WinAssert(hWndList);
      MAKE_TABSTOP_TABLE_ENTRY(hWndList, IDM_LISTBOX);
      SendMessage(hWndList, WM_SETFONT, (WPARAM)hFixedFont, FALSE);
      ShowWindow(hWndList, SW_SHOW);
      UpdateWindow(hWndList);         /* show it now! */
      WinAssert(hWnd);

#ifndef WIN32
      hWndStatus = CreateWindow(szStatusClass, "Status/Display",
                            WS_CHILD|WS_SYSMENU|WS_VISIBLE|WS_BORDER|WS_HSCROLL|WS_VSCROLL|WS_MAXIMIZEBOX|WS_CAPTION,
                            0, 0,
                            0, 0,
                            hWnd, (HMENU)IDM_STATUS,
                            GetWindowWord (hWnd, GWW_HINSTANCE), NULL);
#else
      hWndStatus = CreateWindow(szStatusClass, "Status/Display",
                            WS_CHILD|WS_SYSMENU|WS_VISIBLE|WS_BORDER|WS_HSCROLL|WS_VSCROLL|WS_MAXIMIZEBOX|WS_CAPTION,
                            0, 0,
                            0, 0,
                            hWnd, (HMENU)IDM_STATUS,
                            (HANDLE)GetWindowLong (hWnd, GWL_HINSTANCE), NULL);
#endif
      WinAssert(hWndStatus);
      SendMessage(hWndStatus, WM_SETFONT, (WPARAM)hFixedFont, TRUE);
      MAKE_TABSTOP_TABLE_ENTRY(hWndStatus, IDM_STATUS);

      /* if file spec'd on entry */
      if (lpumb->szFileName[0])
         {
         LPSTR lpch;
         extern int ofretval; /* return value from initial open if filename given
                                during WinMain() */
         /* If valid filename change dir to where it lives */
         if (ofretval >= 0)
            {
            GetArchiveDir(lpumb->szDirName); /* get archive dir. in ANSI char set */
            if (uf.fUnzipToZipDir || /* unzipping to same directory as archive */
               lpumb->szUnzipToDirName[0] == '\0') /* or no default */
               {
               /* take only path portion */
               lstrcpy(lpumb->szUnzipToDirName, lpumb->szDirName);
               }
            lstrcpy(lpumb->szBuffer, lpumb->szDirName);
            DlgDirList(hWnd, lpumb->szBuffer, 0, 0, 0); /* go to where archive lives */
            }
         else /* bad file name */
            {
            OemToAnsi(lpumb->szFileName, lpumb->szFileName); /* temporarily OEM */
            if ((((lpch = lstrrchr(lpumb->szFileName, '\\'))!=0) ||
                 ((lpch = lstrrchr(lpumb->szFileName, ':')))!=0))
               lpch++; /* point to filename */
            else
               lpch = lpumb->szFileName;

            wsprintf (lpumb->szBuffer, "Cannot open %s", lpch);
            MessageBox (hWnd, lpumb->szBuffer, szAppName, MB_ICONINFORMATION | MB_OK);
            lpumb->szFileName[0] = '\0'; /* pretend filename doesn't exist  */
            lpumb->szDirName[0] = '\0'; /* pretend archive dir. doesn't exist  */
            }
        }
        SetCaption(hWnd);
        UpdateListBox(hWnd);
        SendMessage(hWndList, LB_SETSEL, 1, 0L);
        uCommDlgHelpMsg = RegisterWindowMessage((LPSTR)HELPMSGSTRING); /* register open help message */

        if ( uf.fCanDragDrop )
            DragAcceptFiles( hWnd, TRUE );
        break;

    case WM_SETFOCUS:
        SetFocus((wWindowSelection == IDM_MAX_STATUS) ? hWndStatus : hWndList);
        break;

    case WM_ACTIVATE:
        SetCaption(hWnd);
        return DefWindowProc(hWnd, wMessage, wParam, lParam);

    case WM_SIZE:
        SizeWindow(hWnd, FALSE);
        break;

    /*
     * NOTE: WM_CTLCOLOR is not a supported code under Win 32
     */
#ifdef WIN32
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
#else
    case WM_CTLCOLOR: /* color background of buttons and statics */
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
#endif
           {
           SetBkMode((HDC)wParam, TRANSPARENT);
           SetBkColor((HDC)wParam, GetSysColor(BG_SYS_COLOR)); /* custom b.g. color */
           SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
           UnrealizeObject(hBrush);
           point.x = point.y = 0;
           ClientToScreen(hWnd, &point);
#ifndef WIN32
           SetBrushOrg((HDC)wParam, point.x, point.y);
#else
           SetBrushOrgEx((HDC)wParam, point.x, point.y, NULL);
#endif
           return ((DWORD)hBrush);
           }
        /* fall thru to WM_SYSCOMMAND */

    case WM_SYSCOMMAND:
        return DefWindowProc( hWnd, wMessage, wParam, lParam );

   case WM_INITMENUPOPUP: /* popup menu pre-display      */
      {
/*   BOOL bOutcome; MW: Removed to allow multiple pop-up menus */

      switch  (LOWORD(lParam)) {
      case EDIT_MENUITEM_POS: /* index of Edit pop-up      */
/*         bOutcome = EnableMenuItem((HMENU)wParam, IDM_COPY, */
         EnableMenuItem((HMENU)wParam, IDM_COPY,
            (UINT)(StatusInWindow() ? MF_ENABLED : MF_GRAYED));
         EnableWindow(hCopyStatus, (UINT)(StatusInWindow() ? TRUE : FALSE));

/*         assert(bOutcome != -1);    -1 => Menu item doesn't exist */
         break;
      }
      }
      break;

    case WM_COMMAND:
        /* Was F1 just pressed in a menu, or are we in help mode */
        /* (Shift-F1)? */

        if (uf.fHelp)
           {
           DWORD dwHelpContextId =
              (LOWORD(wParam) == IDM_OPEN)            ? (DWORD) HELPID_OPEN :
              (LOWORD(wParam) == IDM_EXIT)            ? (DWORD) HELPID_EXIT_CMD :
              (LOWORD(wParam) == IDM_CHDIR)           ? (DWORD) HELPID_CHDIR :
              (LOWORD(wParam) == IDM_SHORT)           ? (DWORD) HELPID_SHORT :
              (LOWORD(wParam) == IDM_LONG)            ? (DWORD) HELPID_LONG :
              (LOWORD(wParam) == IDM_HELP)            ? (DWORD) HELPID_HELP :
              (LOWORD(wParam) == IDM_HELP_HELP)       ? (DWORD) HELPID_HELP_HELP :
              (LOWORD(wParam) == IDM_ABOUT)           ? (DWORD) HELPID_ABOUT :
              (LOWORD(wParam) == IDM_RECR_DIR_STRUCT) ? (DWORD) HELPID_RECR_DIR_STRUCT :
              (LOWORD(wParam) == IDM_SHOW_BUBBLE_HELP) ? (DWORD) HELPID_SHOW_BUBBLE_HELP :
              (LOWORD(wParam) == IDM_OVERWRITE)       ? (DWORD) HELPID_OVERWRITE :
              (LOWORD(wParam) == IDM_PROMPT_TO_OVERWRITE) ? (DWORD) HELPID_PROMPT_TO_OVERWRITE :
              (LOWORD(wParam) == IDM_TRANSLATE)       ? (DWORD) HELPID_TRANSLATE :
              (LOWORD(wParam) == IDM_UNZIP_TO_ZIP_DIR)? (DWORD) HELPID_UNZIP_TO_ZIP_DIR :
              (LOWORD(wParam) == IDM_UNZIP_FROM_DIR)? (DWORD) HELPID_UNZIP_FROM_DIR :
              (LOWORD(wParam) == IDM_LISTBOX)         ? (DWORD) HELPID_LISTBOX :
              (LOWORD(wParam) == IDM_EXTRACT)         ? (DWORD) HELPID_EXTRACT :
              (LOWORD(wParam) == IDM_DISPLAY)         ? (DWORD) HELPID_DISPLAY :
              (LOWORD(wParam) == IDM_TEST)            ? (DWORD) HELPID_TEST :
              (LOWORD(wParam) == IDM_SHOW_COMMENT)    ? (DWORD) HELPID_SHOW_COMMENT :
              (LOWORD(wParam) == IDM_LB_EXTRACT)      ? (DWORD) HELPID_LB_EXTRACT :
              (LOWORD(wParam) == IDM_LB_DISPLAY)      ? (DWORD) HELPID_LB_DISPLAY :
              (LOWORD(wParam) == IDM_LB_TEST)         ? (DWORD) HELPID_LB_TEST :
              (LOWORD(wParam) == IDM_DESELECT_ALL)    ? (DWORD) HELPID_DESELECT_ALL :
              (LOWORD(wParam) == IDM_SELECT_ALL)      ? (DWORD) HELPID_SELECT_ALL :
              (LOWORD(wParam) == IDM_SELECT_BY_PATTERN) ? (DWORD) HELPID_SELECT_BY_PATTERN :
              (LOWORD(wParam) == IDM_AUTOCLEAR_STATUS) ? (DWORD) HELPID_AUTOCLEAR_STATUS :
              (LOWORD(wParam) == IDM_AUTOCLEAR_DISPLAY) ? (DWORD) HELPID_AUTOCLEAR_DISPLAY :
              (LOWORD(wParam) == IDM_CLEAR_STATUS)    ? (DWORD) HELPID_CLEAR_STATUS :
              (LOWORD(wParam) == IDM_SOUND_OPTIONS)  ? (DWORD) HELPID_SOUND_OPTIONS :
              (LOWORD(wParam) == IDM_MAX_LISTBOX)  ? (DWORD) HELPID_MAX_LISTBOX :
              (LOWORD(wParam) == IDM_MAX_STATUS)  ? (DWORD) HELPID_MAX_STATUS :
              (LOWORD(wParam) == IDM_SPLIT)  ? (DWORD) HELPID_SPLIT :
              (LOWORD(wParam) == IDM_SAVE_UNZIP_FROM_DIR)  ? (DWORD) HELPID_SAVE_UNZIP_FROM_DIR :
              (LOWORD(wParam) == IDM_SAVE_UNZIP_TO_DIR)  ? (DWORD) HELPID_SAVE_UNZIP_TO_DIR :
              (LOWORD(wParam) == IDM_EXTRACT_ONLY_NEWER)  ? (DWORD) HELPID_EXTRACT_ONLY_NEWER :
              (LOWORD(wParam) == IDM_SPACE_TO_UNDERSCORE)  ? (DWORD) HELPID_SPACE_TO_UNDERSCORE :
              (LOWORD(wParam) == IDM_EDIT)  ? (DWORD) HELPID_EDIT :
              (LOWORD(wParam) == IDM_PATH)  ? (DWORD) HELPID_PATH :
              (LOWORD(wParam) == IDM_COMMENT)  ? (DWORD) HELPID_COMMENT :
              (LOWORD(wParam) == IDM_COPY)  ? (DWORD) HELPID_COPY :
              (LOWORD(wParam) == IDM_STATUS)  ? (DWORD) HELPID_STATUS :
              (LOWORD(wParam) == IDM_CHDIR)  ? (DWORD) HELPID_CHDIR :
              (LOWORD(wParam) == IDM_PASSWORD_HELP)  ? (DWORD) HELPID_PASSWORD :
              (LOWORD(wParam) == IDM_COPY_ARCHIVE)  ? (DWORD) HELPID_COPY_ARCHIVE :
              (LOWORD(wParam) == IDM_MOVE_ARCHIVE)  ? (DWORD) HELPID_MOVE_ARCHIVE :
              (LOWORD(wParam) == IDM_DELETE_ARCHIVE)  ? (DWORD) HELPID_DELETE_ARCHIVE :
              (LOWORD(wParam) == IDM_RENAME_ARCHIVE)  ? (DWORD) HELPID_RENAME_ARCHIVE :
              (LOWORD(wParam) == IDM_MAKE_DIR)  ? (DWORD) HELPID_MAKEDIR_HELP :
              (LOWORD(wParam) == IDM_MAKEDIR_HELP)  ? (DWORD) HELPID_MAKEDIR_HELP :
              (LOWORD(wParam) == IDM_SAVE_AS_DEFAULT)  ? (DWORD) HELPID_SAVE_AS_DEFAULT :
                                      (DWORD) 0L;

            if (!dwHelpContextId)
               {
               MessageBox( hWnd, "Help not available for Help Menu item",
                          "Help Example", MB_OK);
               return DefWindowProc(hWnd, wMessage, wParam, lParam);
               }

            uf.fHelp = FALSE;
            WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
        }
        else /* not in help mode */
        {
      RECT  rClient;
            switch (LOWORD(wParam))
            {
            case IDM_OPEN:
                /* If unzipping separately and previous file exists,
                 * go to directory where archive lives.
                 */
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );

            /* If not unzipping to same directory as archive and
             * file already open, go to where file lives.
             * If extracting to different directory, return to
             * that directory after selecting archive to open.
             */
            if (lpumb->szUnzipFromDirName[0])
               {
               lstrcpy(lpumb->szDirName, lpumb->szUnzipFromDirName);
               }
            if (lpumb->szFileName[0])
               {
               /* strip off filename to make directory name    */
               GetArchiveDir(lpumb->szDirName);
               }
            else
               {
               if (!lpumb->szUnzipFromDirName[0])
                  lpumb->szDirName[0] = '\0'; /* assume no dir   */
               }
            lpumb->szBuffer[0] = '\0';
#ifndef WIN32
            _fmemset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#else
            memset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#endif
            lpumb->ofn.lStructSize = sizeof(OPENFILENAME);
            lpumb->ofn.hwndOwner = hWnd;
            lpumb->ofn.lpstrFilter = "Zip Files (*.zip)\0*.zip\0Self-extracting Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";
            lpumb->ofn.nFilterIndex = 1;
            lpumb->ofn.lpstrFile = lpumb->szFileName;
            lpumb->szFileName[0] = '\0';   /* no initial filename   */
            lpumb->ofn.nMaxFile = WIZUNZIP_MAX_PATH;
            lpumb->ofn.lpstrFileTitle = lpumb->szBuffer; /* ignored */
            lpumb->ofn.lpstrInitialDir = (LPSTR)(!lpumb->szDirName[0] ? NULL : lpumb->szDirName);

            lpumb->ofn.nMaxFileTitle = OPTIONS_BUFFER_LEN;
            lpumb->ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                   OFN_HIDEREADONLY;
            dwCommDlgHelpId = HELPID_OPEN; /* specify correct help for open dlg   */
#if defined (CRYPT) && !defined (USEWIZUNZDLL)
            /*
             This if statement insures that when you go between zip files,
             the password is nulled out. Not necessary if using dll.
            */
            if (G.key)
               G.key = (char *)NULL;
#endif
            if (GetOpenFileName(&lpumb->ofn))   /* if successful file open  */
               {
               AnsiToOem(lpumb->szFileName, lpumb->szFileName); /* retain as OEM */
               GetArchiveDir(lpumb->szDirName); /* get archive dir name in ANSI */
               /*
               Save the last "unzip from" directory
               */
               if (lpumb->szUnzipFromDirName[1] == ':')
                  drive = lpumb->szUnzipFromDirName[0];
               else
                  drive = '\0';

               if (uf.fSaveUnZipFromDir &&
                  (toupper(drive) != 'A') &&
                  (toupper(drive) != 'B'))
                  {
                  lstrcpy(lpumb->szUnzipFromDirName, lpumb->szDirName);
                  if (lpumb->szUnzipFromDirName[strlen(lpumb->szUnzipFromDirName)-1]
                     == ':')
                     lstrcat(lpumb->szUnzipFromDirName, "\\");
                  WritePrivateProfileString(szAppName, szDefaultUnzipFromDir,
                                 lpumb->szUnzipFromDirName, szWizUnzipIniFile);
                  }
               else
                  WritePrivateProfileString(szAppName, szDefaultUnzipFromDir,
                                 "", szWizUnzipIniFile);
               if (uf.fUnzipToZipDir || /* unzipping to same directory as archive */
                  lpumb->szUnzipToDirName[0] == '\0') /* or no default */
                  {
                  /* strip off filename to make directory name    */
                  lstrcpy(lpumb->szUnzipToDirName, lpumb->szDirName);
                  }
               }
               UpdateListBox(hWnd); /* fill in list box */
               SendMessage(hWndList, LB_SETSEL, 1, 0L);
               UpdateButtons(hWnd); /* update state of buttons */

               /* Update archive totals area */
               WinAssert(hWndList);
               GetClientRect( hWndList, &rClient );
               OffsetRect( &rClient, 0, dyChar );
               rClient.top = rClient.bottom;
               rClient.bottom = rClient.top + (6*dyChar);
               InvalidateRect( hWnd, &rClient, TRUE);
               UpdateWindow( hWnd );
               SetCaption(hWnd);
               if ( uf.fCanDragDrop )
                  DragAcceptFiles( hWnd, TRUE );
               break;

            case IDM_CHDIR:
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );

#ifndef WIN32
                _fmemset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#else
                memset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#endif
                lpumb->ofn.lStructSize = sizeof(OPENFILENAME);
                lpumb->ofn.hwndOwner = hWnd;
                lpumb->ofn.hInstance = hInst;
                lpumb->ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
                lpumb->ofn.nFilterIndex = 1;
                lstrcpy(lpumb->szUnzipToDirNameTmp, lpumb->szUnzipToDirName); /* initialize */
                { /* Braces are here to allow the declaration of uDirNameLen */
#ifndef WIN32
            size_t uDirNameLen = _fstrlen(lpumb->szUnzipToDirNameTmp);
#else
            size_t uDirNameLen = strlen(lpumb->szUnzipToDirNameTmp);
#endif
               /* If '\\' not at end of directory name, add it now.
                */
                if (uDirNameLen > 0 && lpumb->szUnzipToDirNameTmp[uDirNameLen-1] != '\\')
                   lstrcat(lpumb->szUnzipToDirNameTmp, "\\");
                }
                lstrcat(lpumb->szUnzipToDirNameTmp,
                    "johnny\376\376.\375\374\373"); /* fake name */
                lpumb->ofn.lpstrFile = lpumb->szUnzipToDirNameTmp; /* result goes here! */
                lpumb->ofn.nMaxFile = WIZUNZIP_MAX_PATH;
                lpumb->ofn.lpstrFileTitle = NULL;
                lpumb->ofn.nMaxFileTitle = OPTIONS_BUFFER_LEN; /* ignored ! */
                lpumb->ofn.lpstrInitialDir = lpumb->szUnzipToDirName;
                lpumb->ofn.lpstrTitle = (LPSTR)"Unzip To";
                lpumb->ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK |
                           OFN_HIDEREADONLY|OFN_ENABLETEMPLATE|OFN_NOCHANGEDIR;
#ifndef WIN32
                lpfnSelectDir =
                  MakeProcInstance((FARPROC)SelectDirProc, hInst);
#ifndef MSC
                (UINT CALLBACK *)lpumb->ofn.lpfnHook = (UINT CALLBACK *)lpfnSelectDir;
#else
                lpumb->ofn.lpfnHook = lpfnSelectDir;
#endif

#else
                lpumb->ofn.lpfnHook = (LPOFNHOOKPROC)SelectDirProc;
#endif
                lpumb->ofn.lpTemplateName = "SELDIR";   /* see seldir.dlg   */
                dwCommDlgHelpId = HELPID_CHDIR; /* in case user hits "help" button */
                if (GetSaveFileName(&lpumb->ofn)) /* successfully got dir name ? */
                   {
#ifndef WIN32
                   ptr = _fstrrchr(lpumb->ofn.lpstrFile, '\\');
#else
                   ptr = strrchr(lpumb->ofn.lpstrFile, '\\');
#endif
                   if (ptr != NULL)
                      lpumb->ofn.lpstrFile[(int)(ptr - lpumb->ofn.lpstrFile)] = '\0';
                   lstrcpy(lpumb->szUnzipToDirName, lpumb->ofn.lpstrFile); /* save result */

                   if (lpumb->szUnzipToDirName[1] == ':')
                      {
                      drive = lpumb->szUnzipToDirName[0];
                      if (lstrlen(lpumb->szUnzipToDirName) == 2)
                         { /* We only have a drive letter and a colon */
                         lstrcat(lpumb->szUnzipToDirName, "\\");
                         }
                      }
                   else
                      drive = '\0';
                   if (uf.fSaveUnZipToDir &&
                      (toupper(drive) != 'A') &&
                      (toupper(drive) != 'B'))
                       {
                  /* MW: Always save last directory written to */
                       WritePrivateProfileString(szAppName, szDefaultUnzipToDir,
                          lpumb->szUnzipToDirName, szWizUnzipIniFile);
                       }
                   SetCaption(hWnd);
                   }
                else /* either real error or canceled */
                   {
                   DWORD dwExtdError = CommDlgExtendedError(); /* debugging */

                   if (dwExtdError != 0L) /* if not canceled then real error */
                      {
                      wsprintf (lpumb->szBuffer, szCantChDir, dwExtdError);
                      MessageBox (hWnd, lpumb->szBuffer, szAppName, MB_ICONINFORMATION | MB_OK);
                      }
                   else
                      {
                      RECT mRect;

                      GetWindowRect(hWndMain, &mRect);
                      InvalidateRect(hWndMain, &mRect, TRUE);
                      SendMessage(hWndMain, WM_SIZE, SIZE_RESTORED,
                        MAKELONG(mRect.right-mRect.left, mRect.top-mRect.bottom));
                      UpdateWindow(hWndMain);
                      }
                   }
#ifndef WIN32
                FreeProcInstance(lpfnSelectDir);
#endif
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, TRUE );
                break;

            case IDM_DELETE_ARCHIVE:
                {
                char szStr[WIZUNZIP_MAX_PATH];
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );
                sprintf(szStr, "Are you sure you want to delete\n%s", lpumb->szFileName);
                if (MessageBox(hWnd, szStr, "Deleting File", MB_ICONSTOP | MB_OKCANCEL) == IDOK)
                   {
                   remove(lpumb->szFileName);
                   fprintf(stdout,"Deleting %s\n", lpumb->szFileName);
                   SendMessage(hWndList, LB_RESETCONTENT, 0, 0);
                   lpumb->szFileName[0] = '\0';
                   SetCaption(hWnd);
                   UpdateButtons(hWnd); /* update state of buttons */
                   }
                if ( uf.fCanDragDrop )
                  DragAcceptFiles( hWnd, TRUE );
                break;
                }
            case IDM_MOVE_ARCHIVE:
                /*
                 * Yes - this can be done under Win32 differently, but
                 * you have to go through some hoops, and this just makes
                 * it easier.
                 */
                move_flag = TRUE;
                dwCommDlgHelpId = HELPID_MOVE_ARCHIVE; /* in case user hits "help" button */
            case IDM_RENAME_ARCHIVE:
                if (!move_flag)
                   {
                   rename_flag = TRUE;
                   dwCommDlgHelpId = HELPID_RENAME_ARCHIVE; /* in case user hits "help" button */
                   }
            case IDM_COPY_ARCHIVE:
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );
                if ((!move_flag) && (!rename_flag))
                   dwCommDlgHelpId = HELPID_COPY_ARCHIVE; /* in case user hits "help" button */
                CopyArchive(hWnd, move_flag, rename_flag);
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, TRUE );
                move_flag = FALSE;
                rename_flag = FALSE;
                break;

            case IDM_EXIT:
                SendMessage(hWnd, WM_CLOSE, 0, 0L);
                break;

            case IDM_HELP:  /* Display Help */
                WinHelp(hWnd,szHelpFileName,HELP_INDEX,0L);
                break;

            case IDM_HELP_HELP:
                WinHelp(hWnd,"WINHELP.HLP",HELP_INDEX,0L);
                break;

            case IDM_ABOUT:
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );
#ifndef WIN32
                lpfnAbout = MakeProcInstance(AboutProc, hInst);
                DialogBox(hInst, "About", hWnd, lpfnAbout);
                FreeProcInstance(lpfnAbout);
#else
                DialogBox(hInst, "About", hWnd, AboutProc);
#endif
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, TRUE );
                break;

            case IDM_LISTBOX:       /* command from listbox     */
                if (cZippedFiles)
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_SELCHANGE:
                        UpdateButtons(hWnd);
                        break;
                    case LBN_DBLCLK:
                        UpdateButtons(hWnd);
                        if ( uf.fCanDragDrop )
                            DragAcceptFiles( hWnd, FALSE );

                        if (uf.fAutoClearStatus && wLBSelection == IDM_LB_DISPLAY) /* if autoclear on display */
                           SendMessage(hWndStatus, WM_COMMAND, IDM_CLEAR_STATUS, 0L);

                        Action(hWnd, (WPARAM)(wLBSelection - IDM_LB_EXTRACT));
                        if ( uf.fCanDragDrop )
                            DragAcceptFiles( hWnd, TRUE );
                        break;
                    }
                }
                break;
            case IDM_LONG:
            case IDM_SHORT:
                /* If format change, uncheck old, check new. */
                if ((LOWORD(wParam) - IDM_SHORT) != (signed int)uf.fFormatLong)
                {
                    WPARAM wFormatTmp = (WPARAM)(LOWORD(wParam)  - IDM_SHORT);
                    int __far *pnSelItems; /* pointer to list of selected items */
                    HANDLE  hnd = 0;
                    int cSelLBItems ; /* no. selected items in listbox */
                    RECT    rClient;

                    cSelLBItems = CLBItemsGet(hWndList, &pnSelItems, &hnd);
                    CheckMenuItem(hMenu, (IDM_SHORT+uf.fFormatLong), MF_BYCOMMAND|MF_UNCHECKED);
                    CheckMenuItem(hMenu, (IDM_SHORT+wFormatTmp), MF_BYCOMMAND|MF_CHECKED);
                    uf.fFormatLong = wFormatTmp;
                    UpdateListBox(hWnd);

                    SizeWindow(hWnd, TRUE);
                    WritePrivateProfileString(szAppName, szFormatKey,
                                (LPSTR)(szFormatKeyword[uf.fFormatLong]), szWizUnzipIniFile);
             
                    /* anything previously selected ? */
                    if (cSelLBItems > 0)
                    {
                        ReselectLB(hWndList, cSelLBItems, pnSelItems);
                        GlobalUnlock(hnd);
                        GlobalFree(hnd);
                    }

                    /* enable or disable buttons */
                    UpdateButtons(hWnd);

                    /* make sure labels & Zip archive totals get updated */
                    WinAssert(hWnd);
                    GetClientRect( hWnd, &rClient );
                    rClient.top = 0;
                    rClient.bottom = rClient.top + (4*dyChar);
                    InvalidateRect( hWnd, &rClient, TRUE);
                    WinAssert(hWndList);
                    GetClientRect( hWndList, &rClient );
                    OffsetRect( &rClient, 0, dyChar );
                    rClient.top = rClient.bottom;
                    rClient.bottom = rClient.top + (4*dyChar);
                    InvalidateRect( hWnd, &rClient, TRUE);
                    UpdateWindow( hWnd );
                }
                break;
            case IDM_PROMPT_TO_OVERWRITE:
                lpDCL->PromptToOverwrite = !lpDCL->PromptToOverwrite;
                CheckMenuItem(hMenu,IDM_PROMPT_TO_OVERWRITE,MF_BYCOMMAND|
                   (lpDCL->PromptToOverwrite) ? MF_CHECKED: MF_UNCHECKED);
                if (lpDCL->PromptToOverwrite)
                   {
                   CheckMenuItem(hMenu,IDM_EXTRACT_ONLY_NEWER,
                        MF_BYCOMMAND|MF_UNCHECKED);
                   CheckMenuItem(hMenu,IDM_OVERWRITE,
                        MF_BYCOMMAND|MF_UNCHECKED);
                   EnableMenuItem(hMenu, IDM_EXTRACT_ONLY_NEWER,
                        MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
                   ModifyMenu(hMenu, IDM_OVERWRITE, MF_BYCOMMAND |
                        MF_DISABLED|MF_GRAYED, IDM_OVERWRITE,
                        "Never Overwrite Existing Files");
                   lpDCL->Overwrite = FALSE;
                   WritePrivateProfileString(szAppName, szOverwriteKey,
                        (LPSTR)(lpDCL->Overwrite ? szYes : szNo ),
                        szWizUnzipIniFile);
                   EnableMenuItem(hMenu, IDM_OVERWRITE,
                        MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
                   }
                else
                   {
                   CheckMenuItem(hMenu,IDM_EXTRACT_ONLY_NEWER,MF_BYCOMMAND|
                                (WORD)(lpDCL->ExtractOnlyNewer ? MF_CHECKED: MF_UNCHECKED));
                   CheckMenuItem(hMenu,IDM_OVERWRITE,MF_BYCOMMAND|MF_CHECKED);
                   EnableMenuItem(hMenu, IDM_OVERWRITE, MF_ENABLED|MF_BYCOMMAND);
                   EnableMenuItem(hMenu, IDM_EXTRACT_ONLY_NEWER, MF_ENABLED|MF_BYCOMMAND);
                   }
                WritePrivateProfileString(szAppName, szPromptOverwriteKey,
                        (LPSTR)(lpDCL->PromptToOverwrite ? szYes : szNo ), szWizUnzipIniFile);
                break;

            case IDM_OVERWRITE:
                /* Toggle value of overwrite flag. */
                lpDCL->Overwrite = !lpDCL->Overwrite;
                ModifyMenu(hMenu, IDM_OVERWRITE, MF_BYCOMMAND |
                      MF_ENABLED, IDM_OVERWRITE,
                      (lpDCL->Overwrite ? "Always Overwrite Existing Files":
                        "Never Overwrite Existing Files"));
                CheckMenuItem(hMenu,IDM_OVERWRITE,MF_BYCOMMAND|MF_CHECKED);
                WritePrivateProfileString(szAppName, szOverwriteKey,
                        (LPSTR)(lpDCL->Overwrite ? szYes : szNo ), szWizUnzipIniFile);
                break;

            case IDM_EXTRACT_ONLY_NEWER:
                /* Toggle value of extract_only_newer flag. */
                lpDCL->ExtractOnlyNewer = !lpDCL->ExtractOnlyNewer;
                CheckMenuItem(hMenu,IDM_EXTRACT_ONLY_NEWER,MF_BYCOMMAND|
                                (WORD)(lpDCL->ExtractOnlyNewer ? MF_CHECKED: MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szExtractOnlyNewerKey,
                        (LPSTR)(lpDCL->ExtractOnlyNewer ? szYes : szNo ), szWizUnzipIniFile);
                break;

            case IDM_TRANSLATE:
                /* Toggle value of translate flag. */
                uf.fTranslate = !uf.fTranslate;
                CheckMenuItem(hMenu,IDM_TRANSLATE,MF_BYCOMMAND|
                                (WORD)(uf.fTranslate ? MF_CHECKED: MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szTranslateKey,
                        (LPSTR)(uf.fTranslate ? szYes : szNo ), szWizUnzipIniFile);
                break;

            case IDM_SPACE_TO_UNDERSCORE:
                /* Toggle value of space to underscore flag. */
                lpDCL->SpaceToUnderscore = !lpDCL->SpaceToUnderscore;
                CheckMenuItem(hMenu,IDM_SPACE_TO_UNDERSCORE,MF_BYCOMMAND|
                                (WORD)(lpDCL->SpaceToUnderscore ? MF_CHECKED: MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szSpaceToUnderscoreKey,
                        (LPSTR)(lpDCL->SpaceToUnderscore ? szYes : szNo ), szWizUnzipIniFile);
                break;

            case IDM_SAVE_UNZIP_TO_DIR:
                /* Toggle value of fSaveUnzipToDir flag. */
                uf.fSaveUnZipToDir = !uf.fSaveUnZipToDir;
                CheckMenuItem(hMenu,IDM_SAVE_UNZIP_TO_DIR,MF_BYCOMMAND|
                                (WORD)(uf.fSaveUnZipToDir ? MF_CHECKED: MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szSaveUnZipToKey,
                        (LPSTR)(uf.fSaveUnZipToDir ? szYes : szNo ), szWizUnzipIniFile);
                if (uf.fSaveUnZipToDir)
                   WritePrivateProfileString(szAppName, szDefaultUnzipToDir,
                         "", szWizUnzipIniFile);
                break;

            case IDM_SAVE_UNZIP_FROM_DIR:
                /* Toggle value of fSaveUnzipToDir flag. */
                uf.fSaveUnZipFromDir = !uf.fSaveUnZipFromDir;
                CheckMenuItem(hMenu,IDM_SAVE_UNZIP_FROM_DIR,MF_BYCOMMAND|
                                (WORD)(uf.fSaveUnZipFromDir ? MF_CHECKED: MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szSaveUnZipFromKey,
                        (LPSTR)(uf.fSaveUnZipFromDir ? szYes : szNo ), szWizUnzipIniFile);
                if (uf.fSaveUnZipFromDir)
                   WritePrivateProfileString(szAppName, szDefaultUnzipFromDir,
                         "", szWizUnzipIniFile);
                break;

            case IDM_UNZIP_TO_ZIP_DIR:
                /* toggle value of Unzip to .ZIP  */
                uf.fUnzipToZipDir = !uf.fUnzipToZipDir;
                CheckMenuItem(hMenu,IDM_UNZIP_TO_ZIP_DIR,MF_BYCOMMAND|
                                    (WORD)(uf.fUnzipToZipDir ? MF_CHECKED:MF_UNCHECKED));
                EnableMenuItem(hMenu,IDM_CHDIR,MF_BYCOMMAND|
                                    (WORD)(uf.fUnzipToZipDir ? MF_GRAYED:MF_ENABLED));
                WritePrivateProfileString(szAppName, szUnzipToZipDirKey,
                            (LPSTR)(uf.fUnzipToZipDir ? szYes : szNo ), szWizUnzipIniFile);

                if (uf.fUnzipToZipDir && lpumb->szDirName[0])
                {
                    lstrcpy(lpumb->szUnzipToDirName, lpumb->szDirName); /* get new dirname */
                   SetCaption(hWnd);
                }
                break;
            case IDM_MAKE_DIR:
               {
               FARPROC lpfnMakeDir;
               dwCommDlgHelpId = HELPID_MAKEDIR_HELP; /* if someone hits help */
               lpfnMakeDir = MakeProcInstance(MakeDirProc, hInst);
               DialogBox(hInst, "MAKEDIR", hWnd, lpfnMakeDir);
#ifndef WIN32
               FreeProcInstance(lpfnMakeDir);
#endif
               }
               break;
            case IDM_SOUND_OPTIONS: /* launch Sound Options dialog box   */
             {
            FARPROC lpfnSoundOptions;
            dwCommDlgHelpId = HELPID_SOUND_OPTIONS; /* if someone hits "help" */
                lpfnSoundOptions = MakeProcInstance(SoundProc, hInst);
                DialogBox(hInst, "SOUND", hWnd, lpfnSoundOptions);
#ifndef WIN32
                FreeProcInstance(lpfnSoundOptions);
#endif
            }
                break;

           case IDM_AUTOCLEAR_STATUS:
                /* automatically clear status window before displaying  */
                uf.fAutoClearStatus = !uf.fAutoClearStatus;
                CheckMenuItem(hMenu,IDM_AUTOCLEAR_STATUS,MF_BYCOMMAND|
                                    (WORD)(uf.fAutoClearStatus ? MF_CHECKED:MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szAutoClearStatusKey,
                            (LPSTR)(uf.fAutoClearStatus ? szYes : szNo ), szWizUnzipIniFile);

                 break;

           case IDM_AUTOCLEAR_DISPLAY:
               uf.fAutoClearDisplay = !uf.fAutoClearDisplay;
               CheckMenuItem(hMenu,IDM_AUTOCLEAR_DISPLAY,MF_BYCOMMAND|
                                    (WORD)(uf.fAutoClearDisplay ? MF_CHECKED:MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szAutoClearDisplayKey,
                            (LPSTR)(uf.fAutoClearDisplay ? szYes : szNo ), szWizUnzipIniFile);
               break;
  
            case IDM_LB_EXTRACT:
            case IDM_LB_DISPLAY:
            case IDM_LB_TEST:
                /* If overwrite change, uncheck old, check new. */
                /* wParam is the new default action */
                if (LOWORD(wParam) != wLBSelection)
                   {
                   RECT    rClient, rButton;
                   POINT   ptUpperLeft, ptLowerRight;

                   CheckMenuItem(hMenu,wLBSelection,MF_BYCOMMAND|MF_UNCHECKED);
                   CheckMenuItem(hMenu,wParam,MF_BYCOMMAND|MF_CHECKED);

                   wLBSelection = LOWORD(wParam);
                   WritePrivateProfileString(szAppName, szLBSelectionKey,
                        (LPSTR)(LBSelectionTable[LOWORD(wParam) - IDM_LB_EXTRACT]), szWizUnzipIniFile);
                   WinAssert(hWnd);
                   GetClientRect(hWnd, &rClient);
                   GetWindowRect(hExtract, &rButton); /* any button will do */
                   ptUpperLeft.x = rButton.left;
                   ptUpperLeft.y = rButton.top;
                   ptLowerRight.x = rButton.right;
                   ptLowerRight.y = rButton.bottom;
                   ScreenToClient(hWnd, &ptUpperLeft);
                   ScreenToClient(hWnd, &ptLowerRight);
                   rClient.top = ptUpperLeft.y;
                   rClient.bottom = ptLowerRight.y;
                   InvalidateRect( hWnd, &rClient, TRUE); /* redraw button area */
                   UpdateWindow( hWnd );


                   }
                break;

            case IDM_SHOW_COMMENT:
                /* display the archive comment in mesg window */
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );
                DisplayComment(hWnd);
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, TRUE );
                break;

            case IDM_RECR_DIR_STRUCT:
                /* re-create directories structure */
                uf.fRecreateDirs = !uf.fRecreateDirs;
                CheckMenuItem(hMenu, IDM_RECR_DIR_STRUCT,
                 MF_BYCOMMAND | (uf.fRecreateDirs ? MF_CHECKED : MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szRecreateDirsKey,
                                    (LPSTR)(uf.fRecreateDirs ? szYes : szNo), szWizUnzipIniFile);
                break;

            case IDM_SHOW_BUBBLE_HELP:
                /* Show toolbar help */
                uf.fShowBubbleHelp = !uf.fShowBubbleHelp;
                CheckMenuItem(hMenu, IDM_SHOW_BUBBLE_HELP,
                 MF_BYCOMMAND | (uf.fShowBubbleHelp ? MF_CHECKED : MF_UNCHECKED));
                WritePrivateProfileString(szAppName, szShowBubbleHelpKey,
                                    (LPSTR)(uf.fShowBubbleHelp ? szYes : szNo), szWizUnzipIniFile);
                break;

            case IDM_DISPLAY:
            case IDM_TEST:
            case IDM_EXTRACT:
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, FALSE );

                if (uf.fAutoClearStatus && LOWORD(wParam) == IDM_DISPLAY) /* if autoclear on display */
                   SendMessage(hWndStatus, WM_COMMAND, IDM_CLEAR_STATUS, 0L);

                Action(hWnd, (WPARAM)(LOWORD(wParam) - IDM_EXTRACT));
                if ( uf.fCanDragDrop )
                    DragAcceptFiles( hWnd, TRUE );
                break;

            case IDM_SELECT_ALL:
            case IDM_DESELECT_ALL:
                if (cZippedFiles)
                   {
                   SendMessage(hWndList , LB_SELITEMRANGE,
                          (WPARAM)(LOWORD(wParam) == IDM_DESELECT_ALL ? FALSE : TRUE),
                          MAKELONG(0, (cZippedFiles-1)));
                   UpdateButtons(hWnd);
                   }
                break;

         case IDM_SELECT_BY_PATTERN:
            if (!hPatternSelectDlg)
               {
               DLGPROC lpfnPatternSelect;

               dwCommDlgHelpId = HELPID_SELECT_BY_PATTERN;
               lpfnPatternSelect = (DLGPROC)MakeProcInstance(PatternSelectProc, hInst);
               WinAssert(lpfnPatternSelect)
               hPatternSelectDlg =
               CreateDialog(hInst, "PATTERN", hWnd, lpfnPatternSelect);
               WinAssert(hPatternSelectDlg);
               }
            break;

         case IDM_COPY: /* copy from "Status" window value to clipboard */
               CopyStatusToClipboard(hWnd);
            break;

         case IDM_CLEAR_STATUS:  /* forward to status window */
            PostMessage(hWndStatus, WM_COMMAND, IDM_CLEAR_STATUS, 0L);
            EnableWindow(hCopyStatus, FALSE);
            EnableMenuItem(hMenu, IDM_COPY, MF_GRAYED);
            break;

         case IDM_MAX_LISTBOX:
         case IDM_MAX_STATUS:
         case IDM_SPLIT:
            {
            RECT rClient;
            if (wWindowSelection != LOWORD(wParam) ) /* If state change */
               {
               CheckMenuItem(hMenu, wWindowSelection, MF_BYCOMMAND | MF_UNCHECKED);
               switch (wWindowSelection)
                  {
                  case IDM_MAX_STATUS:
                     ManageStatusWnd(IDM_SPLIT);
                     break;

                  case IDM_MAX_LISTBOX: /* status window has been hidden */
                     WinAssert(hWndStatus);
                     EnableWindow( hWndStatus, TRUE );
                     ShowWindow(hWndStatus, SW_SHOWNORMAL);
                     wWindowSelection = IDM_SPLIT; /* save new state   */
                     SizeWindow(hWnd, FALSE);
                     break;
                  }

               /* listbox and status window are in split state at this point   */
               switch (LOWORD(wParam))
                  {
                  case IDM_MAX_STATUS:
                     ManageStatusWnd(wParam);
                     break;

                  case IDM_MAX_LISTBOX:
                     WinAssert(hWndStatus);
                     EnableWindow( hWndStatus, FALSE );
                     ShowWindow(hWndStatus, SW_HIDE);
                     break;
                  }
               /* Insure "header" is updated as necessary */
               WinAssert(hWnd);
               GetClientRect( hWnd, &rClient );
               rClient.top = 2 * dyChar;
               rClient.left += dxChar/2;
               InvalidateRect(hWnd, &rClient, TRUE);
               UpdateWindow(hWnd);

               wWindowSelection = LOWORD(wParam) ; /* save new state   */
               SizeWindow(hWnd, FALSE);
               CheckMenuItem(hMenu, wWindowSelection, MF_BYCOMMAND | MF_CHECKED);
               }
            break;
            }

         case IDM_SETFOCUS_ON_STATUS: /* posted from Action() following extract-to-Status */
            SetFocus(hWndStatus);   /* set focus on Status so user can scroll */
            break;
            default:

            return DefWindowProc(hWnd, wMessage, wParam, lParam);
            }
        } /* bottom of not in help mode */
        break;

    case WM_SETCURSOR:
        /* In help mode it is necessary to reset the cursor in response */
        /* to every WM_SETCURSOR message.Otherwise, by default, Windows */
        /* will reset the cursor to that of the window class. */

        if (uf.fHelp)
        {
            SetCursor(hHelpCursor);
            break;
        }
        return DefWindowProc(hWnd, wMessage, wParam, lParam);


    case WM_INITMENU:
        if (uf.fHelp)
           {
           SetCursor(hHelpCursor);
           }
        return TRUE;

    case WM_ENTERIDLE:
        if ((LOWORD(wParam) == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000))
           {
           uf.fHelp = TRUE;
           PostMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0L);
           }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        if ( uf.fCanDragDrop )
            DragAcceptFiles( hWnd, FALSE );
        DeleteObject(hBrush);
        WinHelp(hWnd, szHelpFileName, HELP_QUIT, 0L);
        PostQuitMessage(0);
        break;

    case WM_DROPFILES:
        {
        WORD    cFiles;

        /* Get the number of files that have been dropped */
        cFiles = (WORD)DragQueryFile( (HDROP)wParam, (UINT)-1, lpumb->szBuffer, (UINT)256);

        /* Only handle one dropped file until MDI-ness happens */
        if (cFiles == 1)
           {
           RECT    rClient;

           DragQueryFile( (HDROP)wParam, 0, lpumb->szFileName, WIZUNZIP_MAX_PATH);
           AnsiToOem(lpumb->szFileName, lpumb->szFileName); /* retain as OEM */
           GetArchiveDir(lpumb->szDirName); /* get archive dir name in ANSI */
           if (uf.fUnzipToZipDir || /* unzipping to same directory as archive */
               lpumb->szUnzipToDirName[0] == '\0') /* or no default */
              {
              /* strip off filename to make directory name    */
              lstrcpy(lpumb->szUnzipToDirName, lpumb->szDirName);
              }
           lstrcpy(lpumb->szBuffer, lpumb->szDirName); /* get scratch copy */
           DlgDirList (hWnd, lpumb->szBuffer, 0, 0, 0); /* change dir */
           UpdateListBox(hWnd); /* fill in list box */
           SendMessage(hWndList, LB_SETSEL, 1, 0L);
           UpdateButtons(hWnd); /* update state of buttons */

           WinAssert(hWndList);
           GetClientRect( hWndList, &rClient );
           OffsetRect( &rClient, 0, dyChar );
           rClient.top = rClient.bottom;
           rClient.bottom = rClient.top + (2*dyChar);
           InvalidateRect( hWnd, &rClient, TRUE);
           UpdateWindow( hWnd );
           SetCaption(hWnd);
           }
        DragFinish( (HDROP)wParam );
        }
        break;

    case WM_PAINT:
        if (wWindowSelection != IDM_MAX_STATUS)
           {
           PAINTSTRUCT ps;
           RECT    rClient;
           DWORD   dwBackColor;

           hDC = BeginPaint( hWnd, &ps );
           if ( hDC )
              {
              WinAssert(hWndList);
              GetClientRect( hWndList, &rClient );
              if (RectVisible( hDC, &rClient ))
                 UpdateWindow( hWndList );
              hOldFont = SelectObject ( hDC, hFixedFont);

              WinAssert(hWnd);
              GetClientRect( hWnd, &rClient );
              dwBackColor = SetBkColor(hDC,GetSysColor(BG_SYS_COLOR));

              /* Move "header" down two lines for button room at top */
                  rClient.top = 2 * dyChar;

              rClient.left += dxChar/2;
                  DrawText( hDC, (LPSTR)Headers[uf.fFormatLong][0], -1,
                     &rClient, DT_NOPREFIX | DT_TOP);

              if (lpumb->szFileName[0])   /* if file selected */
                 {
                 WinAssert(hWndList);
                 GetClientRect( hWndList, &rClient );
                 OffsetRect( &rClient, 0, dyChar+2);
                 rClient.left += dxChar/2;
                 /* Move totals "trailer" down two lines for button room */
                    rClient.top = rClient.bottom + (2 * dyChar);

                 rClient.bottom = rClient.top + dyChar;
                 DrawText( hDC, (LPSTR)szTrailers[uf.fFormatLong], -1,
                   &rClient, DT_NOPREFIX | DT_TOP);

                 /* Display totals line at bottom of listbox */
                 rClient.top += dyChar;
                 rClient.bottom += dyChar;

                 DrawText( hDC, lpumb->szTotalsLine, -1,
                    &rClient, DT_NOPREFIX | DT_TOP);
                 }
              SetBkColor(hDC, dwBackColor);
              (void)SelectObject ( hDC, hOldFont);
            }
            EndPaint(hWnd, &ps);
            break;
        }
        return DefWindowProc(hWnd, wMessage, wParam, lParam);
   default:
      if (wMessage == uCommDlgHelpMsg)   /* common dialog help message ID */
      {
            WinHelp(hWnd, szHelpFileName, HELP_CONTEXT, dwCommDlgHelpId );
         return 0;
      }
        return DefWindowProc(hWnd, wMessage, wParam, lParam);
    }
    return 0;
}
