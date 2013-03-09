/*--- raw.c --------------------------------------------------------------
 MS-DOS routines to set and reset raw mode, enable or disable break checking,
 and check for characters on stdin.
 Written by Dan Kegel (dank@moc.jpl.nasa.gov).  Enjoy.
 $Log:	raw.c $
 * Revision 1.3  90/07/14  09:02:03  dan_kegel
 * Added raw_set_stdio(), which should make it easy to use RAW mode.
 * 
 * Revision 1.2  90/07/09  23:18:15  dan_kegel
 * Compiles with /w3 now.
 * 
 * Revision 1.1  90/07/09  23:11:13  dan_kegel
 * Initial revision
 * 

 QUICK SUMMARY:

 For fast screen output, call
	raw_set_stdio(1);
 at the beginning of your program, and 
	raw_set_stdio(0);
 at the end, and use getchar() to grab keystrokes from the keyboard.
 Your screen updates will be much faster (assuming you use stdout, and
 have installed NANSI.SYS or FANSI-CONSOLE), and
 getchar() will treat Backspace, Enter, ^S, ^C, ^P, and ESC as normal
 keystrokes rather than as editing characters.

 DETAILS and extra goodies:

 To maximize display speed, applications should set the stdout device to 
 raw mode when they start up, and clear it when they exit or start up a
 subshell.
 It is the device associated with a file handle that has a RAW mode, not
 the file handle itself.  Thus, if stdin and stdout both refer to CON:,
 raw_get(fileno(stdin)) will always equal raw_get(fileno(stdout)).

 The stdin device should not be in RAW mode during normal operation 
 because this disables normal command-line editing.  For example,
 COPY CON: NUL: will never exit if stdin is in RAW mode.
 Because RAW mode is dangerous, applications that use it should prevent
 DOS from checking for Control-C or Control-Break.  This can be done by
 calling break_set(0).

 To check for an input char without waiting, call raw_kbhit().
 Be sure to turn off break checking before you call, unless you want
 the user to be able to break out of your program!

 To use getchar() while the console is in RAW mode, you must first disable
 input buffering by executing
	setbuf(stdin, NULL);
 or else getchar() will simply hang.
 When stdin is in RAW mode, getchar() will not check for control-C,
 control-S, or control-P; it will also not echo.  
 (The ideas here are all UNIX-compatible, but the following code would
  need changes to compile under UNIX.   Under UNIX, the only reason to
  use RAW mode is to cause getchar() to not check for ^C, ^S, etc.)

 If you tend to do lots of single-character outputting to the screen,
 you may need to use output buffering before you see any speed benefits from
 RAW mode.  If you use putchar() to do your outputs, you can enable output
 buffering by executing
 	static char stdoutbuf[BUFSIZ];
	setbuf(stdout, stdoutbuf);
 but then you you have to fflush(stdout) each time you want to be sure your
 output has made it out of the buffer onto the screen.
--------------------------------------------------------------------------*/
#include <dos.h>
#include <stdio.h>
#include "raw.h"

/* IOCTL GETBITS/SETBITS bits. */
#define DEVICE		0x80
#define RAW		0x20

/* IOCTL operations */
#define GETBITS		0
#define SETBITS		1
#define GETINSTATUS	6

/* DOS function numbers. */
#define BREAKCHECK	0x33
#define IOCTL		0x44

/* A nice way to call the DOS IOCTL function */
static int
ioctl(int handle, int mode, unsigned setvalue)
{
	union REGS regs;

	regs.h.ah = IOCTL;
	regs.h.al = (char) mode;
	regs.x.bx = handle;
	regs.h.dl = (char) setvalue;
	regs.h.dh = 0;			/* Zero out dh */
	intdos(&regs, &regs);
	return (regs.x.dx);
}

/*--------------------------------------------------------------------------
 Call this routine to determinte whether the device associated with
 the given file is in RAW mode.
 Returns FALSE if not in raw mode, TRUE if in raw mode.
 Example: old_raw = raw_get(fileno(stdin));
--------------------------------------------------------------------------*/
int
raw_get(fd)
	int fd;
{
	return ( RAW == (RAW & ioctl(fd, GETBITS, 0)));
}

/*--------------------------------------------------------------------------
 Call this routine to set or clear RAW mode for the device associated with
 the given file.
 Example: raw_set(fileno(stdout), TRUE);
--------------------------------------------------------------------------*/
void
raw_set(fd, raw)
	int fd;
	int raw;
{
	int bits;
	bits = ioctl(fd, GETBITS, 0);
	if (DEVICE & bits) {
		if (raw)
			bits |= RAW;
		else
			bits &= ~RAW;
		(void) ioctl(fd, SETBITS, bits);
	}
}

/*--------------------------------------------------------------------------
 If any input is ready on stdin, return a nonzero value.
 Else return zero.
 This works for both input files and devices.
 In RAW mode, if break checking is turned off, does not check for ^C.
 (The kbhit() that comes with Microsoft C seems to always check for ^C.)
----------------------------------------------------------------------------*/
int
raw_kbhit()
{
	union REGS regs;

	regs.h.ah = IOCTL;
	regs.h.al = GETINSTATUS;
	regs.x.bx = fileno(stdin);
	intdos(&regs, &regs);
	return (0xff & regs.h.al);
}


/* A nice way to call the DOS BREAKCHECK function */
static int
breakctl(int mode, int setvalue)
{
	union REGS regs;

	regs.h.ah = BREAKCHECK;
	regs.h.al = (char) mode;
	regs.h.dl = (char) setvalue;
	intdos(&regs, &regs);
	return (regs.x.dx & 0xff);
}

/*--------------------------------------------------------------------------
 Call this routine to determine whether DOS is checking for break (Control-C)
 before it executes any DOS function call.
 Return value is FALSE if it only checks before console I/O function calls,
 TRUE if it checks before any function call.
--------------------------------------------------------------------------*/
int
break_get(void)
{
	return ( 0 != breakctl(GETBITS, 0));
}

/*--------------------------------------------------------------------------
 Call this routine with TRUE to tell DOS to check for break (Control-C)
 before it executes any DOS function call.
 Call this routine with FALSE to tell DOS to only check for break before
 it executes console I/O function calls.
--------------------------------------------------------------------------*/
void
break_set(check)
	int check;
{
	(void) breakctl(SETBITS, check);
}

/*--------------------------------------------------------------------------
 One routine to set (or clear) raw mode on stdin and stdout,
 clear (or restore) break checking, and turn off input buffering on stdin.
 This is the most common configuration; under MS-DOS, since setting raw mode
 on stdout sometimes sets it on stdin, it's best to set it on both & be done
 with it.
--------------------------------------------------------------------------*/
void
raw_set_stdio(raw)
	int raw;	/* TRUE -> set raw mode; FALSE -> clear raw mode */
{
	static int was_break_checking = 0;

	raw_set(fileno(stdin), raw);
	raw_set(fileno(stdout), raw);
	if (raw) {
		setbuf(stdin, NULL);	/* so getchar() won't hang */
		was_break_checking = break_get();
		break_set(0);
	} else {
		break_set(was_break_checking);
	}
}

