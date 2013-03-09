#ifndef ARGVARS_H_
#define ARGVARS_H_

void MethodIsEntered(void);
int  IsMethodEntered(void);

void SetParsedDrive(char drive);
char GetParsedDrive(void);

void RequestReboot(void);
int  IsRebootRequested(void);

void AutomaticallyExit(void);
int  MustAutomaticallyExit(void);

void SetFullOutput(void);
int  GiveFullOutput(void);

void SetAudibleWarning(void);
int  GiveAudibleWarning(void);

#endif
