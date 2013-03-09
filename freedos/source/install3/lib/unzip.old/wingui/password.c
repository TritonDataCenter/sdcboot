#include <windows.h>

#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"

#include "wingui\wizunzip.h"
#include "wingui\password.h"
#include "wingui\helpids.h"

/****************************************************************************

    FUNCTION: PasswordProc(HWND, WORD, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Password" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

****************************************************************************/
BOOL WINAPI PasswordProc(HWND hDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
static char __far *lpsz;

    switch (wMessage)
    {
    case WM_INITDIALOG:
        lpsz = (char __far *)lParam;
        if (lpumb->szBuffer[0] != ' ')
           {
           SetDlgItemText(hDlg, IDM_PASSWORD_INCORRECT, "Incorrect Password");
           }
        else
           SetDlgItemText(hDlg, IDM_PASSWORD_INCORRECT, "");

        wsprintf(lpumb->szBuffer, "Enter Password for %s", (LPSTR)lParam);
        SetDlgItemText(hDlg, IDM_PASSWORD_TEXT, lpumb->szBuffer);
        SetDlgItemText(hDlg, IDM_NEW_PASSWORD_NAME_TEXT, "");
        CenterDialog(GetParent(hDlg), hDlg); /* center on parent */
        return TRUE;

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hDlg, IDM_NEW_PASSWORD_NAME_TEXT));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_PASSWORD_RENAME:
            GetDlgItemText(hDlg, IDM_NEW_PASSWORD_NAME_TEXT, lpumb->lpPassword, 80);
            EndDialog(hDlg, wParam);
            break;
        case IDM_PASSWORD_CANCEL:
            lpumb->szPassword[0] = '\0';
            EndDialog(hDlg, wParam);
            break;
        case IDM_PASSWORD_HELP:
            WinHelp(hDlg,szHelpFileName,HELP_CONTEXT, (DWORD)(HELPID_PASSWORD));
        }
        return TRUE;
    }
    return FALSE;
}

