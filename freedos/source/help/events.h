#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "pes.h"

int processEvents (struct eventState *pes);

void letterkey (struct eventState *pes, char letter);
void tabkey (struct eventState *pes);
void shifttabkey (struct eventState *pes);
void downkey (struct eventState *pes);
void upkey (struct eventState *pes);


void gotolink (struct eventState *pes);
int linkcharmatch (char *clink, char letter);

char *skip2link (char *line, int count);

/* Which link should be displayed on status bar?*/
#define LP_KEYBOARD 1
#define LP_MOUSE    0

#define NAVIGATE_DONT 0
#define NAVIGATE_LINK 1
#define NAVIGATE_BACK 2
#define NAVIGATE_FORWARD 3
#define NAVIGATE_HOME 4
#define NAVIGATE_HELP 5
#define NAVIGATE_EXIT 6
#define NAVIGATE_REFRESH 7
#define NAVIGATE_SEARCH 8


/* Mouse Position */
#ifdef EVENTS_C
char mline;
char mpos;
#else
extern char mline;
extern char mpos;
#endif

#endif
