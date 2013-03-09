#include <stdio.h>
#include "wingui\sound.h"
#include "wingui\wizunzip.h"
#include "wingui\helpids.h"
#include <mmsystem.h>

/* WizUnZip sound control module, sound.c.
 * This module controls what sound WizUnZip makes after during or after extracting a file.
 * All users can optionally produce a beep after unzipping. Users with Windows 3.1
 * or Windows 3.0 + Multimedia extensions can play wave files.
 * WizUnZip uses the presence of the DLL MMSYSTEM.DLL/WIN16MM.DLL to determine
 * whether MM capability is present.  It further queries the number of wave
 * devices in the system to see if wave playing hardware is present.
 * This approach gives maximum system useability without causing Windows
 * errors such as "Can't find dynalink!"
 */

#define MAXFILTERBUF 50

static char __based(__segname("STRINGS_TEXT")) szBeepOnFinish[] = "Beep";
/* sndPlaySound is apparently an obsolete function not properly supported
   by Win32 (or perhaps by WINMM.DLL), use PlaySound instead. Of course,
   PlaySound is not supported under Windows 3.1x
*/
#ifndef WIN32
static char __based(__segname("STRINGS_TEXT")) szMMSystemDll[] = "MMSYSTEM.DLL";
static char __based(__segname("STRINGS_TEXT")) szPlaySound[] = "sndPlaySound";
#else
static char __based(__segname("STRINGS_TEXT")) szMMSystemDll[] = "WINMM.DLL";
static char __based(__segname("STRINGS_TEXT")) szPlaySound[] = "PlaySound";
#endif
static char __based(__segname("STRINGS_TEXT")) szDfltWaveFile[] = "WIZUNZIP.WAV";
static char __based(__segname("STRINGS_TEXT")) szSoundNameKey[] = "SoundName"; /* key in .INI */
static char __based(__segname("STRINGS_TEXT")) szSoundOptKey[] = "SoundOption";
static char __based(__segname("STRINGS_TEXT")) szWaveBrowseTitle[] = "Browse Sound Files";
static char __based(__segname("STRINGS_TEXT")) gszFilter[MAXFILTERBUF] =
                           "Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0\0";
static char  *SoundOptsTbl[] = /* for .INI file */
   {"none", "beep", "PlayDuring", "PlayAfter" };

static HINSTANCE hinstMMSystem;     /* MMSystem/WINMM DLL instance */
static FARPROC lpSndPlaySound;      /* pointer to SndPlaySound()/PlaySound() */
static FARPROC lpWaveOutGetNumDevs; /* pointer to WaveOutGetNumDevs() */
static BOOL CanPlayWave(void);
static WORD uSoundButtonSelected = IDM_SOUND_NONE;
                                    /* state of sound option */
static WORD uSoundButtonSelectedTmp;/* state of sound option while in dialog box */


/* Forward Refs
 */
static BOOL DoOpenFile(HWND hWnd, LPSTR lpDestFileName);
static BOOL CanPlayWave(void);

/* Test for Wave Playing Capability
 */
static BOOL CanPlayWave(void)
{
static bTestedForWave = FALSE; /* true if test for wave playing has been done */
static bCanPlayWave = FALSE;   /* true if system can play wave   */
int   nPrevErrorMode;         /* previous error mode               */

if (bTestedForWave)         /* deja vu ? */
   return(bCanPlayWave);

bTestedForWave = TRUE;
nPrevErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
hinstMMSystem = LoadLibrary(szMMSystemDll);
SetErrorMode(nPrevErrorMode);
#ifndef WIN32
if (hinstMMSystem >  (HINSTANCE) HINSTANCE_ERROR)
#else
if (hinstMMSystem != NULL)
#endif
   {
      /* If can't load the function which looks up no. wave out devices or
       * number of wave output devices is zero, we can't play waves.
       */
#ifdef __BORLANDC__
#pragma warn -pro
#endif

   if ((lpWaveOutGetNumDevs =
      GetProcAddress(hinstMMSystem, "waveOutGetNumDevs")) == NULL ||
      (*lpWaveOutGetNumDevs)() == 0 ||
      (lpSndPlaySound =
      GetProcAddress(hinstMMSystem, szPlaySound)) == NULL)

#ifdef __BORLANDC__
#pragma warn .pro
#endif
      {
      FreeLibrary(hinstMMSystem);   /* unload library   */
      }
   else /* we're set to play waves */
      {
      bCanPlayWave = TRUE;   /* flag that we can play waves   */
      }
   }
return bCanPlayWave;
}

/* Migrate Sound Options translates the former beep-on-finish option into
 * one of the first 2 of the 4 sound options.
 */
void MigrateSoundOptions(void)
{
UINT SoundOptsTableIndex;

   GetProfileString(szAppName, szBeepOnFinish, szNo, lpumb->szBuffer, OPTIONS_BUFFER_LEN);
   SoundOptsTableIndex = (UINT)(!lstrcmpi(lpumb->szBuffer, szNo) ? 0 : 1);
   WritePrivateProfileString(szAppName, szLBSelectionKey,
                        SoundOptsTbl[SoundOptsTableIndex],
                        szWizUnzipIniFile);
}

/* Initialize Sound Options is called on WizUnZip start-up to read
 * the sound option and sound name.
 * Read the Sound Option to see if user wants beep, sound, or nothing.
 * Read chosen sound name or wave file.
 */
void InitSoundOptions(void)
{
   GetPrivateProfileString(szAppName, szSoundOptKey, SoundOptsTbl[0],
                     lpumb->szBuffer, OPTIONS_BUFFER_LEN,
                     szWizUnzipIniFile);
   lpumb->szBuffer[255] = '\0';   /* force truncation */
   for (uSoundButtonSelected = IDM_SOUND_NONE;
       uSoundButtonSelected < IDM_SOUND_WAVE_AFTER &&
       lstrcmpi(lpumb->szBuffer, SoundOptsTbl[uSoundButtonSelected-IDM_SOUND_NONE]) ;
       uSoundButtonSelected++)
      ;

   /* Do range check on sound option. Set to none if necessary.
    */
   if (uSoundButtonSelected > IDM_SOUND_WAVE_AFTER ||
      (uSoundButtonSelected > IDM_SOUND_BEEP &&
       !CanPlayWave()))
      uSoundButtonSelected = IDM_SOUND_NONE;

   GetPrivateProfileString(szAppName, szSoundNameKey, szDfltWaveFile,
                     lpumb->szSoundName, WIZUNZIP_MAX_PATH,
                     szWizUnzipIniFile);
}


/* Play Sound During extraction, test, or display  if requested.
 * Don't use a default if nothing specified.
 */
void SoundDuring(void)
{
   if (uSoundButtonSelected == IDM_SOUND_WAVE_DURING && CanPlayWave())
      {
#ifdef __BORLANDC__
#pragma warn -pro
#endif

#ifdef WIN32
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, NULL, SND_ASYNC|SND_NOSTOP|SND_NODEFAULT);
#else
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, SND_ASYNC|SND_NOSTOP|SND_NODEFAULT);
#endif

#ifdef __BORLANDC__
#pragma warn .pro
#endif
      }
}

/* Play Sound After extraction, test, or display  if requested.
 */
void SoundAfter(void)
{
   switch (uSoundButtonSelected) {
   case IDM_SOUND_BEEP:
      MessageBeep(1);
      break;
   case IDM_SOUND_WAVE_AFTER:
      if (CanPlayWave())
#ifdef __BORLANDC__
#pragma warn -pro
#endif

#ifdef WIN32
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, NULL, SND_ASYNC|SND_NOSTOP);
#else
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, SND_ASYNC|SND_NOSTOP);
#endif

#ifdef __BORLANDC__
#pragma warn .pro
#endif
      break;
   }
}

/* DoFileOpen Dialog calls the common dialog function GetOpenFileName()
 * to browse and select a table file.
 */
static BOOL DoOpenFile(HWND hWnd, LPSTR lpDestFileName)
{
BOOL fRetCode = FALSE;   /* assume failure            */
/* DWORD dwExtdError; Not used */

         lpumb->wofn.lStructSize       = sizeof(OPENFILENAME);
         lpumb->wofn.hwndOwner         = (HWND)hWnd;
         lpumb->wofn.hInstance         = (HANDLE)NULL;
         lpumb->wofn.lpstrFilter       = gszFilter;
         lpumb->wofn.lpstrCustomFilter = (LPSTR)NULL;
         lpumb->wofn.nMaxCustFilter    = 0L;
         lpumb->wofn.nFilterIndex      = 1L;
         lpumb->wofn.lpstrFile         = lpDestFileName;
         lpDestFileName[0]               = '\0';

         lpumb->wofn.nMaxFile          = (DWORD)WIZUNZIP_MAX_PATH;
         lpumb->wofn.lpstrFileTitle    = (LPSTR)NULL;
         lpumb->wofn.nMaxFileTitle     = 0;
         lpumb->wofn.lpstrInitialDir   = (LPSTR)NULL;
         lpumb->wofn.lpstrTitle        = (LPSTR)szWaveBrowseTitle;
         lpumb->wofn.Flags             = OFN_HIDEREADONLY|
                                         OFN_PATHMUSTEXIST|
                                         OFN_FILEMUSTEXIST|
                                         OFN_NOCHANGEDIR|
                                         OFN_SHOWHELP ;
         lpumb->wofn.nFileOffset       = 0;
         lpumb->wofn.nFileExtension    = 0;
         lpumb->wofn.lpstrDefExt       = "WAV";
         lpumb->wofn.lCustData         = 0L;
         lpumb->wofn.lpfnHook         = (FARPROC)NULL;
         lpumb->wofn.lpTemplateName    = (LPSTR)NULL;

   if ( GetOpenFileName( &(lpumb->wofn) ) )
      {
        HFILE   hFile;
        OFSTRUCT ofstruct;

        hFile=OpenFile(lpumb->wofn.lpstrFile, &ofstruct, OF_EXIST);
        if (hFile != -1)
        {
          fRetCode = TRUE;
        }
/* NOTE!!!  On a closed system (ie, not running on a network)
 * this OpenFile call should NEVER fail.  This because we passed in the
 * OFN_FILEMUSTEXIST flag to CD.  However, on a network system,
 * there is a *very* small chance that between the time CD's checked
 * for existance of the file and the time the call to OpenFile
 * was made here, someone else on the network has deleted the file.
 * MORAL: ALWAYS, ALWAYS, ALWAYS check the return code from your
 * call to OpenFile() or _lopen.
 */

      }
      else /* dialog failed */
      {
/*         dwExtdError = CommDlgExtendedError();   dwExtdError code not used */
         CommDlgExtendedError();   /* get error code */
      }
   return(fRetCode);         /* return indication      */
}

/****************************************************************************

    FUNCTION: SoundProc(HWND, unsigned, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Sound Options" dialog box of WizUnZip
    AUTHOR:   Robert A. Heath  2/15/93    Public Domain -- help yourself.

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

****************************************************************************/

#ifdef __BORLANDC__
#pragma argsused
#endif
BOOL WINAPI
SoundProc(HWND hwndDlg, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
static HWND hSoundWaveDuring, hSoundWaveAfter, hFileText, hSoundEdit, hPlay, hBrowse;
static UINT uMaxSoundRadioButton; /* upper boundary of uSoundButtonSelected   */

   switch (wMessage) {
   case WM_INITDIALOG:
      if (CanPlayWave())
      {
         hSoundWaveDuring = GetDlgItem(hwndDlg, IDM_SOUND_WAVE_DURING);
         hSoundWaveAfter = GetDlgItem(hwndDlg, IDM_SOUND_WAVE_AFTER);
         hFileText = GetDlgItem(hwndDlg, IDM_SOUND_FILE_TEXT);
         hSoundEdit = GetDlgItem(hwndDlg, IDM_SOUND_EDIT);
         hPlay = GetDlgItem(hwndDlg, IDM_SOUND_PLAY);
         hBrowse = GetDlgItem(hwndDlg, IDM_SOUND_BROWSE);
         EnableWindow(hSoundWaveDuring, TRUE);
         EnableWindow(hSoundWaveAfter, TRUE);
         WinAssert(hFileText);
         EnableWindow(hFileText, TRUE);
         EnableWindow(hSoundEdit, TRUE);
         EnableWindow(hPlay, TRUE);
         EnableWindow(hBrowse, TRUE);
         SetDlgItemText(hwndDlg, IDM_SOUND_EDIT, lpumb->szSoundName);
         uMaxSoundRadioButton = IDM_SOUND_WAVE_AFTER;
      }
      else   /* Can't play wave */
      {
         uMaxSoundRadioButton = IDM_SOUND_BEEP;
      }
      uSoundButtonSelectedTmp = uSoundButtonSelected; /* initialize temp value */
      CheckRadioButton(hwndDlg, IDM_SOUND_NONE, uMaxSoundRadioButton, uSoundButtonSelectedTmp);
//#ifdef NEEDME
      CenterDialog(GetParent(hwndDlg), hwndDlg);
//#endif
      return TRUE;
   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDM_SOUND_NONE:
      case IDM_SOUND_BEEP:
      case IDM_SOUND_WAVE_DURING:
      case IDM_SOUND_WAVE_AFTER:
         uSoundButtonSelectedTmp = LOWORD(wParam);
         CheckRadioButton(hwndDlg, IDM_SOUND_NONE, uMaxSoundRadioButton,
                        uSoundButtonSelectedTmp);
         break;
      case IDM_SOUND_PLAY:
         GetDlgItemText(hwndDlg, IDM_SOUND_EDIT, lpumb->szSoundName, WIZUNZIP_MAX_PATH);
#ifdef __BORLANDC__
#pragma warn -pro
#endif

#ifdef WIN32
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, NULL, SND_ASYNC|SND_NOSTOP);
#else
      (*lpSndPlaySound)((LPSTR)lpumb->szSoundName, SND_ASYNC|SND_NOSTOP);
#endif

#ifdef __BORLANDC__
#pragma warn .pro
#endif
         break;
      case IDM_SOUND_BROWSE:
         if (DoOpenFile(hwndDlg, lpumb->szSoundName))
         {
            /* transfer to command window          */
                SetDlgItemText(hwndDlg, IDM_SOUND_EDIT, lpumb->szSoundName);
         }

         break;
       case IDOK:
         uSoundButtonSelected = uSoundButtonSelectedTmp;
         WritePrivateProfileString(szAppName, szSoundOptKey,
            SoundOptsTbl[uSoundButtonSelected-IDM_SOUND_NONE], szWizUnzipIniFile);
         GetDlgItemText(hwndDlg, IDM_SOUND_EDIT, lpumb->szSoundName, WIZUNZIP_MAX_PATH);
         WritePrivateProfileString(szAppName, szSoundNameKey,
                     lpumb->szSoundName,   szWizUnzipIniFile);
         EndDialog(hwndDlg, TRUE);
         break;
      case IDCANCEL:
         /* restore former value of sound file name
          */
         GetPrivateProfileString(szAppName, szSoundNameKey, szDfltWaveFile,
                     lpumb->szSoundName, WIZUNZIP_MAX_PATH,
                     szWizUnzipIniFile);
         EndDialog(hwndDlg, FALSE);
         break;
      case IDM_SOUND_HELP:
            WinHelp(hwndDlg,szHelpFileName,HELP_CONTEXT, (DWORD)(HELPID_SOUND_OPTIONS));
         break;
      }
      return TRUE;
   }
   return FALSE;
}

