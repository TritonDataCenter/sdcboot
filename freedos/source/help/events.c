/*  FreeDOS HTML Help Viewer

    EVENTS - High-level processing of keyboard/mouse events

    Copyright (c) Express Software 1998-2003
    See doc\htmlhelp\copying for licensing terms
    Created by: Joseph Cosentino.
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "help_gui.h"
#define EVENTS_C
#include "events.h"
#include "catdefs.h"

/* D E F I N E S ------------------------------------------------------------ */

/* Define this to show what scan codes are being used as each key is pressed.
   useful for adding new keyboard functions: */
/* #define SHOW_SCAN_CODES */


/* F U N C T I O N S -------------------------------------------------------- */

int
processEvents (struct eventState *pes)
{
  static struct event ev;
#ifdef SHOW_SCAN_CODES
  char showscancode[256];
#endif

  if (pes == 0)
    return -1;

  if (pes->enable_timer)
    {
      ev.timer = 1;
      get_event (&ev, EV_KEY | EV_MOUSE | EV_TIMER);
      pes->enable_timer = 0;
    }
  else
    {
      get_event (&ev, EV_KEY | EV_MOUSE);
    }

#ifdef SHOW_SCAN_CODES
  if (ev.ev_type == EV_KEY)
    {
      sprintf (showscancode, "key = %i, scan = %x", ev.key, ev.scan);
      show_error (showscancode);
    }
#endif

  if (ev.ev_type == EV_KEY)
    {
      if (ev.key == 13 && pes->clink)
	{			/* Enter Key */
	  pes->link_text = pes->clink;
	  if (*pes->link_text == '\"')
	    pes->link_text++;
	  pes->p = pes->link_text;
	  pes->force_redraw = 1;
	}
      else if (ev.key == 27)
	return NAVIGATE_BACK;	/* Escape Key */
      else if (ev.key == 9 || ev.scan == 0x4de0 || ev.scan == 0x4d00)
	tabkey (pes);
      else if (ev.scan == 0xf00 || ev.scan == 0x4be0 || ev.scan == 0x4b00)
	shifttabkey (pes);
      else if (ev.scan == 0x50E0 || ev.scan == 0x5000)
	downkey (pes);
      else if (ev.scan == 0x48E0 || ev.scan == 0x4800)
	upkey (pes);
      else if ((ev.key == 8 && ev.scan == 0x0e08) ||
	       ev.scan == 0x3000 || ev.scan == 0x9b00
	       || (ev.key == 4 && ev.scan == 4))
	return NAVIGATE_BACK;	/* Backspace, Alt+B, Alt+Left Arrow */
      else if (ev.scan == 0x2d00 || ev.scan == 0x6B00) /* Alt+F4 */
	return NAVIGATE_EXIT;
      else if (ev.scan == 0x2e00)
	return NAVIGATE_HOME;
      else if (ev.scan == 0x2100 || ev.scan == 0x9d00
	       || (ev.key == 6 && ev.scan == 6))
	return NAVIGATE_FORWARD;	/* Alt+Right Arrow */
      else if (ev.scan == 0x3b00)
	return NAVIGATE_HELP;	/* F1 */
      else if (ev.scan == 0x3f00 || (ev.key == 18 && ev.scan == 0x1312))
	return NAVIGATE_REFRESH;	/* F5 or Ctrl+R */
      else if (ev.scan == 0x1f00)	/* Alt+S */
	return NAVIGATE_SEARCH;
      else if (ev.scan == 0x47E0 || ev.scan == 0x4700)
	{			/* Home Key */
	  pes->top = pes->body_start;
	  pes->clink = 0;
	}
      else if (ev.scan == 0x49E0 || ev.scan == 0x4900 ||
	       ev.scan == 0x99e0 || ev.scan == 0x9900)
	{			/* (Alt+)PgUp. */
	  if (pes->top != pes->body_start)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = 1 - N;
	      pes->clink = 0;
	    }
	}
      else if (ev.scan == 0x4FE0 || ev.scan == 0x4F00)
	{			/* End */
	  if (pes->bottom != pes->body_end)
	    {
	      pes->seek_base = pes->body_end;
	      pes->seek_cnt = -N;
	      pes->clink = 0;
	    }
	}
      else if (ev.scan == 0xa000)
	{			/* Alt+Down. */
	  if (pes->bottom != pes->body_end)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = +1;
	      pes->clink = 0;
	    }
	}
      else if (ev.scan == 0x9800)
	{			/* Alt+Up. */
	  if (pes->top != pes->body_start)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = -1;
	      pes->clink = 0;
	    }
	}
      else if (ev.scan == 0x51E0 || ev.scan == 0x5100 ||
	       ev.scan == 0xa1e0 || ev.scan == 0xa100)
	{			/* (Alt+)PgDn */
	  if (pes->bottom != pes->body_end)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = N - 1;
	      pes->clink = 0;
	    }
	}
      else if (isalpha (ev.key))
	letterkey (pes, ev.key);
    }

  /* Check for mouse events */
  if (ev.ev_type & (EV_MOUSE | EV_TIMER))
    {
      if (((ev.ev_type & EV_TIMER) || pes->left_was_pressed == 0) &&
	  ev.left == 1)
	{
	  pes->link_priority = LP_MOUSE;
	  if (ev.x >= X + 3 && ev.x < X + 3+3+LEN_MENU_BACK+1 && ev.y == H)
	    return NAVIGATE_BACK;
	  else if (ev.x >= X + 19 && ev.x < X + 19+7+LEN_MENU_FORWARD+1 && ev.y == H)
	    return NAVIGATE_FORWARD;
	  else if (ev.x >= X + 40 && ev.x < X + 40+7+LEN_MENU_CONTENTS+1 && ev.y == H)
	    return NAVIGATE_HOME;
	  else if (ev.x >= X + 61 && ev.x < X + 61+7+LEN_MENU_SEARCH+1 && ev.y == H)
	    return NAVIGATE_SEARCH;
	  else if (ev.x >= X + 60 && ev.x < X + 60+4+LEN_MENU_HELP+1 && ev.y == Y)
	    return NAVIGATE_HELP;
	  else if (ev.x >= X + 2 && ev.x < X + 2+7+LEN_MENU_EXIT+1 && ev.y == Y)
	    return NAVIGATE_EXIT;
	  else if (pes->link_under_mouse != 0)
	    {
	      pes->link_text = pes->link_under_mouse;
	      if (*pes->link_text == '\"')
		pes->link_text++;
	      pes->p = pes->link_text;
	    }
	  else if (ev.x == BARX && ev.y >= BARY - 1 && ev.y <= BARY + BARLEN)
	    {
	      if (ev.y == BARY - 1)
		{		/* Up. */
		  if (pes->top != pes->body_start)
		    {
		      pes->seek_base = pes->top;
		      pes->seek_cnt = -1;
		      pes->clink = 0;
		    }
		  pes->enable_timer = 1;
		}
	      else if (ev.y == BARY + BARLEN)
		{		/* Down. */
		  if (pes->bottom != pes->body_end)
		    {
		      pes->seek_base = pes->top;
		      pes->seek_cnt = +1;
		      pes->clink = 0;
		    }
		  pes->enable_timer = 1;
		}
	      else if (ev.y == BARY + pes->barpos)
		pes->bar_hooked = 1;
	      else if (ev.y < BARY + pes->barpos)
		{
		  /* Page Up with mouse on scroll bar */
		  if (pes->top != pes->body_start)
		    {
		      pes->seek_base = pes->top;
		      pes->seek_cnt = 1 - N;
		      pes->clink = 0;
		    }
		}
	      else
		{
		  /* Page Down with mouse on scroll bar */
		  if (pes->bottom != pes->body_end)
		    {
		      pes->seek_base = pes->top;
		      pes->seek_cnt = N - 1;
		      pes->clink = 0;
		    }
		}
	    }
     else if (ev.wheel && WheelSupported == 1)
     {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = ev.wheel;
	      pes->clink = 0;
     }

	}

      if (ev.x > X && ev.x < W && ev.y > Y && ev.y < H - 2)	/* modified by RP for v5.2.1 */
	{
	  pes->check_mouse = 1;
	  mline = ev.y - Y - 1;
	  mpos = ev.x - X - 2;
	  pes->link_priority = LP_MOUSE;
	}
      else
	{
	  pes->check_mouse = 0;
	  pes->link_under_mouse = 0;
	}

      pes->left_was_pressed = ev.left;
      if (ev.left == 0)
	pes->bar_hooked = 0;

      if (pes->bar_hooked)
	{
	  if (ev.y <= BARY)
	    pes->forced_barpos = 0;
	  else if (ev.y < BARY + BARLEN)
	    pes->forced_barpos = ev.y - BARY;
	  else
	    pes->forced_barpos = BARLEN - 1;
	}
    }

  return NAVIGATE_DONT;
}

void
letterkey (struct eventState *pes, char letter)
{
  char *searchpos;
  int backtostart = 1;

  if (linkcharmatch (pes->clink, letter))
    searchpos = pes->clink;
  else
    searchpos = pes->top;

  do
    {
      searchpos = strstr (searchpos, "<a href=");

      /* if necessary go back to top and search from beginning */
      if (searchpos == NULL && backtostart)
	{
	  searchpos = strstr (pes->body_start, "<a href=");
	  backtostart = 0;
	}

      if (searchpos)
	{
	  searchpos += 8;

	  if (*searchpos == '\"')
	    searchpos++;
	}
      else
	return;
    }
  while (!linkcharmatch (searchpos, letter));

  pes->clink = searchpos;
  gotolink (pes);
}


int
linkcharmatch (char *clink, char letter)
{
  char *firstletter;
  char uprcase[1], lwrcase[1];

  uprcase[0] = letter;
  uprcase[1] = '\0';
  strupr (uprcase);
  lwrcase[0] = letter;
  lwrcase[1] = '\0';
  strlwr (lwrcase);

  firstletter = strchr (clink, '>');
  if (firstletter)
    firstletter++;
  else
    return 0;

  if (*firstletter == *uprcase || *firstletter == *lwrcase)
    return 1;
  else
    return 0;
}

/* tab to next link */
void
tabkey (struct eventState *pes)
{
  if (!pes->clink)
    pes->clink = pes->top;


  pes->clink = strstr (pes->clink, "<a href=");
  if (!pes->clink)
    {
      pes->clink = pes->top;
      pes->clink = strstr (pes->clink, "<a href=");
    }

  if (pes->clink)
    {
      pes->clink += 8;

      if (*pes->clink == '\"')
	pes->clink++;

      gotolink (pes);
    }
}


/* Go back from the currently highlighted link. RP */
void
shifttabkey (struct eventState *pes)
{
  if (pes->clink)
    {
      if (pes->clink - 6 >= pes->body_start)
	{
	  char tempchar;
	  char *newclink, *searchfromhere = pes->body_start;

	  tempchar = *(pes->clink - 6);
	  *(pes->clink - 6) = 0;

	  while ((newclink = strstr (searchfromhere, "<a href=")) != 0)
	    searchfromhere = newclink + 1;

	  *(pes->clink - 6) = tempchar;

	  if (searchfromhere != pes->body_start)
	    pes->clink = searchfromhere;
	  else
	    pes->clink = 0;
	}
      else
	pes->clink = 0;
    }

  if (!pes->clink)
    {
      pes->clink = pes->body_start;
      pes->clink = strstr (pes->clink, "<a href=");
    }


  if (pes->clink)
    {
      pes->clink += 8;

      if (*pes->clink == '\"')
	pes->clink++;

      gotolink (pes);
    }
}

/* Code used by tabkey, shiftabkey, and letterkey */
void
gotolink (struct eventState *pes)
{
  if (pes->clink > pes->bottom || pes->clink < pes->top)	/* improved RP 2004 */
    {
      pes->seek_base = pes->clink;
      pes->seek_cnt = -N / 2;	/* centre. RP */
    }

  pes->force_redraw = 1;
  pes->link_priority = LP_KEYBOARD;

}


/* Highlights a link on the line
   below. If we are currently on
   the third link in the current
   line, go to the third link
   on the next line etc. RP */
void
downkey (struct eventState *pes)
{
  char *thisline;
  int countlinks;

  char *oldclink = pes->clink;	/* restore this if we only scroll */

  if (pes->clink)
    {
      thisline = pes->clink;

      /* go to the beginning of the current line */
      while (*thisline != '\n' && thisline > pes->body_start)
	thisline--;

      /* Find out how many links are before the currently
         highlighted link (on the current line) */
      countlinks = 0;
      while (thisline < pes->clink)
	{
	  thisline = strstr (thisline, "<a href=");
	  if (thisline)
	    {
	      ++countlinks;
	      thisline += 9;	/* get past the "<a href=" business */
	    }
	  else
	    thisline = pes->clink;
	}
      countlinks--;		/* discount the current link */

      thisline = strchr (thisline, '\n');

      /* now skip this number of links into the next line */
      pes->clink = skip2link (thisline, countlinks);
      {
	thisline = strstr (thisline, "<a href=");
	pes->clink = thisline;

	if (thisline)
	  {
	    int j = 1;

	    while (j <= countlinks && *thisline != '\n')
	      {
		thisline++;

		if (strncmp (thisline, "<a href=", 8) == 0)
		  {
		    pes->clink = thisline;
		    j++;
		  }
	      }
	  }
      }
    }
  else
    {
      /* if there is no current link, choose
         the first link on the screen (if there is one) */
      pes->clink = strstr (pes->top, "<a href=");
      if (pes->clink > pes->bottom || pes->clink < pes->top)
	{
	  pes->clink = 0;
	}
    }

  if (pes->clink)
    {
      pes->clink += 8;

      if (*pes->clink == '\"')
	pes->clink++;

      if (oldclink && (pes->clink - oldclink > pes->bottom - pes->top))
	{			/* The new link is too far away, and would cause jerky browsing */
	  if (pes->bottom != pes->body_end)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = +1;
	    }
	  /* Keep the same link that we started with (unless
	     it has scrolled off the screen */
	  if (oldclink > pes->top && oldclink < pes->bottom)
	    pes->clink = oldclink;
	  else
	    pes->clink = 0;
	}
      else if (pes->clink - pes->top > pes->bottom - pes->top)
	{
	  pes->seek_base = pes->clink;
	  pes->seek_cnt = -N;
	}

      pes->force_redraw = 1;
      pes->link_priority = LP_KEYBOARD;
    }
  else				/* scroll instead */
    {
      if (pes->bottom != pes->body_end)
	{
	  pes->seek_base = pes->top;
	  pes->seek_cnt = +1;
	}
      /* Keep the same link that we started with (unless
         it has scrolled off the screen */
      if (oldclink > pes->top && oldclink < pes->bottom)
	pes->clink = oldclink;
    }
}


/* Highlights a link on the line
   above. If we are currently on
   the third link in the current
   line, go to the third link
   line above etc. RP */
void
upkey (struct eventState *pes)
{
  char *thisline;
  int countlinks;

  char *oldclink = pes->clink;	/* restore this if we only scroll */

  if (pes->clink)
    {
      thisline = pes->clink;

      /* go to the beginning of the current line */
      while (*thisline != '\n' && thisline > pes->body_start)
	thisline--;

      /* Find out how many links are before the currently
         highlighted link (on the current line) */
      countlinks = 0;
      while (thisline < pes->clink)
	{
	  thisline = strstr (thisline, "<a href=");
	  if (thisline)
	    {
	      ++countlinks;
	      thisline += 9;	/* get past the "<a href=" business */
	    }
	  else
	    thisline = pes->clink;
	}
      countlinks--;		/* discount the current link */

      while (*thisline != '\n' && thisline > pes->body_start)
	thisline--;

      thisline--;
      while (strncmp (thisline, "<a href=", 8) && thisline > pes->body_start)
	thisline--;

      if (thisline > pes->body_start)
	{
	  thisline--;
	  while (*thisline != '\n' && thisline > pes->body_start)
	    thisline--;

	  /* now skip this number of links into the next line */
	  pes->clink = skip2link (thisline, countlinks);
	}
      else
	pes->clink = 0;
    }
  else
    {
      /* if there is no current link, choose
         the first link on the screen (if there is one */
      pes->clink = strstr (pes->top, "<a href=");
      if (pes->clink > pes->bottom || pes->clink < pes->top)
	{
	  pes->clink = 0;
	}
    }

  if (pes->clink)
    {
      pes->clink += 8;

      if (*pes->clink == '\"')
	pes->clink++;

      if (oldclink && (oldclink - pes->clink > pes->bottom - pes->top))
	{			/* The new link is too far away, and would cause jerky browsing */
	  if (pes->top != pes->body_start)
	    {
	      pes->seek_base = pes->top;
	      pes->seek_cnt = -1;
	    }
	  /* Keep the same link that we started with (unless
	     it has scrolled off the screen */
	  if (oldclink > pes->top && oldclink < pes->bottom)
	    pes->clink = oldclink;
	  else
	    pes->clink = 0;
	}
      else if (pes->clink < pes->top)
	{
	  pes->seek_base = pes->clink;
	  pes->seek_cnt = -1;
	}

      pes->link_priority = LP_KEYBOARD;
      pes->force_redraw = 1;
      pes->check_mouse = 0;
    }
  else				/* scroll instead */
    {
      if (pes->top != pes->body_start)
	{
	  pes->seek_base = pes->top;
	  pes->seek_cnt = -1;
	}
      /* Keep the same link that we started with (unless
         it has scrolled off the screen */
      if (oldclink > pes->top && oldclink < pes->bottom)
	pes->clink = oldclink;
    }
}

/* Used for up/down key to find the link a given number of links in
   on a given line */
char *
skip2link (char *line, int count)
{
  char *clink = 0;

  if (line)
    {
      line = strstr (line, "<a href=");
      clink = line;

      if (line)
	{
	  int j = 1;

	  while (j <= count && *line != '\n')
	    {
	      line++;

	      if (strncmp (line, "<a href=", 8) == 0)
		{
		  clink = line;
		  j++;
		}
	    }
	}
    }
  else
    clink = 0;

  return clink;
}
