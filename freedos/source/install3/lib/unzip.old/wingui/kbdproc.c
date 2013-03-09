/* Keyboard procedure used to sub-class all windows which can be
 * tab stops.
 */

#include <stdio.h>
#include "wingui\wizunzip.h"

/* Keyboard procedure
 * This function allows the user to tab and back-tab among the 
 * between the listbox and message window.
 * It traps VK_TAB messages and sets the
 * focus on the next or previous window as required.
 * Skip any disabled windows.
 */
long WINAPI KbdProc(HWND hWnd, WORD wMessage, WPARAM wParam, LPARAM lParam)
{
#ifndef WIN32
int nID = GetWindowWord(hWnd, GWW_ID); /* child window ID no. */
#else
int nID = GetWindowLong(hWnd, GWL_ID); /* child window ID no. */
#endif
int nTabStopTableIndex = nID - TABSTOP_ID_BASE;
int nNextTabStopTableIndex = nTabStopTableIndex;

if (wMessage == WM_KEYDOWN)
   {
   if (wParam == VK_TAB)
      {
      int nRelIndex = /* forward or backward ? */
          (int)(GetKeyState(VK_SHIFT) < 0 ? -1 : 1);

      do {
         nNextTabStopTableIndex += nRelIndex;
         if (nNextTabStopTableIndex < 0)
            nNextTabStopTableIndex = TABSTOP_TABLE_ENTRIES-1;
         else if (nNextTabStopTableIndex >= TABSTOP_TABLE_ENTRIES)
            nNextTabStopTableIndex = 0;

      } while (!IsWindowEnabled(TabStopTable[nNextTabStopTableIndex].hWnd));

   SetFocus(TabStopTable[nNextTabStopTableIndex].hWnd);
   }
else if (wParam == VK_F1)
   {
   /* If Shift-F1, turn help mode on and set help cursor */
   if (GetKeyState(VK_SHIFT)<0)
      {
      uf.fHelp = TRUE;
      SetCursor(hHelpCursor);
      }
   else
      {
      /* If F1 without shift, then call up help main index topic */
      WinHelp(hWndMain, szHelpFileName, HELP_INDEX, 0L);
      }
   }
else if ((wParam == VK_ESCAPE) && uf.fHelp)
   {
   /* Escape during help mode: turn help mode off */
   uf.fHelp = FALSE;
#ifndef WIN32
   SetCursor((HCURSOR)GetClassWord(hWndMain,GCW_HCURSOR));
#else
   SetCursor((HCURSOR)GetClassLong(hWndMain,GCL_HCURSOR));
#endif
   }
}
return CallWindowProc(TabStopTable[nTabStopTableIndex].lpfnOldFunc,
                      hWnd, wMessage, wParam, lParam);
}
