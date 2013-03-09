#include "..\engine\header\FTE.h"

#include "callback.h"
#include "expected.h"


static struct CallBackStruct* CallBacks;

void SetCallBacks(struct CallBackStruct* callbacks)
{
    CallBacks = callbacks;
}

void UpdateInterfaceState(void)
{
    CallBacks->UpdateInterface();
}

void SmallMessage(char* buffer)
{
    CallBacks->SmallMessage(buffer);
}

void LargeMessage(char* buffer)
{
    CallBacks->LargeMessage(buffer);
}

void DrawOnDriveMap(CLUSTER cluster, int symbol)
{
    CallBacks->DrawOnDriveMap(cluster, symbol);
}

void DrawMoreOnDriveMap(CLUSTER cluster, int symbol, unsigned long n)
{
    CallBacks->DrawMoreOnDriveMap(cluster, symbol, n);
}

void DrawDriveMap(CLUSTER maxcluster)
{
    CallBacks->DrawDriveMap(maxcluster);
}

void LogMessage(char* message)
{
    CallBacks->LogMessage(message);
}

int QuerySaveState(void)
{
    return CallBacks->QuerySaveState();
}

void IndicatePercentageDone(CLUSTER cluster, CLUSTER totalclusters)
{
    CallBacks->IndicatePercentageDone(cluster, totalclusters);
}

unsigned GetMaximumDrvMapBlock(void)
{
    return CallBacks->GetMaximumDrvMapBlock();
}
