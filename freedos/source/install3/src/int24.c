/* int24 -- READ.ME
Enclosed find int24.c. This is a source code file for Turbo-C
that allows you to write your own "critical event handler". The
critical event handler is the code in DOS which prints the message
"Disk Error, (A)bort (R)etry (F)ail"... and gets a keystroke from
the user to decide what to do. This tends to mess up people's pretty
screens so it is nice to catch it. You might even disallow aborting, if
you don't wan't to let people abort the program.

The only thing to keep in mind is that you CANNOT call any DOS routines
in the handler, because DOS is not reentrant.

This file is provided on an as-is basis. It is public domain and I make
no claims whatsoever as to what it actually does :-) Hopefully it will
prove useful to PC programmers.

Joel Spolsky
eMail:	spolsky@cs.yale.edu 	(internet)
	SPOLSKY@YALECS		(Bitnet)
	...!harvard!yale!edu	(UUCP)

Let me know if you find any bugs...

Note: There was an earlier version of this floating around that calls
the TC function bioskey(). I got rid of that because bioskey in turn
calls DOS which is not allowed (and often hangs the computer) in
interrupt handlers.
*/

/********************************************************
 int24.c

 (C) 1990 Joel Spolsky, All Rights Reserved.
 
 This code is released into the public domain by the author.
 You can do whatever you want with it. Please let me know
 if you find it useful or if you find any bugs.
 *********************************************************/

/** int24.c
 **
 ** Interrupt 24 Handler version 0.9
 **
 ** When DOS has trouble accessing a peripheral it
 ** calls Interrupt 0x24. This is usually a pointer to
 ** the code in COMMAND.COM that prints the "Abort, Retry,
 ** Ignore" message. This is a completely portable module
 ** that replaces that message with a slightly more aesthetic
 ** one.
 **
 ** To use it, just call install_24(). To restore the DOS
 ** handler, call uninstall_24(). Warning! If you ever go
 ** into graphics mode of some sort, uninstall this! It won't
 ** be able to deal with a graphics screen correctly.
 **
 ** Note: We don't let the user ignore the error, which they shouldn't
 ** be doing anyway.
 **
 ** HISTORY
 ** -------
 ** 19 Mar 90	Created JS
 ** 16 Apr 90   Fixed to eliminate call to bioskey, which was reentering DOS
 **/

#include <bios.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __WATCOMC__
#include <screen.h>
#endif

#define CRIT_ERROR_HANDLER (0x24)

/** FUNCTION PROTOTYPES **/
void install_24(void);
void uninstall_24(void);
void interrupt handle_24 (unsigned bp, unsigned di, unsigned si,
			  unsigned ds, unsigned es, unsigned dx,
			  unsigned cx, unsigned bx, unsigned ax);
void fastprintz(int x, int y, int attr, char *s);
int getbioskey(void);

void interrupt (*oldvect)();			  
unsigned scr;				/* The segment where the screen is */

#ifdef __WATCOMC__
#define getvect _dos_getvect
#define setvect _dos_setvect
#define pokeb POKEB
#define peekb PEEKB
#endif

/** install_24
 **
 ** Installs the fancy interrupt handler.
 **/

#ifdef __WATCOMC__
unsigned getthis (void);
#pragma aux getthis = \
    "mov ah, 0Fh" \
    "int 0x10" \
    "mov ah, 0" \
    value [ax] modify [ax];
#endif
 
void install_24(void)
{
	oldvect = getvect(CRIT_ERROR_HANDLER);	/* save old handler */
	setvect(CRIT_ERROR_HANDLER, handle_24); /* and install ours */

	/* Find out if the screen is at 0xB000 or 0xB800 */
#ifdef __WATCOMC__
    scr = (getthis() != 7) ? 0xB800 : 0xB000;
#else
	_AH = 0x0F;
	geninterrupt (0x10);
	if (_AL == 7)
		scr = 0xB000;
	else
		scr = 0xB800;
#endif
}


void uninstall_24(void)
{
	/* Restore old handler */
	setvect(CRIT_ERROR_HANDLER, oldvect);
}

static char screen_buf[9][52];	/* room for the saved part of screen */

void interrupt handle_24 (unsigned bp, unsigned di, unsigned si,
			  unsigned ds, unsigned es, unsigned dx,
			  unsigned cx, unsigned bx, unsigned ax)
{

	int err,key,ret=-1;
	int r,c,start;
	
	err = di & 0x00FF;	/* Error message, from DOS. */

	/* Save section of screen that will be overwritten */
	for (r=8; r<17; r++) {
		start = (160 * r + 54);
		for (c=0; c<26; c++) {
			screen_buf[r-8][c*2] = peekb(scr, start++);
			screen_buf[r-8][c*2+1] = peekb(scr, start++);
		} 
	}

	/* Pop up error message */
	fastprintz( 8,27,0x07,"ÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·");
	fastprintz( 9,27,0x07,"ºError!                  º");
	fastprintz(10,27,0x07,"º                        º");

	/* Common diagnosable problems */
	switch(err) {
		case 0x00:
	fastprintz(11,27,0x07,"ºDisk is write protected.º"); break;
		case 0x02:
	fastprintz(11,27,0x07,"ºDisk drive is not ready.º"); break;
		default:
	fastprintz(11,17,0x07,"ºDisk error.             º"); break;
	}

	fastprintz(12,27,0x07,"º                        º");
	fastprintz(13,27,0x07,"º Try again              º");
	fastprintz(13,29,0x0f,"T");
	fastprintz(14,27,0x07,"º Exit this program      º");
	fastprintz(14,29,0x0f,"E");

	/* In DOS 3.00 and later, they can also fail the disk access */
	if (_osmajor > 2) {
	fastprintz(15,27,0x07,"º Cancel this operation  º");
	fastprintz(15,29,0x0f,"C");
	}
	else
	fastprintz(15,27,0x07,"º                        º");

	fastprintz(16,27,0x07,"ÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½");

	/* Call BIOS to get a single keystroke from the user */
	do {
		key=getbioskey();

		if (key & 0x00FF)
			key &= 0x00FF;

		switch(key) {
		case 't': case 'T': ret = 0x0001; break;
		case 'e': case 'E': ret = 0x0002; break;
		case 'c': case 'C': if (_osmajor > 2) ret = 0x0003; break;
		default: break;
		}

	} while (ret < 0);
	
	/* Restore that section of the screen */
	for (r=8; r<17; r++) {
		start = (160*r + 54);
		for (c=0; c<26; c++) {
			pokeb(scr, start++, screen_buf[r-8][c*2]);
			pokeb(scr, start++, screen_buf[r-8][c*2+1]);
		}
	}

	ax = ret;
/* And please don't tell me I didn't use any of those parameters. */
#pragma warn -par
}
#pragma warn .par


/* fastprintz - shove an asciz string onto the screen */
void
fastprintz(int y, int x, int attr, char *s)
{
	int i=0,base;

	base = (80*y+x)<<1;  /* determine offset into screen */
	while (s[i]!=0) {
		pokeb(scr, base++, s[i++]);
		pokeb(scr, base++, attr);
	}
}

/** getbioskey
 **
 ** get one key from the BIOS
 **
 ** Like TC bioskey(0), but doesn't nab control Break. It seems
 ** that the TC bioskey was trying to block Ctrl-Breaks, which
 ** it did by changing the Ctrl-Break handler, which it called
 ** DOS to do. This made the interrupt handler reenter DOS which
 ** is illegal.
 **/

int getbioskey(void)
{
	union REGS regs;
//	struct SREGS segregs;

//	segread(&segregs);
	regs.h.ah = 0;
	int86 (0x16, &regs, &regs);
	return regs.x.ax;
}

