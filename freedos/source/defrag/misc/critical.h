#ifndef CRITICAL_H_
#define CRITICAL_H_

/* Macro's to interpret the status code. */

/* Causes: */
#define WRITEPROTECTED       0    /* Medium is write protected.  */
#define UNKNOWNDEVICE        1    /* Device unknown.             */
#define DRIVENOTREADY        2    /* Drive not ready.            */
#define UNKNOWNINSTRUCTION   3    /* Unknown instruction.        */
#define CRC_ERROR            4    /* CRC error.                  */
#define WRONGLENGTH          5    /* Wrong length of data block. */
#define SEEKERROR            6    /* Seek error.                 */
#define UNKNOWNMEDIUM        7    /* Unknown medium.             */
#define SECTOR_NOT_FOUND     8    /* Sector not found.           */
#define NOPAPER              9    /* No paper left in printer.   */
#define WRITEERROR          10    /* Write error.                */
#define READERROR           11    /* Read error.                 */
#define GENERALFAILURE      12    /* General failure.            */

/* Status values. */

#define ERRORONWRITE  0x01    /* Application where the error occured. */
                              /* 0 = read, 1 = write.                 */

#define ERRORRANGE    0x06    /* Range involved.                      */
                              /*  0 = system data.                    */
                              /*  1 = FAT.                            */
                              /*  2 = directory.                      */
                              /*  3 = data range.                     */

#define ABORTALLOWED  0x08    /* Abort allowed.                       */
#define RETRYALLOWED  0x10    /* Retry allowed.                       */
#define IGNOREALLOWED 0x20    /* Ignore allowed.                      */

#define IGNORE  0x00
#define RETRY   0x01
#define ABORT   0x02
#define FAIL    0x03

#endif
