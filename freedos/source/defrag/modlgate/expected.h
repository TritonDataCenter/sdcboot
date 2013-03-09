#ifndef EXPECTED_H_ 
#define EXPECTED_H_

/* Routines expected to be implemented in the interface. */

void UpdateInterfaceState(void);
void SmallMessage(char* buffer);
void LargeMessage(char* buffer);
void DrawOnDriveMap(CLUSTER cluster, int symbol);
void DrawDriveMap(CLUSTER maxcluster);
void LogMessage(char* message);
void DrawMoreOnDriveMap(CLUSTER cluster, int symbol, unsigned long n);

int  QuerySaveState(void);

void IndicatePercentageDone(CLUSTER cluster, CLUSTER totalclusters);

unsigned GetMaximumDrvMapBlock(void);


#define WRITESYMBOL     1
#define READSYMBOL      2
#define USEDSYMBOL      3
#define UNUSEDSYMBOL    4
#define BADSYMBOL       5
#define UNMOVABLESYMBOL 6   
#define OPTIMIZEDSYMBOL 7

#endif
