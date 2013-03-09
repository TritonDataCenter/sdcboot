#ifndef _VERTICALSCROLL_CONTROL_H_
#define _VERTICALSCROLL_CONTROL_H_

struct VerticalScrollControl {

       /* data members. */
       int xlen;        /* x length of control. */
       int ylen;        /* y length of control. */
       
       int BarY1;       /* Top position of vertical scrollbar.    */
       int BarY2;       /* Bottom position of vertical scrollbar. */
       int BarX;        /* X position of vertical scrollbar.      */

       int BorderForC;  /* Forground color of scroll bar.          */
       int BorderBackC; /* Background color of scroll bar.         */
       
       
       int AnswersEvents; /* Should HandleEvent be called. */

       int SavedEntry;  /* Last position of cursor on bar. */

       void* ControlData; /* Control data for descendent. */
       
       /* Event handlers. */
       
       /* Event raised when up or down pressed or when pressed on 
          the terminating arrows.                                 */
       void (*OneUp)  (struct Control* This);
       void (*OneDown)(struct Control* This);

       /* Event raised when clicked inside the control area. 
          Position still in mouse management routines.            
          Returns: event answered (something on that position?). */
       int (*OnClick)(struct Control* This);
       
       /* Event raised when pressed on the scroll bar. */
       void (*EntryChosen)(struct Control* This, int index);
       
       /* Called after the event has been processed by this control. */
       int (*HandleEvent)(struct Control* This, int event);   
       
       /* Draw control event. */
       void (*DrawControl) (struct Control* _this);

       /* On entering and leaving events. */
       void (*OnEntering) (struct Control* This);
       void (*OnLeaving)  (struct Control* This);

       /* Call back to adjust the scroll bar. */
       void (*AdjustScrollBar)(struct Control* control, int index, 
                               int amofentries);
};

#endif
