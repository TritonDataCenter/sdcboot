/*  FreeDOS HTML Help Viewer

    HELP_HTM - Browses the HTML

    Copyright (c) Express Software 1998-2003
    See doc\htmlhelp\copying for licensing terms
    Created by: Joseph Cosentino.

    21st May 2000 - Forward only keyboard selection of links - BS
		11th Sep 2002 - Changes to improve memory usage, and code clarity - PH
                    (comment by RP: PH's changes solved a problem where
                     recursion crashed the stack)
    11th Sep 2002 - Scroll bar area can be used to page up and down - RP
    13th Sep 2002 - & character code support added - PH
    16th Sep 2002 - HTML does not have to be preformatted now - RP
    17th Sep 2002 - Multiple spaces are ignored - as per normal HTML - RP
    25th Sep 2002 - Recursion is no longer used to navigate links. A
                    resizeable array (not a linked list - faster than this)
                    is implemented in pes.c. It allows navigation back
                    and forth - RP.
    Alterations Documented in HISTORY from here on.
*/

/* I N C L U D E S ---------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "conio.h"
#include "help.h"
#include "help_htm.h"
#include "help_gui.h"
#include "events.h"
#include "search.h"
#include "catdefs.h"
#include "readfile.h"

#include <dir.h>

/* F U N C T I O N S -------------------------------------------------------- */

void
html_view (const char *target,
	   char *base_dir, char *home_page, const char *help_page)
{
  struct eventState *pes = 0;
  char *p, *q;
  char fullpath[257];
  char target_name[257];
  char *after_base_dir;
  int pleaseLoad = 1;

  if (pesListInitialise () == FAIL)
    return;

  if (target == 0)
    *target_name = 0;
  else
    strcpy (target_name, target);

  strcpy (fullpath, base_dir);

  while (1)
    {
      if (pleaseLoad)
	{
	  if (*target_name == 0 || *target_name == '#')
	    {
	      strcpy (fullpath, base_dir);
	      /* Cosmetic:
	         since default home_page is in ..\help
	         clear the .. and take a directory of the
	         base directory if possible.
	         When files aren't found, the error msgs
	         will look better: */
	      if (home_page[0] == '.' && home_page[1] == '.'
		  && *base_dir != 0)
		{
		  char *forwardslash, *backwardslash;
		  char *lastchar;
		  lastchar = fullpath + strlen (fullpath) - 1;
		  if (*lastchar == '\\' || *lastchar == '/')
		    *lastchar = 0;
		  forwardslash = strrchr (fullpath, '/');
		  backwardslash = strrchr (fullpath, '\\');
		  after_base_dir = (forwardslash > backwardslash)
		    ? forwardslash : backwardslash;
		  if (after_base_dir == 0)
		    after_base_dir = fullpath;

		  strcpy (after_base_dir, home_page + 2);
		}
	      else
		{
		  after_base_dir = fullpath + strlen (fullpath);
		  strcat (after_base_dir, home_page);
		}
	      q = target_name;
	    }
	  else
	    {
	      after_base_dir = fullpath + strlen (fullpath);

	      for (q = target_name, p = after_base_dir;
		   (*q != 0) && (*q != '#'); q++, p++)
		*p = *q;

	      *p = 0;
	    }

	  if (pes)
	    if (pes->hidden)
	      pes = pesListBackward ();

	  /* The following strange combination of an if and a ? is there
	     to compare pes->filename and fullpath, so as not to create the pes
	     if the desired file is already the current page.
	     But if pes is 0, then it can't be the current page, and also
	     there is nothing to compare. So if pes is 0, we wish to
	     create the pes for sure - hence the ': 1' condition. -RP */
	  if (pes ? stricmp (pes->filename, fullpath) : 1)
	    {
	      /* Attempt to create a new pes in the list */
	      if ((pes = pesListAdd (fullpath, home_page)) != 0)
		   {
		      preparepes (pes);
		   }
	      else
         {
            readFileError(after_base_dir);
            if ((pes = pesListCurrent ()) == 0)
               return;
            q = NULL;
         }
	    }

	  if (q != 0)
	    pes->link_text = (*q != 0) ? q : 0;
	  else
	    pes->link_text = 0;
	}

      *after_base_dir = 0;	/* Truncate fullpath to just base_dir */

      switch (html_view_internal (pes, base_dir, home_page))
	{
	case NAVIGATE_BACK:
     pes->seek_base = backBookmark(pes);
     if (pes->seek_base)
     {
        pes->seek_cnt = -1;
        pes->clink = NULL;
        pleaseLoad = 0;
        break;
     }

	  save_window (X, Y, W, H - 3, pes->screen_buf);
	  if ((pes = pesListBackward ()) == 0)
	    {
	      pesListDestroy ();
	      return;
	    }
	  pes->left_was_pressed = 0;
	  load_window (X, Y, W, H - 3, pes->screen_buf);
	  pleaseLoad = 0;
	  break;

	case NAVIGATE_LINK:
	  save_window (X, Y, W, H - 3, pes->screen_buf);

	  if (pes->text_holder != 0)
	    {
	      /* Start the target_name off with the current files' path */
	      char *forwardslash, *backwardslash;
	      strcpy (target_name, pes->filename);

	      /* Find the last / or \ and truncate the string */
	      forwardslash = strrchr (target_name, '/');
	      backwardslash = strrchr (target_name, '\\');
	      p =
		(forwardslash > backwardslash) ? forwardslash : backwardslash;

	      if (p != NULL)
		*p = 0;
	      else
		*target_name = 0;

	      /* Clip off directories, if the target goes 'up' the
	         directory tree with ".."s. */
	      while ((strncmp (pes->text_holder, "..", 2) == 0)
		     && (p != NULL))
		{
		  forwardslash = strrchr (target_name, '/');
		  backwardslash = strrchr (target_name, '\\');
		  p = (forwardslash > backwardslash) ?
		    forwardslash : backwardslash;

		  if (p != NULL)
		    {
		      *p = 0;	/* modify target name */

		      /* Cut out the .. by copying backwards */
		      /* first make sure the string isn't just ".." */
		      if (strlen (pes->text_holder) >= 3)
			{
			  p = pes->text_holder;
			  while (p[2] != 0)
			    {
			      *p = p[3];	/* fixed. changed from 2 to 3. 9-MAR-04 RP */
			      ++p;
			    }
			}
		      else	/* treat as just "index.htm" */
			strcpy (pes->text_holder, "index.htm");
		    }
		}

	      /* Minor fix: changed || into && here - RP 2004 */
	      if ((*(pes->text_holder) != '/')
		  && (*(pes->text_holder) != '\\')
		  && (target_name[strlen (target_name)] != '/')
		  && (target_name[strlen (target_name)] != '\\')
		  && *target_name != 0)
		strcat (target_name, "\\");

	      strcat (target_name, pes->text_holder);
	      /* Since the target link is relative to the present link,
	         there is no need for the fullpath to contain the base
	         directory: */
	      *fullpath = 0;
	    }
	  else
	    *target_name = 0;

	  pleaseLoad = 1;
	  *pes->text_holder = 0;
	  break;

	case NAVIGATE_FORWARD:
     pes->seek_base = forwardBookmark(pes);
     if (pes->seek_base)
     {
        pes->seek_cnt = -1;
        pes->clink = NULL;
        pleaseLoad = 0;
        break;
     }
	  save_window (X, Y, W, H - 3, pes->screen_buf);
	  if ((pes = pesListForward ()) != 0)
	    load_window (X, Y, W, H - 3, pes->screen_buf);
	  else
	    pes = pesListCurrent ();
	  pes->left_was_pressed = 0;
	  pleaseLoad = 0;
	  break;

	case NAVIGATE_HOME:
	  save_window (X, Y, W, H - 3, pes->screen_buf);
	  *fullpath = 0;
	  *target_name = 0;
	  pleaseLoad = 1;
	  break;

	case NAVIGATE_HELP:
	  save_window (X, Y, W, H - 3, pes->screen_buf);
     /* help_page NOT relative to current page, but to base directory: */
     strcpy (fullpath, base_dir);
	  strcpy (target_name, help_page);
	  pleaseLoad = 1;
	  break;

	case NAVIGATE_EXIT:
	  pesListDestroy ();
	  return;

	case NAVIGATE_REFRESH:
	  strcpy (target_name, pes->filename);
	  *fullpath = 0;
	  pes = pesListBackward ();
	  pleaseLoad = 1;
	  break;

	case NAVIGATE_SEARCH:
	  save_window (X, Y, W, H - 3, pes->screen_buf);
	  if ((pes = helpsearch (pes, base_dir, home_page)) != NULL)
	    {
	      pesListDeleteForwards ();
	      preparepes (pes);
	      pes->link_text = 0;
	      *fullpath = 0;
	    }
	  else
	    pes = pesListCurrent ();
	  pleaseLoad = 0;
	  break;

	default:
	  pleaseLoad = 0;
	}
    }				/* END while(1) */
}


int
html_view_internal (struct eventState *pes, char *base_dir, char *home_page)
{
  int i;

  if (pes->link_text != 0)
    {
      statusbar (hcatStatusLooking);
      if (*pes->link_text == '#')
	{
	  char *anchor;

	  if (strnicmp (pes->link_text + 1, HTMLHELP_INTERNAL_SEARCHLINK,
			strlen (HTMLHELP_INTERNAL_SEARCHLINK)) == 0)
	    {
	      /* Internal search engine link */
	      anchor = pes->body_start + atoi (pes->link_text +
					       strlen
					       (HTMLHELP_INTERNAL_SEARCHLINK)
					       + 1);
	    }
	  else
	    {
	      sprintf (pes->text_holder, "<a name=\"%.*s\"",
		       link_length (pes->link_text + 1), pes->link_text + 1);
	      anchor = strstr (pes->body_start, pes->text_holder);
	    }

	  if (anchor)
	    {
	      /* Locate start of line. */
	      while (anchor != pes->body_start && *anchor != '\n')
		anchor--;

	      if (*anchor == '\n')
		anchor++;

	      /* Move to this line, just near top of screen */
         if (!pes->first_time)
            addBookmark(pes);
	      pes->seek_base = anchor;
	      pes->seek_cnt = -1;
         pes->clink = 0; /* RP - Added 5.3.2 */
	      pes->link_text = 0;
	      *pes->text_holder = 0;
	      statusbar (NULL);
	      return NAVIGATE_DONT;
	    }
	  else
	    {
	      int triedasfile = 0;
         int tryexact = 1;
	      sprintf (pes->text_holder, "%.*s",
		       link_length (pes->link_text + 1), pes->link_text + 1);
	      strlwr (pes->text_holder);
		   write_string (TEXT_COLOR, X + strlen(hcatStatusLooking) + 3, TY,
				                       hcatStatusEscape);
	      do
		{

		  pes->p = pes->body_start;
		  while ((pes->p = strstr (pes->p, "<a")) != 0)
		    {
		      static struct event ev;
		      get_event (&ev, EV_KEY | EV_NONBLOCK);
		      if (ev.ev_type == EV_KEY && ev.key == 27)
			{
			  pes->link_text = 0;
           show_error(hcatUserAborted);
			  return NAVIGATE_DONT;
			}

		      pes->p = strchr (pes->p, '>');
		      if (pes->p)
			{
			  ++(pes->p);
			  if ((strnicmp(pes->p, pes->text_holder,
			                        strlen (pes->text_holder)) == 0) &&
                (pes->p[strlen(pes->text_holder)] == '<' || !tryexact))
			    {
			      while (pes->p != pes->body_start &&
				     strncmp (pes->p, "<a", 2))
				pes->p--;

			      pes->p = strstr (pes->p, "href=");
			      if (pes->p != 0)
				{
				  pes->p += 6;
				  if (*pes->p == '\"')
				    pes->p++;

				  if (*pes->p == '#')
				    break;

				  if (prepare_link (pes))
				    {
                  if (tryexact == 0)
                     show_error(hcatNoExactFound);
				      return NAVIGATE_LINK;
				    }
				}
			    }
			}
		    }
		  if (triedasfile == 0)
		    {
		      int undostrcat = strlen (base_dir);
		      if (base_dir[strlen (base_dir) - 1] != '\\'
			  && base_dir[0] != 0)
			strcat (base_dir, "\\");
		      strcat (base_dir, pes->text_holder);
		      if (checkForFile (base_dir))
			{
			  /* Didn't work */
			  base_dir[undostrcat] = '\0';
			}
		      else
			{
			  if (pes->first_time)
			    pes->hidden = 1;
			  get_home_page (home_page, base_dir);
			  get_base_dir (base_dir, base_dir);
			  return NAVIGATE_HOME;
			}
		      triedasfile = 1;
		    }
		  else
		    {
            /* don't try to match end with the < */
            if (tryexact)
            {
               tryexact = 0;
            }
            else
            {
		         pes->p = pes->text_holder + strlen (pes->text_holder) - 1;
		         if (*pes->p > 'a')
			         --(*pes->p);
		         else
			         *pes->p = 0;
            }
		    }
		}
	      while (*(pes->text_holder) != 0);
	    }
     statusbar (NULL);

	  sprintf (pes->text_holder, "%s '%.*s'", hcatCouldntFind,
		   link_length (pes->link_text + 1), pes->link_text + 1);
	  show_error (pes->text_holder);
	  pes->link_text = 0;
	  *(pes->text_holder) = 0;
	  return NAVIGATE_DONT;
	}
      else if (prepare_link (pes))
	return NAVIGATE_LINK;

      pes->link_text = 0;
    }

  if (pes->first_time)
  {
     drawmenu();
     save_window (X, Y, W, H, pes->screen_buf);
     pes->first_time = 0;
  }

  if (pes->seek_cnt != 0)
    {
      if (pes->seek_cnt <= 0)
	{
	  i = -pes->seek_cnt;
	  pes->p = pes->seek_base - 1;
	  if (*pes->p == '\n')
	    pes->p--;

	  for (; (pes->p != pes->body_start); pes->p--)
	    if (*pes->p == '\n')
	      {
		i--;
		if (i == 0)
		  {
		    pes->p++;
		    break;
		  }
	      }
	  pes->top = pes->p;
	}
      else if (pes->seek_cnt > 0)
	{
	  i = pes->seek_cnt;
	  pes->p = pes->seek_base;
	  for (; pes->p != pes->body_end; pes->p++)
	    if (*pes->p == '\n')
	      {
		i--;
		if (i == 0)
		  {
		    pes->p++;
		    break;
		  }
	      }

	  if (pes->p == pes->body_end)
	    {
	      pes->seek_base = pes->body_end - 1;
	      pes->seek_cnt = -1;
	      return NAVIGATE_DONT;
	    }

	  pes->top = pes->p;
	}
      pes->seek_cnt = 0;
    }

  if (pes->maxtop != 0)
    pes->barpos = ((pes->top - pes->body_start) * BARLEN) / pes->maxtop;
  else
    pes->barpos = BARLEN - 1;

  if (pes->barpos >= BARLEN || pes->barpos < 0)	/* <0 fix - RP */
    pes->barpos = BARLEN - 1;

  if (pes->bar_hooked)
    {
      pes->seek_base = pes->top;
      if (pes->forced_barpos == 0)
	{
	  pes->top = pes->body_start;
	  pes->barpos = 0;
	}
      else if (pes->forced_barpos == BARLEN - 1
	       && pes->barpos != pes->forced_barpos)
	{
	  pes->old_barpos = pes->barpos;
	  pes->seek_base = pes->body_end;
	  pes->seek_cnt = -N;
	  return NAVIGATE_DONT;
	}
      else if (pes->forced_barpos < pes->barpos
	       && pes->forced_barpos < pes->old_barpos)
	{
	  pes->old_barpos = pes->barpos;
	  pes->seek_cnt = -1;
	  return NAVIGATE_DONT;
	}
      else if (pes->forced_barpos > pes->barpos
	       && pes->forced_barpos > pes->old_barpos)
	{
	  pes->old_barpos = pes->barpos;
	  pes->seek_cnt = +1;
	  return NAVIGATE_DONT;
	}
    }

  pes->old_barpos = pes->barpos;
  if (pes->top != pes->old_top)
    pes->force_redraw = 1;

  /* ------ Display processing ------------------------------------ */
  displayProcess (pes);

  /* ------ Event processing -------------------------------------- */
  return processEvents (pes);
}


void
displayProcess (struct eventState *pes)
{
  long i, k = 0, ch, line = 0, len = 0;
  char link_started, bold_started, it_started;
  char *link_reference;
  char str[2 * LEN + 3], col[LEN + 1], tmp[LEN + 1];

  if (pes->force_redraw == 1 || pes->check_mouse == 1)
    {
      pes->p = pes->top;
      pes->link_under_mouse = 0;
      /* The following bit checks for formatting and links started above
         the top of the screen (before pes->top in the text buffer)
         RP 11-mar-04 */
      bold_started = checkAboveTop (pes, "<b>", "</b>");
      it_started = checkAboveTop (pes, "<i>", "</i>");
      link_started = checkAboveTop (pes, "<a href=", "</a>");
      if (link_started)
	{
	  link_reference = pes->p;
	  while (strncmp (link_reference, "<a href=", 8))
	    {
	      link_reference--;
	      if (link_reference == pes->body_start)
		break;
	    }
	  link_reference += 8;
	  if (*link_reference == '"')
	    link_reference++;
	}

      while (1)
	{
	  if (*pes->p == '\n' || len == LEN || pes->p == pes->body_end)
	    {
	      if (*pes->p != '\n' && pes->p != pes->body_end)
		while (*pes->p != '\n' && pes->p != pes->body_end)
		  pes->p++;

	      if (*pes->p == '\n' && pes->p != pes->body_end)
		pes->p++;

	      while (len != LEN)
		{
		  tmp[(size_t) len] = ' ';
		  col[(size_t) len] = TEXT_COLOR;
		  len++;
		}

	      if (pes->force_redraw)
		{
		  int j;
		  for (i = 0, j = 0; i < LEN; i++)
		    {
		      str[j++] = tmp[i];
		      str[j++] = col[i];
		    }
		  load_window (X + 2, Y + 1 + line, LEN, 1, str);
		}

	      line++;
	      len = 0;

	      if (pes->p == pes->body_end || line == N)
		{
		  if (line != N)
		    clear_window (TEXT_COLOR, X + 2, Y + 1 + line, LEN,
				  N - line);

		  break;
		}

	      continue;
	    }

	  if (*pes->p == '\r')
	    {
	      pes->p++;
	      continue;
	    }

	  if (*pes->p == '<' && (pes->p[1] >= 'A' && pes->p[1] <= 'Z' ||
				 pes->p[1] >= 'a' && pes->p[1] <= 'z' ||
				 pes->p[1] == '/' || pes->p[1] == '!'))
	    {
	      if (link_started == 0 && (pes->p[1] == 'a' || pes->p[1] == 'A'))
		{
		  /* modified for help 5.0.2 so that links without href= in them
		     e.g <A NAME="..."> are not accidentally loaded. - RP */
		  char *closearrow;
		  closearrow = strchr (pes->p, '>');
		  *closearrow = 0;
		  link_reference = strstr (pes->p, "href=");
		  *closearrow = '>';

		  if (link_reference)
		    {
		      link_reference += 5;
		      if (*link_reference == '\"')
			link_reference++;

		      link_started = 1;
		    }
		}
	      else if (link_started == 1 && strncmp (pes->p, "</a>", 4) == 0)
		link_started = 0;
	      else if (bold_started == 0 && strncmp (pes->p, "<b>", 3) == 0)
		bold_started = 1;
	      else if (bold_started == 1 && strncmp (pes->p, "</b>", 4) == 0)
		bold_started = 0;
	      else if (it_started == 0 && strncmp (pes->p, "<i>", 3) == 0)
		it_started = 1;
	      else if (it_started == 1 && strncmp (pes->p, "</i>", 4) == 0)
		it_started = 0;

	      while (*pes->p != '>' && pes->p != pes->body_end)
		pes->p++;

	      if (*pes->p == '>')
		pes->p++;
	    }
	  else
	    {
	      /* Translate some &-escape codes. */
	      ch = *pes->p;
	      if (*pes->p == '&')
		ch = ampSubs (pes);

	      tmp[len] = (ch != '\t') ? ch : ' ';

	      col[len] = link_started ? LINK_COLOR : TEXT_COLOR &&
		bold_started ? BOLD_COLOR : TEXT_COLOR &&
		it_started ? ITALIC_COLOR : TEXT_COLOR;

	      if (link_started && link_reference == pes->clink)
		col[len] = LINK_HIGHLIGHTED_COLOR;

	      if (line == mline && len == mpos && link_started
		  && pes->check_mouse == 1)
		pes->link_under_mouse = link_reference;

	      len++;

	      if (ch != '\t' || len % 8 == 0)
		pes->p++;
	    }
	}

      if (pes->force_redraw == 1)
	{
	  pes->bottom = pes->p;
	  for (i = 0; i < BARLEN; i++)
	    {
	      write_char (BORDER_BOX_COLOR, BARX, BARY + i, ((i != pes->barpos) && (pes->maxtop != 0)) ? BarBlock1 : BarBlock2);	/* modified RP */
	    }
	  pes->old_top = pes->top;
	  pes->force_redraw = 0;
	}
    }

  /* The following has lines of code in this function were modified
     to show the link selected by the tab key for those users without
     mice - RP. */
  if (pes->link_under_mouse && pes->link_priority == LP_MOUSE)
    pes->p = pes->link_under_mouse;
  else if (pes->clink)
    pes->p = pes->clink;

  if (pes->clink || pes->link_under_mouse)
    {
      k = strchr (pes->p, '\"') - pes->p;
      if (k < 0 || k > LEN - 12)
	k = LEN - 12;

      sprintf (pes->text_holder, "%.*s", (int) k, pes->p);
      statusbar (pes->text_holder);
      *pes->text_holder = 0;
    }
  else
    statusbar (NULL);		/* 11--mar-04 */

  pes->p = pes->link_text;
}				/* end displayProcess. */


char
prepare_link (struct eventState *pes)
{
  if (isFileLink (pes->p))
    {
      sprintf (pes->text_holder, "%.*s", link_length (pes->p), pes->p);

      pes->link_text = 0;
      if (pes->first_time)
	pes->hidden = 1;

      statusbar (NULL);
      return 1;
    }

  return 0;
}


int
isFileLink (const char *string)
{
  if (strnicmp (string, "http://", 7) != 0 &&
      strnicmp (string, "ftp://", 6) != 0 &&
      strnicmp (string, "mailto:", 7) != 0)
    return 1;
  else
    return 0;
}


/* Solves the problem of bold, italic etc not effective if it starts
   off-screen RP 11-MAR-04 */
int
checkAboveTop (struct eventState *pes, const char *openingtag,
	       const char *closingtag)
{
  char *searchpos = pes->body_start;
  char backupchar = *pes->p;
  int retvalue = 0;

  *pes->p = '\0';

  while ((searchpos = strstr (searchpos, openingtag)) != NULL)
    {
      if ((searchpos = strstr (searchpos, closingtag)) != NULL)
	{
	  retvalue = 0;
	}
      else
	{
	  retvalue = 1;
	  break;
	}
    }

  *pes->p = backupchar;

  return retvalue;
}
