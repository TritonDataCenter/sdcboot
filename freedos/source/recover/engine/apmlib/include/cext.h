/**********************************************
** C-Ext                                     **
**                                           **
** Common extensions to C language           **
** ----------------------------------------- **
** Author: Aitor Santamaria - Cronos project **
** Created: 13th - June - 2000               **
** Distribute: GNU-GPL v. 2.0 or later       **
**             (see GPL.TXT)                 **
***********************************************/


#ifndef _Cr_CExt
#define _Cr_CExt


/* Common types */
#define BYTE unsigned char
#define WORD unsigned int

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef TRUE
#define TRUE  (0==0)
#endif

#ifndef FALSE
#define FALSE (0==1)
#endif


/* Register flags */
#define FCarry 1

#define LO(x) (BYTE)(x&0x00ff)
#define HI(x) (BYTE)(x>>8)

#endif