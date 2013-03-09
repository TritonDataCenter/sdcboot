/* sizewndw.c module of WizUnZip.
 * Author: Robert A. Heath
 * I, Robert Heath, place this source code module in the public domain.
 *
 * Modifications M. White 1995
 */

#include <stdio.h>
#include "wingui\wizunzip.h"

#define MIN_LISTBOX_LINES 2

/* Call this when the window size changes or needs to change. */
void SizeWindow(HWND hWnd, BOOL bOKtoMovehWnd)
{
    WORD wMinClientWidth;       /* minimum client width     */
    int nListBoxHeight;         /* height of listbox in pix         */
    WORD wVariableHeight;       /* no. variable pixels on client    */
    WORD wVariableLines;            /* no. variable lines on client window */
    WORD wMessageBoxHeight;     /* message box height in pixels     */
    WORD wClientWidth, wClientHeight;       /* size of client area  */
    RECT rectT;
    int nCxBorder;
    int nCyBorder;
    int nCxVscroll; /* vertical scroll width */
    int nCyHscroll; /* vertical scroll width */
    int nCyCaption; /* caption height       */

    BtnMult = (float)BTNWIDTH; /* Reset multiplier to original setting */
    BtnSeparator = 1;   /* Set distance between buttons back to 1 */
    Width = (int)(BtnMult*dxChar);

    WinAssert(hWndMain);
    GetClientRect(hWndMain, &rectT);
    if (((Width * NumOfBtns) + NumOfBtns) > rectT.right)
       {
       while (BtnMult > MIN_BTN_WIDTH)
          {
          BtnMult = (float)(BtnMult - 0.1);
          Width = (int)(BtnMult*dxChar);
          if (((Width * NumOfBtns) + NumOfBtns) < rectT.right)
             {
             MoveButtons();
             break;
             }
          }
       }
    else
       {
       MoveButtons();
       }

    if (((Width * NumOfBtns) + NumOfBtns) > rectT.right)
       {
       BtnSeparator = 0;
       MoveButtons();
       }

    WinAssert(hWnd);
    GetClientRect(hWnd, &rectT);
    wClientWidth = (WORD)(rectT.right-rectT.left+1); /* x size of client area */
    wClientHeight = (WORD)(rectT.bottom-rectT.top+1); /* y size of client area */
    if (wWindowSelection == IDM_MAX_STATUS)
       {
       /* position the status window to fill entire client window   */
       WinAssert(hWndStatus);
       MoveWindow(hWndStatus, 0, (3 * dyChar),
            wClientWidth, wClientHeight-(3*dyChar), TRUE);
       cLinesMessageWin = (WORD)(wClientHeight / dyChar);
       return;
       }

    nCxBorder = GetSystemMetrics(SM_CXBORDER);
    nCyBorder = GetSystemMetrics(SM_CYBORDER);
    nCxVscroll = GetSystemMetrics(SM_CXVSCROLL);
    nCyHscroll = GetSystemMetrics(SM_CYHSCROLL);
    nCyCaption = GetSystemMetrics(SM_CYCAPTION);

    if (wClientHeight < (WORD)(11*dyChar))
        wClientHeight = (WORD)(11*dyChar);

    /* List Box gets roughly 1/2 of lines left over on client
     * window after subtracting fixed overhead for borders,
     * horizontal scroll bar,
     * button margin spacing, header, and trailer lines.
     * unless the status window is minimized
     */
    wVariableHeight =  (WORD)(wClientHeight - (2 * nCyBorder) - (6 * dyChar));
    if (wWindowSelection != IDM_MAX_LISTBOX)
        wVariableHeight -= (WORD)(nCyHscroll + nCyCaption + (2*nCyBorder) + dyChar);
    wVariableLines = (WORD)(wVariableHeight / dyChar);
    cListBoxLines =  (WORD)((wWindowSelection == IDM_MAX_LISTBOX) ?
               wVariableLines : wVariableLines / 2);

    if (cListBoxLines < MIN_LISTBOX_LINES)
        cListBoxLines = MIN_LISTBOX_LINES;

    cLinesMessageWin = (WORD)(wVariableLines - cListBoxLines); /* vis. msg. wnd lines */

    wMinClientWidth = (WORD)
       ((!uf.fFormatLong ? MIN_SHORT_FORMAT_CHARS : MIN_LONG_FORMAT_CHARS) * dxChar +
                      nCxVscroll + 2 * nCxBorder);

    /* if we moved the hWnd from WM_SIZE, we'd probably get into
     * a nasty, tight loop since this generates a WM_SIZE.
     */
    if (bOKtoMovehWnd && (wClientWidth < wMinClientWidth))
       {
       wClientWidth = wMinClientWidth;
       GetWindowRect(hWnd, &rectT);
       WinAssert(hWnd);
       MoveWindow(hWnd, rectT.left, rectT.top,
          wClientWidth + (2*GetSystemMetrics(SM_CXFRAME)),
          wClientHeight, TRUE);
       }

    WinAssert(hWndButtonBar);
    MoveWindow(hWndButtonBar,
         0, 1,
         wClientWidth,28,
         TRUE);
    /*
     * Position the "display" listbox.
     */
    nListBoxHeight = (cListBoxLines * dyChar) + (2 * nCyBorder) +
      (int)(1.5 * dyChar);

       WinAssert(hWndList);
       MoveWindow(hWndList,
            0, dyChar+ (2 * dyChar), /* drop display listbox two lines down */
            wClientWidth,nListBoxHeight + (2*dyChar),
            TRUE);

    /* Position the status (Message) window.
     * The Message windows is positioned relative to the bottom
     * of the client area rather than relative to the top of the client.
     */
    wMessageBoxHeight = (WORD)(wVariableHeight - nListBoxHeight +
                        2 * nCyBorder +
                        nCyHscroll + nCyCaption);

    WinAssert(hWndStatus);
    MoveWindow(hWndStatus,
            0, wClientHeight - wMessageBoxHeight,
            wClientWidth, wMessageBoxHeight,
            TRUE);
}
