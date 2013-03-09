/*  Browses the HTML

    Copyright (c) Express Software 1998-2002
    See doc/htmlhelp/copying for licensting terms.
*/

#ifndef HELP_HTM_H_INCLUDED
#define HELP_HTM_H_INCLUDED

#include "pes.h"


/* Function Definitions */
void
html_view (const char *target,
	   char *base_dir, char *home_page, const char *help_page);
int html_view_internal (struct eventState *pes,
			char *base_dir, char *home_page);
void displayProcess (struct eventState *pes);
char prepare_link (struct eventState *pes);
int isFileLink (const char *string);
int checkAboveTop (struct eventState *pes, const char *openingtag,
		   const char *closingtag);


#endif
