/*  memory management for HTML Help Browser

    Copyright (c) Express Software 2002
    All Rights Reserved.

    This file created by: Robert Platt.
*/

#ifndef PES_H_INCLUDED
#define PES_H_INCLUDED

struct eventState
{
  /* -- File ---- */
  char *filename;
  char *text_buf;

  /* -- Event --- */
  char enable_timer;
  char force_redraw;

  char *p;
  char *body_start;
  char *body_end;

  char *top;
  char *bottom;
  char *clink;			/* BS */

  char *seek_base;
  int seek_cnt;
  char **bookmarklist;
  int bookmarksize;
  int bookmarkpos;

  char *link_under_mouse;
  char link_priority;		/* RP */
  char check_mouse;
  char left_was_pressed;

  char barpos;
  char forced_barpos;
  char old_barpos;
  char bar_hooked;

  /* --- Display --- */
  char *screen_buf;

  char *old_top;
  long maxtop;

  /* --- Process and navigation --- */
  char *text_holder;
  char *link_text;
  char first_time;
  char hidden;
};

int pesListInitialise (void);
struct eventState *pesListAdd (const char *fullpath, const char *homepage);
struct eventState *pesListAdd2 (int space, const char *fullpath);
struct eventState *pesListBackward (void);
struct eventState *pesListForward (void);
struct eventState *pesListCurrent (void);
struct eventState *pesResizeCurrentTextbuf (long newSize);
struct eventState *pesResizeTextbuf (struct eventState *pes, long newSize);
void pesListDestroy (void);

/* Used internally by pes.c: */
void pesListDeleteForwards (void);
int pesListDeleteOldest (void);
void pesDelete (struct eventState *pes);

/* Bookmark browsing support */
void initBookmarkList (struct eventState *pes);
void uninitBookmarkList (struct eventState *pes);
void addBookmark (struct eventState *pes);
char *backBookmark (struct eventState *pes);
char *forwardBookmark (struct eventState *pes);


#define FAIL 1
#define SUCCESS 0

#endif
