#ifndef CALLBACK_H_
#define CALLBACK_H_

struct CallBackStruct 
{
    void (*UpdateInterface)(void);
    void (*SmallMessage)(char* buffer);
    void (*LargeMessage)(char* buffer);
    void (*DrawDriveMap)(CLUSTER maxcluster);
    void (*DrawOnDriveMap)(CLUSTER cluster, int symbol);
    void (*DrawMoreOnDriveMap)(CLUSTER cluster, int symbol, unsigned long n);
    void (*LogMessage)(char* message);
    int  (*QuerySaveState)(void);
    void (*IndicatePercentageDone)(CLUSTER cluster, CLUSTER totalclusters);
    unsigned (*GetMaximumDrvMapBlock)(void);
};

void SetCallBacks(struct CallBackStruct* callbacks);

#endif
