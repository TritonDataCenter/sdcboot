#ifndef SCREEN_MASKS_H_
#define SCREEN_MASKS_H_

void DrawTime (int hours, int minutes, int seconds);
void DrawBlockSize (CLUSTER size);
void DrawMethod (char* method);
void SetStatusOnBar (int thiscluster, int endcluster);
void ClearStatusBar (void);
void DrawCurrentDrive (char drive);
void DrawStatus(CLUSTER cluster, CLUSTER amountofclusters);
void DrawFunctionKey(int key, char* action);

#endif
