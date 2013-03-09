#include <string.h>
#include "events.h"
#include "help_gui.h"
#include "parse.h"
#include "prepare.h"
#include "catdefs.h"

void
preparepes (struct eventState *pes)
{
  int i;
  /* Initialise the pes variables */
  pes->hidden = 0;
  pes->old_top = 0;
  pes->seek_base = 0;
  pes->forced_barpos = 0;
  mline = 0;
  mpos = 0;
  pes->left_was_pressed = 0;

  /* Check for a plain text file (actually checking that it isn't
     one of the supported file types: htm and zip */
  /* If it is, convert all < and > into &lt; and &gt; */
  /* This was the file won't be converted at all */
  if ((strcmpi (pes->filename+strlen(pes->filename)-4, ".htm")) &&
      (strcmpi (pes->filename+strlen(pes->filename)-4, ".zip")))
    simpleTagSubstitutions (pes, plainToHTMLsubs);

  if (isUTF8html(pes->text_buf))
     UTF8ToCodepage(pes->text_buf);

  tags2lwr (pes->text_buf);
  preformatTrim (pes->text_buf);

  simpleTagSubstitutions (pes, tagSubsTable1);
  headerTagSubstitution (pes);
  simpleTagSubstitutions (pes, tagSubsTable2);

  wordwrap (pes->text_buf);
  sensiblebreaks (pes->text_buf);

  pes->text_holder = pes->screen_buf + H * W * 2;

  preparebodyinfo (pes);

  pes->seek_cnt = 0;
  pes->bar_hooked = 0;
  pes->check_mouse = 0;
  pes->force_redraw = 1;
  pes->enable_timer = 0;
  pes->clink = 0;
  pes->link_priority = LP_MOUSE;

  pes->first_time = 1;

  if (pes->maxtop != 0)
    pes->barpos = ((pes->top - pes->body_start) * BARLEN) / pes->maxtop;
  else
    pes->barpos = BARLEN - 1;

  if (pes->barpos >= BARLEN)
    pes->barpos = BARLEN - 1;
}


void
searchpreparepes (struct eventState *pes)
{
  tags2lwr (pes->text_buf);
  pes->text_holder = pes->screen_buf + H * W * 2;
  preparebodyinfo (pes);
}


void
preparebodyinfo (struct eventState *pes)
{
  int i;

/* Changed to allow help to ignore tag options such as bgcolor - RP */
  pes->body_end = pes->text_buf + strlen (pes->text_buf);
  if ((pes->body_start = strstr (pes->text_buf, "<body")) != NULL)
    {
      pes->body_start = strchr (pes->body_start, '>');

      if (pes->body_start)
	{
	  pes->body_start++;	/* gets past the '>' */
	  if ((pes->body_end = strstr (pes->body_start, "</body>")) == NULL)
	    pes->body_end = pes->text_buf + strlen (pes->text_buf) - 1;
	}
      else
	pes->body_start = pes->text_buf;
    }
  else
    pes->body_start = pes->text_buf;

  while (*pes->body_start == '\r' || *pes->body_start == '\n')
    pes->body_start++;

  /* Following changed by RP. A literal string was being assigned
     to a non-constant pointer - bad. Now just set pes->body_start back to
     beginning of entire buffer if it isn't in a useful place. (May'04) */
  if (pes->body_start >= pes->body_end)
       pes->body_start = pes->text_buf;

  i = N;
  pes->p = pes->body_end - 1;
  if (*pes->p == '\n')
    pes->p--;

  for (; (pes->p != pes->body_start); pes->p--)
    {
      if (*pes->p == '\n')
	{
	  i--;
	  if (i == 0)
	    {
	      pes->p++;
	      break;
	    }
	}
    }

  pes->maxtop = pes->p - pes->body_start;
  pes->top = pes->body_start;
}


int isUTF8html(unsigned char *string)
{
   unsigned char *p;

   p = stristr(string, "<meta");

   if (p == 0)
      return 0;

   while (*p != '>' && *p != '\0')
   {
      if (strnicmp(p, "UTF-8", 5) == 0)
         return 1;
      p++;
   }

   return 0;
}