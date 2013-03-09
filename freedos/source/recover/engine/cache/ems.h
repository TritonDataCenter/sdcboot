/* +++Date last modified: 05-Jul-1997 */
/* +++Date last modified(Imre): 18-Nov-2002 */

/*
** EMS.H
**
** Header file Expanded Memory Routines
*/

#ifndef EMS_H_
#define EMS_H_

#define EMS_PAGE_SIZE   16384   /* Each page is this size */

unsigned  EMSbaseaddress(void);
int       EMSversion(void);
int       EMSstatus(void);
unsigned  EMSpages(void);
int       EMSalloc(unsigned pages);
int       EMSfree(int handle);
int       EMSmap(int bank, int handle, unsigned page);

#endif
