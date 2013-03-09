/* updatelb.c module of WizUnzip.
 * Author: Robert A. Heath
 * I, Robert Heath, place this source code module in the public domain.
 */

#include <stdio.h>
#include "wingui\wizunzip.h"

#define UNZIP_INTERNAL
#include "unzip.h"

/* Update Buttons is called when an event possibly modifies the
 * number of selected items in the listbox.
 * The function reads the number of selected items.
 * A non-zero value enables relevant buttons and menu items.
 * A zero value disables them.
 */
#ifdef __BORLANDC__
#pragma argsused
#endif
void UpdateButtons(HWND hWnd)
{
BOOL fButtonState;

if (lpumb->szFileName[0] &&
   SendMessage(hWndList, LB_GETSELCOUNT, 0, 0L)) /* anything selected ? */
   {
   fButtonState = TRUE;
   }
else
   {
   fButtonState = FALSE;
   }
WinAssert(hExtract);
EnableWindow(hExtract, fButtonState);
WinAssert(hDisplay);
EnableWindow(hDisplay, fButtonState);
WinAssert(hTest);
EnableWindow(hTest, fButtonState);
WinAssert(hShowComment);
EnableWindow(hShowComment, (BOOL)(fButtonState && cchComment ? TRUE : FALSE));

WinAssert(hCopyArchive);
EnableWindow(hCopyArchive, fButtonState);
WinAssert(hMoveArchive);
EnableWindow(hMoveArchive, fButtonState);
WinAssert(hRenameArchive);
EnableWindow(hRenameArchive, fButtonState);
WinAssert(hDeleteArchive);
EnableWindow(hDeleteArchive, fButtonState);

EnableMenuItem(hMenu, IDM_EXTRACT, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_DISPLAY, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_TEST, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);

EnableMenuItem(hMenu, IDM_COPY_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_MOVE_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_RENAME_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
EnableMenuItem(hMenu, IDM_DELETE_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);

EnableMenuItem(hMenu, IDM_SHOW_COMMENT,
   (BOOL)(fButtonState && cchComment ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);

if (lpumb->szFileName[0] != '\0')
   {
   EnableMenuItem(hMenu, IDM_COPY_ARCHIVE, MF_ENABLED|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_MOVE_ARCHIVE, MF_ENABLED|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_DELETE_ARCHIVE, MF_ENABLED|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_RENAME_ARCHIVE, MF_ENABLED|MF_BYCOMMAND);
   }
else
   {
   EnableMenuItem(hMenu, IDM_COPY_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_MOVE_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_DELETE_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
   EnableMenuItem(hMenu, IDM_RENAME_ARCHIVE, (fButtonState ? MF_ENABLED : MF_DISABLED|MF_GRAYED)|MF_BYCOMMAND);
   }
}

/* Update List Box attempts to fill the list box on the parent
 * window with the next "cListBoxLines" of personal data from the
 * current position in the file.
 * UpdateListBox() assumes that the a record has been read in when called.
 * The cZippedFiles variable indicates whether or not a record exists.
 * The bForward parameter controls whether updating precedes forward
 * or reverse.
 */
#ifdef __BORLANDC__
#pragma argsused
#endif
void UpdateListBox(HWND hWnd)
{
SendMessage(hWndList, LB_RESETCONTENT, 0, 0L);
InvalidateRect( hWndList, NULL, TRUE );
UpdateWindow( hWndList );
cZippedFiles = 0;

if (lpumb->szFileName[0])       /* file selected? */
   {
        /* if so -- stuff list box              */
   SendMessage(hWndList, WM_SETREDRAW, FALSE, 0L);
   /* Call here when a file has been initially selected */

lpDCL->ncflag = 0;
lpDCL->ntflag = 0;
lpDCL->nvflag = (int)(!uf.fFormatLong ? 1 : 2);
lpDCL->nUflag = 1;
lpDCL->nzflag = 0;
lpDCL->ndflag = 0;
lpDCL->noflag = 0;
lpDCL->naflag = 0;
lpDCL->argc   = 0;
lpDCL->lpszZipFN = lpumb->szFileName;
lpDCL->FNV = NULL;
#ifdef USEWIZUNZDLL
(*DllProcessZipFiles)(lpDCL);
#else
   if (FSetUpToProcessZipFile(lpDCL))
      {
      process_zipfiles(__G);
      }
   else
      {
      MessageBox(hWndMain, szNoMemory, NULL,
                  MB_OK|MB_ICONEXCLAMATION);
      }

   TakeDownFromProcessZipFile();
#endif
#ifndef NEED_EARLY_REDRAW
   SendMessage(hWndList, WM_SETREDRAW, TRUE, 0L);
   InvalidateRect(hWndList, NULL, TRUE);   /* force redraw         */
#endif
   cZippedFiles = (WORD)SendMessage(hWndList, LB_GETCOUNT, 0, 0L);
   assert((int)cZippedFiles != LB_ERR);
   if (cZippedFiles)   /* if anything went into listbox set to top */
      {
#ifdef NEED_EARLY_REDRAW
      UpdateWindow(hWndList); /* paint now!                   */
#endif
      SendMessage(hWndList, LB_SETTOPINDEX, 0, 0L);
      }
#ifdef NEED_EARLY_REDRAW
   else /* no files were unarchived!                           */
      {
            /* Add dummy message to initialize list box then clear it
             * to prevent strange problem where later calls to
             * UpdateListBox() do not result in displaying of all contents.
             */
      SendMessage(hWndList, LB_ADDSTRING, 0, (LONG)(LPSTR)" ");
      UpdateWindow(hWndList); /* paint now!                   */
      }
#endif
   }
#ifdef NEED_EARLY_REDRAW
else
   {
        /* Add dummy message to initialize list box then clear it
         * to prevent strange problem where later calls to
         * UpdateListBox() do not result in displaying of all contents.
         */
   SendMessage(hWndList, LB_ADDSTRING, 0, (LONG)(LPSTR)" ");
   UpdateWindow(hWndList); /* paint now!                   */
   }
#endif
}
