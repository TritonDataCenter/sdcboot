#ifndef CHKARGS_H_
#define CHKARGS_H_

void ParseInteractiveArguments (int argc, char** argv, char optchar);
char GetParsedDrive(void);
int  IsRebootRequested(void);
int  MustAutomaticallyExit(void);
int  IsMethodEntered(void);

#endif
