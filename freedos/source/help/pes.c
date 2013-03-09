/*  FreeDOS HTML Help Viewer

    PES - Memory Manager for PES data structures

    Copyright (c) Express Software 2002-3
    See doc\htmlhelp\copying for licensing terms
    This file created by: Robert Platt.
*/

#include <string.h>
#include <malloc.h>
#include "pes.h"
#include "help_gui.h"
#include "catdefs.h"
#include "readfile.h"

struct eventState **thePesList;	/* list of pointers to pes structs */
long pesListSize = 0;		/* size of the list */
long pesListCurr = 0;		/* current position in the list */
long pesListBottom = 0;

int
pesListInitialise (void)
{
  pesListSize = 0;
  pesListCurr = -1;
  pesListBottom = 0;

  thePesList = (struct eventState **) malloc (sizeof (struct eventState *));
  if (thePesList == NULL)
    {
      show_error (hcatMemErr);
      return FAIL;
    }

  return SUCCESS;
}

struct eventState *
pesListAdd (const char *fullpath, const char *homepage)
{
  struct eventState *newpes;
  struct eventState **thePesListResized;

  pesListDeleteForwards ();

  /* Create a new process state structure */
  /* delete old ones if need be */
  while ((newpes = (struct eventState *) malloc (sizeof (struct eventState)))
	 == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	return NULL;
      }

  /* Create a buffer filled with the text */
  /* delete old pes structures if necessary */
  if ((newpes->text_buf = readfile (fullpath, homepage)) == NULL)
    {
      free (newpes);
      return NULL;
    }

  /* Create a buffer for screen information */
  /* delete old pes structures if necessary */
  /* The 512 bytes are for pes->text_holder */
  while ((newpes->screen_buf = (char *) malloc (H * W * 2 + 512)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	free (newpes->text_buf);
	free (newpes);
	return NULL;
      }

  while ((newpes->filename = (char *) malloc (strlen (fullpath) + 1)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	free (newpes->screen_buf);
	free (newpes->text_buf);
	free (newpes);
	return NULL;
      }
  strcpy (newpes->filename, fullpath);

  initBookmarkList(newpes);

  while ((thePesListResized = (struct eventState **) realloc (thePesList,
							      sizeof (struct
								      eventState
								      *) *
							      (pesListSize +
							       1))) == NULL)
    if (pesListDeleteOldest ())
      {
	pesDelete (newpes);
	return NULL;
      }

  thePesList = thePesListResized;

  /* update pes list information */
  ++pesListCurr;
  ++pesListSize;
  return (thePesList[pesListCurr] = newpes);
}


struct eventState *
pesListAdd2 (int space, const char *fullpath)
{
  struct eventState *newpes;
  struct eventState **thePesListResized;

  pesListDeleteForwards ();

  /* Create a new process state structure */
  /* delete old ones if need be */
  while ((newpes = (struct eventState *) malloc (sizeof (struct eventState)))
	 == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	return NULL;
      }

  /* Create a buffer with a basic amount of space */
  /* delete old pes structures if necessary */
  if ((newpes->text_buf = malloc (space)) == NULL)
    {
      free (newpes);
      return NULL;
    }
  *(newpes->text_buf) = '\0';

  /* Create a buffer for screen information */
  /* delete old pes structures if necessary */
  /* The 512 bytes are for pes->text_holder */
  while ((newpes->screen_buf = (char *) malloc (H * W * 2 + 512)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	free (newpes->text_buf);
	free (newpes);
	return NULL;
      }

  while ((newpes->filename = (char *) malloc (strlen (fullpath) + 1)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	free (newpes->screen_buf);
	free (newpes->text_buf);
	free (newpes);
	return NULL;
      }
  strcpy (newpes->filename, fullpath);

  initBookmarkList(newpes);

  while ((thePesListResized = (struct eventState **) realloc (thePesList,
							      sizeof (struct
								      eventState
								      *) *
							      (pesListSize +
							       1))) == NULL)
    if (pesListDeleteOldest ())
      {
	pesDelete (newpes);
	return NULL;
      }

  thePesList = thePesListResized;

  /* update pes list information */
  ++pesListCurr;
  ++pesListSize;
  return (thePesList[pesListCurr] = newpes);
}



struct eventState *
pesListBackward (void)
{
  --pesListCurr;

  if (pesListCurr < pesListBottom)
    {
      pesListCurr = -1;
      return NULL;
    }

  return thePesList[pesListCurr];
}


struct eventState *
pesListForward (void)
{
  if (pesListCurr == pesListSize - 1)
    return NULL;

  return thePesList[++pesListCurr];
}

struct eventState *
pesListCurrent (void)
{
  if (pesListCurr < pesListBottom || pesListSize <= 0)
    return NULL;

  return thePesList[pesListCurr];
}

struct eventState *
pesResizeCurrentTextbuf (long newSize)
{
  if (pesListSize <= 0)
    return NULL;

  return pesResizeTextbuf (thePesList[pesListCurr], newSize);
}


struct eventState *
pesResizeTextbuf (struct eventState *pes, long newSize)
{
  char *newbuf;

  if (newSize == 0)
    return pes;

  if (newSize >= 0xFFFFU) /* Jun 2004 */
  {
    show_error (hcatMemErr);
    return NULL;
  }

  while ((newbuf = (char *) realloc (pes->text_buf, newSize)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	return NULL;
      }

  pes->text_buf = newbuf;
  return pes;
}


void
pesListDestroy (void)
{
  int i;

  for (i = pesListBottom; i < pesListSize; i++)
    pesDelete (thePesList[i]);

  free (thePesList);
}

int
pesListDeleteOldest (void)
{
/* Semi-infinite browsing only
   at the moment. After a very
   long chain of browsing, there
   will be no more memory
   available. The true-infinite
   code, that will delete old
   files on the list completely,
   doesn't work yet - RP */

#define FUDGE_PES
#ifdef FUDGE_PES
  if (pesListBottom < pesListCurr)
    {
      pesDelete (thePesList[pesListBottom]);
      pesListBottom++;
      return SUCCESS;
    }
  return FAIL;
#else
  struct eventState **thePesListResized;
  int i;


  /* Shift the remaining pes's back so that
     the array begins at thePesList[0] */
  for (i = 1; i < pesListSize; i++)
    thePesList[i - 1] = thePesList[i];

  /* there is one less pes */
  --pesListSize;
  /* and the current pes is now less one in the list */
  --pesListCurr;

  /* Resize the list of pointers to the pes's to clear up the unused space */
  if ((thePesListResized = (struct eventState **) realloc (thePesList,
							   sizeof (struct
								   eventState
								   *) *
							   (pesListSize))) !=
      NULL)
    thePesList = thePesListResized;

  return SUCCESS;
#endif
}

void
pesListDeleteForwards (void)
{
  long i;
  struct eventState **thePesListResized;
  long newPesListSize = pesListSize;

  /* Delete any pes's that are after the current pes */
  for (i = pesListCurr + 1; i < pesListSize; i++)
    {
      pesDelete (thePesList[i]);
      --newPesListSize;
    }

  if (pesListSize != newPesListSize)
    {
      /* Can't reallocate to zero, so just reinitialise
         This can happen if pesListDeleteForwards is
         called with pesListCurr = -1 */
      if (newPesListSize == 0)
	{
	  pesListSize = 0;
	  pesListDestroy ();
	  pesListInitialise ();
	  return;
	}

      if ((thePesListResized = (struct eventState **) realloc (thePesList,
							       sizeof (struct
								       eventState
								       *) *
							       (newPesListSize)))
	  != NULL)
	{
	  thePesList = thePesListResized;
	  pesListSize = newPesListSize;
	}
      else
	show_error (hcatResizeFail);
    }
}

void
pesDelete (struct eventState *pes)
{
  if (pes == NULL)
    return;

  uninitBookmarkList(pes);
  free (pes->screen_buf);
  free (pes->text_buf);
  free (pes->filename);
  free (pes);
}


/* Bookmark browsing support */
void initBookmarkList (struct eventState *pes)
{
   pes->bookmarklist = NULL;
   pes->bookmarksize = 0;
   pes->bookmarkpos = -1;
}


void uninitBookmarkList (struct eventState *pes)
{
   if (pes->bookmarklist != NULL)
      free(pes->bookmarklist);
}


void addBookmark (struct eventState *pes)
{
   if (pes->bookmarklist == NULL)
   {
      if ((pes->bookmarklist = (char**)malloc(sizeof(char *))) != NULL)
      {
         pes->bookmarklist[0] = pes->top;
         pes->bookmarksize++;
         pes->bookmarkpos++;
      }
   }
   else
   {
      pes->bookmarkpos++;

      if (pes->bookmarksize != pes->bookmarkpos+1)
      {
         pes->bookmarksize = pes->bookmarkpos+1;
         pes->bookmarklist = (char **) realloc (pes->bookmarklist,
							      sizeof(char*) * pes->bookmarksize);
      }

      if (pes->bookmarklist)
      {
         pes->bookmarklist[pes->bookmarkpos] = pes->top;
      }
   }
}


char *backBookmark (struct eventState *pes)
{
   if (pes->bookmarkpos < 0 || pes->bookmarklist == NULL)
      return NULL;
   else
   {
      char *retvalue = pes->bookmarklist[pes->bookmarkpos];
      /* store for going forward */
      pes->bookmarklist[pes->bookmarkpos] = pes->top;
      pes->bookmarkpos--;
      return retvalue;
   }
}


char *forwardBookmark (struct eventState *pes)
{
   if (pes->bookmarkpos == pes->bookmarksize-1)
   {
      return NULL;
   }
   else
   {
      char *retvalue = pes->bookmarklist[++pes->bookmarkpos];
      /* store for going forward */
      pes->bookmarklist[pes->bookmarkpos] = pes->top;
      return retvalue;
   }
}