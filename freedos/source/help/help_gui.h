/*  HTML Stuff

    Copyright (c) Express Software 1998-2002
    All Rights Reserved.

    Created by: Joseph Cosentino.
*/

#ifndef HELP_GUI_H_INCLUDED
#define HELP_GUI_H_INCLUDED

#include "conio.h"

#define X (1)
#define Y (1)
#define W (80)
#define H (ScreenHeight)
#define N (H-4)
#define TX (X+10)
#define TY (Y+H-2)
#define TW (W-12)
#define LEN (W-X-3)
#define BARX (X+W-1)
#define BARY (Y+2)
#define BARLEN (N-2)

/* User Interface macros */

#ifdef HELP_GUI_C

unsigned char TEXT_COLOR;
unsigned char BOLD_COLOR;
unsigned char ITALIC_COLOR;
unsigned char BORDER_BOX_COLOR;
unsigned char BORDER_TEXT_COLOR;
unsigned char LINK_COLOR;
unsigned char LINK_HIGHLIGHTED_COLOR;

#else

extern unsigned char TEXT_COLOR;
extern unsigned char BOLD_COLOR;
extern unsigned char ITALIC_COLOR;
extern unsigned char BORDER_BOX_COLOR;
extern unsigned char BORDER_TEXT_COLOR;
extern unsigned char LINK_COLOR;
extern unsigned char LINK_HIGHLIGHTED_COLOR;

/* COLOR MONITOR SCHEME */
#define C_TEXT_COLOR             (White    + BakBlack)
#define C_BOLD_COLOR             (BrWhite  + BakBlack)
#define C_ITALIC_COLOR           (Magenta  + BakBlack)
#define C_BORDER_COLOR           (BrBlue   + BakBlack)
#define C_BORDER_TEXT_COLOR      (Yellow   + BakBlack)
#define C_LINK_COLOR             (Yellow   + BakBlack)
#define C_LINK_HIGHLIGHTED_COLOR (Black    + BakWhite)

/* MONO MONITOR SCHEME */
#define M_TEXT_COLOR             (White    + BakBlack)
#define M_BOLD_COLOR             (BrWhite  + BakBlack)
#define M_ITALIC_COLOR           (BrWhite  + BakBlack)
#define M_BORDER_COLOR           (White    + BakBlack)
#define M_BORDER_TEXT_COLOR      (BrBlue   + BakBlack)
#define M_LINK_COLOR             (BrBlue   + BakBlack)
#define M_LINK_HIGHLIGHTED_COLOR (Black    + BakWhite)

/* FANCY COLOR SCHEME 1 */
#define F1_TEXT_COLOR             (Black    + BakWhite)
#define F1_BOLD_COLOR             (BrWhite  + BakWhite)
#define F1_ITALIC_COLOR           (Magenta  + BakWhite)
#define F1_BORDER_COLOR           (Yellow   + BakWhite)
#define F1_BORDER_TEXT_COLOR      (Yellow   + BakWhite)
#define F1_LINK_COLOR             (BrBlue   + BakWhite)
#define F1_LINK_HIGHLIGHTED_COLOR (BrWhite    + BakBlack)

/* FANCY COLOR SCHEME 2 */
#define F2_TEXT_COLOR             (Black    + BakCyan)
#define F2_BOLD_COLOR             (BrWhite  + BakCyan)
#define F2_ITALIC_COLOR           (Magenta  + BakCyan)
#define F2_BORDER_COLOR           (Black   + BakCyan)
#define F2_BORDER_TEXT_COLOR      (Black   + BakCyan)
#define F2_LINK_COLOR             (Yellow   + BakCyan)
#define F2_LINK_HIGHLIGHTED_COLOR (Yellow    + BakBlue)


#endif

void statusbar (const char *msg);
void statuswithesc (const char *msg);
void show_error (const char *msg);
void message_box (int borderattr, int msgattr, int btnattir, const char *msg);
int searchbox (char *str);
void drawmenu (void);
void writeButton(int attr, int x, int y, const char *label);

#define HCAT_MAX_TOPMENU     14
#define HCAT_MAX_BOTTOMMENU  10

#define LEN_MENU_EXIT min(HCAT_MAX_TOPMENU, strlen(hcatMenuExit))
#define LEN_MENU_HELP min(HCAT_MAX_TOPMENU, strlen(hcatMenuHelp))
#define LEN_MENU_BACK min(HCAT_MAX_BOTTOMMENU, strlen(hcatMenuBack))
#define LEN_MENU_FORWARD min(HCAT_MAX_BOTTOMMENU, strlen(hcatMenuForward))
#define LEN_MENU_CONTENTS min(HCAT_MAX_BOTTOMMENU, strlen(hcatMenuContents))
#define LEN_MENU_SEARCH min(HCAT_MAX_BOTTOMMENU, strlen(hcatMenuSearch))

#endif
