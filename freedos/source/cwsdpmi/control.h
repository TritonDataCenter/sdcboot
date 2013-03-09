/* Copyright (C) 1995-2009 CW Sandmann (cwsdpmi@earthlink.net) 1206 Braelinn, Sugar Land, TX 77479
** Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
**
** This file is distributed under the terms listed in the document
** "copying.cws", available from CW Sandmann at the address above.
** A copy of "copying.cws" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.cws".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#if 1
#define DEBUG(a,b)
#else
#define DEBUG(a,b) {\
  FILE *f = fopen ("con", "w"); \
  fprintf (f, a, b); \
  fprintf (f, "<wait>"); \
  fflush (f); \
  bioskey (0); \
  fprintf (f, "\r      \r"); \
  fclose (f); }
#endif

#ifdef DIAGNOSE
#define SHOW_MEM_INFO(a,b) errmsg(a,b);
#else
#define SHOW_MEM_INFO(a,b) 
#endif

/* Also in "unload.asm".  */
#define ONE_PASS_MAGIC 0x69151151L

extern word8 vcpi_installed;
extern word8 use_xms;
extern word8 mtype;
#define PC98 1			/* Machine type */

void errmsg(char *fmt, ...);
void cleanup(int exitcode);
void do_faulting_finish_message(void);
word16 get_pid(void);
void set_pid(word16 pid);
void memsetf(word16 offset, word8 value, word16 size, word16 seg);

typedef struct {
  char   magic[8];	/* Must contain CWSPBLK\0 */
  char   swapname[48];	/* Must have disk and directory, empty for no paging */
  word16 flags;		/* Bit flags */
  word16 pagedir;	/* Default 0 (auto), one per 4Mb */
  word16 minapp;	/* PAGES of free extended memory; paging in 1Mb area */
  word16 savepar;	/* PARAGRAPHS DOS memory to save if paging in 1Mb */
  word32 maxdblock;	/* Maximum 4K pages to swap to disk */
} CWSDPMI_pblk;

extern CWSDPMI_pblk CWSpar;

#define CWSFLAG_NOUMB (word8)CWSpar.flags&1	/* Don't use UMB blocks */
#define CWSFLAG_EARLY (word8)CWSpar.flags&2	/* Pre-Allocate PT memory */
#define CWSFLAG_NOEXT (word8)CWSpar.flags&4	/* No 1.0 Extensions */
#define CWSFLAG_NO4M  (CWSpar.flags&8)		/* No 4MB pages */
