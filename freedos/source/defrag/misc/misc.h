#ifndef MISC_H_
#define MISC_H_


/* In Switchch.asm */
char cdecl SwitchChar(void);

/* In critical.asm */
void cdecl SetCriticalHandler(int (*handler)(int status));
void cdecl RenewCriticalHandler(int (*handler)(int status));

/* In hicritical.asm */
void cdecl CriticalHandlerOn(void);
int  cdecl CriticalErrorOccured(void);
int  cdecl GetCriticalCause(void);
int  cdecl GetCriticalStatus(void);

/* In gtdrvnms.c */
void GetDriveNames(char* drives);

/* In gtcdex.asm */
int CDEXinstalled(void);
char* cdecl GetCDROMLetters(void);
int cdecl IsCDROM(int drive);

#endif
