#ifndef HELP_LINE_PARSE_H_
#define HELP_LINE_PARSE_H_

int   IsHlpReference(char* line);
int   IsFullLine(char* line);
int   IsHlpAsciiChar(char* line);
int   IsCenterOnLine(char* line);
char* PassHlpReferencePart(char* line);
char* PassHlpFullLinePart(char* line);
char* PassHlpAsciiChar(char* line);
char* PassCenterOnLinePart(char* line);
char* PassNextHelpPart(char* line);
void  GetHlpLinkCaption(char* line, char** start, char** end);
int   GetHlpLinkIndex(char* line);
char  GetHlpAsciiChar(char* line);
void  GetHlpTextToCenter(char* line, char** begin, char** end);

#endif
