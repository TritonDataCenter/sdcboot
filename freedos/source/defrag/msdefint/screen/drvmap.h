#ifndef DRIVEMAP_H_
#define DRIVEMAP_H_

void DrawDrvMap(CLUSTER maxcluster);

void DrawWriteBlock(CLUSTER cluster);
void DrawReadBlock(CLUSTER cluster);
void DrawBadBlock(CLUSTER cluster);
void DrawUnmovableBlock(CLUSTER cluster);
void DrawUnusedBlock(CLUSTER cluster);
void DrawOptimizedBlock(CLUSTER cluster);
void DrawUsedBlock(CLUSTER cluster);

void DrawUsedBlocks(CLUSTER cluster,      unsigned long n);
void DrawOptimizedBlocks(CLUSTER cluster, unsigned long n);
void DrawUnusedBlocks(CLUSTER cluster,     unsigned long n);
void DrawUnmovableBlocks(CLUSTER cluster,  unsigned long n);
void DrawBadBlocks(CLUSTER cluster,       unsigned long n);
void DrawReadBlocks(CLUSTER cluster,      unsigned long n);
void DrawWriteBlocks(CLUSTER cluster,     unsigned long n);


#define BLOCKSPERLINE 72
#define MAXLINE       15
#define MAXBLOCK      (BLOCKSPERLINE * MAXLINE)

#endif
