#ifndef MSGBXS_H_
#define MSGBXS_H_

int MessageBox(char* msg, int btncount, char** buttons,
               int forcolor, int backcolor,
               int btnforc, int btnbackc, int btnhighc,
               char* caption);

void InformUser(char* msg);
int WarningBox(char* msg, int btncount);
int ErrorBox(char* msg, int btncount, char** buttons);
void ShowModalMessage(char* msg);

#endif
