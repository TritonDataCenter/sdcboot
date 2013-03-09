#include <string.h>
#include <io.h>
#include <stdio.h>
#include <ctype.h>
#define UNZIP_INTERNAL
#include "unzip.h"
#include "wingui\wizunzip.h"
#include "wingui\helpids.h"
#ifndef WIN32
#include <lzexpand.h>
#endif                           

void StripDirectory(LPSTR lpDir);

static char szTargetDirName[128] = "";
static char __based(__segname("STRINGS_TEXT")) szCantCopyFile[] =
   "Internal error: Cannot copy file. Common dialog error code is 0x%lX.";

#ifdef __BORLANDC__
#pragma warn -par
#endif
BOOL WINAPI CopyFileProc(HWND hDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
    static OPENFILENAME __far *lpofn = 0;
    static WORD wClose;

    switch (wMessage)
    {
    case WM_DESTROY:
        if (lpofn && lpofn->lpstrFile && (wClose == IDOK))
        {
         LPSTR lpszSeparator;

        /* If we have a trailing '\\' we need to get it */
        GetDlgItemText(hDlg, edt1, lpofn->lpstrFile, WIZUNZIP_MAX_PATH);

        /* Flag whether there is a trailing backslash */
        lpszSeparator = lpofn->lpstrFile+lstrlen(lpofn->lpstrFile)-1;
        if (*lpszSeparator == '\\')
            uf.fTrailingSlash = TRUE;
        }
        break;
    case WM_INITDIALOG:
        {
            RECT    rT1, rT2;
            short   nWidth, nHeight;
            HWND hTemp;

            lpofn = (OPENFILENAME __far *)lParam;
            CenterDialog(GetParent(hDlg), hDlg); /* center on parent */

            /* Disable the file type label
               and the file type combo box

               Note: stc2, cmb1 etc are defined in DLGS.H in the standard
               windows include files.
            */
            hTemp = GetDlgItem(hDlg, stc2);
            WinAssert(hTemp);
            EnableWindow(hTemp, FALSE);
            ShowWindow(hTemp, SW_HIDE);

            hTemp = GetDlgItem(hDlg, cmb1);
            WinAssert(hTemp);
            EnableWindow(hTemp, FALSE);
            ShowWindow(hTemp, SW_HIDE);

            hTemp = GetDlgItem(hDlg, cmb2);
            WinAssert(hTemp);
            GetWindowRect(hTemp, &rT1);

            /* Extend the rectangle of the list of files
               in the current directory so that it's flush
               with the bottom of the Drives combo box
            */
            hTemp = GetDlgItem(hDlg, lst1);
            WinAssert(hTemp);
            GetWindowRect(hTemp, &rT2);
            nWidth = (short)(rT2.right - rT2.left);
            nHeight = (short)(rT1.bottom - rT2.top);
            ScreenToClient(hDlg, (LPPOINT)&rT2);
            MoveWindow(hTemp,
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
#ifdef __BORLANDC__
#pragma warn .par
#endif

/* Strip off the directory leaving only the file name */
void StripDirectory(LPSTR lpDir)
{
LPSTR lpchLast, i, j;
/* If no '\\' then simply return */
if ((lpchLast = lstrrchr(lpDir, '\\')) == 0)
   {
   return;
   }
if ((lpchLast = lstrrchr(lpDir, '\\'))!=0)
   {
   for (i = lpchLast+1, j = lpDir; *i; i++, j++)
      {
      *j = *i;
      }
   *j = '\0';
   }

else if ((lpchLast = lstrrchr(lpDir, ':'))!=0)
   {
   for (i = lpchLast+1, j = lpDir; *i; i++, j++)
      {
      *j = *i;
      }
   *j = '\0';
   }
}

void CopyArchive(HWND hWnd, BOOL move_flag, BOOL rename_flag)
{
char tempFileName[WIZUNZIP_MAX_PATH], tempPath[WIZUNZIP_MAX_PATH];
#ifndef WIN32
   FARPROC lpfnSelectDir;
#endif

#ifndef WIN32
_fmemset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#else
memset(&lpumb->ofn, '\0', sizeof(OPENFILENAME)); /* initialize struct */
#endif
WinAssert(hWnd);
lpumb->ofn.lStructSize = sizeof(OPENFILENAME);
lpumb->ofn.hwndOwner = hWnd;
lpumb->ofn.hInstance = hInst;
lpumb->ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
lpumb->ofn.nFilterIndex = 1;

               /* strip off filename */
#ifdef __BORLANDC__
#pragma warn -def
#endif
OemToAnsi(lpumb->szFileName, lpumb->szUnzipToDirNameTmp);
#ifdef __BORLANDC__
#pragma warn .def
#endif
                /* Strip off path */
StripDirectory(lpumb->szUnzipToDirNameTmp);
                /* Now we've got just the file name */
lpumb->ofn.lpstrFile = lpumb->szUnzipToDirNameTmp; /* result goes here! */
                /* Save file name only */
lstrcpy(tempFileName, lpumb->szUnzipToDirNameTmp);
lpumb->ofn.nMaxFile = WIZUNZIP_MAX_PATH;
lpumb->ofn.lpstrFileTitle = NULL;
lpumb->ofn.nMaxFileTitle = OPTIONS_BUFFER_LEN; /* ignored ! */
if (move_flag)
   {
   lpumb->ofn.lpstrTitle = (LPSTR)"Move Archive To";
   lpumb->ofn.lpstrInitialDir =
   (LPSTR)(!szTargetDirName[0] ? NULL : szTargetDirName);
   }
else
   if (rename_flag)
      {
      lpumb->ofn.lpstrTitle = (LPSTR)"Rename Archive To";
      lpumb->ofn.lpstrInitialDir = (LPSTR)((uf.fUnzipToZipDir ||
         !lpumb->szDirName[0]) ? NULL : lpumb->szDirName);
      }
   else
      {
      lpumb->ofn.lpstrTitle = (LPSTR)"Copy Archive To";
      lpumb->ofn.lpstrInitialDir =
         (LPSTR)(!szTargetDirName[0] ? NULL : szTargetDirName);
      }
lpumb->ofn.Flags = OFN_SHOWHELP | OFN_ENABLEHOOK | OFN_CREATEPROMPT |
   OFN_HIDEREADONLY|OFN_ENABLETEMPLATE|OFN_NOCHANGEDIR;
#ifndef WIN32
                /* Just using an available pointer */
lpfnSelectDir = MakeProcInstance((FARPROC)CopyFileProc, hInst);
#   ifndef MSC
(UINT CALLBACK *)lpumb->ofn.lpfnHook = (UINT CALLBACK *)lpfnSelectDir;
#   else
lpumb->ofn.lpfnHook = lpfnSelectDir;
#   endif

#else
lpumb->ofn.lpfnHook = (LPOFNHOOKPROC)CopyFileProc;
#endif
lpumb->ofn.lpTemplateName = "COPYFILE";   /* see copyfile.dlg   */
uf.fTrailingSlash = FALSE; /* set trailing slash to FALSE */
if (GetSaveFileName(&lpumb->ofn)) /* successfully got dir name ? */
   {
   if (lstrcmpi(lpumb->szUnzipToDirNameTmp, lpumb->szFileName)!=0)
      {
      lstrcpy(tempPath, lpumb->szUnzipToDirNameTmp);
      if (!uf.fTrailingSlash) /* Do we have a trailing slash? */
         { /* No - strip file name off and save it */
         StripDirectory(tempPath);
         lstrcpy(tempFileName, tempPath);
         /* Strip file name off, we've just saved it */
         GetDirectory(lpumb->szUnzipToDirNameTmp);
         }
      uf.fTrailingSlash = FALSE;
      /* Okay, make the directory, if necessary */
      if (MakeDirectory(lpumb->szUnzipToDirNameTmp, FALSE))
         {
         OFSTRUCT ofStrSrc;
         OFSTRUCT ofStrDest;
         HFILE hfSrcFile, hfDstFile;

         if (lpumb->szUnzipToDirNameTmp[0] != '\0')
            lstrcat(lpumb->szUnzipToDirNameTmp, "\\");
         lstrcat(lpumb->szUnzipToDirNameTmp, tempFileName);

         if (move_flag == TRUE)
            fprintf(stdout, "Moving: \n%s\nto\n%s",
               lpumb->szFileName, lpumb->ofn.lpstrFile);
         else
            if (rename_flag == FALSE)
               fprintf(stdout, "Copying: \n%s\nto\n%s",
               lpumb->szFileName, lpumb->ofn.lpstrFile);

         if ((rename_flag == TRUE) &&
            ((lpumb->szFileName[0] == lpumb->ofn.lpstrFile[0]) ||
            (lpumb->ofn.lpstrFile[1] != ':')))
            {
            fprintf(stdout, "Renaming: \n%s\nto\n%s\n",
               lpumb->szFileName, lpumb->ofn.lpstrFile);
            rename(lpumb->szFileName, lpumb->ofn.lpstrFile);
            lstrcpy(lpumb->szFileName, lpumb->ofn.lpstrFile);
            UpdateButtons(hWnd); /* update state of buttons */
            SetCaption(hWnd);
            }
         else if (rename_flag == FALSE)
            {
            hfSrcFile = LZOpenFile(lpumb->szFileName,
               &ofStrSrc, OF_READ);
            hfDstFile = LZOpenFile(lpumb->ofn.lpstrFile,
               &ofStrDest, OF_CREATE);

            LZCopy(hfSrcFile, hfDstFile);

            LZClose(hfSrcFile);
            LZClose(hfDstFile);
            fprintf(stdout, " ...%s\n", "Done");
            if ((toupper(lpumb->ofn.lpstrFile[0]) != 'A') &&
               (toupper(lpumb->ofn.lpstrFile[0]) != 'B'))
               {
               lstrcpy(szTargetDirName, lpumb->ofn.lpstrFile);
               if ((lpchLast = lstrrchr(szTargetDirName, '\\'))!=0)
                  {
                  *lpchLast = '\0';
                  }
               else if ((lpchLast = lstrrchr(szTargetDirName, ':'))!=0)
                  {
                  *(lpchLast+1) = '\0';
                  lstrcat(szTargetDirName, "\\");
                  }
               }
            }
         if (move_flag)
            {
            /*
            * It's not clear what should be done here, just
            * clear the list box and be done with it, or
            * actually go to the new location of the zipfile.
            * The election at this point is to just be done
            * with it.
            */
            SendMessage(hWndList, LB_RESETCONTENT, 0, 0);
            remove(lpumb->szFileName);
            lpumb->szFileName[0] = '\0';
            UpdateButtons(hWnd); /* update state of buttons */
            }
         }
      }
   }
else /* either real error or canceled */
   {
   DWORD dwExtdError = CommDlgExtendedError(); /* debugging */

   if (dwExtdError != 0L) /* if not canceled then real error */
      {
      wsprintf (lpumb->szBuffer, szCantCopyFile, dwExtdError);
      MessageBox (hWnd, lpumb->szBuffer, szAppName, MB_ICONINFORMATION | MB_OK);
      }
   else
      {
      RECT mRect;

      WinAssert(hWndMain);
      GetWindowRect(hWndMain, &mRect);
      InvalidateRect(hWndMain, &mRect, TRUE);
      SendMessage(hWndMain, WM_SIZE, SIZE_RESTORED,
         MAKELONG(mRect.right-mRect.left, mRect.top-mRect.bottom));
      UpdateWindow(hWndMain);
      }
   }
#ifndef WIN32
                /* Just using an available pointer */
   FreeProcInstance(lpfnSelectDir);
#endif
}
