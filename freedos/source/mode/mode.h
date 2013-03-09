#ifndef MODE_H

#define MODE_H 42

#include <stdlib.h> /* getenv, putenv, exit */ 
#include <string.h> /* strlen */ 
#include <stdio.h>  /* sprintf, printf */ 
#include <dos.h>    /* MK_FP, _psp, int86, union REGS */ 
#include <ctype.h>  /* toupper isdigit isalpha */
 
/* #define DMODE 1 */	/* define for debug mode */ 
/* #define DMODE2 1 */	/* define for more verbose debug mode */ 
#define VMODE 1		/* define for slightly verbose mode */

#ifdef DMODE2
#define DMODE 1		/* verbose debug mode implies normal debug mode */
#endif

#define VERSION "12may2005"	/* *** displayed by modeopt.c in help() *** */

char * skipspace(char * str);
unsigned int grabarg(char * str, const char * label);
unsigned int posarg(char * str, int commas);
int xlatretry(char retry);
void help(void);

int printer (int pnum, char * what); /* pnum 0 means LPT1 */
int serial (int snum, char * what);  /* snum 0 means COM1 */

int set_vesa (int cols, int lines);
int set_lines (int lines);
int testCRTC (unsigned int port);

int console (char * what);
int screen (int mode, char * what);

unsigned int ShowStatus (void); /* codepage status */
unsigned int CodePageSelect (unsigned int cp);
unsigned int CodePagePrepare (char *bufferpos, char *filename);
unsigned int GetCodePage (void);

int RedirectLptCom(int pnum, int cnum); /* negative cnum stops redirect */
int SetRetry(int pnum, int xretry);     /* see xlatretry for values */
int DescribeTSR(int portnum);           /* tell about status of TSR */

#endif
