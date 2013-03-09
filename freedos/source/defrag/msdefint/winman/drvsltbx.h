#ifndef DRIVE_SELECT_BOX_H_
#define DRIVE_SELECT_BOX_H_

struct DriveSelectionBox {

       int   HighColor;    /* Forground color of the cursor.         */
       int   ForColor;     /* Forground color of unselected entries. */

       char* Drives;       /* String of drive letters.               */  
};

#endif
