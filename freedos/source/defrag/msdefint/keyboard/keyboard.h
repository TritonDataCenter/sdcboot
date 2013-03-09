#ifndef KEYBOARD_H_
#define KEYBOARD_H_

int  cdecl AltKeyDown    (void);
int  cdecl KeyPressed    (void); /* Returns boolean. */
int  cdecl ReadKey       (void);
void cdecl ClearKeyboard (void);

int        AltKeyPressed (void);

#endif
