

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**************************************************************************

   APPREL - Finds relocation table, sorts and compresses it, appends it to
            the end of an EXE file, removes the original table, and adjusts
            the header accordingly

   Copyright 1991-1993, J.P. Software Inc., Arlington, MA, USA.  All Rights
   Reserved.

   Author:  Tom Rawson, 3/31/91
            Substantial mods, 7/93

   Usage:
      APPREL [filename]

      filename    name of file to append relocation table to

      If the filename is not given APPREL will prompt for it. An EXE
      extension is assumed unless otherwise specified.

      If an error occurs the message "APPREL failed:  " is printed,
      followed by a specific error message.  See the array errmsg below
      for the individual messages, which are largely self-explanatory.

***************************************************************************/

#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <process.h>
#include <string.h>

                           /* size of a relocation table entry */
#define RELOC_SIZE (unsigned int)sizeof(void far *)
                           /* size of EXE header */
#define HEADER_SIZE (unsigned int)sizeof(header)
                           /* size of relocation table elements */
#define MARK_SIZE 4
#define NB_SIZE (sizeof new_reloc_bytes)
#define SC_SIZE (sizeof seg_cnt)
#define RS_SIZE (sizeof reloc_segs[0])
#define RC_SIZE (sizeof reloc_counts[0])
#define NR_SIZE (sizeof new_reloc[0][0])

                           /* size of read / write buffer */
#define BUF_SIZE 16384
                           /* maximum number of segments in reloc table */
#define MAX_SEGS 16
                           /* maximum number of entries per segment */
#define MAX_SEG_RELOCS 2000
                           /* maximum number of segments to keep in
                              base relocation table */
#define MAX_KEEP 16
									/* maximum number of entries remaining in
										base table */
#define MAX_KEEP_RELOCS 2000

                           /* make far pointer from segment and offset */
#define MAKEP(seg,off) ((void far *)(((unsigned long)(seg) << 16) | \
   (unsigned int)(off)))

#define ERR_OPEN_IN 1
#define ERR_OPEN_OUT 2
#define ERR_READ_HEAD 3
#define ERR_BAD_EXE 4
#define ERR_SEEK 5
#define ERR_READ 6
#define ERR_READ_CNT 7
#define ERR_WRITE 8
#define ERR_WRITE_CNT 9
#define ERR_CLOSE_IN 10
#define ERR_CLOSE_OUT 11
#define ERR_DEL_BACKUP 12
#define ERR_RENAME_OLD 13
#define ERR_RENAME_NEW 14
#define ERR_KEEP_OVER 15
#define ERR_RELOC_SIZE 16
#define ERR_SEG_CNT 17
#define ERR_SEG_RELOCS 18
#define ERR_KEEP_RELOCS 19
#define ERR_MAX 19
#define ERR_UNKNOWN 20

const char *errmsg[] = {
   "Input file open error",                  /* 1 */
   "Output file open error",                 /* 2 */
   "Header read error",                      /* 3 */
   "Not an EXE file",                        /* 4 */
   "File seek error",                        /* 5 */
   "Input file read error",                  /* 6 */
   "Input file read byte count error",       /* 7 */
   "Output file write error",                /* 8 */
   "Output file write byte count error",     /* 9 */
   "Input file close error",                 /* 10 */
   "Output file close error",                /* 11 */
   "Backup file deletion error",             /* 12 */
   "Old EXE file rename error",              /* 13 */
   "New EXE file rename error",              /* 14 */
   "Too many keep segments",              	/* 15 */
   "Relocation table too large",             /* 16 */
   "Too many segments in table",             /* 17 */
   "Too many relocations in segment",        /* 18 */
   "Too many relocations in new base",			/* 19 */
   "Unknown internal error"                  /* 20 */
};

   /* variables */
char buffer[BUF_SIZE];                    /* read / write buffer */
char filename[80],                        /* input file name */
     outname[80],                         /* output file name */
     backname[80];                        /* backup file name */
char mark[MARK_SIZE + 1] = "ENDP";        /* marker for relocation table */
unsigned int reloc_segs[MAX_SEGS];        /* segments in relocation table */
unsigned int reloc_counts[MAX_SEGS];      /* entry counts for each segment */
unsigned int new_reloc[MAX_SEGS][MAX_SEG_RELOCS];  /* reorganized relocation table */
unsigned int new_base[MAX_KEEP_RELOCS * 2];  /* new base relocation table */
unsigned int keep_segs[MAX_KEEP];     		/* segments to keep in base table*/
int in,                                   /* input exe file handle */
    out;                                  /* output exe file handle */
unsigned int seg_cnt = 0,                 /* segments in relocation table */
    keep_cnt = 0,                       	/* segments to keep */
    new_reloc_cnt = 0,                    /* entries in new reloc table */
    new_base_cnt = 0,                    	/* entries in new base table */
    new_reloc_bytes,                      /* bytes in new reloc table */
    new_base_len,                        	/* full length of new base table including mark etc. */
	 orig_header_len,								/* original header length */
	 header_pad_len;								/* header pad length to mae even paras */
long original_end,                        /* original EOF offset */
    exe_len,                              /* length of EXE portion */
    new_exe_len,                          /* expanded length of file */
    reloc_len,                            /* length of reloc table */
    new_reloc_len;                        /* full length of new reloc table including mark etc. */

struct exe_head {                         /* exe file header */
   char           sigM, sigZ;             /* signature "MZ" */
   unsigned int   last_page_len,          /* last page length */
                  file_pages,             /* pages in file */
                  reloc_cnt,              /* relocation item count */
                  head_para_cnt,          /* paragraphs in header */
                  extra_para_min,         /* minimum space beyond stack */
                  extra_para_max;         /* maximum space beyond stack */
   char far       *stack_init;            /* initial SS:SP */
   unsigned int   checksum;               /* module checksum */
   int            (far *entry)();         /* initial CS:IP */
   unsigned int   reloc_start,            /* relocation table offset */
                  overlay_num;            /* overlay number */
} header;


   /* local prototypes */
void copy_part(unsigned long, long);
void write_part(void *, int, int);
void write_zeros(int);
void error(unsigned int);


main(int argc, char **argv)
{

   unsigned int *rtptr;
   unsigned int keep_seg, rcnt, scnt;
   int keep_flag;

   // sign on and parse command line

   printf("\nAPPREL 1.11  Copyright 1991-1995, JP Software Inc., All Rights Reserved.\n");

   // get file name if not there
   if (--argc > 0)
      strcpy(filename, *(++argv));
   else {
      printf("\nEnter file name: ");
      scanf("%s", filename);
   }

   // scan for segments to keep
   while (--argc > 0) {
      if (sscanf(*(++argv), "%x", &keep_seg) != 1)
         break;
      if (keep_cnt >= MAX_KEEP)
         error(ERR_KEEP_OVER);
      keep_segs[keep_cnt++] = keep_seg;
   }

//    for (scnt = 0; scnt < keep_cnt; scnt++)
//       printf("Keep segment %d is %.04X\n", scnt, keep_segs[scnt]);

   // fix up file names with proper extensions etc.
   if (strrchr(filename,'.') == NULL)
      strcpy(&filename[strlen(filename)], ".EXE");    /* add EXE extension */
   strcpy(outname, filename);                         /* copy name */
   strcpy(strrchr(outname,'.'), ".EX$");              /* add EX$ extension */
   strcpy(backname, filename);                        /* copy name */
   strcpy(strrchr(backname,'.'), ".BAK");             /* add BAK extension */

   // open input file, read header, check for "MZ" signature
   if ((in = open(filename, O_RDONLY | O_BINARY)) < 0)
      error(ERR_OPEN_IN);
   if (read(in, (char *)&header, HEADER_SIZE) != HEADER_SIZE)
      error(ERR_READ_HEAD);
   if (header.sigM != 'M' || header.sigZ != 'Z')
      error(ERR_BAD_EXE);

   // check count
   if (header.reloc_cnt == 0) {
      printf("\nNo relocation table entries present, APPREL aborted\n");
      return 1;
   } else
      printf("\n%u relocation entries found\n", header.reloc_cnt);

   // check for relocation table too large
   reloc_len = header.reloc_cnt * RELOC_SIZE;            // calc tbl length
   if (reloc_len > BUF_SIZE)
      error(ERR_RELOC_SIZE);

   // read old relocation table
   if (lseek(in, (unsigned long)(header.reloc_start), SEEK_SET) == -1L)
      error(ERR_SEEK);
   if (read(in, buffer, (int)reloc_len) != (int)reloc_len)
      error(ERR_READ);

	// save old header info
   orig_header_len = (16 * header.head_para_cnt);

   // split relocation info by segment
   for (rcnt = header.reloc_cnt, rtptr = (unsigned int *)(&buffer); rcnt > 0; rcnt--) {

// printf("Entry %.04X:%.04X -- ", rtptr[1], rtptr[0]);
      // see if this entry is in the keep list
      keep_flag = 0;
      for (scnt = 0; scnt < keep_cnt; scnt++) {
         if (rtptr[1] == keep_segs[scnt]) {
            keep_flag++;
// printf("keepd\n");
            break;
         }
      }

      // if we are keeping this entry, stick it into the new base table
      // otherwise add it to the new table
      if (keep_flag) {

         // add this entry to the new base table
         if (new_base_cnt >= MAX_KEEP_RELOCS) {
            error(ERR_KEEP_RELOCS);
         }
// printf("copying to new base table as entry %d\n", (new_base_cnt / 2));
//printf("New base table entry is %04x:%04x\n", rtptr[1], rtptr[0]);
         new_base[2 * new_base_cnt] = rtptr[0];
         new_base[(2 * new_base_cnt) + 1] = rtptr[1];
         new_base_cnt++;

		} else {

         // scan the segment list and see if the segment is there
         for (scnt = 0; scnt < seg_cnt; scnt++) {
            if (rtptr[1] == reloc_segs[scnt])
               break;
         }

         // if we need to add this segment to the list, do so
         if (scnt == seg_cnt) {
// printf("NEW segment %d -- ", scnt);
            if (seg_cnt >= MAX_SEGS)
               error(ERR_SEG_CNT);
            reloc_segs[seg_cnt++] = rtptr[1];
         }
// else printf("old segment %d -- ", scnt);

         // now add this entry to the list for the segment
         if (reloc_counts[scnt] >= MAX_SEG_RELOCS) {
            printf("\nError on segment %.04X", reloc_segs[scnt]);
            error(ERR_SEG_RELOCS);
         }
// printf("adding as entry %d\n", reloc_counts[scnt]);
         new_reloc[scnt][(reloc_counts[scnt]++)] = rtptr[0];
         new_reloc_cnt++;

      }

      // point to next entry
      rtptr += 2;

   }

   printf("%u relocation entries copied\n", new_reloc_cnt);
   printf("%u relocation entries in base table\n", new_base_cnt);

   // calculate length of all elements of new table
   new_reloc_bytes = SC_SIZE + (seg_cnt * RS_SIZE) + (seg_cnt * RC_SIZE);
   for (scnt = 0; scnt < seg_cnt; scnt++) {
      new_reloc_bytes += (reloc_counts[scnt] * NR_SIZE);
   }
   new_reloc_len = (long)(MARK_SIZE + NB_SIZE + new_reloc_bytes);
   new_base_len = new_base_cnt * 2 * sizeof(unsigned int);
//    printf("%d total segments \n\n", seg_cnt);
//    for (scnt = 0; scnt < seg_cnt; scnt++) {
//       printf("Segment %d is %.04X, with %d entries:\n", scnt, reloc_segs[scnt], reloc_counts[scnt]);
//       for (rcnt = 0; rcnt < reloc_counts[scnt]; rcnt++)
//          printf("    %.04X", new_reloc[scnt][rcnt]);
//       printf("\n");
//    }

//    for (scnt = 0; scnt < seg_cnt; scnt++) {
//       for (rcnt = 0; rcnt < reloc_counts[scnt]; rcnt++)
//          printf("%.04X:%.04X\n", reloc_segs[scnt], new_reloc[scnt][rcnt]);
//    }

   // open output file
	if ((out = open(outname, (O_CREAT | O_TRUNC | O_RDWR | O_BINARY),
         (S_IREAD | S_IWRITE))) < 0)
      error(ERR_OPEN_OUT);

   // Get EOF and calculate length of true EXE portion (there may be debugger
   // info beyond this length)

   if ((original_end = lseek(in, 0L, SEEK_END)) == -1L)
      error(ERR_SEEK);
   exe_len = ((long)header.file_pages * 512L) - (long)((header.last_page_len == 0) ? 0 : (512 - header.last_page_len));

   // Adjust header lengths for relocation table info removed at start of
   // file, and new relocation table added to end of file.  This gets the
   // header adjusted before we write it, avoiding a rewrite.

	header.head_para_cnt = (header.reloc_start + new_base_len + 0xF) >> 4;
	header_pad_len = (16 * header.head_para_cnt) - header.reloc_start - new_base_len;
   new_exe_len = (16 * header.head_para_cnt) + (exe_len - orig_header_len) + new_reloc_len;
   header.reloc_cnt = new_base_cnt;
   header.file_pages = (unsigned int)((new_exe_len + 511L) / 512L);
   header.last_page_len = (unsigned int)(new_exe_len % 512L);
//printf("Old len %lX, new len %lX, pages %X, leftover %X\n", exe_len, new_exe_len, header.file_pages, header.last_page_len);

   // copy the new header, anything afte it up to the relocation table
   // (normally just two bytes), the new base table, and the EXE portion
   // of the file (skipping the old relocation table)
   if (write(out, (char *)&header, HEADER_SIZE) != HEADER_SIZE)
      error(ERR_WRITE);
   copy_part((unsigned long)HEADER_SIZE, header.reloc_start - HEADER_SIZE);
   write_part(new_base, new_base_len, 0);
	write_zeros(header_pad_len);
   copy_part(orig_header_len, exe_len - orig_header_len);
//   copy_part((unsigned long)HEADER_SIZE, exe_len - HEADER_SIZE);

   // Write the mark, the total and segment counts, and the segments, counts,
   // and data for the new relocation table
   write_part(&mark, MARK_SIZE, 0);
   write_part(&new_reloc_bytes, NB_SIZE, 0);
   write_part(&seg_cnt, SC_SIZE, 0);
   for (scnt = 0; scnt < seg_cnt; scnt++) {
      write_part(&(reloc_segs[scnt]), RS_SIZE, 0);
      write_part(&(reloc_counts[scnt]), RC_SIZE, 0);
      write_part(&(new_reloc[scnt][0]), (reloc_counts[scnt] * NR_SIZE), 0);
   }

   // copy the rest of the file (debugger info etc. beyond the EXE portion)
   copy_part(exe_len, (original_end - exe_len));

   // delete any backup file, rename the old file to the backup name, and
   // rename the new file to an EXE
   if (close(in) < 0)
      error(ERR_CLOSE_IN);
   if (close(out) < 0)
      error(ERR_CLOSE_OUT);
   if ((unlink(backname) != 0) && (errno != ENOENT)) // file not found is OK
      error(ERR_DEL_BACKUP);
   if (rename(filename,backname) != 0)
      error(ERR_RENAME_OLD);
   if (rename(outname,filename) != 0)
      error(ERR_RENAME_NEW);

   return 0;                              /* all done */
}


// Copy part of the input file to the output file, beginning at location
// start_offset in the input file and at the current position in the
// output file

void copy_part(unsigned long start_offset, long count) {

   int   bytes_req,                       /* bytes requested on read */
         bytes_read,                      /* bytes actually read */
         bytes_written;                   /* bytes actually written */

//printf("Copy from %lu for %lu bytes\n", start_offset, count);
   if (lseek(in, start_offset, SEEK_SET) == -1L)
      error(ERR_SEEK);
   for ( ; count > 0; count -= (unsigned long)bytes_read) {
      bytes_req = (int)((count > (long)BUF_SIZE) ? BUF_SIZE : (int)count);
      if ((bytes_read = read(in, buffer, bytes_req)) <= 0)
         error(ERR_READ);
      if (bytes_read != bytes_req)
         error(ERR_READ_CNT);
      if ((bytes_written = write(out, buffer, bytes_read)) < 0)
         error(ERR_WRITE);
      if (bytes_written != bytes_read)
         error(ERR_WRITE_CNT);
   }
}


// Write some information to the output file from a buffer

void write_part(void *buf, int count, int encrypt) {

   int bytes_written;                     /* bytes actually written */

//printf("Write %u bytes\n", count);
	if (count <= 0)
		return;
   if ((bytes_written = write(out, buf, count)) < 0)
      error(ERR_WRITE);
   if (bytes_written != count)
      error(ERR_WRITE_CNT);
}


// Write zeros to the output file

void write_zeros(int count) {

   int bytes_req, bytes_written;
	char zbuf[16];

//printf("Write %u zeros", count);
	memset(zbuf, '\0', 16);
   for ( ; count > 0; count -= bytes_written) {
      bytes_req = (int)((count > 16) ? 16 : (int)count);
      if ((bytes_written = write(out, buffer, bytes_req)) < 0)
         error(ERR_WRITE);
      if (bytes_written != bytes_req)
         error(ERR_WRITE_CNT);
   }
}


// Handle internal errors
void error(unsigned int errnum) {
   if (errnum > ERR_MAX)
      errnum = ERR_UNKNOWN;
   printf("\nAPPREL failed:  %s\n", errmsg[errnum - 1]);
   exit(1);
}


