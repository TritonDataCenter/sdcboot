#ifndef EVENT_H_
#define EVENT_H_

#define ALTKEY  0x0101          /* Alt key. */

/* Basic mouse events */
#define MSEVENTBASE 0x0201      /* Mouse event base. */
#define MSLEFT      0x0201      /* Left button.      */
#define MSRIGHT     0x0202      /* Right button.     */
#define MSMIDDLE    0x0203      /* Middle button.    */

/* Software mouse events */
#define MSDBLBASE   0x0204
#define MSDBLLEFT   0x0204      /* Mouse double clicked left   */
#define MSDBLRIGHT  0x0205      /* Mouse double clicked right  */
#define MSDBLMIDLE  0x0206      /* Mouse double clicked middle */

#define MSRPTBASE   0x0207
#define MSRPTLEFT   0x0207      /* Mouse repeat left            */
#define MSRPTRIGHT  0x0208      /* Mouse repeat right           */
#define MSRPTMIDLE  0x0209      /* Mouse repeat midle           */


#define MSMOVED     0x0204      /* Mouse moved between button down
                                   and button up                    */


#define TABKEY      9           /* Tab.    */
#define ENTERKEY   13           /* Enter.  */
#define ESCAPEKEY  27           /* Escape. */
#define SPACEKEY   32           /* Space.  */

#define SHIFTTAB 0xF00          /* Shift Tab. */

#define F1  0x3B00              /* Function keys. */
#define F2  0x3C00
#define F3  0x3D00
#define F4  0x3E00
#define F5  0x3F00
#define F6  0x4000
#define F7  0x4100
#define F8  0x4200
#define F9  0x4300
#define F10 0x4400

#define ALT_Q  0x1000           /* Alt combinations. */
#define ALT_W  0x1100
#define ALT_E  0x1200
#define ALT_R  0x1300
#define ALT_T  0x1400
#define ALT_Y  0x1500
#define ALT_U  0x1600
#define ALT_I  0x1700
#define ALT_O  0x1800
#define ALT_P  0x1900

#define ALT_A  0x1E00
#define ALT_S  0x1F00
#define ALT_D  0x2000
#define ALT_F  0x2100
#define ALT_G  0x2200
#define ALT_H  0x2300
#define ALT_J  0x2400
#define ALT_K  0x2500
#define ALT_L  0x2600

#define ALT_Z  0x2C00
#define ALT_X  0x2D00
#define ALT_C  0x2E00
#define ALT_V  0x2F00
#define ALT_B  0x3000
#define ALT_N  0x3100
#define ALT_M  0x3200

#define HOME         0x4700     /* Cursor keys. */
#define CURSORUP     0x4800
#define PAGEUP       0x4900
#define CURSORLEFT   0x4B00
#define CURSORRIGHT  0x4D00
#define END          0x4F00
#define CURSORDOWN   0x5000
#define PAGEDOWN     0x5100
#define INSERT       0x5200
#define DELETE       0x5300

#define SHIFT_F1     0x5400     /* Shift-function keys. */
#define SHIFT_F2     0x5500
#define SHIFT_F3     0x5600
#define SHIFT_F4     0x5700
#define SHIFT_F5     0x5800
#define SHIFT_F6     0x5900
#define SHIFT_F7     0x5A00
#define SHIFT_F8     0x5B00
#define SHIFT_F9     0x5C00
#define SHIFT_F10    0x5D00

#define CTRL_F1     0x5E00      /* CTRL-function keys. */
#define CTRL_F2     0x5F00
#define CTRL_F3     0x6000
#define CTRL_F4     0x6100
#define CTRL_F5     0x6200
#define CTRL_F6     0x6300
#define CTRL_F7     0x6400
#define CTRL_F8     0x6500
#define CTRL_F9     0x6600
#define CTRL_F10    0x6700

#define ALT_F1      0x6800      /* CTRL-function keys. */
#define ALT_F2      0x6900
#define ALT_F3      0x6A00
#define ALT_F4      0x6B00
#define ALT_F5      0x6C00
#define ALT_F6      0x6D00
#define ALT_F7      0x6E00
#define ALT_F8      0x6F00
#define ALT_F9      0x7000
#define ALT_F10     0x7100

#define CTRL_CURSOR_LEFT  0x7300 /* CTRL cursor keys. */
#define CTRL_CURSOR_RIGHT 0x7400
#define CTRL_END          0x7500
#define CTRL_PAGE_DOWN    0x7600
#define CTRL_HOME         0x7700

#define ALT_1 0x7800             /* Alt-number keys. */
#define ALT_2 0x7900
#define ALT_3 0x7A00
#define ALT_4 0x7B00
#define ALT_5 0x7C00
#define ALT_6 0x7D00
#define ALT_7 0x7E00
#define ALT_8 0x7F00
#define ALT_9 0x8000
#define ALT_0 0x8100

unsigned GetEvent(void);
void     ClearEvent(void);

#endif
