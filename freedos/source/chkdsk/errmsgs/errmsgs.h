#ifndef ERROR_MESSAGES_H_
#define ERROR_MESSAGES_H_

int ReportFileSysError(char* message,
                       unsigned numfixes,
                       char** fixmessages,
                       unsigned defaultfix,
                       BOOL allowignore);

int ReportFileSysWarning(char* message,
                         unsigned numfixes,
                         char** fixmessages,
                         unsigned defaultfix,
                         BOOL allowignore);

void ReportGeneralRemark(char* message);                         

#define REPORT_IGNORED -1

#endif