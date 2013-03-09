#include <windows.h>
#include <ctype.h>
#include <dos.h>
#ifndef WIN32
#   ifndef __BORLANDC__
#      include <direct.h>
#   else
#      include <dir.h>
#   endif /* !__BORLANDC__ */
#else
#   ifndef __BORLANDC__
#      include <direct.h>
#   else
#      include <dir.h>
#   endif /* !__BORLANDC__ */
#endif /* !WIN32 */
#include <stdio.h>
#include <string.h>
#include "wingui\wizunzip.h"
#include "wingui\helpids.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef WIZUNZIP_MAX_PATH
#define WIZUNZIP_MAX_PATH 128
#endif

/*
 * Function: BOOL MakeDirectory(char *path, BOOL fileAttached)
 *
 * Return:   TRUE if successful
 *           FALSE if unsuccessful
 *
 * Creates directories specified by path. These can be any level of
 * previously non-existent directories, up to WIZUNZIP_MAX_PATH.
 *
 * First checks to see if a drive is designated, and if so, if the
 * drive exists. If drive doesn't exist, returns FALSE.
 *
 * Then successively creates the directories (assuming they don't exist).
 *
 * Placed in the public domain by Mike White, 1995
 *
 */

BOOL MakeDirectory(char *path, BOOL fileAttached);

BOOL MakeDirectory(char *path, BOOL fileAttached)
{
char tempPath[WIZUNZIP_MAX_PATH], tempChar;
BOOL slash = FALSE;
#ifdef WIN32
DWORD attrib;
unsigned int dReturn;
#else
unsigned int attrib;
WORD dReturn;
#endif
int i;

if (path[0] == '\0')
   return TRUE; /* Don't need to make a directory */

lstrcpy(tempPath, path);
if (tempPath[1] == ':') /* check if drive included */
    {
    if (lstrlen(tempPath) < 4)
        return TRUE; /* specified only a drive and a root directory */
#ifndef WIN32
    dReturn = GetDriveType(toupper(tempPath[0]) - 'A');
#else
    tempPath[2] = '\\';
    tempPath[3] = '\0';
    dReturn = GetDriveType(tempPath);
    lstrcpy(tempPath, path);
#endif

    if (dReturn == 0) /* drive type can't be determined */
        return FALSE;
#ifdef WIN32
    if (dReturn == 1) /* root directory doesn't exist */
        return FALSE;
#endif
    }

/* Is this a valid path already? If so, return TRUE */
/* There appears to be a bug (feature?) in NT 3.51 (and perhaps other
   versions as well) that allows you to create a path the first time for a
   given application, but erroneously reports that the path exists, if
   another application deletes the directories, hence the following code
   has been commented out until such time as a fix is figured out.

#ifndef WIN32
_dos_getfileattr(tempPath, &attrib);
if (attrib & _A_SUBDIR)
   {
   return TRUE;
   }
#else
attrib = GetFileAttributes(tempPath);
if (attrib & FILE_ATTRIBUTE_DIRECTORY)
   {
   return TRUE;
   }
#endif
*/

if (tempPath[1] == ':')
   for (i = 3; i < lstrlen(path); i++)
      {
      if (tempPath[i] == '\\')
         {
         tempChar = tempPath[i];
         tempPath[i] = '\0';
         mkdir(tempPath); /* We don't care what the return value is, if it
                             already exists, we'll get an error. Check later
                             on.
                          */
         tempPath[i] = tempChar;
         slash = TRUE;
         }
      }
else
   for (i = 1; i < lstrlen(path); i++)
      {
      if (tempPath[i] == '\\')
         {
         tempChar = tempPath[i];
         tempPath[i] = '\0';
         mkdir(tempPath); /* We don't care what the return value is, if it
                             already exists, we'll get an error. Check later
                             on.
                          */
         tempPath[i] = tempChar;
         slash = TRUE;
         }
      }
if (slash && !fileAttached)
    mkdir(tempPath);
else
    if (!fileAttached)
       mkdir(tempPath);


if (fileAttached)
   {
   char *ptr;
   if ((ptr = strrchr(tempPath, '\\')) != NULL)
      *ptr = '\0';
   }

/* Now, do we have a valid path? */
#ifndef WIN32
_dos_getfileattr(tempPath, &attrib);
if (attrib & _A_SUBDIR)
   return TRUE;
#else
attrib = GetFileAttributes(tempPath);
if (attrib & FILE_ATTRIBUTE_DIRECTORY)
   return TRUE;
#endif

/* Still no valid path, can't create it. return false */
return FALSE;
}


/****************************************************************************

    FUNCTION: MakeDirProc(HWND, unsigned, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Make Directory" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

****************************************************************************/

#ifdef __BORLANDC__
#pragma warn -par
#endif
BOOL WINAPI
MakeDirProc(HWND hDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
static HWND hSelect, hMakeDir;
char tempPath[128];
int nPatternLength;   /* length of data in pattern edit window */


   switch (wMessage) {
   case WM_INITDIALOG:
      hSelect = GetDlgItem(hDlg, IDOK);
      hMakeDir = GetDlgItem(hDlg, IDM_MAKEDIR_PATH);
      SetDlgItemText(hDlg, IDM_CURRENT_PATH,lpumb->szUnzipToDirName);
      CenterDialog(GetParent(hDlg), hDlg);
      return TRUE;
   case WM_COMMAND:
      switch (LOWORD(wParam))
        {
        case IDM_MAKEDIR_PATH:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
               {
               if ((nPatternLength = GetWindowTextLength(hMakeDir))==1)
                  {
         /* Enable or disable buttons depending on fullness of
          * "Suffix" box.
          */
                  BOOL fButtonState = TRUE ;
                  WinAssert(hSelect);
                  EnableWindow(hSelect, fButtonState);
                  }
               if (nPatternLength == 0)
                  {
                  BOOL fButtonState = FALSE;
                  WinAssert(hSelect);
                  EnableWindow(hSelect, fButtonState);
                  }
               }
            break;

      case IDOK: /* Make Directory */
         GetDlgItemText(hDlg, IDM_MAKEDIR_PATH, tempPath, WIZUNZIP_MAX_PATH);
         if (tempPath[lstrlen(tempPath) - 1] == '\\')
            tempPath[lstrlen(tempPath) - 1] = '\0'; /* strip trailing backslash */
         if (strchr(tempPath, '\\') == NULL)
            {
            char sz[WIZUNZIP_MAX_PATH];
            lstrcpy(sz, lpumb->szUnzipToDirName);
            strcat(sz, "\\");
            strcat(sz, tempPath);
            lstrcpy(tempPath, sz);
            }
         if (!MakeDirectory(tempPath, FALSE))
            {
            char tStr[WIZUNZIP_MAX_PATH];
            sprintf(tStr, "Could not create %s", tempPath);
            MessageBox(hDlg, tStr, NULL, MB_OK | MB_ICONSTOP);
            }
         else
            {
            char tStr[WIZUNZIP_MAX_PATH];
            sprintf(tStr, "Created directory: %s\n", tempPath);
            win_fprintf(stdout, strlen(tStr), tStr);
            }
         EndDialog(hDlg, wParam);
         break;

      case IDCANCEL:
         EndDialog(hDlg, wParam);
         break;
      case IDM_MAKEDIR_HELP:
            WinHelp(hDlg,szHelpFileName,HELP_CONTEXT, (DWORD)(HELPID_MAKEDIR_HELP));
      }
      return TRUE;
   case WM_CLOSE:
      DestroyWindow(hDlg);
      return TRUE;
   }
   return FALSE;
}
#ifdef __BORLANDC__
#pragma warn .par
#endif

