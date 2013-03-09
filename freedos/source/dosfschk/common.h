/* common.h  -  Common functions AND includes */

/* Written 1993 by Werner Almesberger / includes 2004 by Eric Auer */


#ifndef _COMMON_H
#define _COMMON_H

#ifndef __DJGPP__
#define _LINUX_STAT_H           /* avoid inclusion of <linux/stat.h>   */
#define _LINUX_STRING_H_        /* avoid inclusion of <linux/string.h> */
#define _LINUX_FS_H             /* avoid inclusion of <linux/fs.h>     */
#include <linux/msdos_fs.h>
#else
#include "inc/msdos_fs.h"	/* local copy for DOS version */
#include "inc/types.h"		/* extra __{u,s}{8,16,32,64} DOS typedefs */
#endif

void showalloc(void); /* 2.11b: show memory usage info on stderr if verbose */

void die(char *msg,...) __attribute((noreturn));

/* Displays a prinf-style message and terminates the program. */

void pdie(char *msg,...) __attribute((noreturn));

/* Like die, but appends an error message according to the state of errno. */
/* Also log stats (2.11b) */

void *alloc(int size); /* Also log stats (2.11b) */

/* mallocs SIZE bytes and returns a pointer to the data. Terminates the program
   if malloc fails. */

void *qalloc(void **root,int size);

/* Like alloc, but registers the data area in a list described by ROOT. */

void qfree(void **root);

/* Deallocates all qalloc'ed data areas described by ROOT. */

void myfree(void *what, int size); /* Also log stats (2.11b) */

int min(int a,int b);

/* Returns the smaller integer value of a and b. */

char get_key(char *valid,char *prompt);

/* Displays PROMPT and waits for user input. Only characters in VALID are
   accepted. Terminates the program on EOF. Returns the character. */

#endif
