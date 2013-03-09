/* MS Windows Setup  and Take-Down functions bracket calls to
 * process_zipfiles().
 * These functions allocate and free the necessary buffers, set and clear
 * any global variables so that  process_zipfiles()  can be called multiple
 * times in the same session of WizUnzip. You'll recognize some of the
 * code from main() in SetUpToProcessZipFile().
 */

#include <stdio.h>
#include <windows.h>
#include "version.h"
#include "wingui\wizunzip.h"
#define UNZIP_INTERNAL
#include "unzip.h"
#include "consts.h"

#ifndef USEWIZUNZDLL
static long ziplen;
static uch *outout;
#endif /* USEWIZUNZDLL */

HANDLE hOutBuf;
HANDLE hInBuf;
HANDLE hZipFN;
HANDLE hwildZipFN;
HANDLE hFileName;

/*
    ncflag    = write to stdout if true
    ntflag    = test zip file
    nvflag    = verbose listing
    nUflag    = "update" (extract only newer/new files
    nzflag    = display zip file comment
    ndflag    = all args are files/dir to be extracted
    noflag    =
    naflag    = do ASCII-EBCDIC and/or end of line translation
    argc      =
    lpszZipFN = zip file name
    FNV       = file name vector to list of files to extract
*/
#ifndef USEWIZUNZDLL
BOOL FSetUpToProcessZipFile(LPDCL C)
{
    /* clear all global flags -- need to or not. */
    G.tflag=G.vflag=G.cflag=G.aflag=G.uflag=G.qflag=G.zflag=0;
    G.overwrite_all=G.overwrite_none=0;
    G.pfnames = &fnames[0];       /* assign default file name vector */

    G.jflag = !C->ndflag; /* WizUnZip perspective is "recreate dir structure" */
    G.cflag = C->ncflag ;
    G.overwrite_all = C->noflag;
    G.tflag = C->ntflag ;
    G.vflag = C->nvflag;
    G.zflag = C->nzflag;
    G.uflag = C->nUflag;
    G.aflag = C->naflag;
    G.uflag = C->ExtractOnlyNewer;
    if (C->Overwrite) {
        G.overwrite_all = TRUE;
        G.overwrite_none = FALSE;
    } else {
        G.overwrite_all = FALSE;
        G.overwrite_none = TRUE;
    }
    G.sflag = C->SpaceToUnderscore; /* Translate spaces to underscores? */

    G.filespecs = C->argc;
    G.xfilespecs = 0;

    G.local_hdr_sig[0] = G.central_hdr_sig[0] = G.end_central_sig[0] = '\120';
    G.local_hdr_sig[1] = G.central_hdr_sig[1] = G.end_central_sig[1] = '\0';

    G.cur_zipfile_bufstart = 0L;
    if ((hZipFN = GlobalAlloc(GMEM_MOVEABLE, FILNAMSIZ))== NULL)
        return FALSE;

    G.zipfn = (char far *)GlobalLock(hZipFN);
    lstrcpy(G.zipfn, C->lpszZipFN);

/* MW: G.wildzipfn needs to be initialized so that do_wild does not wind
   up clearing out the zip file name when it returns in process.c
*/
    if ((hwildZipFN = GlobalAlloc(GMEM_MOVEABLE, FILNAMSIZ))== NULL)
        return FALSE;

    G.wildzipfn = GlobalLock(hwildZipFN);
    lstrcpy(G.wildzipfn, C->lpszZipFN);

    if (stat(G.zipfn, &G.statbuf) || (G.statbuf.st_mode & S_IFMT) == S_IFDIR)
        strcat(G.zipfn, ZSUFX);

    if (stat(G.zipfn, &G.statbuf)) {  /* try again */
        fprintf(stdout, "error:  can't find zipfile [ %s ]\n", G.zipfn);
        return FALSE;
    } else
        ziplen = G.statbuf.st_size;

    if (C->argc != 0) {
        G.pfnames = C->FNV;
        G.process_all_files = FALSE;
    } else
        G.process_all_files = TRUE;       /* for speed */

/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

    if ((hInBuf = GlobalAlloc(GMEM_MOVEABLE, INBUFSIZ+4)) != NULL) {
        G.inbuf = (uch *) GlobalLock(hInBuf);
        WinAssert(G.inbuf);
    }
    if ((hOutBuf = GlobalAlloc(GMEM_MOVEABLE, OUTBUFSIZ+1)) != NULL) {
        G.outbuf = (uch *)GlobalLock(hOutBuf);
        WinAssert(G.outbuf);
        outout = G.outbuf;  /*  point to outbuf */
    }
    if ((hFileName = GlobalAlloc(GMEM_MOVEABLE, FILNAMSIZ)) != 0) {
        G.filename = GlobalLock(hFileName);
        WinAssert(G.filename);
    }

    if ((G.inbuf == NULL) || (G.outbuf == NULL) || (outout == NULL) ||
        (G.zipfn == NULL) || (G.filename == NULL))
        return FALSE;

    G.hold = &G.inbuf[INBUFSIZ]; /* to check for boundary-spanning signatures */

    return TRUE;    /* set up was OK */
}

void TakeDownFromProcessZipFile(void)
{
    if (G.inbuf) {
        GlobalUnlock(hInBuf);
        G.inbuf = NULL;
    }
    if (hInBuf)
        hInBuf = GlobalFree(hInBuf);

    if (G.outbuf) {
        GlobalUnlock(hOutBuf);
        G.outbuf = NULL;
    }
    if (hOutBuf)
        hOutBuf = GlobalFree(hOutBuf);

    if (G.zipfn) {
        GlobalUnlock(hZipFN);
        G.zipfn = NULL;
    }
    if (hZipFN)
        hZipFN = GlobalFree(hZipFN);

    if (G.wildzipfn) {
        GlobalUnlock(hwildZipFN);
        G.wildzipfn = NULL;
    }
    if (hwildZipFN)
        hwildZipFN = GlobalFree(hwildZipFN);

    if (G.filename) {
        GlobalUnlock(hFileName);
        G.filename = NULL;
    }
    if (hFileName)
        hFileName = GlobalFree(hFileName);
}

int GetReplaceDlgRetVal(void)
{
    FARPROC lpfnprocReplace;
    int ReplaceDlgRetVal;   /* replace dialog return value */

    ShowCursor(FALSE);      /* turn off cursor */
    SetCursor(hSaveCursor); /* restore the cursor */
    lpfnprocReplace = MakeProcInstance(ReplaceProc, hInst);
    ReplaceDlgRetVal = DialogBoxParam(hInst, "Replace",
      hWndMain, lpfnprocReplace, (DWORD)(LPSTR)G.filename);
#ifndef WIN32
    FreeProcInstance(lpfnprocReplace);
#endif
    hSaveCursor = SetCursor(hHourGlass);
    ShowCursor(TRUE);
    return ReplaceDlgRetVal;
}

#endif /* USEWIZUNZDLL */
