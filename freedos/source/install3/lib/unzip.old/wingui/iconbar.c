#define STRICT
#include <windows.h>
#include <string.h>
#include <commdlg.h>
#include <stdio.h>
#include <dos.h>
#include <time.h>
#include "wingui\wizunzip.h"

#define DKGRAY_PEN      RGB(128, 128, 128)
#define IB_SPACE        -1

typedef struct BUTTON_STRUCT
{
int    idBtn;
LPCSTR ButtonHelp;
LPCSTR hBMP;
HWND   hWndBtn;
} BUTTON, *LPBUTTON;

static WNDPROC pfnOldProc;
static BOOL first = FALSE;

/* Forward declarations */
void ButtonBar(HWND, LPBUTTON);
void CreateButtonBar(HWND hWndParent);
VOID SubClassButton (HWND hwnd, WNDPROC SubClassBtnProc);
LRESULT CALLBACK SubClassBtnProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static HWND hHelpBtnWnd = NULL;
HWND      hWndButtonBarOwner;
HWND      hWndButtonBar;
int       ButtonBarHeight;
int       Width, Height;
int BtnSeparator = 1;
float BtnMult = (float)BTNWIDTH;

#define NumOfBtns 21

BUTTON    Buttons[NumOfBtns + 1];
LPBUTTON  lpIB;

/*
 *  function:  SubClassBtnProc
 *
 *  input parameters:  normal window procedure parameters.
 *  global variables:
 *   hWndButtonBar  - parent window of the control.
 */

LRESULT CALLBACK SubClassBtnProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
RECT rc, rw;
POINT p;

  switch (message) {
    case WM_MOUSEMOVE: {
      int left;

      if (!uf.fShowBubbleHelp)
         return 0;

      p.x = LOWORD(lParam); /* Get mouse coordinates */
      p.y = HIWORD(lParam);

      for (lpIB = Buttons; lpIB->idBtn != 0; lpIB++)
         {
         if (lpIB->hWndBtn == hwnd)
            {
            if  (ChildWindowFromPoint(hwnd, p) != hwnd)
                {
                if (hHelpBtnWnd != NULL)
                   DestroyWindow(hHelpBtnWnd);
                hHelpBtnWnd = NULL;
                if (GetCapture != NULL)
                   ReleaseCapture();
                break; /* Mouse has moved off the button */
                }

            if (GetCapture() != hwnd)
               {
               SetCapture(hwnd);
               /* Convert left, right corners of button to hWndMain
                  coordinates
                */
               WinAssert(hwnd);
               GetClientRect(hwnd, &rc);
               MapWindowPoints(hwnd, hWndMain, (POINT FAR *)&rc, 2);
               WinAssert(hWndMain);
               GetClientRect(hWndMain, &rw);

               if ((rc.left + ((lstrlen(lpIB->ButtonHelp)+1) * dxChar)) < rw.right)
                  left = rc.left;
               else
                  left = rc.right - ((lstrlen(lpIB->ButtonHelp)+1) * dxChar);
               hHelpBtnWnd = CreateWindow("EDIT", NULL,
                  WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | ES_READONLY|WS_BORDER,
                  left, rc.bottom,
                  (lstrlen(lpIB->ButtonHelp)+1) * dxChar, (int)(1.4 * dyChar),
                  hWndButtonBarOwner,
                  (HMENU) NULL,
                  (HANDLE) hInst,
                  NULL);
               SendMessage(hHelpBtnWnd, WM_SETFONT, (WPARAM)hFixedFont, FALSE);
               SendMessage(hHelpBtnWnd, WM_SETTEXT, 0, (LPARAM)lpIB->ButtonHelp);
               break;
              }
            }
         }
      return 0;
      }
    /*
     * For messages that are not handled explicitly, pass them on
     * to the original window procedure.
     */
    default:
      if (hHelpBtnWnd != NULL)
         {
         DestroyWindow(hHelpBtnWnd);
         hHelpBtnWnd = NULL;
         if (GetCapture() != NULL)
            ReleaseCapture();
         }
      return (CallWindowProc ((pfnOldProc), hwnd, message, wParam, lParam));
    } /* end switch */

}

/*
 *  function:  SubClassButton
 *
 *  input parameters:
 *     hwnd            - window handle to be subclassed,
 *     SubClassBtnProc - the new window procedure.
 *
 *  Set in a new window procedure for this window.  Store the old window
 *  procedure in the first field of the extrabytes structure.  This routine
 *  is specific to this program in the use of this particular extrabyte
 *  structure.  Note that the pointer to the user bytes needs to be freed
 *  later (in WM_DESTROY).
 */

VOID SubClassButton (HWND hwnd, WNDPROC SubClassBtnProc)
{
WinAssert(hwnd);
if (!first)
   {
   pfnOldProc = (WNDPROC) GetWindowLong (hwnd, GWL_WNDPROC);
   first = TRUE;
   }
SetWindowLong (hwnd, GWL_WNDPROC,  (LONG) SubClassBtnProc);
}

/* WARNING If you add a button, you must change the #define above for
   NumOfBtns. Any more buttons, and you might have trouble with VGA, or
   640x480 display modes.
*/
void CreateButtonBar(HWND hWndParent)
{
int i=0;

Buttons[i].idBtn     = IDM_OPEN;
Buttons[i].ButtonHelp = "Open Archive";
Buttons[i++].hBMP = "OPEN_BUTTON";

Buttons[i].idBtn     = IDM_EXTRACT;
Buttons[i].ButtonHelp = "Extract File(s) From Archive";
Buttons[i++].hBMP = "EXTRACT_BUTTON";

Buttons[i].idBtn     = IDM_DISPLAY;
Buttons[i].ButtonHelp = "Display File From Archive";
Buttons[i++].hBMP = "DISPLAY_BUTTON";

Buttons[i].idBtn     = IDM_TEST;
Buttons[i].ButtonHelp = "Test Archive";
Buttons[i++].hBMP = "TEST_BUTTON";

Buttons[i].idBtn     = IDM_SHOW_COMMENT;
Buttons[i].ButtonHelp = "Show Comment";
Buttons[i++].hBMP = "COMMENT_BUTTON";

Buttons[i].idBtn     = IDM_COPY_ARCHIVE;
Buttons[i].ButtonHelp = "Copy Archive";
Buttons[i++].hBMP = "COPY_BUTTON";

Buttons[i].idBtn     = IDM_MOVE_ARCHIVE;
Buttons[i].ButtonHelp = "Move Archive";
Buttons[i++].hBMP = "MOVE_BUTTON";

Buttons[i].idBtn     = IDM_RENAME_ARCHIVE;
Buttons[i].ButtonHelp = "Rename Archive";
Buttons[i++].hBMP = "RENAME_BUTTON";

Buttons[i].idBtn     = IDM_DELETE_ARCHIVE;
Buttons[i].ButtonHelp = "Delete Archive";
Buttons[i++].hBMP = "DELETE_BUTTON";

Buttons[i].idBtn     = IDM_MAKE_DIR;
Buttons[i].ButtonHelp = "Make Directory";
Buttons[i++].hBMP = "MAKEDIR_BUTTON";

Buttons[i].idBtn     = IDM_SELECT_ALL;
Buttons[i].ButtonHelp = "Select All Files";
Buttons[i++].hBMP = "SELECTALL_BUTTON";

Buttons[i].idBtn     = IDM_DESELECT_ALL;
Buttons[i].ButtonHelp = "De-Select All Files";
Buttons[i++].hBMP = "DESELECTALL_BUTTON";

Buttons[i].idBtn     = IDM_SELECT_BY_PATTERN;
Buttons[i].ButtonHelp = "Select Files By Pattern";
Buttons[i++].hBMP = "SELECTPATTERN_BUTTON";

Buttons[i].idBtn     = IDM_CLEAR_STATUS;
Buttons[i].ButtonHelp = "Clear Status/Display Window";
Buttons[i++].hBMP = "CLEARSTATUS_BUTTON";

Buttons[i].idBtn     = IDM_COPY;
Buttons[i].ButtonHelp = "Copy Status/Display Window To Clipboard";
Buttons[i++].hBMP = "COPYSTATUS_BUTTON";

Buttons[i].idBtn     = IDM_CHDIR;
Buttons[i].ButtonHelp = "Unzip To Directory";
Buttons[i++].hBMP = "UNZIPTODIR_BUTTON";

Buttons[i].idBtn     = IDM_MAX_LISTBOX;
Buttons[i].ButtonHelp = "Maximize Archive Listbox";
Buttons[i++].hBMP = "LIST_BUTTON";

Buttons[i].idBtn     = IDM_SPLIT;
Buttons[i].ButtonHelp = "Split Display Windows";
Buttons[i++].hBMP = "SPLIT_BUTTON";

Buttons[i].idBtn     = IDM_MAX_STATUS;
Buttons[i].ButtonHelp = "Maximize Status/Display Window";
Buttons[i++].hBMP = "STATUS_BUTTON";

Buttons[i].idBtn     = IDM_EXIT;
Buttons[i].ButtonHelp = "Exit WizUnZip";
Buttons[i++].hBMP = "EXIT_BUTTON";

Buttons[i].idBtn     = IDM_HELP;
Buttons[i].ButtonHelp = "WizUnZip Help";
Buttons[i++].hBMP = "HELP_BUTTON";

Buttons[i].idBtn     = 0;
Buttons[i].ButtonHelp = NULL;
Buttons[i].hWndBtn   = NULL;

ButtonBar(hWndParent, Buttons);
return;
}

void ButtonBar(HWND hWndParent, LPBUTTON lpButton)
{
int          i, x;
RECT         rect;

hWndButtonBarOwner  = hWndParent;
Height    = (int)(1.75*dyChar);
ButtonBarHeight = 2 + Height;
WinAssert(hWndParent);
GetClientRect(hWndParent, &rect);
hWndButtonBar = CreateWindow("ButtonBar", NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER,
        0, 0, rect.right - rect.left, ButtonBarHeight,
        hWndButtonBarOwner, (HMENU) NULL, (HINSTANCE) hInst, NULL);
SetWindowPos(hWndButtonBar, (HWND) 1, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
WinAssert(hWndButtonBar);
ShowWindow(hWndButtonBar, SW_SHOW);
BtnMult = (float)BTNWIDTH; /* Reset multiplier to original setting */
BtnSeparator = 1;   /* Set distance between buttons back to 1 */
Width = (int)(BtnMult*dxChar);

WinAssert(hWndParent);
GetClientRect(hWndParent, &rect);
if (((Width * NumOfBtns) + NumOfBtns) > rect.right)
   {
   while (BtnMult > MIN_BTN_WIDTH)
      {
      BtnMult = (float)(BtnMult - 0.1);
      Width = (int)(BtnMult*dxChar);
      if (((Width * NumOfBtns) + NumOfBtns) < rect.right)
         continue;
      }
   }

/* Last ditch effor to get all the buttons on the screen */
if (((Width * NumOfBtns) + NumOfBtns) > rect.right)
   {
   BtnSeparator = 0;
   }

x = BtnSeparator;

for (lpIB = lpButton; lpIB->idBtn != 0; lpIB++)
    {
    if (lpIB->idBtn == -1)
       continue;
    (lpIB->hWndBtn) = CreateWindow("BUTTON", "",
                      WS_VISIBLE | BS_OWNERDRAW | WS_CHILD,
                      x, 0,
                      Width, Height,
                      hWndButtonBar,
                      (HMENU) lpIB->idBtn, (HINSTANCE) hInst, NULL );
    SubClassButton (lpIB->hWndBtn, SubClassBtnProc);
    x += Width + BtnSeparator;
    }
i = 0;
hOpen          = Buttons[i++].hWndBtn ;
hExtract       = Buttons[i++].hWndBtn ;
hDisplay       = Buttons[i++].hWndBtn ;
hTest          = Buttons[i++].hWndBtn ;
hShowComment   = Buttons[i++].hWndBtn ;
hCopyArchive   = Buttons[i++].hWndBtn ;
hMoveArchive   = Buttons[i++].hWndBtn ;
hRenameArchive = Buttons[i++].hWndBtn ;
hDeleteArchive = Buttons[i++].hWndBtn ;
hMakeDir       = Buttons[i++].hWndBtn ;
hSelectAll     = Buttons[i++].hWndBtn ;
hDeselectAll   = Buttons[i++].hWndBtn ;
hSelectPattern = Buttons[i++].hWndBtn ;
hClearStatus   = Buttons[i++].hWndBtn ;
hCopyStatus    = Buttons[i++].hWndBtn ;
hUnzipToDir    = Buttons[i++].hWndBtn ;
hListBoxButton = Buttons[i++].hWndBtn ;
hSplitButton   = Buttons[i++].hWndBtn ;
hStatusButton  = Buttons[i++].hWndBtn ;
hExit          = Buttons[i++].hWndBtn ;
hHelp          = Buttons[i].hWndBtn ;

EnableWindow(hCopyStatus, FALSE);

return;
}

void MoveButtons(void)
{
int x = BtnSeparator;

for (lpIB = Buttons; lpIB->idBtn != 0; lpIB++)
    {
    WinAssert(lpIB->hWndBtn);
    MoveWindow(lpIB->hWndBtn, x, 0, Width, Height, TRUE);
    x += Width + BtnSeparator;
    }
}

LRESULT WINAPI ButtonBarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
HPEN             hPen, hOldPen;
HBRUSH           hBrush;
RECT             rect;
PAINTSTRUCT      ps;
LPDRAWITEMSTRUCT lpdis;
HDC              hdcMem;
HBITMAP          hBitmap;
long             flag;
int              i;

switch(message)
   {
   case WM_COMMAND:
        SendMessage(hWndButtonBarOwner, message, wParam, lParam);
        return(FALSE);

   case WM_DRAWITEM:
        lpdis = (LPDRAWITEMSTRUCT) lParam;
        lpIB = Buttons;
        for (i=0;;i++)
           {
           if (lpIB[i].idBtn == 0)
               return (FALSE); /* Not one of our controls */
           if (lpIB[i].idBtn == (signed int)lpdis->CtlID)
               break;          /* Found the right control */
           }
        lpdis->rcItem.right = Width;
        lpdis->rcItem.bottom = Height;

        lpdis->rcItem.right--;
        lpdis->rcItem.bottom--;

        hBrush = CreateSolidBrush(RGB(192, 192, 192));
        FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
        DeleteObject(hBrush);
        hOldPen = SelectObject(lpdis->hDC, GetStockObject(BLACK_PEN));
        MoveToEx(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top, NULL);
        LineTo(lpdis->hDC,lpdis->rcItem.right,lpdis->rcItem.top);
        LineTo(lpdis->hDC,lpdis->rcItem.right,lpdis->rcItem.bottom);
        LineTo(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.bottom);
        LineTo(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top);
        SelectObject(lpdis->hDC, hOldPen);
        lpdis->rcItem.left++;
        lpdis->rcItem.right--;
        lpdis->rcItem.top++;
        lpdis->rcItem.bottom--;
        if (!IsWindowEnabled(lpIB[i].hWndBtn))
           {
           flag = SRCINVERT;  /* "Gray" out button */
           }
        else
           {
           flag = SRCCOPY;
           }
        if (lpdis->itemState & ODS_SELECTED)
           {
           /* Button has been depressed - make it look depressed */
           hPen = CreatePen(PS_SOLID, 1, DKGRAY_PEN);
           hOldPen = SelectObject(lpdis->hDC, hPen);
           MoveToEx(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.bottom,NULL);
           LineTo(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top);
           LineTo(lpdis->hDC, lpdis->rcItem.right, lpdis->rcItem.top);
           SelectObject(lpdis->hDC, hOldPen);
           DeleteObject(hPen);
           hdcMem = CreateCompatibleDC(lpdis->hDC);
           hBitmap = LoadBitmap(hInst, lpIB[i].hBMP);
           hOldPen = SelectObject(hdcMem, hBitmap);
           StretchBlt(lpdis->hDC,   /* device to be drawn */
              lpdis->rcItem.left+2, /* x upper left destination */
              lpdis->rcItem.top+2,  /* y upper left destination */
              lpdis->rcItem.right - lpdis->rcItem.left -2, /* width */
              lpdis->rcItem.bottom - lpdis->rcItem.top - 2,/* height */
              hdcMem, /* source bitmap */
              0,      /* x upper left source */
              0,      /* y upper left source */
              32,     /* source bitmap width */
              32,     /* source bitmap height */
              flag);
           SelectObject(hdcMem, hOldPen);
           DeleteDC(hdcMem);
           DeleteObject(hBitmap);
           }
        else
           {
           /* Draw button */
           hOldPen = SelectObject(lpdis->hDC, GetStockObject(WHITE_PEN));
           MoveToEx(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.bottom,NULL);
           LineTo(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top);
           LineTo(lpdis->hDC,lpdis->rcItem.right,lpdis->rcItem.top);
           SelectObject(lpdis->hDC, hOldPen);
           hPen = CreatePen(PS_SOLID, 1, DKGRAY_PEN);
           hOldPen = SelectObject(lpdis->hDC, hPen);
           MoveToEx(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.bottom,NULL);
           LineTo(lpdis->hDC,lpdis->rcItem.right,lpdis->rcItem.bottom);
           LineTo(lpdis->hDC,lpdis->rcItem.right,lpdis->rcItem.top);
           SelectObject(lpdis->hDC, hOldPen);
           DeleteObject(hPen);
           hdcMem = CreateCompatibleDC(lpdis->hDC);
           hBitmap = LoadBitmap(hInst, lpIB[i].hBMP);
           hOldPen = SelectObject(hdcMem, hBitmap);
           StretchBlt(lpdis->hDC,
              lpdis->rcItem.left+2,
              lpdis->rcItem.top+2,
              lpdis->rcItem.right - lpdis->rcItem.left -2,
              lpdis->rcItem.bottom - lpdis->rcItem.top - 2,
              hdcMem,
              0,
              0,
              32,
              32,
              flag); /* This flag makes it look either enabled or greyed */
           SelectObject(hdcMem, hOldPen);
           DeleteDC(hdcMem);
           DeleteObject(hBitmap);
           }
        return TRUE;
   case WM_PAINT:
        BeginPaint(hWnd, &ps);
        WinAssert(hWnd);
        GetClientRect(hWnd, &rect);
        hOldPen = SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
        MoveToEx(ps.hdc, rect.left, rect.top, NULL);
        LineTo(ps.hdc, rect.right+1, rect.top);
        SelectObject(ps.hdc, hOldPen);
        hPen = CreatePen(PS_SOLID, 1, DKGRAY_PEN);
        hOldPen = SelectObject(ps.hdc, hPen);
        MoveToEx(ps.hdc, rect.left, rect.bottom-2, NULL);
        LineTo(ps.hdc, rect.right+1, rect.bottom-2);
        SelectObject(ps.hdc, hOldPen);
        DeleteObject(hPen);
        hPen = CreatePen( PS_SOLID, 1, BLACK_PEN);
        hOldPen = SelectObject(ps.hdc, hPen);
        MoveToEx(ps.hdc, rect.left, rect.bottom-1, NULL);
        LineTo(ps.hdc, rect.right+1, rect.bottom-1);
        SelectObject(ps.hdc, hOldPen);
        DeleteObject(hPen);
        EndPaint(hWnd, &ps);
        return (FALSE);
   default:
      return(DefWindowProc(hWnd, message, wParam, lParam));
   }
}

