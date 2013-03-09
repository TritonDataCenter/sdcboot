/* Windows Info-ZIP UnZip DLL module
 *
 * Author: Mike White
 *
 * Original: 1996
 *
 * This module has essentially the entry point for "unzipping" a zip file. It
 * also contains "fake" calls for the sound and printing functions in WizUnZip,
 * which are simply passed on to the application calling the dll.
 */

/*

A pointer to this structure is passed to the DLL when it is called.

typedef int (far *DLLPRNT) (FILE *, unsigned int, char *);
typedef void (far *DLLSND) (void);

typedef struct {
DLLPRNT print;          = a pointer to the application's print routine.
DLLSND sound;           = a pointer to the application's sound routine.
FILE *Stdout;           = stdout for the calling application
LPUMB lpUMB;            = stucture shown below
HWND hWndList;          = application's handle for a list box to display
                          the contents of an archive in
HWND hWndMain;          = application's main window
HWND hInst;             = application's instance
int ExtractOnlyNewer;   = true if you are to extract only newer
int Overwrite;          = true if always overwrite files
int SpaceToUnderscore;  = true if convert space to underscore
int PromptToOverwrite;  = true if prompt to overwrite is wanted
int ncflag              = write to stdout if true
int ntflag              = test zip file
int nvflag              = verbose listing
int nUflag              = "update" (extract only newer/new files
int nzflag              = display zip file comment
int ndflag              = all args are files/dir to be extracted
int noflag              =
int naflag              = do ASCII-EBCDIC and/or end of line translation
int argc                =
LPSTR lpszZipFN         = zip file name
char **FNV              = file name vector to list of files to extract
} DCL, _far *LPDCL;

LPUMB structure referred to above:
typedef struct
{
char szFileName[WIZUNZIP_MAX_PATH]; = fully-qualified archive file name
                                      in OEM char set
char szDirName[WIZUNZIP_MAX_PATH];  = directory of archive file in ANSI char set
char szUnzipToDirName[WIZUNZIP_MAX_PATH];    = extraction ("unzip to") directory
                                               name in ANSI
char szUnzipToDirNameTmp[WIZUNZIP_MAX_PATH]; = temp extraction ("unzip to")
                                               directory name in ANSI
char szUnzipFromDirName[WIZUNZIP_MAX_PATH];  = extraction ("unzip from")
                                               directory name in ANSI
char szTotalsLine[80];              = text for totals of zip archive
char szBuffer[OPTIONS_BUFFER_LEN];  = option strings from .INI, & general
                                      scratch buf
char szSoundName[WIZUNZIP_MAX_PATH];= wave file name or sound from .INI file
char szPassword[81];                = password for encrypted files
LPSTR lpPassword;                   = pointer to szPassword. This is passed
                                      only to allow common usage between the
                                      DLL and WizUnZip as a stand-alone
                                      application. It is not used
OPENFILENAME ofn;                   = archive open file name structure
OPENFILENAME wofn;                  = wave open file name struct
MSG msg;                            =
OFSTRUCT of;                        = archive open file struct
OFSTRUCT wof;                       = wave open file struct
} UMB, __far *LPUMB;
*/

#define UNZIP_INTERNAL
#include <windows.h>
#include "unzip.h"
#include "version.h"
#include "wingui\wizunzip.h"
#include "wizdll\wizdll.h"

LPUMB lpumb;

DLLPRNT lpPrint;
DLLSND lpSound;

FILE *orig_stdout;

/*  DLL Entry Point */

#ifdef __BORLANDC__
#pragma warn -par
#endif
#if defined WIN32
BOOL WINAPI DllEntryPoint( HINSTANCE hinstDll,
                           DWORD fdwRreason,
                           LPVOID plvReserved)
#else
int FAR PASCAL LibMain( HINSTANCE hInstance,
                        WORD wDataSegment,
                        WORD wHeapSize,
                        LPSTR lpszCmdLine )
#endif
{
#ifndef WIN32
/* The startup code for the DLL initializes the local heap(if there is one)
 * with a call to LocalInit which locks the data segment.
 */

if ( wHeapSize != 0 )
   {
   UnlockData( 0 );
   }
#endif
return 1;   /* Indicate that the DLL was initialized successfully. */
}

int FAR PASCAL WEP ( int bSystemExit )
{
return 1;
}
#ifdef __BORLANDC__
#pragma warn .par
#endif

/* DLL calls */

WINAPI DllProcessZipFiles(DCL far*C)
{
int retcode;
if (C->lpszZipFN == NULL) /* Something has screwed up, we don't have a filename */
    return PK_NOZIP;

CONSTRUCTGLOBALS();

lpDCL = C;
lpumb = C->lpUMB;
lpPrint = C->print;
lpSound = C->sound;
hWndList = C->hWndList;
hWndMain = C->hWndMain;
hInst = C->hInst;
orig_stdout = C->Stdout;

G.key = (char *)NULL;

FSetUpToProcessZipFile(C);
retcode = process_zipfiles(__G);
TakeDownFromProcessZipFile();

DESTROYGLOBALS();
return retcode;
}

WINAPI GetDllVersion(DWORD far *dwVersion)
{
*dwVersion = MAKELONG(DLL_MINOR, DLL_MAJOR);
return;
}

void SoundDuring(void)
{
#ifdef __BORLANDC__
#pragma warn -pro
#endif
lpSound();
#ifdef __BORLANDC__
#pragma warn .pro
#endif
}
int win_fprintf(FILE *file, unsigned int size, char *buffer)
{
if ((file == stderr) || (file == stdout))
   {
   file = orig_stdout;
#ifdef __BORLANDC__
#pragma warn -pro
#endif
   /* If this is a message of any sort, or "unzip" to stdout, then it needs
    * to be sent to the calling application for processing.
    */
   return lpPrint(file, size, buffer);
#ifdef __BORLANDC__
#pragma warn .pro
#endif
   }
else
   {
   write(fileno(file),(char *)(buffer),size);
   return size;
   }
}

