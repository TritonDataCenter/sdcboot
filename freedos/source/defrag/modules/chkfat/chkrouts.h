#ifndef CHECK_ROUTINES_H_
#define CHECK_ROUTINES_H_

struct CheckRoutineStruct {
       int (*routine)(RDWRHandle handle);
       char* message;
};

struct CheckRoutineStruct CheckRoutines[] =
       {{DescriptorCheck,      "Checking floppy boot sector."},
        {CheckDescriptorInFat, "Checking descriptor in fat."},
        {MultipleFatCheck,     "Checking fats for differences."}};

#define AMOFCHECKS (sizeof(CheckRoutines)/sizeof(*CheckRoutines))

#endif
