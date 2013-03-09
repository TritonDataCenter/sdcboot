#ifndef HLPREAD_H_
#define HLPREAD_H_

/* Help system error codes. */
#define HELPSUCCESS         0
#define HELPMEMINSUFFICIENT 1
#define HELPREADERROR       2

int ReadHelpFile(int index);

void CheckHelpFile(char* helpfile);

#endif
