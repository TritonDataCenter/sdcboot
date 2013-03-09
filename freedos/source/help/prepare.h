/*  Prepares HTML files for reading

    Copyright (c) Express Software 1998-2002
    See doc/htmlhelp/copying for licensing terms.
*/

#ifndef PREPARE_H_INCLUDED
#define PREPARE_H_INCLUDED

#include "pes.h"

#define HTMLHELP_INTERNAL_SEARCHLINK "__HTMLHELP_INTERNAL_SEARCHLINK_OFFSET__"
#define SEARCH_TEMPFILE              "hhlpsrch.$$$"

#define SEARCH_FOR_ONE_ONLY 1
#define SEARCH_FOR_ALL      0

/* Function Definitions */
void preparepes (struct eventState *pes);
void searchpreparepes (struct eventState *pes);
void preparebodyinfo (struct eventState *pes);
int isUTF8html(unsigned char *string);

#endif
