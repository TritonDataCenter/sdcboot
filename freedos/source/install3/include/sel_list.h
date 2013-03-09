#ifndef _SEL_LIST_H
#define _SEL_LIST_H

/* Function prototypes */

int select_list (int optc, char *optv[]);
/* Note: yesToAll and noToAll may be NULL to not display those prompts */
int select_yn (char *prompt, char *yes, char *no, char *yesToAll, char *noToAll);

#endif /* _SEL_LIST_H */
