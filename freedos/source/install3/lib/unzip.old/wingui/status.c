/* Status.c -- the status module of WizUnZip
 * Robert Heath. 1991.
 *
 * Modifications: 1995, Mike White
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <stdarg.h>

#include "wingui\wizunzip.h"

#define UNZIP_INTERNAL
#include "unzip.h"

#define STATUS_INCREMENT    512 /* incremental status data size     */
#define MAX_H_CHARS 160         /* max horizontal chars.            */
#define MAX_INDEX_ENTRIES 16    /* Message Window index max entries */
#define cchBufferMax 0xffffL    /* max Message Buffer size. Must fit
                                 * within one memory segment!       */
#define cchTextOutMax 0x7fffL /* max no. bytes TextOut() accepts */
#define STDIO_BUF_SIZE (FILNAMSIZ+LONG_FORM_FNAME_INX) /* buffer size during printf or fprintf */

static short yClient;               /* height of client area */
static short nVscrollPos = 0;       /* scroll position of mesg. window  */
static short nNumLines = 0;         /* number of lines in buffer */
static short nVscrollMax;           /* max scroll position of mesg. window  */
static DWORD dwStatusSize = 0L;     /* status data size */
static DWORD dwBufferSize = 0L;     /* Status buffer size.  Never
                                       exceeds cchBufferMax     */
static HANDLE hStatusBuffer = NULL;        /* global mesg. handle  */
static DWORD rgidwMsgWin[MAX_INDEX_ENTRIES]; /* max index entries   */
static short    cMsgWinEntries;         /* no. active index entries, with
                                       MAX_INDEX_ENTRIES as its max.
                                       When set to 0, it's time to
                                       re-index.*/
static short nLinesPerEntry;        /* lines per index entry */

/* displayed when buffer shouldn't grow or can't grow */
static char __based(__segname("STRINGS_TEXT")) szClearBufferMsg[] =
            "Clearing Status window to make room for more information.";
static char __based(__segname("STRINGS_TEXT")) szCantClipboard[] =
            "Cannot get enough memory to copy Status window to clipboard.";

struct KeyEntry
{
    WORD    wVirtKey;
    BOOL    bCntl;
    WORD    wMessage;
    WORD    wRequest;
} __based(__segname("STRINGS_TEXT")) KeyTable[] =
{
    /* vertical scroll control */
    {VK_HOME,   TRUE,   WM_VSCROLL, SB_TOP },
    {VK_END,    TRUE,   WM_VSCROLL, SB_BOTTOM },
    {VK_PRIOR,  FALSE,  WM_VSCROLL, SB_PAGEUP },
    {VK_NEXT,   FALSE,  WM_VSCROLL, SB_PAGEDOWN },
    {VK_UP,     FALSE,  WM_VSCROLL, SB_LINEUP },
    {VK_DOWN,   FALSE,  WM_VSCROLL, SB_LINEDOWN },

    /* horizontal scroll control */
    {VK_HOME,   FALSE,  WM_HSCROLL, SB_TOP },
    {VK_END,    FALSE,  WM_HSCROLL, SB_BOTTOM },
    {VK_PRIOR,  TRUE,   WM_HSCROLL, SB_PAGEUP },
    {VK_NEXT,   TRUE,   WM_HSCROLL, SB_PAGEDOWN },
    {VK_LEFT,   FALSE,  WM_HSCROLL, SB_LINEUP },
    {VK_RIGHT,  FALSE,  WM_HSCROLL, SB_LINEDOWN },
} ;

#define NUMKEYS (sizeof(KeyTable)/sizeof(struct KeyEntry))

/* Forward Refs
 */
static void FreeStatusLog(void);

/* Globals */
BOOL bRealTimeMsgUpdate = TRUE; /* update messages window in real-time.
                                 * Reset by callers when update can be
                                 * be deferred.
                                 */
BOOL gfCancelDisplay = FALSE;   /* cancel display if got in over our heads */

/* Clears status buffer. Frees buffer.
 */
static void FreeStatusLog(void)
{
if (hStatusBuffer)
   {
   GlobalFree(hStatusBuffer);
   hStatusBuffer = (HANDLE)0;
   }
dwStatusSize = 0L;      /* status data size             */
dwBufferSize = 0L;      /* status buffer size           */
nNumLines = 0;          /* number of lines in buffer    */
nVscrollMax = 1;
SetScrollRange(hWndStatus, SB_VERT, 0, 1, FALSE);
nVscrollPos = 0;
SetScrollPos(hWndStatus, SB_VERT, nVscrollPos, TRUE);
}

/* Update Message Window Position is called after adding
 * a number of lines to the message window without updating it.
 * The function invalidates then updates the window.
 */
void UpdateMsgWndPos(void)
{
#ifdef __BORLANDC__
#pragma warn -ccc
#endif
nVscrollPos = (short)max(0,(nNumLines-cLinesMessageWin+1));     /* set position to next to last line   */
#ifdef __BORLANDC__
#pragma warn .ccc
#endif
if (nVscrollPos < 0)
   nVscrollPos = 0;
SetScrollPos(hWndStatus, SB_VERT, nVscrollPos, TRUE);
InvalidateRect(hWndStatus, NULL, TRUE);
UpdateWindow(hWndStatus);
}

/* Set Status Top Window Position is called after adding
 * a number of lines to the message window without updating it.
 * Unlike UpdateMsgWndPos() above, this function sets the
 * status window position to the top.
 * The function invalidates then updates the window.
 */
void SetStatusTopWndPos(void)
{
nVscrollPos = 0;     /* set position to next to top line   */
SetScrollPos(hWndStatus, SB_VERT, nVscrollPos, TRUE);
InvalidateRect(hWndStatus, NULL, TRUE);
UpdateWindow(hWndStatus);
}

/* Add message line (or part of a line) to the global status buffer
 * that is the contents of the Message Window.
 * Assumes that global data is unlocked when called.
 */
void WriteStringToMsgWin(PSTR psz, BOOL bUpdate)
{
WriteBufferToMsgWin(psz, strlen(psz), bUpdate);
}

/* Add message buffer (maybe part of a line) to the global status buffer
 * that is the contents of the Message Window.
 * Assumes that global data is unlocked when called.
 */
void WriteBufferToMsgWin(LPSTR pszBuffer, int nBufferLen, BOOL bUpdate)
{
LPSTR   lpszT;                   /* pointer into buffer */
HANDLE hStatusBufferTmp;
LPSTR lpGlobalBuffer;            /* pointer into global buffer */
DWORD dwNewSize = dwStatusSize + (DWORD)nBufferLen;
int nIncrLines = 0;              /* incremental lines in buffer */
int nIncompleteExistingLine = 0; /* add -1 if incomplete existing last line */
int nIncompleteAddedLine = 0;    /* add +1 if incomplete added last line */
DWORD dwRequestedSize;           /* Size needed to hold all data. Can't
                                    practically exceeded cchBufferMax.*/

if (gfCancelDisplay)      /* if canceling display (in the middle of a lengthy operation) */
   return;               /* just discard data */

if (!nBufferLen)    /* if no data */
   return;         /* just beat it */

/* count LF's in buffer to later add to total */
for (lpszT = pszBuffer;
   lpszT != NULL && (lpszT - pszBuffer) < nBufferLen; )
   {
   /* use memchr() for speed (?) considerations */
#ifndef WIN32   
   if ((lpszT = _fmemchr(lpszT, '\n',
      (size_t)(nBufferLen - (lpszT - pszBuffer))))!=0)
#else
   if ((lpszT = memchr(lpszT, '\n',
      (size_t)(nBufferLen - (lpszT - pszBuffer))))!=0)
#endif
      {
      nIncrLines++;   /* tally line found */
      lpszT++;        /* point beyond LF for next pass */
      }
   }
if (dwNewSize > dwBufferSize)   /* if won't fit or 1st time */
   {
   /* Round up if necessary to nearest whole increment */
   dwRequestedSize = ((dwNewSize + STATUS_INCREMENT - 1) /
                        STATUS_INCREMENT) * STATUS_INCREMENT;
   if (hStatusBuffer)  /* if buffer exists, realloc */
      {
      if (dwRequestedSize <= cchBufferMax &&
         ((hStatusBufferTmp = GlobalReAlloc(hStatusBuffer,
         dwRequestedSize, GMEM_MOVEABLE))!=0))
         {
         /* successful re-allocation */
         hStatusBuffer = hStatusBufferTmp;
         dwBufferSize = dwRequestedSize;
         }
      else /* re-allocation failed, make last-ditch attempt! */
         {
         /* Here's where the message is generated when a display
            is done that fills up the list box requiring us to
            clear the window entirely to display any more.
         */
         FreeStatusLog();        /* free own buffers */
         if (uf.fAutoClearDisplay == FALSE)
            {
            if (MessageBox (hWndMain, szClearBufferMsg,
               "Note", MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL)
               gfCancelDisplay = TRUE;

            else /* no cancel */
               WriteBufferToMsgWin(pszBuffer, nBufferLen, bUpdate);
            return;
            }
         else
            {
            WriteBufferToMsgWin(pszBuffer, nBufferLen, bUpdate);
            return;
            }
         }
      }
   else    /* 1st time */
      {
      if ((hStatusBuffer = GlobalAlloc(GMEM_MOVEABLE,
         dwRequestedSize))!=0)
         {
         dwBufferSize = dwRequestedSize; /* save it      */
         }
      else    /* 1st allocation failed! */
         {
         WinAssert(hStatusBuffer);
         return;
         }
      }
   }
    /* should be easy copy of data from here */
lpGlobalBuffer = GlobalLock(hStatusBuffer);
if (lpGlobalBuffer)
   {
   /* Account for partial lines existing and being added. */
   if (dwStatusSize  &&
      lpGlobalBuffer[(int)dwStatusSize-1] != '\n')
      nIncompleteExistingLine-- ; /* subtract 1 */

      if (pszBuffer[nBufferLen-1] != '\n') /* nBufferLen guaranteed >0 */
         nIncompleteAddedLine++ ;  /* add 1 */

        /* copy data into global buffer */
      if (nBufferLen)   /* map to ANSI; if 0 don't copy; 0 means 65K  */
         {
         OemToAnsiBuff(pszBuffer, &lpGlobalBuffer[(int)dwStatusSize], nBufferLen);
         }
        /* bump no. lines accounting for incomplete lines */
      nNumLines += (short)(nIncrLines+nIncompleteExistingLine+nIncompleteAddedLine);
      dwStatusSize = dwNewSize;       /* new data size counting end null  */
      GlobalUnlock(hStatusBuffer);
      nVscrollMax = (short)max(1, nNumLines + 2 - yClient/dyChar);
      SetScrollRange(hWndStatus, SB_VERT, 0, nVscrollMax, FALSE);
      cMsgWinEntries = 0; /* re-index whenever more data is added */
      if (bUpdate)        /* if requested to update message box */
         {
#ifdef __BORLANDC__
#pragma warn -ccc
#endif
         nVscrollPos = (short)max(0,(nNumLines-cLinesMessageWin+1));     /* set position to next to last line   */
#ifdef __BORLANDC__
#pragma warn .ccc
#endif
         if (nVscrollPos < 0)
            nVscrollPos = 0;
         SetScrollPos(hWndStatus, SB_VERT, nVscrollPos, TRUE);
         InvalidateRect(hWndStatus, NULL, TRUE);
         UpdateWindow(hWndStatus);
         }
      }
   else
      {
      WinAssert(lpGlobalBuffer);
      }
EnableWindow(hCopyStatus, TRUE);
EnableMenuItem(hMenu, IDM_COPY, MF_ENABLED);
}

long WINAPI StatusProc(HWND hWnd, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
short xClient ;        /* size of client area  */
HDC     hDC;           /* device context */
PAINTSTRUCT ps;
struct KeyEntry __far *pKE;     /* pointer to key entry */
LPSTR   lpStatusBuffer;/* pointer to global msg. buffer */
int     nMenuItemCount;/* no. items in System menu before deleting separators */
BOOL    bCntl;         /* control shift pressed ? */
static short nHscrollMax;
static short nHscrollPos;
static short nMaxWidth;/* in pixels */
short nVscrollInc;
short nHscrollInc;
short i, x, y, nPaintBeg, nPaintEnd;
HMENU   hSysMenu;           /* this guy's system menu */

switch (wMessage)
   {
   case WM_CREATE:
      nMaxWidth = (short) (MAX_H_CHARS * dxChar);
      nVscrollPos = 0;
      nVscrollMax = (short)max(1,nNumLines);
      SetScrollRange(hWnd, SB_VERT, 0, nVscrollMax, FALSE);
      SetScrollPos(hWnd, SB_VERT, 0, TRUE);

      /* Remove system menu items to limit user actions on status window */
      hSysMenu = GetSystemMenu(hWnd, FALSE);
      DeleteMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
      DeleteMenu(hSysMenu, SC_MOVE, MF_BYCOMMAND);
      DeleteMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
      DeleteMenu(hSysMenu, SC_TASKLIST, MF_BYCOMMAND);

      /* walk thru menu and delete all separator bars */
      for (nMenuItemCount = GetMenuItemCount(hMenu);
         nMenuItemCount ; nMenuItemCount--)
         {
         if (GetMenuState(hSysMenu, nMenuItemCount-1, MF_BYPOSITION) & MF_SEPARATOR)
            {
            DeleteMenu(hSysMenu, nMenuItemCount-1, MF_BYPOSITION);
            }
         }
      return 0;

   case WM_SIZE:
      xClient = LOWORD(lParam);/* x size of client area */
      yClient = HIWORD(lParam);/* y size of client area */

      nVscrollMax = (short)max(1, nNumLines + 2 - yClient/dyChar);
      nVscrollPos = min(nVscrollPos, nVscrollMax);

      SetScrollRange(hWnd, SB_VERT, 0, nVscrollMax, FALSE);
      SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);

      nHscrollMax = (short)max(0, 2 + (nMaxWidth - xClient) / dxChar);
      nHscrollPos = min(nHscrollPos, nHscrollMax);

      SetScrollRange(hWnd, SB_HORZ, 0, nHscrollMax, FALSE);
      SetScrollPos(hWnd, SB_HORZ, nHscrollPos, TRUE);

      return 0;

   case WM_SYSCOMMAND:
      switch ((wParam & 0xFFF0))
         {
         case SC_RESTORE:    /* alert parent         */
            PostMessage(hWndMain, WM_COMMAND, IDM_SPLIT, 0L);
            break;
         case SC_MAXIMIZE:
            PostMessage(hWndMain, WM_COMMAND, IDM_MAX_STATUS, 0L);
            break;
         default:
            return DefWindowProc(hWnd, wMessage, wParam, lParam);
         }
      break;

   case WM_COMMAND:
      if (LOWORD(wParam) == IDM_CLEAR_STATUS)
         {
         FreeStatusLog();
         InvalidateRect(hWnd, NULL, TRUE);
         UpdateWindow(hWnd);
         }
      break;
   case WM_VSCROLL:    /* scroll bar action on list box */
      switch (LOWORD(wParam))
         {
         case SB_TOP:
            nVscrollInc = (short)-nVscrollPos;
            break;
         case SB_BOTTOM:
            nVscrollInc = (short)(nVscrollMax - nVscrollPos);
            break;
         case SB_LINEUP:
            nVscrollInc = -1;
            break;
         case SB_LINEDOWN:
            nVscrollInc = 1;
            break;
         case SB_PAGEUP:
            nVscrollInc = (short)min(-1, -yClient/dyChar);
            break;
         case SB_PAGEDOWN:
            nVscrollInc = (short)max(1, yClient/dyChar);
            break;
         case SB_THUMBPOSITION:
            nVscrollInc = (short)(LOWORD(lParam) - nVscrollPos);
            break;
         default:    /* END_SCROLL comes thru here */
            nVscrollInc = 0;
         }

      if ((nVscrollInc = (short)max(-nVscrollPos,
         min(nVscrollInc, nVscrollMax - nVscrollPos)))!=0)
         {
         nVscrollPos += nVscrollInc;
         ScrollWindow(hWnd, 0, -dyChar * nVscrollInc, NULL, NULL);
         SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);
         UpdateWindow(hWnd);
         }
      return 0;

   case WM_HSCROLL:    /* scroll bar action on list box */
        switch (LOWORD(wParam))
        {
        case SB_TOP:
            nHscrollInc = (short)-nHscrollPos;
            break;
        case SB_BOTTOM:
            nHscrollInc = (short)(nHscrollMax - nHscrollPos);
            break;
        case SB_LINEUP:
            nHscrollInc = -1;
            break;
        case SB_LINEDOWN:
            nHscrollInc = 1;
            break;
        case SB_PAGEUP:
            nHscrollInc = -8;
            break;
        case SB_PAGEDOWN:
            nHscrollInc = 8;
            break;
        case SB_THUMBPOSITION:
            nHscrollInc = (short)(LOWORD(lParam) - nHscrollPos);
            break;
        default:
            return DefWindowProc(hWnd, wMessage, wParam, lParam);
        }

        if ((nHscrollInc = (short)max(-nHscrollPos,
            min(nHscrollInc, nHscrollMax - nHscrollPos)))!= (short)0)
        {
            nHscrollPos += nHscrollInc;
            ScrollWindow(hWnd, -dxChar * nHscrollInc, 0, NULL, NULL);
            SetScrollPos(hWnd, SB_HORZ, nHscrollPos, TRUE);
        }
        return 0;

    case WM_KEYDOWN:
        bCntl = (BOOL)(GetKeyState(VK_CONTROL) < 0 ? TRUE : FALSE);
        for (i = 0, pKE = KeyTable; i < NUMKEYS; i++, pKE++)
        {
            if ((wParam == pKE->wVirtKey) && (bCntl == pKE->bCntl))
            {
                SendMessage(hWnd, pKE->wMessage, pKE->wRequest, 0L);
                break;
            }
        }
        break;
    case WM_PAINT:
        if (!hStatusBuffer)         /* if nothing to paint  */
           {
           return DefWindowProc(hWnd, wMessage, wParam, lParam);
           }
        {
        register LPSTR lpsz; /* current char */
        LPSTR lpszNextLF;    /* address of next '\n' in buffer */
        LPSTR lpszStart;     /* paint starting character */
        LPSTR lpszLineCur;   /* beginning of current line */
        HANDLE hNew;
        DWORD   cchLine;     /* length of current line */
        short  nLinesSinceLastEntry; /* lines since last entry */
        DWORD  dwSearchLen;  /* length for _fmemchr() to search */

        lpszStart = NULL;    /* paint starting character */
        lpStatusBuffer = GlobalLock(hStatusBuffer);
        WinAssert(lpStatusBuffer);  /* DEBUG */
        hDC = BeginPaint(hWnd, &ps);
        WinAssert(hDC);             /* DEBUG */
        hNew = SelectObject( hDC, hFixedFont);
        WinAssert(hNew);
        nPaintBeg = (short)max(0, nVscrollPos+ps.rcPaint.top/dyChar -1);
        nPaintEnd = (short)min(nNumLines, nVscrollPos + ps.rcPaint.bottom/dyChar);
        if (nPaintBeg >= nPaintEnd) /* if no painting to do ... */
           {
           EndPaint(hWnd, &ps);
           GlobalUnlock(hStatusBuffer);
           return 0;
           }
        if (!cMsgWinEntries)    /* re-index whenever more data is added */
           {
           /* Round up to make lines_per_entry encompass all
            * possible lines in buffer.
            */
           nLinesPerEntry = (short)((nNumLines+MAX_INDEX_ENTRIES-1) / MAX_INDEX_ENTRIES);
           if (!nLinesPerEntry)    /* if zero */
              nLinesPerEntry++;   /* set to 1 as minimum */

           /* Count lines from beginning of buffer to:
            * 1) mark beginning of paint sequence (lpszStart) and
            * 2) periodically save buffer index in MsgWinIndex[] table.
            */
           for (lpsz = lpStatusBuffer, i = 0, nLinesSinceLastEntry = 0;
              (DWORD)(lpsz - lpStatusBuffer) < dwStatusSize ; i++)
              {
              /* We are at the 1st character position in the line  */
              if (i == nPaintBeg) /* Starting point for paint ? */
                 lpszStart = lpsz;   /* If so, mark starting point */

              /* Entry time ? */
              if (!nLinesSinceLastEntry++ &&
                 cMsgWinEntries < MAX_INDEX_ENTRIES)
                 {
                 rgidwMsgWin[cMsgWinEntries] =
                    (DWORD)(lpsz - lpStatusBuffer); /* save index */
                 cMsgWinEntries++;
                 }

              if (nLinesSinceLastEntry >= nLinesPerEntry)
                 nLinesSinceLastEntry = 0;

              /* Use _fmemchr() to find next LF.
               * It's probably optimized for searches.
               */
              dwSearchLen = dwStatusSize -
                 (DWORD)(lpsz - lpStatusBuffer);
#ifndef WIN32
              if (((lpszNextLF = _fmemchr(lpsz, '\n', (size_t)dwSearchLen)))!=0)
#else
              if (((lpszNextLF = memchr(lpsz, '\n', (size_t)dwSearchLen)))!=0)
#endif
                 lpsz = ++lpszNextLF;    /* use next char as beg of line */
              else /* use lpsz with incremented value */
                 lpsz += (int)dwSearchLen;

              } /* bottom of still-counting-lines loop */

           WinAssert(lpszStart);
           lpsz = lpszStart;       /* restore starting point */
           WinAssert((DWORD)lpsz >= (DWORD)lpStatusBuffer &&
              (DWORD)lpsz < (DWORD)&lpStatusBuffer[(int)dwStatusSize]);

           }   /* bottom of need-to-build-index block  */
        else    /* index is still valid */
           {
           short nIndexEntry;

           /* Find index of line number which is equal to or just
            * below the starting line to paint. Work backwards
            * thru the table. Here, "i" is the line no. corresponding
            * to the current table index.
            */
           for (nIndexEntry = (short)(cMsgWinEntries - 1),
              i = (short)(nIndexEntry * nLinesPerEntry);
              nIndexEntry >= 0 &&
              nPaintBeg < i ;
              nIndexEntry--, i -= nLinesPerEntry )
              {
              ;
              }

           WinAssert(nIndexEntry >= 0);
           WinAssert(i <= nPaintBeg);

           /* OK, we've got a head start on the search.
            * Start checking characters from the position found
            * in the index table.
            */
           for (lpsz = &lpStatusBuffer[(int)rgidwMsgWin[nIndexEntry]];
              i < nPaintBeg &&
              (DWORD)(lpsz - lpStatusBuffer) < dwStatusSize;
              ++i)
              {
              /* Find length of current line.  Use _fmemchr() to
               * find next LF.
               */
              dwSearchLen = dwStatusSize - (DWORD)(lpsz - lpStatusBuffer);
#ifndef WIN32
              if ((lpszNextLF = _fmemchr(lpsz, '\n', (size_t)dwSearchLen)) != NULL)
#else
              if ((lpszNextLF = memchr(lpsz, '\n', (size_t)dwSearchLen)) != NULL)
#endif
                 lpsz = ++lpszNextLF; /* point to next char. past '\n' */

              else /* If search fails, pretend LF exists, go past it. */
                 lpsz += (int)dwSearchLen;

              }
           } /* bottom of index-is-still-valid block. */

            /* At this point we've got the buffer address, lpsz, for the 1st
             * line at which we begin painting, nPaintBeg.
             */
        for (i = nPaintBeg;
             (i < nPaintEnd) &&
             ((DWORD)lpsz  >= (DWORD)lpStatusBuffer) &&
             ((DWORD)(lpsz  - lpStatusBuffer) < dwStatusSize) ;
             ++i)
             {
             lpszLineCur = lpsz;
             /* Find length of current line. Use _fmemchr() to find next LF.
              */
             dwSearchLen = dwStatusSize - (DWORD)(lpsz - lpStatusBuffer);
#ifndef WIN32
             if ((lpszNextLF = _fmemchr(lpsz, '\n', (size_t)dwSearchLen)) == NULL)
#else
             if ((lpszNextLF = memchr(lpsz, '\n', (size_t)dwSearchLen)) == NULL)
#endif
                {
                /* If search fails, pretend we found LF, we won't
                 * display it anyway.
                 */
                lpszNextLF = lpsz + (int)dwSearchLen;
                }
             WinAssert((DWORD)lpszNextLF >= (DWORD)lpszLineCur); /* should be non-negative   */
             WinAssert((DWORD)lpszNextLF >= (DWORD)lpStatusBuffer && /* DEBUG */
                (DWORD)lpszNextLF <= (DWORD)&lpStatusBuffer[(int)dwStatusSize]);

             x = (short)(dxChar * (1 - nHscrollPos));
             y = (short)(dyChar * (1 - nVscrollPos + i));
             cchLine = (DWORD)(lpszNextLF - lpszLineCur);/* calc length*/
             /* don't display '\r'   */
             if (cchLine && lpszLineCur[(int)cchLine-1] == '\r')
                cchLine--;

             /* may be displaying long lines if binary file */
             if (cchLine > cchTextOutMax)
                cchLine = cchTextOutMax;

             TabbedTextOut(hDC, x, y, lpszLineCur, (int)cchLine, 0, NULL, 0);
             lpsz = ++lpszNextLF; /* point to char. past '\n' */
            }
            EndPaint(hWnd, &ps);
            GlobalUnlock(hStatusBuffer);
            return 0;
        }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        FreeStatusLog();
        break;
    default:
        return DefWindowProc(hWnd, wMessage, wParam, lParam);
    }
    return 0L;
}

/* Map CR to CRLF is called by the printf and fprintf clones below
 * to guarantee that lines added to the Status window will be
 * properly terminated DOS-style in case they are copied
 * to the Windows clipboard.
 * This converts the buffer in-place, provided it won't
 * grow beyond the original buffer allocation.
 */

static void MapCRtoCRLF(PSTR pszOrigBuffer);
static void MapCRtoCRLF(PSTR pszOrigBuffer)
{
PSTR pC, pDest;
UINT cBufLen = 0;  // no. chars in buffer, including final null
UINT cLFs = 0;    // no. LF's in string. \n is linefeed

for (pC = pszOrigBuffer; *pC; pC++)
   {
   cBufLen++;
   if (*pC == '\n')   // found a linefeed
      cLFs++;
   }
cBufLen++;   /* buffer length includes final null */
pC = &pszOrigBuffer[cBufLen-1]; /* point to old end's null char */
if (cBufLen + cLFs > STDIO_BUF_SIZE) /* room to add CR's ? */
   return;             /* if not, don't bother */

/* copy data back-to-front, effectively inserting CR before LF */
pDest = &pszOrigBuffer[cBufLen+cLFs-1]; /* point to new end */
for  (; cBufLen; pC--, cBufLen--)
   {
   *pDest-- = *pC ;    /* copy data byte */
   if ((*pC == '\n') && ((*(pC-1)) != '\r'))    /* was that a linefeed? */
      *pDest-- = '\r'; /* if so, insert CR */
   }
}


/* Printf buffers the current output and counts the number of lines
 * within it.  It makes sure there is enough space in the global
 * buffer, then copies the buffered data to the global buffer.
 * It then triggers a repaint of the status buffer.
 */
int __far __cdecl printf(const char *format, ...)
{
va_list argptr;
HANDLE hMemory;
PSTR pszBuffer;
int len;

va_start(argptr, format);
hMemory = LocalAlloc(LMEM_MOVEABLE, STDIO_BUF_SIZE);
WinAssert(hMemory);
if (!hMemory)
   {
   return 0;
   }
pszBuffer = (PSTR)LocalLock(hMemory);
WinAssert(pszBuffer);
len = vsprintf(pszBuffer, format, argptr);
va_end(argptr);
WinAssert(strlen(pszBuffer) < STDIO_BUF_SIZE);  /* raise STDIO_BUF_SIZE ?   */
MapCRtoCRLF(pszBuffer);   /*  map CR's to CRLF's */
WriteStringToMsgWin(pszBuffer, bRealTimeMsgUpdate);
LocalUnlock(hMemory);
LocalFree(hMemory);
/* MW: changed to return the actual number of bytes being printed */
return len;
}

#define FPRINTF_BUF_SIZE 16384
#define MSG_WIN_BUF_SIZE 8192

int win_fprintf(FILE *file, unsigned int size, char *buf)
{
unsigned int len, count, p = 0;
char * buffer;
char * pbuf;

if ((file != stderr) && (file != stdout))
   {
   len = write(fileno(file),(char *)(buf),size);
   return len;
   }
if (size < MSG_WIN_BUF_SIZE)
   count = size;
else
   count = MSG_WIN_BUF_SIZE;
pbuf = buf;
/* Note the additional buffer size here (8K which is excessive,
   but makes lots of room available for extra line feeds etc)
*/
buffer = (char *)malloc(FPRINTF_BUF_SIZE);
WinAssert(buffer);
do
{
memcpy(buffer, pbuf, count);
if ((p + count) >= size)
   {
   count = size - p;
   p = size;
   }
else
   {
   p += count;
   }
pbuf += count;
buffer[count] = '\0';
fprintf(file, "%s", buffer);
} while (p < size);
free(buffer);
return size;
}

#ifdef __BORLANDC__
#pragma argsused
#endif

/* fprintf clone for code in unzip.c, etc. */
int __far __cdecl fprintf(FILE *file, const char *format, ...)
{
va_list argptr;
HANDLE hMemory;
PSTR pszBuffer;
unsigned int len;

va_start(argptr, format);
hMemory = LocalAlloc(LMEM_MOVEABLE, (FPRINTF_BUF_SIZE + STDIO_BUF_SIZE));
WinAssert(hMemory);
if (!hMemory)
   {
   return FALSE;
   }
pszBuffer = (PSTR)LocalLock(hMemory);
WinAssert(pszBuffer);
len = vsprintf(pszBuffer, format, argptr);
va_end(argptr);
WinAssert(strlen(pszBuffer) < (FPRINTF_BUF_SIZE + STDIO_BUF_SIZE));  /* raise STDIO_BUF_SIZE ?   */
MapCRtoCRLF(pszBuffer);   /*   map CR's to CRLF's */
WriteStringToMsgWin(pszBuffer, bRealTimeMsgUpdate);
LocalUnlock(hMemory);
LocalFree(hMemory);
/* MW: changed to return the actual number of bytes being printed */
return len;
}

void __far __cdecl perror(const char *parm1)
{
printf(parm1);
}

/* Copy contents of status window to clipboard.  Called on receipt of IDM_COPY message.
 */
void CopyStatusToClipboard(HWND hWnd)
{
HANDLE hGlobalMemory; /* memory handles */
LPSTR  lpGlobalMemory, lpStatusBuffer ; /* memory pointers */
DWORD  dwTextSize = dwStatusSize;

if (!StatusInWindow())
   return;

if (dwTextSize > 0x10000L - 2)
   dwTextSize = 0x10000L - 2;

if ((hGlobalMemory = GlobalAlloc(GHND, dwTextSize+1L))== 0)
   {
   MessageBox (hWnd, szCantClipboard,
      "Error", MB_ICONEXCLAMATION | MB_OK);
   }
else /* Got it! */
   {
   lpGlobalMemory = GlobalLock(hGlobalMemory);
   WinAssert(lpGlobalMemory);   /* DEBUG */
   lpStatusBuffer = GlobalLock(hStatusBuffer);
   WinAssert(lpStatusBuffer);   /* DEBUG */
   memcpy((void __far *)lpGlobalMemory,
      (void __far *)lpStatusBuffer, (size_t)dwTextSize);
   lpGlobalMemory[(int)dwTextSize] = '\0'; /* CF_TEXT requires null */
   GlobalUnlock(hStatusBuffer);   /* unlock status buffer */
   GlobalUnlock(hGlobalMemory);
   OpenClipboard(hWnd);
   EmptyClipboard();
   SetClipboardData(CF_TEXT, hGlobalMemory);
   CloseClipboard();
   }
}

/* Returns TRUE if Status window has anything in it; i.e. is non-clear.
 */
BOOL StatusInWindow(void)
{
return((BOOL)dwStatusSize);
}

