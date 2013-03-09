/*  FreeDOS HTML Help Viewer

    PARSE - Parses HTML

    Copyright (c) Express Software 1998-2003
    See doc\htmlhelp\copying for licensing terms
    Created by: Joe Cosentino, Brian E. Reifsnyder,
                Paul Hsieh, and Robert Platt
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <dos.h>

#include "pes.h"
#include "help.h"
#include "help_gui.h"
/* The following define tells parse.h to include stuff relevent only
   to parse.c itself - to eliminate the declared but never used warning
   for tagSubsTable in other modules */
#define _PARSE_C_
#include "parse.h"
#include "utfcp.h"
#undef _PARSE_C_

int ampSubsNCR (struct eventState *pes);

int ampSubsInternal (struct eventState *pes,
                     struct ampCharEntryStruct *table,
                     enum entityTableType type);

/* tags2lwr

   Much of the parsing code uses strstr to search for tags. strstr is
   case sensitive, but in html tags are case insensitive!

   This code converts all tag strings to lower case

   created by Robert Platt
*/
void
tags2lwr (char *text)
{
  char *leftarrow = text;
  char *rightarrow;

  while ((leftarrow = strchr (leftarrow, '<')) != 0)
    {
      ++leftarrow;		/* point to text just after left arrow */
      rightarrow = strchr (leftarrow, '>');
      if (rightarrow)
	{
	  *rightarrow = 0;
	  strlwr (leftarrow);
	  *rightarrow = '>';
	}
      else
	{
	  /* a tag has been begun but there is no closing '>'. So make all
	     remaining characters lower case and exit */
	  strlwr (leftarrow);
	  return;
	}
    }
}

/* tag substitution.
   Adapted from Brian E. Reifsnyder's changes by Paul Hsieh,
   and subsequently generalised by Robert Platt to allow the .before
   and .after text to be of different sizes */
void
simpleTagSubstitutions (struct eventState *pes, tagSubsEntry * tagSubsTable)
{
  int i;
  char *tptr;
  char *searchpos;
  const char *before;
  const char *after;
  int lenbefore;
  int lenafter;
  int diff;

  for (i = 0; tagSubsTable[i].before != 0; i++)
  {
      searchpos = pes->text_buf;
      before = tagSubsTable[i].before;
      after = tagSubsTable[i].after;
      lenbefore = strlen (before);
      lenafter = strlen (after);
      diff = lenafter - lenbefore;

      while ((tptr = strstr (searchpos, before)) != NULL)
	   {
         if (diff > 0)
         {
            long oldlen = strlen (pes->text_buf);
	         long newlen = oldlen + diff + 1; /* must be long to check if > UINT_MAX */
            long tptrlen = strlen (tptr);
            long searchposlen = strlen(searchpos);

            if (!pesResizeCurrentTextbuf (newlen))
               return;
            tptr = pes->text_buf + (oldlen - tptrlen);
            searchpos = pes->text_buf + (oldlen - searchposlen);

	         memmove (tptr + lenafter,
                     tptr + lenbefore,
                     tptrlen - lenbefore);
         }
         else if (diff < 0)
         {
            long oldlen = strlen (pes->text_buf);
	         long newlen = oldlen + diff + 1;
            long tptrlen = strlen (tptr);
            long searchposlen = strlen(searchpos);

	         memmove (tptr + lenafter,
                     tptr + lenbefore,
                     tptrlen - lenbefore);

            if (!pesResizeCurrentTextbuf (newlen))
               return;
            tptr = pes->text_buf + (oldlen - tptrlen);
            searchpos = pes->text_buf + (oldlen - searchposlen);

         }

	      strncpy (tptr, tagSubsTable[i].after, lenafter);
	      /* Skip completed occurence */
	      searchpos = tptr + lenafter;
	   }
  }
} /* end simpleTagSubstitutions. */


/* The following creates a 3D-style border around text enlcosed in <h1>
   and </h1>

   by Robert Platt
*/
void
headerTagSubstitution (struct eventState *pes)
{
  long newlen, headerlength, difference;
  char *opentag, *afteropentag, *closetag;
  int ignore;

  while (1)
    {
      char *p, *q;		/* temporary-use pointers */

      difference = 0;
      ignore = 0;

      opentag = strstr (pes->text_buf, "<h1");
      if (opentag == 0)
	break;

      afteropentag = strchr (opentag, '>');
      if (afteropentag == 0)
	break;

      closetag = strstr (afteropentag, "</h1");
      if (closetag == 0)
	break;
      /* find any tags etc within the header: */
      *closetag = 0;

      q = afteropentag + 1;	/* modified for 5.1.2 - RP */
      while (1)
	{
	  p = strchr (q, '<');
	  if (p == 0)
	    break;
	  q = strchr (p, '>');
	  if (q == 0)
	    break;

	  ignore += q - p + 1;
	}
      /* now ampersands: */
      while (1)
	{
	  char *q = afteropentag + 1;
	  p = strchr (q, '&');
	  if (p == 0)
	    break;
	  q = strchr (p, ';');

	  ignore += q - p;
	}
      *closetag = '<';
      difference = closetag - pes->text_buf;

      if (afteropentag - opentag >= 3)
	{
	  /* go back far enough before the '>' to write a line break: */
	  p = afteropentag - 3;
	  strncpy (p, "<br", 3);
	  /* then put blank spaces before that */
	  while (p-- > opentag)
	    *p = ' ';
	}

      /* How many characters wide is the contents of the header? */
      headerlength = closetag - afteropentag - ignore;
      newlen = strlen (pes->text_buf) + headerlength + 10;

      if (pesResizeCurrentTextbuf (newlen))
	{
	  long i;

	  /* the address of pes->text_buf may have changed. So use searchpos
	     to calculate the position of the closing </h1> tag, and update
	     closetag.

	     Relocate the text that is located after the header, so as
	     not to overwrite it with the border that is put around the
	     header. */
	  p = pes->text_buf + newlen - headerlength - 8;	/* modified for 5.1.2 - RP */
	  closetag = pes->text_buf + difference;
	  while (--p >= closetag)
	    p[headerlength + 7] = *p;

	  /* Now create the border */
	  strncpy (pes->text_buf + difference, " Ü<br> ", 7);
	  difference += 7;
	  for (i = 1; i <= headerlength; i++)
	    pes->text_buf[difference++] = 'ß';
	}
    }
}


/*	preformatTrim removes any unwanted whitespace from HTML.
        wordwrap should then be called to ensure that no line of text
        exceeds the screen width.

        Also removes comments.

	by Robert Platt
*/
void
preformatTrim (char *text)
{
  char *tptr, *tptrtemp;
  char previous_char;

  previous_char = 0;

  /* Blank out comments */
  while ((tptr = strstr (text, "<!--")) != 0)
    {
      tptrtemp = strstr (tptr, "-->");

      if (tptrtemp)
	{
	  tptrtemp += 2;	/* go to end of --> */
	  *tptrtemp = '\0';
	  strset (tptr, ' ');	/* blank out comment */
	  *tptrtemp = ' ';
	  tptr = tptrtemp;	/* continue search at end of comment */
	}
      else
	break;
    }

  tptr = strstr (text, "<body");
  if (tptr == 0)
    return;

  while (*tptr != 0)
    {
      /* Leave preformatted text in the <pre> tag */
      if (strncmp (tptr, "<pre", 4) == 0)
	{
	  tptr = strstr (tptr, "</pre");
	  if (tptr == 0)
	    return;
	}

      switch (*tptr)
	{
	case '\r':
	case '\t':
	case '\n':
	  *tptr = ' ';

	case ' ':
	  /* If there is already one space then copy
	     the rest of the text over rather than
	     have another space */
	  if (previous_char == ' ')
	    {
	      /* memmove is faster - RP */
	      memmove (tptr, tptr + 1, strlen (tptr));
	      --tptr;		/* Easy way of cancelling out the ++tptr which is not
				   needed since we've copied the next character into
				   this one */
	    }
	}
      previous_char = *tptr;
      ++tptr;
    }
}

void
wordwrap (char *text)
{
  /* This wraps any text that may not fit on one line. Hence preformatted
     text is not required anymore.

     It works by recording the last space found whilst checking a particular
     line of text. The length of line is stored in i. i is not incremented
     whilst reading through tags, and it is reset whenever any preformatted
     line breaks are found.

     Format tags such as <br> and <p>, and any newline characters outside
     of <pre> preformatted text, will already have been dealt with by
     simpleTagSubstitutions() and preformatTrim() - RP
   */
  int i, intag, inentity;
  char *tptr, *lastspace;

  tptr = strstr (text, "<body");
  if (tptr == 0)
    tptr = text;

  intag = 0;
  inentity = 0;

  while (1)
    {
      i = 0;
      lastspace = 0;

      while (i <= LEN)
	{
	  switch (*tptr)
	    {
	    case 0:
	      return;

	    case '<':
	      intag = 1;
	      break;

	    case '>':
	      --i;		/* do not count this character as part of the visible text */
	      intag = 0;
	      break;

	    case '&':
	      if (strncmp (tptr, "&copy;", 6) == 0)
		i += 3;		/* (c) */
	      else
		i += 1;
	      inentity = 1;
	      break;

	    case ';':
	      --i;
	      inentity = 0;
	      break;

	    case '\n':
	      lastspace = 0;
	      i = -1;		/* d not count this character as part of the visible text */
	      break;

	    case '\r':
	      *tptr = ' ';
	    case ' ':
	      if (!intag && !inentity)
		lastspace = tptr;
	      break;
	    }

	  ++tptr;
	  if (!intag && !inentity)
	    ++i;
	}

      if (lastspace != 0)
	{
	  *lastspace = '\n';
	  tptr = lastspace + 1;
	}
    }
}


void
sensiblebreaks (char *text)
{
  /* Removes excessive '\n's.
     So

     "\n\n\n" becomes "\n \n", and
     "\n\n\n\n" becomes "\n  \n", etc

     tags between these whitespaces are skipped.
     -RP */

  char *tptr, *tptrtemp, *lastnewline;
  int newlines;

  tptr = strstr (text, "<body");
  if (tptr == 0)
    return;

  while (*tptr != 0)
    if (*(tptr++) == '\n')
      {
	newlines = 1;
	tptrtemp = tptr;
	while (*tptr == '\n' || *tptr == '\r' || *tptr == ' ' || *tptr == '<')
	  {
	    if (*tptr == '<')
	      {
		tptr = strchr (tptr, '>');
		if (tptr == 0)
		  return;
	      }
	    else if (*tptr == '\n')
	      {
		lastnewline = tptr;
		++newlines;
	      }

	    ++tptr;
	  }

	if (newlines > 2)
	  {
	    while (tptrtemp < lastnewline)
	      {
		if (*tptrtemp == '<')
		  {
		    tptrtemp = strchr (tptrtemp, '>');
		    if (tptrtemp == 0)
		      return;
		    else
		      ++tptrtemp;
		  }
		else
		  *(tptrtemp++) = ' ';
	      }
	  }
      }
  /* end while */
}


/* Substitute &-escape codes. */
int ampSubs (struct eventState *pes)
{
  int ch;

  ch = ampSubsNCR(pes);
  if (ch != -1)
     return ch;

  ch = ampSubsInternal(pes, unicodeEntityTable, unicode);
  if (ch != -1)
     return ch;

  ch = ampSubsInternal(pes, asciiEntityTable, ascii);
  if (ch != -1)
     return ch;

  if (strnicmp (pes->p, "&copy;", 6) == 0 && pes->body_end - pes->p > 6)
  {
      /* copyright symbol. added by RP
         Note the dummy tag <!> which is there so that these three
         characters will be skipped next time displayProcess comes across it
       */
      strncpy (pes->p, "<!>(c)", 6);
      ch = '(';
      pes->p += 3;
      return ch;
  }

  return '&';
}

int ampSubsNCR (struct eventState *pes)
{
   if (pes->p[1] == '#')
   {
      unsigned long unicode;
      char *endchar;
      char ch;
      if (pes->p[2] == 'x' || pes->p[2] == 'X')
         unicode = strtoul(pes->p+3, &endchar, 16);
      else
         unicode = strtoul(pes->p+2, &endchar, 10);

      if (unicode == 0)
         return -1;

      if (endchar == NULL)
         return -1;

      if (*endchar != ';')
         return -1;

      if (unicode <= 0x7F)
      {
         ch = unicode;
      }
      else
      {
         ch = unicodeToCodepage(unicode);
         if (ch == '?')
            return -1;
      }

      pes->p = endchar;
      return ch;
   }

   return -1;
}


int
ampSubsInternal (struct eventState *pes,
                 struct ampCharEntryStruct *table,
                 enum entityTableType type)
{
  int i, lenbefore;

  if (table == NULL)
     return -1;

  for (i = 0; table[i].before != 0; i++)
    {
      lenbefore = strlen (table[i].before);
      if ((strncmp (pes->p, table[i].before, lenbefore) == 0)
	  && pes->body_end - pes->p > lenbefore)
	{
     int ch;

     if (type == ascii)
       ch = table[i].after;
     else
       ch = unicodeToCodepage(table[i].after);

     if (ch != '?')
     {
	     pes->p += lenbefore - 1;
        return ch;
     }
     else return -1;
	}
    }

  return -1;
}


/* link_length: Determines the length of the link

   Adapted from Joe Cosentino's code by Robert Platt
*/

int
link_length (char *link_text)
{
  char *search;
  int l;

  if ((search = strchr (link_text, '\"')) != 0)
    l = search - link_text;
  else if ((search = strchr (link_text, ' ')) != 0)
    l = search - link_text;
  else
    l = strlen (link_text);
  /* restriction that link had to be less than LEN-12 has been lifted
     to 257 (half size of pes->text_holder) */
  if (l < 0 || l > 257)
    l = 257;

  return l;
}

/* stristr is taken from my WHATIS program - Rob Platt */
char *
stristr (const char *s1, const char *s2)
{
  const char *retvalue = NULL;
  char *slwr1 = NULL, *slwr2 = NULL;
  char *searchresult = NULL;

  slwr1 = (char *) malloc (strlen (s1) + 1);
  if (slwr1 == NULL)
    return NULL;
  strcpy (slwr1, s1);
  strlwr (slwr1);

  slwr2 = (char *) malloc (strlen (s2) + 1);
  if (slwr2 == NULL)
    return NULL;
  strcpy (slwr2, s2);
  strlwr (slwr2);

  searchresult = strstr (slwr1, slwr2);
  if (searchresult)
    retvalue = s1 + (searchresult - slwr1);
  else
    retvalue = 0;

  free (slwr1);
  free (slwr2);

  return (char *) retvalue;
}
