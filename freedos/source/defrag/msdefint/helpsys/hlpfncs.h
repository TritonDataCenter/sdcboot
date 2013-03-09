#ifndef HELP_FUNCS_H_
#define HELP_FUNCS_H_

struct LinkPosition {
  int line;            /* Line on which the link is located    */
  int nr;              /* The number of the link on that line.
                          Starts from 0.                       */
};


/* Draw a help line. */
void DrawHelpLine(int line, int x, int y, int xlen);

/* Returns -1 if not a link, the help index otherwise. */
int  CheckHelpClick(int cx, int cy, int cxlen, int cylen,
                    int msx, int msy, int top);

/* The user pressed tab somewhere. */
int CheckHelpTab(int top, int ylen);

/* Returns -1 if not a link, the help index otherwise */
int CheckHelpEnter(int top, int ylen);

/* Select the current help page. */
void SelectHelpPage(int index);

/* Did we pass the end of help. */
int PastEndOfHelp(int top, int ylen);

/* Return the last help top: what the top should be when the user presses
   the page down key when there is no longer a full screen of help lines. */
int GetLastHelpTop(int ylen);

#endif
