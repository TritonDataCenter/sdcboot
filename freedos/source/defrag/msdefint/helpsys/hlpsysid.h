#ifndef HELP_SYS_INDEX_H_
#define HELP_SYS_INDEX_H_

int  GetHelpIndex(void);

void RememberHelpIndex(int index);
int  PreviousHelpIndex(void);

/* Index of the topics help page. */
#define TOPICINDEX 0

#define SetTopicIndex SetHelpIndex(TOPICINDEX)

#endif
