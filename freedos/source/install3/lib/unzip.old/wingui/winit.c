#include <stdio.h>
#include "wingui\wizunzip.h"

long extern WINAPI WizUnzipWndProc(HWND, WORD, WPARAM, LPARAM);

/****************************************************************************

    FUNCTION: WizUnzipInit(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        Sets up a structures to register the window class.  Structure includes
        such information as what function will process messages, what cursor
        and icon to use, etc.

****************************************************************************/
BOOL WizUnzipInit(HANDLE hInstance)
{
    WNDCLASS wndclass;

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (long (WINAPI*)(HWND,unsigned ,WPARAM,LPARAM)) WizUnzipWndProc;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, "WizUnzip");
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(BG_SYS_COLOR+1); /* set background color */
    wndclass.lpszMenuName = (LPSTR) "WizUnzip";
    wndclass.lpszClassName = (LPSTR) szAppName;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;

    if ( !RegisterClass(&wndclass) )
       {
        return FALSE;
       }

    /* define status class */
    wndclass.lpszClassName = (LPSTR) szStatusClass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (long (WINAPI*)(HWND,unsigned,unsigned,LONG))StatusProc;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, "UNZIPPED");
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;

    if ( !RegisterClass(&wndclass) )
       {
        return FALSE;
       }
   wndclass.style         = (UINT) NULL;
   wndclass.lpfnWndProc   = (WNDPROC) ButtonBarWndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = NULL;
   wndclass.hCursor       = NULL;
   wndclass.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = "ButtonBar";
   if( !RegisterClass(&wndclass))
      {
		return( FALSE );
      }

    return TRUE;
}

