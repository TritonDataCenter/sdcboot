#ifndef OS_ID_H_
#define OS_ID_H_

struct i_os_ver
{
  long maj;                 /* Long because of linux DOSEMU compatibility. */
  long min;
};

#define FreeDOS  0
#define DOS      1
#define OS2      2
#define DESQVIEW 3
#define WINS     4
#define WIN3     5
#define DOSEMU   6
#define TOT_OS   7
                            /*   76543210  */
#define is_FreeDOS  0x01    /* b'00000001' */
#define is_DOS      0x02    /* b'00000010' */
#define is_OS2      0x04    /* b'00000100' */
#define is_DESQVIEW 0x08    /* b'00001000' */
#define is_DOSEMU   0x10    /* b'00010000' */
#define is_WINS     0x20    /* b'00100000' */
#define is_WIN3     0x40    /* b'01000000' */

#define IN_WINDOWS(os) ((os >= WINS) && (os < DOSEMU))
#define PLAIN_DOS(os)  (os <= DOS)

#ifndef OS_ID_MAIN
  extern int id_os_type;
  extern int id_os;
  extern const char *id_os_name[TOT_OS];
  extern struct i_os_ver id_os_ver[TOT_OS];
#endif

int  get_os(void);          /* Determine OS                     */

#endif
