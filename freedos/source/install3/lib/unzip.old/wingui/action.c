/* Action.c module of WizUnZip.
 * Author: Robert A. Heath, 1993
 * I, Robert Heath, place this source code module in the public domain.
 *
 * Modifications: Mike White 1995
 */

#define UNZIP_INTERNAL
#include "unzip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef  WIN32
#include <dos.h>
#endif
#include "wingui\wizunzip.h"

#define cchFilesMax 4096
char rgszFiles[cchFilesMax]; /* Moved out of action to allow for larger number
                                of files to be selected. */
unsigned long dSpaceToUse;

char __based(__segname("STRINGS_TEXT")) szNoMemory[] =
            "Insufficient memory for this operation!";

char __based(__segname("STRINGS_TEXT")) szCantChDir[] =
         "Can't change directory to %s!";

/* Get Selection Count returns a count of the selected
 * list box items. If the count is  greater than zero, it also returns
 * a pointer to a locked list in local memory of the item nos.
 * and their local memory handle.
 * A value of -1 indicates an error.
 */
int CLBItemsGet(HWND hListBox, int __far * __far *ppnSelItems, HANDLE *phnd)
{
int cSelLBItems = (int)SendMessage(hListBox, LB_GETSELCOUNT, 0, 0L);

if ( !phnd )
   return -1;

*phnd = 0;
if (cSelLBItems)
   {
   *phnd = GlobalAlloc(GMEM_FIXED, cSelLBItems * sizeof(int));
   if ( !*phnd )
      return -1;

   *ppnSelItems = (int __far *)GlobalLock( *phnd );
   if ( !*ppnSelItems )
      {
      GlobalFree( *phnd );
      *phnd = 0;
      return -1;
      }

   /* Get list of selected items. Return value is number of selected items */
   if (SendMessage(hWndList, LB_GETSELITEMS, cSelLBItems, (LONG)*ppnSelItems) != cSelLBItems)
      {
      GlobalUnlock(*phnd);
      GlobalFree(*phnd);
      *phnd = 0;
      return -1;
      }
   }
   return cSelLBItems;
}

/* Re-select listbox contents from given list. The pnSelItems is a
 * list containing the indices of those items selected in the listbox.
 * This list was probably created by GetLBSelCount() above.
 */
void ReselectLB(HWND hListBox, int cSelLBItems, int __far *pnSelItems)
{
int i;

for (i = 0; i < cSelLBItems; ++i)
   {
   SendMessage(hListBox, LB_SETSEL, TRUE, MAKELPARAM(pnSelItems[i],0));
   }
}


/* Action is called on double-clicking, or selecting one of the 3
 * main action buttons. The action code is the action
 * relative to the listbox or the button ID.
 */
void Action(HWND hWnd, WPARAM wActionCode)
{
HANDLE  hMem;
int i;
int iSelection;
int cch;
int cchCur;
int cchTotal;
unsigned long dFreeSpace;
int __far *pnSelItems;  /* pointer to list of selected items */
#ifndef WIN32
struct diskfree_t df;
#else
DWORD SectorsPerCluster, BytesPerSector, FreeClusters, Clusters;
#endif
HANDLE  hnd = 0;
int cSelLBItems = CLBItemsGet(hWndList, &pnSelItems, &hnd);
int argc;
LPSTR   lpszT;
char **pszIndex;
char *sz;
WORD wIndex = (WORD)(!uf.fFormatLong ? SHORT_FORM_FNAME_INX
   : LONG_FORM_FNAME_INX);

gfCancelDisplay = FALSE;   /* clear any previous cancel */
    /* if no items were selected */
if (cSelLBItems < 1)
   return;

    /* Note: this global value can be overriden in replace.c */
uf.fDoAll = (lpDCL->Overwrite) ? 1 : 0;

SetCapture(hWnd);
hSaveCursor = SetCursor(hHourGlass);
ShowCursor(TRUE);

    /* If all the files are selected pass in no filenames */
    /* since unzipping all files is the default */
hMem = GlobalAlloc( GPTR, 4096 );
if ( !hMem )
   goto done;
lpszT = (LPSTR)GlobalLock( hMem );
if ( !lpszT )
   {
   GlobalFree( hMem );
   goto done;
   }

argc = ((WORD)cSelLBItems == cZippedFiles) ? 0 : 1;
iSelection = 0;

do
   {

   if (argc)
      {
      cchCur = 0;
      dSpaceToUse = 0;
      pszIndex = (char **)rgszFiles;
      cch = (sizeof(char *) * ((cSelLBItems > (cchFilesMax/16)-1 ) ? (cchFilesMax/16) : cSelLBItems+1));
      cchTotal = (cchFilesMax-1) - cch;
      sz = rgszFiles + cch;

      for (i=0; ((i+iSelection)<cSelLBItems) && (i<(cchFilesMax/16)-1); ++i)
         {
         cch = (int)SendMessage(hWndList, LB_GETTEXTLEN, pnSelItems[i+iSelection], 0L);
         if (cch != LB_ERR)
            {
            if ((cchCur+cch+1-wIndex) > cchTotal)
               break;
            cch = (int)SendMessage(hWndList, LB_GETTEXT, pnSelItems[i+iSelection], (LONG)lpszT);
            if ((cch != LB_ERR) && (cch>wIndex))
               {
               /* Get uncompressed totals to pre-flight the extraction process */
               strncpy(sz, lpszT, 9);
               sz[9] = '\0';
               dSpaceToUse += atol(sz);
               lstrcpy(sz, lpszT+wIndex);
               pszIndex[i] = sz;
               cchCur += (cch + 1 - wIndex);
               sz += (cch + 1 - wIndex);
               }
            else
               {
               break;
               }
            }
         else
            {
            MessageBeep(1);
            goto done;
            }
         }
      if (i == 0)
         goto done;
      argc = i;

      pszIndex[i] = 0;
      iSelection += i;
      }
else
   {
   iSelection = cSelLBItems;
   }

switch (wActionCode)
   {
#ifdef WIN32
	char tempChar;
#endif
	int Ok;
	 case 0:         /* extract button */
		 /* Get amount of free space on target drive */
#ifndef WIN32
       _dos_getdiskfree((lpumb->szUnzipToDirName[0] - 'A'+1), &df);
       dFreeSpace = (long)df.avail_clusters *
                    ((long)df.sectors_per_cluster *
                    (long)df.bytes_per_sector);
#else
      tempChar = lpumb->szUnzipToDirName[3];
      lpumb->szUnzipToDirName[3] = '\0';
      GetDiskFreeSpace(lpumb->szUnzipToDirName, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &Clusters);
      lpumb->szUnzipToDirName[3] = tempChar;
      dFreeSpace = (long)SectorsPerCluster * (long)BytesPerSector * (long)FreeClusters;
#endif
      if (dFreeSpace < dSpaceToUse)
         {
         sprintf(sz, "Free Disk Space = %lu bytes\nUncompressed Files Total %lu bytes\n",
            dFreeSpace, dSpaceToUse);
         Ok = MessageBox(hWndMain, sz, "Insufficient Disk Space?", MB_OKCANCEL |
            MB_ICONEXCLAMATION);
         }
      else
         Ok = IDOK;
      if (Ok != IDOK)
         break;
      lpDCL->ncflag = 0;
      lpDCL->ntflag = 0;
      lpDCL->nvflag = 0;
      lpDCL->nUflag = lpDCL->ExtractOnlyNewer;
      lpDCL->nzflag = 0;
      lpDCL->ndflag = (int)(uf.fRecreateDirs ? 1 : 0);
      lpDCL->noflag = uf.fDoAll;
      lpDCL->naflag = (int)(uf.fTranslate ? 1 : 0);
      lpDCL->argc   = argc;
		lpDCL->lpszZipFN = lpumb->szFileName;
      lpDCL->FNV = (char **)rgszFiles;
#ifdef USEWIZUNZDLL
      {
      int DlgDirListOK = 1; /* non-zero when DlgDirList() succeeds */
            /* If extracting to different directory from archive dir.,
             * temporarily go to "unzip to" directory.
             */
      if (!uf.fUnzipToZipDir && lpumb->szUnzipToDirName[0])
         {
         lstrcpy(lpumb->szBuffer, lpumb->szUnzipToDirName); /* OK to clobber szBuffer! */
         DlgDirListOK = DlgDirList(hWnd, lpumb->szBuffer, 0, 0, 0);
         }
      if (!DlgDirListOK)   /* if DlgDirList failed  */
         {
         wsprintf(lpumb->szBuffer, szCantChDir, lpumb->szUnzipToDirName);
         MessageBox(hWndMain, lpumb->szBuffer, NULL, MB_OK | MB_ICONEXCLAMATION);
         }
      else
         {
			(*DllProcessZipFiles)(lpDCL);
         if (!uf.fUnzipToZipDir && lpumb->szUnzipToDirName[0])
            {
            lstrcpy(lpumb->szBuffer, lpumb->szDirName); /* OK to clobber szBuffer! */
            if (!DlgDirList(hWnd, lpumb->szBuffer, 0, 0, 0)) /* cd back */
               {
               wsprintf(lpumb->szBuffer, szCantChDir, lpumb->szDirName);
               MessageBox(hWndMain, lpumb->szBuffer, NULL, MB_OK | MB_ICONEXCLAMATION);
               }
            }
         }
         }
#else
         if (FSetUpToProcessZipFile(lpDCL))
         {
         int DlgDirListOK = 1; /* non-zero when DlgDirList() succeeds */

            /* If extracting to different directory from archive dir.,
             * temporarily go to "unzip to" directory.
             */
         if (!uf.fUnzipToZipDir && lpumb->szUnzipToDirName[0])
            {
            lstrcpy(lpumb->szBuffer, lpumb->szUnzipToDirName); /* OK to clobber szBuffer! */
            DlgDirListOK = DlgDirList(hWnd, lpumb->szBuffer, 0, 0, 0);
            }
         if (!DlgDirListOK)   /* if DlgDirList failed  */
            {
            wsprintf(lpumb->szBuffer, szCantChDir, lpumb->szUnzipToDirName);
            MessageBox(hWndMain, lpumb->szBuffer, NULL, MB_OK | MB_ICONEXCLAMATION);
            }
         else
            {
            process_zipfiles(__G);   /* extract the file(s)         */
               /* Then return to archive dir. after extraction.
                * (szDirName is always defined if archive file defined)
                */
            if (!uf.fUnzipToZipDir && lpumb->szUnzipToDirName[0])
               {
               lstrcpy(lpumb->szBuffer, lpumb->szDirName); /* OK to clobber szBuffer! */
               if (!DlgDirList(hWnd, lpumb->szBuffer, 0, 0, 0)) /* cd back */
                  {
                  wsprintf(lpumb->szBuffer, szCantChDir, lpumb->szDirName);
                  MessageBox(hWndMain, lpumb->szBuffer, NULL, MB_OK | MB_ICONEXCLAMATION);
                  }
               }
            }
            }
            else

                /* FSetUpToProcessZipFile failed */
               {
                MessageBox(hWndMain, szNoMemory, NULL, MB_OK | MB_ICONEXCLAMATION);
               }
            TakeDownFromProcessZipFile();
#endif
            break;
        case 1:     /* display to message window */
            bRealTimeMsgUpdate = FALSE;
            lpDCL->ncflag = 1;
            lpDCL->ntflag = 0;
            lpDCL->nvflag = 0;
            lpDCL->nUflag = 1;
            lpDCL->nzflag = 0;
            lpDCL->ndflag = 0;
            lpDCL->noflag = 0;
            lpDCL->naflag = 0;
            lpDCL->argc   = argc;
				lpDCL->lpszZipFN = lpumb->szFileName;
            lpDCL->FNV = (char **)rgszFiles;
#ifdef USEWIZUNZDLL
				(*DllProcessZipFiles)(lpDCL);
#else
            if (FSetUpToProcessZipFile(lpDCL))
            {
                process_zipfiles(__G);
            }
            else
            {
                MessageBox(hWndMain, szNoMemory, NULL, MB_OK | MB_ICONEXCLAMATION);
            }

            TakeDownFromProcessZipFile();
#endif
            bRealTimeMsgUpdate = TRUE;
         if (uf.fAutoClearStatus)   /* if automatically clearing status, leave user at top */
            SetStatusTopWndPos();

         else   /* traditional behavior leaves user at bottom of window */
            UpdateMsgWndPos();

         /* Following extraction to status window, user will want
          * to scroll around, so leave him/her on Status window.
          */
           if (wWindowSelection != IDM_MAX_LISTBOX)
              PostMessage(hWnd, WM_COMMAND, IDM_SETFOCUS_ON_STATUS, 0L);

            break;
        case 2:     /* test */
            lpDCL->ncflag = 0;
            lpDCL->ntflag = 1;
            lpDCL->nvflag = 0;
            lpDCL->nUflag = 1;
            lpDCL->nzflag = 0;
            lpDCL->ndflag = 0;
            lpDCL->noflag = 0;
            lpDCL->naflag = 0;
				lpDCL->argc   = argc;
				lpDCL->lpszZipFN = lpumb->szFileName;
            lpDCL->FNV = (char **)rgszFiles;
#ifdef USEWIZUNZDLL
				(*DllProcessZipFiles)(lpDCL);
#else
            if (FSetUpToProcessZipFile(lpDCL))
            {
                process_zipfiles(__G);
            }
            else
            {
                MessageBox(hWndMain, szNoMemory, NULL, MB_OK | MB_ICONEXCLAMATION);
            }

            TakeDownFromProcessZipFile();
#endif
            break;
		  }
    } while (iSelection < cSelLBItems);


    /* march through list box checking what's selected
     * and what is not.
     */

done:

    if ( hMem )
    {
        GlobalUnlock( hMem );
        GlobalFree( hMem );
    }
    GlobalUnlock(hnd);
    GlobalFree(hnd);


    ShowCursor(FALSE);
    SetCursor(hSaveCursor);
    ReleaseCapture();
   SoundAfter();      /* play sound afterward if requested */
    if (!uf.fIconSwitched)  /* if haven't already, switch icons */
    {
        HANDLE hIcon;

        hIcon = LoadIcon(hInst,"UNZIPPED"); /* load final icon   */
        assert(hIcon);
#ifndef WIN32
        SetClassWord(hWndMain, GCW_HICON, hIcon);
#else
        SetClassLong(hWndMain, GCL_HICON, (LONG)hIcon);
#endif
        uf.fIconSwitched = TRUE;    /* flag that we've switched it  */
    }
}

/* Display the archive comment using the Info-ZIP engine. */
void DisplayComment(HWND hWnd)
{

   SetCapture(hWnd);
   hSaveCursor = SetCursor(hHourGlass);
   ShowCursor(TRUE);
   bRealTimeMsgUpdate = FALSE;
   gfCancelDisplay = FALSE;   /* clear any previous cancel */
   /* Called here when showing zipfile comment */
   lpDCL->ncflag = 0;
   lpDCL->ntflag = 0;
   lpDCL->nvflag = 0;
   lpDCL->nUflag = 1;
   lpDCL->nzflag = 1;
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
        MessageBox(hWndMain, szNoMemory, NULL, MB_OK | MB_ICONEXCLAMATION);
    }

   TakeDownFromProcessZipFile();
#endif
   ShowCursor(FALSE);
   SetCursor(hSaveCursor);
   bRealTimeMsgUpdate = TRUE;
   UpdateMsgWndPos();
   ReleaseCapture();
   SoundAfter();      /* play sound during if requested         */
}
