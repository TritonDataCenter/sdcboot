/* io.c  -  Virtual disk input/output */

/* Written 1993 by Werner Almesberger */

/*
 * Thu Feb 26 01:15:36 CET 1998: Martin Schulze <joey@infodrom.north.de>
 *	Fixed nasty bug that caused every file with a name like
 *	xxxxxxxx.xxx to be treated as bad name that needed to be fixed.
 */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

/*
 * 2003/10/19
 *
 *		Changed to support the linux version of chkdsk.
 *
 *      Imre Leber (imre.leber@worldonline.be)
 */
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
*/

 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fd.h>

#include "bool.h"

/* Use the _llseek system call directly, because there (once?) was a bug in
 * the glibc implementation of it. */
#include <linux/unistd.h>
#if defined __alpha || defined __ia64__ || defined __s390x__ || defined __x86_64__ || defined __ppc64__
/* On alpha, the syscall is simply lseek, because it's a 64 bit system. */
static loff_t llseek( int fd, loff_t offset, int whence )
{
    return lseek(fd, offset, whence);
}
#else
# ifndef __NR__llseek
# error _llseek system call not present
# endif
static _syscall5( int, _llseek, uint, fd, ulong, hi, ulong, lo,
		  loff_t *, res, uint, wh );

static loff_t llseek( int fd, loff_t offset, int whence )
{
    loff_t actual;

    if (_llseek(fd, offset>>32, offset&0xffffffff, &actual, whence) != 0)
	return (loff_t)-1;
    return actual;
}
#endif


int fs_open(char *path,int modus)
{
    return open(path, modus);
}


BOOL fs_read(int fd, loff_t pos,int size,void *data)
{
    if (llseek(fd,pos,0) == pos) 
	{
		if (read(fd, data, size) == size)
		{
		   return TRUE;
	    }
    }
	
	return FALSE;
}

BOOL fs_write(int fd, loff_t pos,int size,void *data)
{
	if (llseek(fd,pos,0) == pos)
	{
		if (write(fd,data,size) == size) 
		   return TRUE;
			
	}
	
	return FALSE;
}


void fs_close(int fd)
{
    close(fd);
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
