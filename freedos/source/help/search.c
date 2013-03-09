#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <ctype.h>
#include "conio.h"
#include "help_htm.h"
#include "parse.h"
#include "search.h"
#include "catdefs.h"

int tablesize = 0;

char *
searchtablealloc (void)
{
  char *searchtable;
  tablesize = 1024;

  while ((searchtable = malloc (tablesize)) == NULL)
    {
      tablesize /= 2;
      if (tablesize < 30)
	{
	  tablesize = 0;
	  return NULL;
	}
    }

  return searchtable;
}

void
searchtablefree (char *searchtable)
{
  free (searchtable);
  tablesize = 0;
}

int
searchtablecheck (char *searchtable, char *key)
{
  char fullkey[258];

  strcpy (fullkey, key);
  strcat (fullkey, ",");

  /* have we seen this file already? */
  if (strstr (searchtable, fullkey))
    return 0;

  /* if not, add it to the table, coz we're about too... */
  if (strlen (fullkey) + strlen (searchtable) < tablesize)
    strcat (searchtable, fullkey);

  return 1;
}


void
searchprintf (struct eventState *pes, const char *format, ...)
{
  va_list *vlist;
  size_t currentsize = strlen (pes->text_buf);
  if (currentsize > 64000U)
    return;

  if (pesResizeTextbuf (pes, currentsize + 1024) == NULL)
    return;

  va_start (vlist, format);
  vsprintf (pes->text_buf + currentsize, format, vlist);
}


int
searchafile (const struct eventState *pes, char *searchterm,
	     int searchtype, struct eventState *resultspes,
	     int results_so_far, int oneorall, int basedirlen)
{
  char *searchpos = pes->body_start;
  char *lasttag = searchpos;
  char *nexttag = NULL;
  int result = results_so_far;

  nexttag = strchr (searchpos, '<');
  do
    {
      if (nexttag != NULL)
	*nexttag = '\0';

      if (searchtype & 2)
	searchpos = strstr (searchpos, searchterm);
      else
	searchpos = stristr (searchpos, searchterm);

      while ((searchpos != NULL) && searchpos <= pes->body_end)
	{
	  char backupchar = *searchpos;

	  if (searchtype & 8)
	    {
	      if (searchpos > pes->body_start)
		{
		  char chbefore = *(searchpos - 1);
		  char chafter = *(searchpos + strlen (searchterm));

		  if (isalpha (chbefore) || isdigit (chbefore) ||
		      isalpha (chafter) || isdigit (chafter))
		    {
		      searchpos++;
		      if (searchtype & 2)
			searchpos = strstr (searchpos, searchterm);
		      else
			searchpos = stristr (searchpos, searchterm);
		      continue;
		    }
		}
	    }

	  ++result;
	  if (searchpos - lasttag > 50)
	    {
	      lasttag = strchr (searchpos - 50, ' ');
	      if (lasttag == NULL)
		lasttag = searchpos - 50;
	    }
	  *searchpos = 0;
	  searchprintf (resultspes, "%i. <a href=\"%s#%s%li\">%s</a><br>%s",
			result,
			pes->filename + basedirlen,
			HTMLHELP_INTERNAL_SEARCHLINK,
			(long) (searchpos - pes->body_start),
			pes->filename + basedirlen, lasttag);
	  *searchpos = backupchar;
	  backupchar = searchpos[strlen (searchterm)];
	  searchpos[strlen (searchterm)] = '\0';
	  searchprintf (resultspes, "<b>%s</b>", searchpos);
	  searchpos[strlen (searchterm)] = backupchar;
	  searchprintf (resultspes, "%.250s<br><br>",
			searchpos + strlen (searchterm));

	  ++searchpos;
	  lasttag = strchr (searchpos, ' ');
	  if (lasttag == NULL)
	    lasttag = searchpos;

	  if (searchtype & 2)
	    searchpos = strstr (searchpos, searchterm);
	  else
	    searchpos = stristr (searchpos, searchterm);

	  if (oneorall == SEARCH_FOR_ONE_ONLY)
	    {
	      /* Done. Only wanted one occurance */
	      if (nexttag != NULL)
		*nexttag = '<';
	      return result;
	    }
	}

      if (nexttag != NULL)
	{
	  *nexttag = '<';
	  searchpos = strchr (nexttag, '>');
	}

      if (searchpos == NULL)
	break;
      ++searchpos;
      lasttag = searchpos;
    }
  while ((nexttag = strchr (searchpos, '<')) != NULL);

  return result;
}


struct eventState *
helpsearch (const struct eventState *pes,
	    char *base_dir, const char *home_page)
{
  char searchterm[20];
  int searchtype;
  int basedirlen = strlen (base_dir);

  searchtype = searchbox (searchterm);
  if (searchtype)
    {
      int results = 0;
      char tfilename[257];
      char sourcefilename[257];
      int openedhomepage = 0;
      struct eventState *resultspes;
      const struct eventState *sourcepes;

      if (*searchterm == 0)
	return NULL;

      strcpy (tfilename, base_dir);
      if (tfilename[strlen (tfilename) - 1] != '\\' &&
	  tfilename[strlen (tfilename) - 1] != '/')
	strcat (tfilename, "\\");
      strcat (tfilename, SEARCH_TEMPFILE);

      if ((stricmp (tfilename, pes->filename) == 0) && !(searchtype & 4))
	{
	  show_error (hcatCantSrchSrch);
	  return NULL;
	}

      resultspes = pesListAdd2 (512, tfilename);

      if (resultspes == NULL)
	{
	  show_error (hcatMemErr);
	  return NULL;
	}

      if (searchtype & 4)
	{
	  char openhomepage[257];
	  strcpy (openhomepage, base_dir);
	  strcat (openhomepage, home_page);
	  if (stricmp (openhomepage, pes->filename))
	    {
	      struct eventState *newpes;
	      if ((newpes = pesListAdd (openhomepage, home_page)) != NULL)
		{
		  pes = newpes;
		  searchpreparepes (pes);
		  openedhomepage = 1;
		}
	    }
	}

      searchprintf (resultspes,
		    "<html><head><title>%s</title></head><body>"
		    "<h1>%s</h1>", hcatSearchResults, hcatSearchResults);
      if (searchtype & 4)
	searchprintf (resultspes, hcatSearchFileSet);
      else
	searchprintf (resultspes, "%s <b>%s</b>", hcatSearched,
		      pes->filename + strlen (base_dir));
      searchprintf (resultspes, " %s: <b>%s</b><br><br>", hcatSearchFor, searchterm);

      sourcepes = pes;

      results =
	searchafile (pes, searchterm, searchtype, resultspes, results,
		     SEARCH_FOR_ALL, basedirlen);

      if (searchtype & 4)
	{
	  static struct event ev;
	  char *findlink;
	  char *table;

	  statuswithesc (hcatStatusSearching);

	  findlink = pes->body_start;
	  table = searchtablealloc ();
	  do
	    {
	      findlink = strstr (findlink, "<a");

	      get_event (&ev, EV_KEY | EV_NONBLOCK);
	      if (ev.ev_type == EV_KEY && ev.key == 27)
		{
		  searchprintf (resultspes,
				"<br><b>*** %s ***</b><br><br>", hcatUserAborted);
		  findlink = NULL;
		}

	      if (findlink)
		{
		  findlink = strstr (findlink, "href=");
		  if (findlink)
		    {
		      findlink += 5;
		      if (*findlink == '\"')
			findlink++;

		      if (*findlink == '#')
			break;

		      if (isFileLink (findlink))
			{
			  char searchtemp[257];
			  char *hashchar;
			  sprintf (searchtemp, "%.*s", link_length (findlink),
				   findlink);
			  if ((hashchar = strchr (searchtemp, '#')) != NULL)
			    *hashchar = 0;
			  if (searchtablecheck (table, searchtemp))
			    {
			      char searchthis[257];
			      sprintf (searchthis, "%s%s", base_dir,
				       searchtemp);

			      if ((pes =
				   pesListAdd (searchthis, home_page)) != 0)
				{
				  searchpreparepes (pes);
				  results =
				    searchafile (pes, searchterm, searchtype,
						 resultspes, results,
						 SEARCH_FOR_ONE_ONLY,
						 basedirlen);
				  pesListBackward ();
				}
			      pes = sourcepes;
			    }
			}
		    }
		}
	    }
	  while (findlink);
	  statusbar (NULL);
	  searchtablefree (table);
	}

      if (openedhomepage)
	pesListBackward ();

      if (results == 0)
	searchprintf (resultspes, "<b>%s.</b>", hcatNoResults);

      searchprintf (resultspes, "</body></html>");

      return resultspes;
    }
  return NULL;
}
