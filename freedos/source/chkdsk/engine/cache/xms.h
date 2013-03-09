/*
** XMS.H
**
** Header file for Extended Memory routines in XMS.ASM.
*/

#ifndef _XMS_H
#define _XMS_H

/* initialisation routine. */
int XMMinit(void);

/* routines for managing EMB's */
int  XMSinit(void);
int  XMSversion(void);
unsigned long XMScoreleft(void);
int  XMSfree(unsigned int handle);
/*long XMSmemcpy(unsigned int desthandle, unsigned long destoff,
               unsigned int srchandle, unsigned long srcoff, long n);*/
unsigned DOStoXMSmove(unsigned int desthandle, unsigned long destoff,
                  const char *src, unsigned n);
unsigned XMStoDOSmove(char *dest, unsigned int srchandle, unsigned long srcoff, unsigned n);

unsigned int XMSalloc(unsigned long size);

int XMSrealloc(unsigned int handle, unsigned long size);

/* routines for managing the HMA. */
int HMAalloc(void);
unsigned HMAcoreleft(void);
int HMAfree(void);


/* routines for managing UMB's. */
unsigned int UMBalloc(void);
unsigned int GetUMBsize(void);
int UMBfree (unsigned int segment);

#endif
