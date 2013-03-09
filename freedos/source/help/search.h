#ifndef HTMLHELP_SEARCH_H_INCLUDED
#define HTMLHELP_SEARCH_H_INCLUDED

#include "pes.h"

#define HTMLHELP_INTERNAL_SEARCHLINK "__HTMLHELP_INTERNAL_SEARCHLINK_OFFSET__"
#define SEARCH_TEMPFILE              "hh$earch.htm"


#define SEARCH_FOR_ONE_ONLY 1
#define SEARCH_FOR_ALL      0

char *searchtablealloc (void);
void searchtablefree (char *searchtable);
int searchtablecheck (char *searchtable, char *key);
void searchresultprintf (struct eventState *pes, const char *format, ...);
int searchafile (const struct eventState *pes, char *searchterm,
		 int searchtype, struct eventState *resultspes,
		 int results_so_far, int oneorall, int basedirlen);

struct eventState *helpsearch (const struct eventState *pes,
			       char *base_dir, const char *home_page);

#endif
