#ifndef DIALOG_H_
#define DIALOG_H_

#include <conio.h>

/* Dialog colors. */
#define DIALOGBACKCOLOR   CYAN
#define DIALOGFORCOLOR    BLACK

#define DIALOGWARNINGFOR  BLUE
#define DIALOGWARNINGBACK GREEN

#define DIALOGERRORFOR    BLACK
#define DIALOGERRORBACK   RED

/* Button colors. */
#define BUTTONBACKCOLOR       LIGHTGRAY
#define BUTTONFORCOLOR        BLACK
#define BUTTONHIGHLIGHTCOLOR  WHITE

/* Main menu colors. */
#define MENUBACKCOLOR     WHITE
#define MENUFORCOLOR      BLACK
#define MENUHIGHLIGHTFOR  WHITE
#define MENUHIGHLIGHTBACK BLACK
#define MENUSHORTCUTCOLOR RED

/* Macro's to be able to put a dialog in the middle of the screen. */
#define HideDialog PasteScreen(0, 0)
#define GetDialogX (xlen) ((MAX_X / 2) - (xlen / 2))
#define GetDialogY (ylen) ((MAX_Y / 2) - (ylen / 2))

/* Macro to facilitate drawing normal static labels. */
#define StaticLabel(x, y, text) \
        DrawText(x, y, text, DIALOGFORCOLOR, DIALOGBACKCOLOR)


#endif
