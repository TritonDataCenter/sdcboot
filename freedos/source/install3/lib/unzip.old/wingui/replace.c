#include <stdio.h>
#include "wingui\wizunzip.h"
#include "wingui\replace.h"
#include "wingui\rename.h"
#include "wingui\helpids.h"

/****************************************************************************

    FUNCTION: Replace(HWND, WORD, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Replace" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

****************************************************************************/

BOOL WINAPI ReplaceProc(HWND hDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
    static char __far *lpsz;
    switch (wMessage)
    {
    case WM_INITDIALOG:
        lpsz = (char __far *)lParam;
        wsprintf(lpumb->szBuffer, "Replace %s ?", (LPSTR)lpsz);
        SetDlgItemText(hDlg, IDM_REPLACE_TEXT, lpumb->szBuffer);
        CenterDialog(GetParent(hDlg), hDlg); /* center on parent */
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:              /* ESC key      */
        case IDOK:                  /* Enter key    */
            EndDialog(hDlg, IDM_REPLACE_NO);
            break;
        case IDM_REPLACE_RENAME:
            {
                FARPROC lpProcRename = MakeProcInstance(RenameProc, hInst);
                if (DialogBoxParam(hInst, "Rename", hWndMain, 
                                        lpProcRename, (DWORD)(LPSTR)lpsz) != IDM_RENAME_RENAME)
                {
                    wParam = 0;
                }
#ifndef WIN32
                FreeProcInstance(lpProcRename);
#endif
            }
            if (LOWORD(wParam))
                EndDialog(hDlg, wParam);
            else
                SetFocus(hDlg);
            SetCursor(LoadCursor(0,IDC_ARROW));
            break;
        case IDM_REPLACE_ALL:
            uf.fDoAll = 1;
        case IDM_REPLACE_NONE:
        case IDM_REPLACE_YES:
        case IDM_REPLACE_NO:
            EndDialog(hDlg, wParam);
            break;
        case IDM_REPLACE_HELP:
            WinHelp(hDlg,szHelpFileName,HELP_CONTEXT, (DWORD)(HELPID_OVERWRITE));
        }
        return TRUE;
    }
    return FALSE;
}

