#ifndef MOUSE_H_
#define MOUSE_H_

#define LEFTBUTTON   1
#define RIGHTBUTTON  2
#define MIDDLEBUTTON 4

#define LEFTBUTTONACCENT   0
#define RIGHTBUTTONACCENT  1
#define MIDDLEBUTTONACCENT 2

#define SOTWARECURSOR   0
#define HARDWARECURSOR  1

/* Mouse.asm */

int  cdecl InitMouse   (void);
int  cdecl MousePresent(void);
void cdecl ShowMouse   (void);
void cdecl HideMouse   (void);
int  cdecl WhereMouse  (int* x, int* y);
void cdecl MouseGotoXY (int x, int y);
int  cdecl CountButtonPresses  (int button, int* releases, int* x, int* y);
int  cdecl CountButtonReleases (int button, int* releases, int* x, int* y);
void cdecl MouseWindow (int x1, int y1, int x2, int y2);
void cdecl DefineTextMouseCursor(int type, int andmask, int ormask);
void cdecl GetMouseMoved (int* distx, int* disty);
void cdecl SetLightPenOn(void);
void cdecl SetLightPenOff(void);
void cdecl SetMickey (int hm, int vm);
void cdecl CloseMouse (void);

/* Himouse.c */

void ClearMouse       (void);
int  MousePressed     (int button);
int  MouseReleased    (int button);
int  PressedInRange   (int x1, int y1, int x2, int y2);
int  ReleasedInRange  (int x1, int y1, int x2, int y2);
int  AnyButtonPressed (void);

int GetPressedX  (void);
int GetPressedY  (void);
int GetReleasedX (void);
int GetReleasedY (void);

void LockMouse (int x1, int y1, int x2, int y2);
void UnLockMouse (void);

int CheckMouseRelease (int x1, int y1, int x2, int y2);

int GetNumSupportedButtons(void);

#endif
