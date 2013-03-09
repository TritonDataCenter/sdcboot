/*  Console I/O
    Copyright (c) Express Software 1998.
    All Rights Reserved.

    Created by: Joseph Cosentino.

*/

#ifndef _CONIO_H_INCLUDED
#define _CONIO_H_INCLUDED

/* The middle character will be
   used to fill the window space */
#ifdef NOTEXTERN_IN_CONIO
char *Border22f = "…Õª∫ ∫»Õº";
char *Border22if = "ÃÕπ∫ ∫»Õº";
char BarBlock1 = 176;
char BarBlock2 = 178;
char BarUpArrow = 24;		/* '' */
char BarDownArrow = 25;		/* '' */
#else
extern const char *Border22f;
extern const char *Border22if;
extern char BarBlock1;
extern char BarBlock2;
extern char BarUpArrow;
extern char BarDownArrow;
#endif

/* Foreground colours */
#define Black     0x00
#define Blue      0x01
#define Green     0x02
#define Cyan      0x03
#define Red       0x04
#define Magenta   0x05
#define Brown     0x06
#define White     0x07

#define Gray      0x08
#define BrBlue    0x09
#define BrGreen   0x0A
#define BrCyan    0x0B
#define BrRed     0x0C
#define BrMagenta 0x0D
#define Yellow    0x0E
#define BrWhite   0x0F

/* Background Colours */
#define BakBlack   0x00
#define BakBlue    0x10
#define BakGreen   0x20
#define BakCyan    0x30
#define BakRed     0x40
#define BakMagenta 0x50
#define BakBrown   0x60
#define BakWhite   0x70
#define Blink      0x80

#define EV_KEY   1
#define EV_SHIFT 2
#define EV_MOUSE 4
#define EV_TIMER 8
#define EV_NONBLOCK 16

#define CONIO_TICKS_PER_SEC  18.2
#define CONIO_TIMER(seconds) ((seconds)*18.2)

struct event
{
  unsigned int ev_type;

  unsigned int key;
  unsigned int scan;
  unsigned int shift;
  unsigned int shiftX;

  unsigned int x, y;
  unsigned int left;
  unsigned int right;
  unsigned int middle;
  signed   int wheel;

  long timer;
};

extern unsigned char const ScreenWidth;
extern unsigned char const ScreenHeight;
extern unsigned int const MouseInstalled;
extern unsigned int const WheelSupported;
extern unsigned char const MonoOrColor;

#define COLOR_MODE 0
#define MONO_MODE  1

#ifdef __cplusplus
extern "C"
{
#endif

/* Use these to define the memory model for
   the external conio.asm functions listed
   below
*/
#define __CON_FUNC far
#define __CON_DATA far

  void __CON_FUNC conio_init (int force_mono);
  void __CON_FUNC conio_exit (void);
  void __CON_FUNC show_mouse (void);
  void __CON_FUNC hide_mouse (void);
  void __CON_FUNC move_mouse (int x, int y);
  void __CON_FUNC move_cursor (int x, int y);
  void __CON_FUNC cursor_size (int top, int bottom);
  void __CON_FUNC get_event (struct event __CON_DATA * ev, int flags);
  void __CON_FUNC write_char (int attr, int x, int y, int ch);
  void __CON_FUNC write_string (int attr, int x, int y,
				const char __CON_DATA * str);
  void __CON_FUNC save_window (int x, int y, int w, int h,
			       char __CON_DATA * buf);
  void __CON_FUNC load_window (int x, int y, int w, int h,
			       char __CON_DATA * buf);
  void __CON_FUNC clear_window (int attr, int x, int y, int w, int h);
  void __CON_FUNC scroll_window (int attr, int x, int y, int w, int h,
				 int len);
  void __CON_FUNC border_window (int attr, int x, int y, int w, int h,
				 const char __CON_DATA * border);

#ifdef __cplusplus
}
#endif

#define SK_R_SHIFT       0x01
#define SK_L_SHIFT       0x02
#define SK_SHIFT         0x03
#define SK_CTRL          0x04
#define SK_ALT           0x08
#define SK_SCROLL_LOCKED 0x10
#define SK_NUM_LOCKED    0x20
#define SK_CAPS_LOCKED   0x40
#define SK_INSERT        0x80

#define SK_L_CTRL        0x0100
#define SK_L_ALT         0x0200
#define SK_R_CTRL        0x0400
#define SK_R_ALT         0x0800
#define SK_SCROLL_LOCK   0x1000
#define SK_NUM_LOCK      0x2000
#define SK_CAPS_LOCK     0x4000
#define SK_SYS_REQ       0x8000

#endif
