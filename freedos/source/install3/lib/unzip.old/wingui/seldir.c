#include <string.h>
#include <io.h>
#include <stdio.h>

#include "wingui\wizunzip.h"
#include "wingui\helpids.h"


BOOL WINAPI SelectDirProc(HWND hDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
    static OPENFILENAME __far *lpofn = 0;
    static WORD wClose;

    switch (wMessage)
    {
    case WM_DESTROY:
        if (lpofn && lpofn->lpstrFile && (wClose == IDOK))
        {
         DWORD dwResult;      /* result of Save As Default button query */

         /* get state of Save As Default checkbox, if appropriate */
         if (!uf.fUnzipToZipDir)
            {
             dwResult = SendDlgItemMessage(hDlg , IDM_SAVE_AS_DEFAULT, BM_GETSTATE, 0, 0);
             if (dwResult & 1)   /* if checked */
                 {
                 /* save as future default */
                 WritePrivateProfileString(szAppName, szDefaultUnzipToDir,
                                lpofn->lpstrFile, szWizUnzipIniFile);

                 }
            }
        }
        break;
    case WM_COMMAND:
        /* When the user presses the OK button, stick text
           into the filename edit ctrl to fool the commdlg
           into thinking a file has been chosen.
           We're just interested in the path, so any file
           name will do - so long as it doesn't match
           a directory name, we're fine
        */

        if (LOWORD(wParam) == IDOK)
        {
            SetDlgItemText(hDlg, edt1, "johnny\376\376.\375\374\373");
            wClose = LOWORD(wParam);
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            wClose = LOWORD(wParam);
        }
        break;
    case WM_INITDIALOG:
        {
            RECT    rT1, rT2;
            short   nWidth, nHeight;

#ifndef WIN32
            lpofn = (OPENFILENAME __far *)lParam;
#else
            lpofn = (OPENFILENAME *)lParam;
#endif
            CenterDialog(GetParent(hDlg), hDlg); /* center on parent */

            wClose = 0;

            /* Disable and hide the "save as default" checkbox */
            EnableWindow(GetDlgItem(hDlg, IDM_SAVE_AS_DEFAULT), FALSE);
            ShowWindow(GetDlgItem(hDlg, IDM_SAVE_AS_DEFAULT), SW_HIDE);

            /* Disable the filename edit ctrl
               and the file type label
               and the file type combo box
               Note: stc2, cmb1 etc are defined in DLGS.H in the standard
               windows include files.
            */
            EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
            EnableWindow(GetDlgItem(hDlg, stc2), FALSE);
            EnableWindow(GetDlgItem(hDlg, cmb1), FALSE);

            GetWindowRect(GetDlgItem(hDlg, cmb2), &rT1);

            /*  Hide the file type label & combo box */
            ShowWindow(GetDlgItem(hDlg, stc2), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, cmb1), SW_HIDE);

            /* Extend the rectangle of the list of files
               in the current directory so that it's flush
               with the bottom of the Drives combo box
            */
            GetWindowRect(GetDlgItem(hDlg, lst1), &rT2);
            nWidth = (short)(rT2.right - rT2.left);
            nHeight = (short)(rT1.bottom - rT2.top);
            ScreenToClient(hDlg, (LPPOINT)&rT2);
            MoveWindow(GetDlgItem(hDlg, lst1),
                        rT2.left, rT2.top,
                        nWidth,
                        nHeight,
                        TRUE);
        }
    default:
        break;
    }

    /* message not handled */
    return 0;
}
