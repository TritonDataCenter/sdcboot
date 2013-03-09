#ifndef MENU_H_
#define MENU_H_

/* Constants for menu entries. */
#define NONESELECTED      0
#define BEGINOPTIMIZATION 1
#define CHANGEDRIVE       3
#define CHANGEMETHOD      4
#define SPECIFYFILEORDER  6
#define SHOWMAP           7
#define DISPLAYCOPYRIGHT  9
#define EXITDEFRAG       10

/* Menu coordinates. */
#define MENU_X 3
#define MENU_Y 2

#define MENU_CAPTION_X1  5
#define MENU_CAPTION_X2 14
#define MENU_CAPTION_Y   1

#define AMOFITEMS   10
#define LONGESTITEM 33

int MainMenu(void);

#endif
