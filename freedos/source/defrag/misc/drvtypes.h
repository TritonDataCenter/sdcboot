#ifndef DRVTYPES_H_
#define DRVTYPES_H_

#define dtError     0x00   /* Invalid drive, letter not assigned         */
#define dtFixed     0x01   /* Fixed drive                                */
#define dtRemovable 0x02   /* Removable (floppy, etc.) drive             */
#define dtRemote    0x03   /* Remote (network) drive                     */
#define dtCDROM     0x04   /* MSCDEX V2.00+ driven CD-ROM drive          */
#define dtDblSpace  0x05   /* DoubleSpace compressed drive               */
#define dtSUBST     0x06   /* SUBST'ed drive                             */
/* dudes, where are Stacker 1,2,3 check-routines?                        */
#define dtStacker4  0x08   /* Stacker 4 compressed drive                 */
#define dtRAMDrive  0x09   /* RAM drive                                  */
#define dtDublDisk  0x0a   /* Vertisoft DoubleDisk 2.6 compressed drive  */
#define dtBernoully 0x0b   /* IOmega Bernoully drive                     */
#define dtDiskreet  0x0c   /* Norton Diskreet drive                      */
#define dtSuperStor 0x0d   /* SuperStor compressed drive                 */

int CheckStacker4(char drive);
int CheckDiskreet(char drive);
int CheckSuperStor(char drive);
int GetDriveType(char drive);
int CountValidDrives(void);
int FloppyDrives(void);

#endif
