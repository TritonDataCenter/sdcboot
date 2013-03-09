/* Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
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

typedef struct IDT_S {
  word16 offset0;
  word16 selector;
  word16 stype;
  word16 offset1;
} IDT;

extern IDT idt[256];

/* Routines in tables.asm */
extern void far ivec0(void), ivec1(void), ivec7(void), ivec31(void);
extern void interrupt_common(void), page_fault(void);
