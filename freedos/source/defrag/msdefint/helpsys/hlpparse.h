#ifndef HELP_PARSE_H_
#define HELP_PARSE_H_

int    ParseHelpFile(char* RawData, size_t bufsize);
size_t GetHelpLineCount(void);
char*  GetHelpLine(int line);
void   FreeHelpSysData(void);

#endif
