#include<stdio.h>
#include<dos.h>

int main ( void )
{
	union REGS r;

	puts ("FreeDOS KEYB 2.0: KEYCODE v. 1.0");
	puts ("Enter any key to see its keycode. Press Esc or F1 to exit.");
	do
	{
		r.h.ah = 0x10;
		int86 (0x16, &r, &r);
		printf ("          Scan Code: %3u     Ascii Code: %3u     Char: %c\n",
			  r.h.ah, r.h.al, r.h.al>31 ? r.h.al : 32 );
	}
	while ( (r.h.ah != 1) && (r.h.ah != 59));

}