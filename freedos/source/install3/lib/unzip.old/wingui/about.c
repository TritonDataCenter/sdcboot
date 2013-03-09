#include <windows.h>    /* required for all Windows applications */
#include <stdio.h>

#include "wingui\wizunzip.h"
#include "version.h"

/*
 -      CenterDialog
 -      
 *      Purpose:
 *              Moves the dialog specified by hwndDlg so that it is centered on
 *              the window specified by hwndParent. If hwndParent is null,
 *              hwndDlg gets centered on the screen.
 *      
 *              Should be called while processing the WM_INITDIALOG message
 *              from the dialog's DlgProc().
 *      
 *      Arguments:
 *              HWND    parent hwnd
 *              HWND    dialog's hwnd
 *      
 *      Returns:
 *              Nothing.
 *
 */
void
CenterDialog(HWND hwndParent, HWND hwndDlg)
{
        RECT    rectDlg;
        RECT    rect;
        int             dx;
        int             dy;

        if (hwndParent == NULL)
        {
                rect.top = rect.left = 0;
                rect.right = GetSystemMetrics(SM_CXSCREEN);
                rect.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
        else
        {
                GetWindowRect(hwndParent, &rect);
        }

        GetWindowRect(hwndDlg, &rectDlg);
        OffsetRect(&rectDlg, -rectDlg.left, -rectDlg.top);

        dx = (rect.left + (rect.right - rect.left -
                        rectDlg.right) / 2 + 4) & ~7;
        dy = rect.top + (rect.bottom - rect.top -
                        rectDlg.bottom) / 2;
        WinAssert(hwndDlg);
        MoveWindow(hwndDlg, dx, dy, rectDlg.right, rectDlg.bottom, 0);
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

****************************************************************************/

#ifdef __BORLANDC__
#pragma argsused
#endif
BOOL WINAPI
AboutProc(HWND hwndDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
char string[80];

if ((wMessage == WM_CLOSE) ||
    (wMessage == WM_COMMAND && LOWORD(wParam) == IDOK))
        EndDialog(hwndDlg, TRUE);

if (wMessage == WM_INITDIALOG)
   {
#ifndef USEWIZUNZDLL
#  ifndef WIN32
     SetWindowText(hwndDlg, "About WizUnZip (16-Bit) Version");
#  else
     SetWindowText(hwndDlg, "About WizUnZip (32-Bit) Version");
#  endif
#else
#  ifndef WIN32
     SetWindowText(hwndDlg, "About WizUnZip (16-Bit) DLL Version");
#  else
     SetWindowText(hwndDlg, "About WizUnZip (32-Bit) DLL Version");
#  endif
#endif
   sprintf(string, "Version %d.%d%s %s", WIZUZ_MAJORVER, WIZUZ_MINORVER,
      BETALEVEL, WIN_VERSION_DATE);
   SetDlgItemText(hwndDlg, IDM_ABOUT_VERSION_INFO, string);
   sprintf(string, "Based on Info-ZIP's UnZip version %d.%d%s",
      UZ_MAJORVER, UZ_MINORVER, BETALEVEL);
   SetDlgItemText(hwndDlg, IDM_ABOUT_UNZIP_INFO, string);
   CenterDialog(GetParent(hwndDlg), hwndDlg);
   }
return ((wMessage == WM_CLOSE) || (wMessage == WM_INITDIALOG) || (wMessage == WM_COMMAND))
            ? TRUE : FALSE;
}

