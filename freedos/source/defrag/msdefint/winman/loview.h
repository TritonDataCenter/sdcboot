#ifndef LOW_VIEW_H_
#define LOW_VIEW_H_

struct LowView {

       /* Data members. */
       int top;                   /* The top line of the view control. */

       void* ControlData;

       /* Event handlers. */
       /* Fill a certain screen full of text. */
       void (*FillControl)(struct Control* control, int top);

       /* Check wether we didn't scroll to far. */
       int (*PastTop)(struct Control* control, int top);

       /* Adjust scroll bar */
       void (*UpdateScrollBar)(struct Control* control);

       /*
          Remark: the events:
                  - OnClick
                  - HandleEvent

                  from VerticalScrollControl must be filled in by the
                  concrete view control.

      */
};


#endif
