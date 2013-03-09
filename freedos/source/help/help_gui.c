/*  FreeDOS HTML Help Viewer

    GUI - Graphical User Interface

    Copyright (c) Express Software 1998-2003
    See doc\htmlhelp\copying for licensing terms
    Created by: Joseph Cosentino.

*/

/* I N C L U D E S *********************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <conio.h>
#define HELP_GUI_C
#include "help_gui.h"
#include "conio.h"
#include "catdefs.h"

/* F U N C T I O N S *******************************************************/


void
statusbar (const char *msg)
{
  clear_window (TEXT_COLOR, X + 2, TY, LEN, 1);
  if (msg != NULL)
    write_string (TEXT_COLOR, X + 2, TY, msg);
}

/* Adds "press escape to abort" message after msg. */
void
statuswithesc (const char *msg)
{
   statusbar(msg);
	write_string (TEXT_COLOR, X + strlen(msg) + 3, TY,
				                   hcatStatusEscape);
}


void
show_error (const char *msg)
{
  message_box (Red + BakBlack, Yellow + BakBlack, Black + BakWhite, msg);
}


void
message_box (int borderattr, int msgattr, int btnattr, const char *msg)
{
  int x, y = H / 2 - 1, w, h = 4, pressed;
  struct event ev;
  char *buf;

  w = strlen (msg) + 6;
  if (w % 2 == 1)
    w++;

  if (w > W - 2)
    w = W - 2;

  x = (W - w) / 2 + 1;

  buf = malloc (4 * W * 2);
  if (buf)
    save_window (x, y, w, h, buf);

  border_window (borderattr, x, y, w, h, Border22f);
  write_string (msgattr, x + 3, y + 1, msg);
  writeButton (btnattr, x + w / 2 - 4, y + 2, hcatButtonOK);
  pressed = 0;
  while (1)
    {
      get_event (&ev, EV_KEY | EV_MOUSE);

      if ((ev.ev_type & EV_KEY) && (ev.key == 27 || ev.key == 13))
	break;

      if (ev.ev_type & EV_MOUSE)
	if (ev.x >= x + w / 2 - 4 && ev.x < x + w / 2 + 4 && ev.y == y + 2)
	  {
	    if (ev.left == 1)
	      pressed = 1;

	    if (ev.left == 0 && pressed == 1)
	      break;

	  }
	else
	  pressed = 0;

    }

  if (buf)
    {
      load_window (x, y, w, h, buf);
      free (buf);
    }
}



/* Collected together and defined out some of the gui functions,
   which are currently not used.
   Not including them saves on binary size */
#ifdef UNUSED_GUI

int
get_keys (unsigned short *keys, int num_keys)
{
  struct event ev;
  int i, x, y, w, h, q;
  char *buf, *tmp;

  x = 1;
  w = W;
  y = H - 2;
  h = 3;

  tmp = malloc (512 * 3);
  if (tmp == 0)
    return 0;

  buf = malloc (H * W * 2);
  if (buf != 0)
    save_window (x, y, w, h, buf);

  border_window (Yellow + BakWhite, x, y, w, h, Border22f);
  write_string (Yellow + BakWhite, x + 53, y + h - 1,
		" Esc, BkSp, \\ - quote ");

  q = 0;
  while (1)
    {
      clear_window (BrWhite + BakBlack, x + 2, y + 1, w - 4, 1);
      for (i = 0; i < num_keys; i++)
	{
	  sprintf (tmp, "%04X", keys[i]);
	  write_string (BrWhite + BakBlack, x + 2 + 5 * i, y + 1, tmp);
	}

      move_cursor (x + 2 + 5 * i, y + 1);
      get_event (&ev, EV_KEY);
      if (q == 0 && ev.key == 27)
	break;
      else if (q == 0 && ev.key == 8)
	{
	  if (num_keys > 0)
	    num_keys--;

	}
      else if (q == 0 && ev.key == '\\')
	{
	  if (num_keys < 15)
	    q = 1;

	}
      else if (num_keys < 15)
	{
	  keys[num_keys++] = ev.scan;
	  q = 0;
	}

    }

  for (i = num_keys; i < 15; i++)
    keys[i] = 0;

  if (buf)
    {
      load_window (x, y, w, h, buf);
      free (buf);
    }

  free (tmp);
  return num_keys;
}

/***************************************************************************/

int
write_int (int attr, int x, int y, int w, unsigned long xx)
{
  char tmp[30];

  sprintf (tmp, "%*ld", w, xx);
  write_string (attr, x, y, tmp);
  return 0;

}


void
edit_int_field (struct event *ev, int ev_mask, int attr, int x, int y,
		int field_len, unsigned long *n, unsigned long limit)
{
  char tmp[257];		/* RP */
  int i;

  while (1)
    {
      if (field_len > 0)
	{
	  sprintf (tmp, "%*lu", field_len, *n);
	  move_cursor (x + field_len - 1, y);
	}
      else
	{
	  sprintf (tmp, "%-*lu", -field_len, *n);
	  for (i = 0; tmp[i] != ' ' && tmp[i] != 0; i++);
	  move_cursor (x + i, y);
	}

      write_string (attr, x, y, tmp);
      get_event (ev, ev_mask | EV_KEY);
      if (!(ev->ev_type & EV_KEY))
	break;

      if (ev->key == 8 || ev->scan == 0x5300 || ev->scan == 0x53E0)	// Backspace || Del.
	{
	  (*n) /= 10;
	}
      else if (ev->key == '+')
	{
	  if ((*n) < limit)
	    (*n)++;

	}
      else if (ev->key == '-')
	{
	  if ((*n) > 0)
	    (*n)--;

	}
      else if (ev->key >= '0' && ev->key <= '9')
	{
	  i = ev->key - '0';
	  if (limit >= i && (*n) <= (limit - i) / 10)
	    (*n) = (*n) * 10 + i;

	}
      else
	break;

    }
}
int
enter_string (int x, int y, char *prompt, int maxlen, char *str, char *help)
{
  struct event ev;
  int i, w, x2, w2;
  char *buf;			/* RP */

  w2 = W - 7 - x - strlen (prompt);
  if (w2 > maxlen)
    w2 = maxlen;

  w = strlen (prompt) + 4 + w2;	/* changed 5 to 4 - RP */
  x2 = x + strlen (prompt) + 3;

  buf = malloc (w * 3 * 2);	/* RP */
  if (buf == NULL)
    return 0;

  save_window (x, y, w, 3, buf);
  border_window (Black + BakWhite, x, y, w, 3, Border22f);
  write_string (Black + BakWhite, x + 2, y + 1, prompt);

  i = 0;
  str[0] = 0;

  while (1)
    {
      edit_str_field (&ev, 0, BrWhite + BakBlack, x2, y + 1, w2, str, &i);

      if (ev.key == 27)
	{
	  i = 0;
	  break;
	}

      if (ev.key == 13)
	{
	  i = 1;
	  break;
	}

      if (ev.scan == 0x3B00 && help != 0)	// F1 - Help.
	{
	  html_view (help);
	}

    }

  load_window (x, y, w, 3, buf);
  free (buf);			/* RP */
  return i;

}


#endif

void
edit_str_field (struct event *ev, int ev_mask, int attr, int x, int y,
		int maxlen, char *str, int *pos)
{
  int i;

  while (1)
    {
      i = strlen (str);
      if (*pos > i)
	*pos = i;

      if (*pos == maxlen - 1)
	(*pos)--;

      clear_window (attr, x + i, y, maxlen - i - 1, 1);
      write_string (attr, x, y, str);
      move_cursor (x + *pos, y);

      get_event (ev, ev_mask | EV_KEY);
      if (!(ev->ev_type & EV_KEY))
	break;

      if (ev->scan == 0x47E0 || ev->scan == 0x4700)	/* Home. */
	{
	  *pos = 0;
	}
      else if (ev->scan == 0x4FE0 || ev->scan == 0x4F00)	/* End. */
	{
	  *pos = strlen (str);
	}
      else if (*pos > 0 && (ev->scan == 0x4BE0 || ev->scan == 0x4B00))	/* Left. */
	{
	  (*pos)--;
	}
      else if (*pos < strlen (str) && (ev->scan == 0x4DE0 || ev->scan == 0x4D00))	/* Right. */
	{
	  (*pos)++;
	}
      else if (ev->key == 8 && *pos > 0)	/* Backspace. */
	{
	  for (i = *pos; i < maxlen && str[i] != 0; i++)
	    str[i - 1] = str[i];

	  str[i - 1] = 0;
	  (*pos)--;
	}
      else if (ev->scan == 0x5300 || ev->scan == 0x53E0)	/* Del. */
	{
	  for (i = *pos; i < maxlen && str[i] != 0; i++)
	    str[i] = str[i + 1];

	  str[i] = 0;
	}
      else if (ev->key >= ' ')
	{
	  if (strlen (str) < maxlen - 1)
	    {
	      for (i = maxlen - 1; i > *pos; i--)
		str[i] = str[i - 1];

	      str[i] = ev->key;
	      (*pos)++;
	    }

	}
      else
	break;

    }
}


/* DEFINITIONS FOR TAB ORDER IN SEARCH DIALOG BOX */
#define SEARCH_FIRSTFOCUS SEARCH_TEXTBOX
#define SEARCH_LASTFOCUS SEARCH_HELP_BUTTON

#define SEARCH_TEXTBOX 0
#define SEARCH_CASE_CHECKBOX 1
#define SEARCH_WHOLEWORD_CHECKBOX 2
#define SEARCH_FULLSEARCH_RADIO 3
#define SEARCH_OK_BUTTON 4
#define SEARCH_CANCEL_BUTTON 5
#define SEARCH_HELP_BUTTON 6

/* DEFINITIONS FOR MAXIMUM LENGTH OF TEXT STRINGS */
#define HCAT_MAX_PROMPT 20
#define HCAT_MAX_STRA 35
#define HCAT_MAX_STRB 28

int
searchbox (char *str)
{
  int x = W / 4, y = H / 4;
  int w = W / 2;
  int xEditBox, wEditBox;

  struct event ev;
  int i;
  int height = 12;
  char *buf;			/* RP */
  int focus = SEARCH_TEXTBOX;

  /* Options */
  /* static ones stored between uses of search dialog, these will be
     overwritten if user presses OK */
  static unsigned char stored_casesensitive = 0;
  static unsigned char stored_fullsearch = 1;
  static unsigned char stored_wholewordonly = 0;
  int casesensitive;
  int fullsearch;
  int wholewordonly;

  /* text translation strings: make sure they do not exceed
     maximum lengths */
  char prompt[HCAT_MAX_PROMPT+1];
  char title[HCAT_MAX_STRA+1];
  char casestr[HCAT_MAX_STRB+1];
  char wholestr[HCAT_MAX_STRB+1];
  char fullstr[HCAT_MAX_STRB+1];
  char pagestr[HCAT_MAX_STRB+1];
  char hlptitle[HCAT_MAX_STRA+1];
  char hlpline1str[HCAT_MAX_STRA+1];
  char hlpline2str[HCAT_MAX_STRA+1];
  char hlpline3str[HCAT_MAX_STRA+1];
  char hlpline4str[HCAT_MAX_STRA+1];
  char hlpline5str[HCAT_MAX_STRA+1];
  char hlpline6str[HCAT_MAX_STRA+1];
  char hlpline7str[HCAT_MAX_STRA+1];
  strncpy(prompt, hcatSearchboxPrompt, HCAT_MAX_PROMPT);
  prompt[HCAT_MAX_PROMPT] = '\0';
  strncpy(title, hcatSearchboxTitle, HCAT_MAX_STRA);
  title[HCAT_MAX_STRA] = '\0';
  strncpy(casestr, hcatSearchboxCase, HCAT_MAX_STRB);
  casestr[HCAT_MAX_STRB] = '\0';
  strncpy(wholestr, hcatSearchboxWhole, HCAT_MAX_STRB);
  wholestr[HCAT_MAX_STRB] = '\0';
  strncpy(fullstr, hcatSearchboxFull, HCAT_MAX_STRB);
  fullstr[HCAT_MAX_STRB] = '\0';
  strncpy(pagestr, hcatSearchboxPage, HCAT_MAX_STRB);
  pagestr[HCAT_MAX_STRB] = '\0';
  strncpy(hlptitle, hcatSearchhlpTitle, HCAT_MAX_STRA);
  hlptitle[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline1str, hcatSearchhlpLine1, HCAT_MAX_STRA);
  hlpline1str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline2str, hcatSearchhlpLine2, HCAT_MAX_STRA);
  hlpline2str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline3str, hcatSearchhlpLine3, HCAT_MAX_STRA);
  hlpline3str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline4str, hcatSearchhlpLine4, HCAT_MAX_STRA);
  hlpline4str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline5str, hcatSearchhlpLine5, HCAT_MAX_STRA);
  hlpline5str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline6str, hcatSearchhlpLine6, HCAT_MAX_STRA);
  hlpline6str[HCAT_MAX_STRA] = '\0';
  strncpy(hlpline7str, hcatSearchhlpLine7, HCAT_MAX_STRA);
  hlpline7str[HCAT_MAX_STRA] = '\0';

  xEditBox = x + strlen (prompt) + 3;
  wEditBox = w - strlen (prompt) - 4;

  buf = malloc (w * height * 2);	/* RP */
  if (buf == NULL)
    return 0;

  save_window (x, y, w, height, buf);
  border_window (Black + BakWhite, x, y, w, height, Border22f);
  /* title */
  write_string (Black + BakWhite, x + 1, y, " ");
  write_string (Black + BakWhite, x + 2, y, title);
  write_string (Black + BakWhite, x+strlen(title)+2, y, " ");
  /* prompt */
  write_string (Black + BakWhite, x + 2, y + 2, prompt);
  /* case sensitivty Check Box */
  write_string (Black + BakWhite, x + 2, y + 4, "[ ]");
  write_string (Black + BakWhite, x + 6, y + 4, casestr);
  /* whole word only Check Box */
  write_string (Black + BakWhite, x + 2, y + 5, "[ ]");
  write_string (Black + BakWhite, x + 6, y + 5, wholestr);
  /* full search Radio Button */
  write_string (Black + BakWhite, x + 6, y + 7, "( )");
  write_string (Black + BakWhite, x + 10, y + 7, fullstr);
  /* on this page Radio Button */
  write_string (Black + BakWhite, x + 6, y + 8, "( )");
  write_string (Black + BakWhite, x + 10, y + 8, pagestr);

  writeButton (White + BakBlue, x + 6, y + 10, hcatButtonOK);
  writeButton (White + BakBlue, x + 16, y + 10, hcatButtonCancel);
  writeButton (White + BakBlue, x + 26, y + 10, hcatButtonHelp);
  _setcursortype (_NORMALCURSOR);

  i = 0;
  str[0] = 0;

  /* Ensure controls match static variables (options are stored
     between searches for the user convenience, by using static
     variables for fullsearch, casesensitive, and wholewordonly RP */
  fullsearch = stored_fullsearch;
  casesensitive = stored_casesensitive;
  wholewordonly = stored_wholewordonly;
  if (fullsearch)
    write_string (Black + BakWhite, x + 7, y + 7, "*");
  else
    write_string (Black + BakWhite, x + 7, y + 8, "*");

  if (casesensitive)
    write_string (Black + BakWhite, x + 3, y + 4, "X");

  if (wholewordonly)
    write_string (Black + BakWhite, x + 3, y + 5, "X");

  /* User interface loop */
  while (1)
    {
      if (focus == SEARCH_TEXTBOX)
	edit_str_field (&ev, EV_MOUSE, BrWhite + BakBlack,
			xEditBox, y + 2, wEditBox, str, &i);
      else if (focus == SEARCH_CASE_CHECKBOX)
	{
	  move_cursor (x + 3, y + 4);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	  if (ev.ev_type == EV_KEY && ev.key == ' ')
	    {
	      ev.ev_type = EV_MOUSE;
	      ev.left = 1;
	      ev.x = x + 3;
	      ev.y = y + 4;
	    }
	}
      else if (focus == SEARCH_WHOLEWORD_CHECKBOX)
	{
	  move_cursor (x + 3, y + 5);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	  if (ev.ev_type == EV_KEY && ev.key == ' ')
	    {
	      ev.ev_type = EV_MOUSE;
	      ev.left = 1;
	      ev.x = x + 3;
	      ev.y = y + 5;
	    }
	}
      else if (focus == SEARCH_OK_BUTTON)
	{
	  move_cursor (x + 9, y + 10);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	}
      else if (focus == SEARCH_CANCEL_BUTTON)
	{
	  move_cursor (x + 17, y + 10);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	  if (ev.key == 13)
	    ev.key = 27;	/* Enter key while on cancel button cancels */
	}
      else if (focus == SEARCH_HELP_BUTTON)
	{
	  move_cursor (x + 28, y + 10);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	  if (ev.key == 13)
	    {
	      ev.key = 0;
	      ev.scan = 0x3B00;
	    }
	}
      else if (focus == SEARCH_FULLSEARCH_RADIO)
	{
	  move_cursor (x + 5, y + 7);
	  get_event (&ev, EV_MOUSE | EV_KEY);
	  if (ev.ev_type == EV_KEY && ev.key == ' ')
	    {
	      fullsearch = !fullsearch;
	      if (fullsearch)
		{
		  write_string (Black + BakWhite, x + 7, y + 7, "*");
		  write_string (Black + BakWhite, x + 7, y + 8, " ");
		}
	      else
		{
		  write_string (Black + BakWhite, x + 7, y + 7, " ");
		  write_string (Black + BakWhite, x + 7, y + 8, "*");
		}
	    }
	}

      if (ev.ev_type == EV_MOUSE && ev.left == 1)
	{
	  if (ev.x >= x + 2 &&
         ev.x <= x + strlen(casestr)+5 &&
         ev.y == y + 4)
	    {
	      focus = SEARCH_CASE_CHECKBOX;
	      casesensitive = !casesensitive;
	      if (casesensitive)
		write_string (Black + BakWhite, x + 3, y + 4, "X");
	      else
		write_string (Black + BakWhite, x + 3, y + 4, " ");
	    }
	  else if (ev.x >= x + 2 &&
              ev.x <= x + strlen(wholestr)+5 &&
              ev.y == y + 5)
	    {
	      focus = SEARCH_WHOLEWORD_CHECKBOX;
	      wholewordonly = !wholewordonly;
	      if (wholewordonly)
		write_string (Black + BakWhite, x + 3, y + 5, "X");
	      else
		write_string (Black + BakWhite, x + 3, y + 5, " ");
	    }
	  else if (ev.x >= xEditBox && ev.x <= x + w - 3 && ev.y == y + 2)
	    focus = SEARCH_TEXTBOX;
	  else if (ev.x >= x + 6 && ev.x <= x + 13 && ev.y == y + 10)
	    ev.key = 13;
	  else if (ev.x >= x + 16 && ev.x <= x + 23 && ev.y == y + 10)
	    ev.key = 27;
	  else if (ev.x >= x + 26 && ev.x <= x + 33 && ev.y == y + 10)
	    {
	      ev.key = 0;
	      ev.scan = 0x3B00;
	    }
	  else if (ev.x >= x + 6 &&
              ev.x <= x + strlen(fullstr)+9
              && ev.y == y + 7)
	    {
	      focus = SEARCH_FULLSEARCH_RADIO;
	      fullsearch = 1;
	      write_string (Black + BakWhite, x + 7, y + 7, "*");
	      write_string (Black + BakWhite, x + 7, y + 8, " ");
	    }
	  else if (ev.x >= x + 6 &&
              ev.x <= x + strlen(pagestr)+9 &&
              ev.y == y + 8)
	    {
	      focus = SEARCH_FULLSEARCH_RADIO;
	      fullsearch = 0;
	      write_string (Black + BakWhite, x + 7, y + 7, " ");
	      write_string (Black + BakWhite, x + 7, y + 8, "*");
	    }

	}

      if (ev.key == 27)
	{
	  i = 0;
	  break;
	}

      if (ev.key == 13)		/* Enter or click on OK */
	{
	  /* store options for next time user opens search dialog box */
	  stored_fullsearch = fullsearch;
	  stored_casesensitive = casesensitive;
	  stored_wholewordonly = wholewordonly;
	  /* return result */
	  i = 1 + casesensitive + 4 * fullsearch + 8 * wholewordonly;
	  break;
	}

      if (ev.key == 9)		/* Tab */
	{
	  if ((focus++) == SEARCH_LASTFOCUS)
	    focus = SEARCH_FIRSTFOCUS;
	}

      if (ev.scan == 0xf00)	/* Shift+Tab */
	{
	  if ((focus--) == SEARCH_FIRSTFOCUS)
	    focus = SEARCH_LASTFOCUS;
	}

      if (ev.scan == 0x3B00)	// F1 for help screen */
	{
	  char *buf2 = malloc (w * height * 2);	/* RP */
	  if (buf == NULL)
	    return 0;

	  save_window (x + 1, y + 1, w, height, buf2);
	  border_window (Black + BakWhite, x + 1, y + 1, w, height,
			 Border22f);
     /* search help title */
	  write_string (Black + BakWhite, x + 2, y + 1, " ");
     write_string (Black + BakWhite, x + 3, y + 1, hlptitle);
     write_string (Black + BakWhite, x + strlen(hlptitle)+3,
                                     y + 1, " ");

     write_string (Black + BakWhite, x + 3, y + 3, hlpline1str);
     write_string (Black + BakWhite, x + 3, y + 4, hlpline2str);
     write_string (Black + BakWhite, x + 3, y + 5, hlpline3str);
     write_string (Black + BakWhite, x + 3, y + 6, hlpline4str);
     write_string (Black + BakWhite, x + 3, y + 7, hlpline5str);
     write_string (Black + BakWhite, x + 3, y + 8, hlpline6str);
     write_string (Black + BakWhite, x + 3, y + 9, hlpline7str);

	  writeButton (White + BakBlue, x + 17, y + 11, hcatButtonOK);
	  move_cursor (x + 20, y + 11);
	  do
	    {
	      get_event (&ev, EV_KEY | EV_MOUSE);
	    }
	  while (!(ev.ev_type == EV_KEY) && !(ev.ev_type == EV_MOUSE &&
					      ev.left == 1 &&
					      ev.x >= x + 17 &&
					      ev.x <= x + 24 &&
					      ev.y == y + 11));
	  load_window (x + 1, y + 1, w, height, buf2);
	  free (buf2);
	}
    }

  _setcursortype (_NOCURSOR);
  load_window (x, y, w, height, buf);
  free (buf);			/* RP */
  return i;

}


void drawmenu (void)
{
  char topstr[HCAT_MAX_TOPMENU+1] = "";
  char bottomstr[HCAT_MAX_BOTTOMMENU+1] = "";

   border_window (BORDER_BOX_COLOR, X, Y, W, H, Border22f);
   border_window (BORDER_BOX_COLOR, X, Y + H - 3, W, 3, Border22if);
   write_char (BORDER_BOX_COLOR, BARX, BARY - 1, BarUpArrow);
   write_char (BORDER_BOX_COLOR, BARX, BARY + BARLEN, BarDownArrow);

   /* Top of screen menu: */
   /* Exit */
   strncpy(topstr, hcatMenuExit, HCAT_MAX_TOPMENU);
   topstr[HCAT_MAX_TOPMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 2, Y, " Alt+X ");
   write_string (BORDER_TEXT_COLOR, X + 2+7, Y, topstr);
   write_string (BORDER_TEXT_COLOR, X + 2+7 + strlen(topstr), Y, " ");
   /* Help on Help */
   strncpy(topstr, hcatMenuHelp, HCAT_MAX_TOPMENU);
   topstr[HCAT_MAX_TOPMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 60, Y, " F1 ");
   write_string (BORDER_TEXT_COLOR, X + 60+4, Y, topstr);
   write_string (BORDER_TEXT_COLOR, X + 60+4+ strlen(topstr), Y, " ");

   /* Bottom of screen menu: */
   /* Back */
   strncpy(bottomstr, hcatMenuBack, HCAT_MAX_BOTTOMMENU);
   bottomstr[HCAT_MAX_BOTTOMMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 3, H, " \x1B ");
   write_string (BORDER_TEXT_COLOR, X + 3+3, H, bottomstr);
   write_string (BORDER_TEXT_COLOR, X + 3+3+strlen(bottomstr), H, " ");
   /* Forward */
   strncpy(bottomstr, hcatMenuForward, HCAT_MAX_BOTTOMMENU);
   bottomstr[HCAT_MAX_BOTTOMMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 19, H, " Alt+F ");
   write_string (BORDER_TEXT_COLOR, X + 19+7, H, bottomstr);
   write_string (BORDER_TEXT_COLOR, X + 19+7+strlen(bottomstr), H, " ");
   /* Contents */
   strncpy(bottomstr, hcatMenuContents, HCAT_MAX_BOTTOMMENU);
   bottomstr[HCAT_MAX_BOTTOMMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 40, H, " Alt+C ");
   write_string (BORDER_TEXT_COLOR, X + 40+7, H, bottomstr);
   write_string (BORDER_TEXT_COLOR, X + 40+7+strlen(bottomstr), H, " ");
   /* Search */
   strncpy(bottomstr, hcatMenuSearch, HCAT_MAX_BOTTOMMENU);
   bottomstr[HCAT_MAX_BOTTOMMENU] = '\0';
   write_string (BORDER_TEXT_COLOR, X + 61, H, " Alt+S ");
   write_string (BORDER_TEXT_COLOR, X + 61+7, H, bottomstr);
   write_string (BORDER_TEXT_COLOR, X + 61+7+strlen(bottomstr), H, " ");
}


void writeButton(int attr, int x, int y, const char *label)
{
   char btn[16]; /* only 9 really required but a bit extra for good luck */

   switch(strlen(label))
   {
      case 0:
      sprintf(btn, "        ");
      break;

      case 1:
      sprintf(btn, "   %s    ", label);
      break;

      case 2:
      sprintf(btn, "   %s   ", label);
      break;

      case 3:
      sprintf(btn, "  %s   ", label);
      break;

      case 4:
      sprintf(btn, "  %s  ", label);
      break;

      case 5:
      sprintf(btn, " %s  ", label);
      break;

      case 6:
      sprintf(btn, " %s ", label);
      break;

      case 7:
      sprintf(btn, "%s ", label);
      break;

      default:
      sprintf(btn, "%.8s", label);
      break;
   }

   write_string(attr, x, y, btn);
}