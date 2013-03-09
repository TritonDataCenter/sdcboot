/* Copyright (C) 1995-1999 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
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

/* This type must be consistent with maxdblock of CWSDPMI_pblk in control.h */
#if 0
typedef unsigned short da_pn;
#define MAX_DBLOCK 0xffff
#else
typedef unsigned long da_pn;	/* Must be unsigned long for > 256Mb virtual */
#define MAX_DBLOCK 0xffffffffUL
#endif

#if run_ring != 0

void dalloc_file(char *swapname);
void dalloc_init(void);
void dalloc_uninit(void);

da_pn dalloc(void);
void dfree(da_pn);

/* These return pages */
da_pn dalloc_max_size(void);

void dwrite(word8 *buf, da_pn block);
void dread(word8 *buf, da_pn block);

#else

#define dalloc_init()
#define dalloc_uninit()

#define dalloc() 0
#define dfree(a)

#define dalloc_max_size() 0

#define dwrite(a, b)
#define dread(a, b)
#endif
