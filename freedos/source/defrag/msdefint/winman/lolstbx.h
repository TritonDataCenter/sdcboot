#ifndef LOLISTBOX_H_
#define LOLISTBOX_H_

struct LowListBox {

       /* Data members. */
       int top;         /* The entry on the top of the visible part. */
       int CursorPos;   /* Position of the cursor.                   */
       
       void* ControlData; 
       
       /* Event handlers. */ 
       /* Get number of entries. */
       int (*AmOfEntries)(struct Control* This);
       
       /* Print a certain entry on the screen. */
       void (*PrintEntry)(struct Control* This, int top, int index);
       void (*SelectEntry)(struct Control* This, int top, int index);
       void (*LeaveEntry)(struct Control* This, int top, int index);
       void (*EnterEntry)(struct Control* This, int top, int index);
};

#endif
