#ifndef FATCONST_H_
#define FATCONST_H_

#define BYTESPERSECTOR 512             /* Bytes per sector.       */
                                       /* Actualy can be one of: 
                                          512,1024,2048 and 4096,
                                          This should be fixed one day. */

#define HD32MB         33554432L       /* Size of 32Mb hard disk. */

#define ENTRIESPERSECTOR  16           /* directory entries per sector. */

#define FATREADBUFSIZE  1536           /* size of buffer for working with
					  the fat: scm(12,15,512)         */

				       /* amount of sectors that fit in the
					  buffer.                          */

#define SECTORSPERREADBUF (FATREADBUFSIZE / BYTESPERSECTOR)

#endif
