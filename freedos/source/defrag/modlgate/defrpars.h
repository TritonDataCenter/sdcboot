#ifndef DEFRAG_PARAMETERS_H_
#define DEFRAG_PARAMETERS_H_

/* Constants for defragmentation method. */
#define NO_DEFRAGMENTATION 0
#define FULL_OPTIMIZATION  1
#define UNFRAGMENT_FILES   2
#define FILES_FIRST        3
#define DIRECTORIES_FIRST  4
#define DIRECTORIES_FILES  5
#define CRUNCH_ONLY        6
#define QUICK_TRY          7
#define COMPLETE_QUICK_TRY 8

/* Constants for sort options. */
#define UNSORTED          0
#define NAMESORTED        1
#define EXTENSIONSORTED   2
#define DATETIMESORTED    3
#define SIZESORTED        4

/* Sort orders. */       
#define ASCENDING         0
#define DESCENDING        1

int  SetOptimizationDrive(char drive);
char GetOptimizationDrive(void);
void SetSortOptions(int criterium, int order);
int  GetSortCriterium (void);
void SetOptimizationMethod(int method);
int  GetOptimizationMethod(void);
int  GetSortOrder (void);

#endif