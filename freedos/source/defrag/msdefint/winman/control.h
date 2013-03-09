#ifndef CONTROL_H_
#define CONTROL_H_

struct Control {
        
        /* Properties. */
        /* User oriented. */
        int forcolor;     /* Control forground color.  */
        int backcolor;    /* Control background color. */

        int posx;         /* Left x position. */
        int posy;         /* Top y position.  */
       
        /* Control oriented. */
        int Visible:1;      /* Is this a visible control? */

        int CanEnter;     /* Can the control receive focus. */
        int HandlesEvents;/* Handles events?                */

        int DefaultControl;/* Is this a default control? */
        int BeforeDefault; /* Does this control have a higher priority 
                              than the default control?                */

        int CursorUsage;  /* What to do with cursor movements. */
        int MustBeTab;    /* The control cannot be entered with a key
                             except for tab/shift-tab.                 */

        int active;       /* Is this control active? */

        /* Events. */

        /* v Returns how the control has reacted to the event. */
        int  (*OnEvent) (struct Control* control, int event);
        
        void (*OnEnter) (struct Control* control);
        void (*OnLeave) (struct Control* control);
        void (*OnRedraw)(struct Control* control);

        void (*OnDefault)  (struct Control* control);
        void (*OnUnDefault)(struct Control* control);

        void (*OnNormalDraw)(struct Control* control);
        
        /* Individual control data. */
        void* ControlData;

        int  SecondPass;
};

/* Event return codes. */
#define NOT_ANSWERED     0       /* The event not answered.                */
#define EVENT_ANSWERED   1       /* The event answered (processed).        */
#define LEAVE_WINDOW     2       /* Request to close window.               */
#define NEXT_CONTROL     3       /* Request to go to next control.         */
#define PREVIOUS_CONTROL 4       /* Request to go to the previous control. */
#define CONTROL_ACTIVATE 5       /* Activate the control wich raised this 
                                    event.                                 */
#define REQUEST_AGAIN    6       /* Make sure that i can change my 
                                    appearance then send the event again.  */

/* Cursor usage. */
#define CURSOR_DEFAULT 0     /* Let the window manager decide.             */
#define CURSOR_SELF    1     /* We will handle cursor movement our selves. */


/* Function to facilitate control initialisation. */
void InitializeControlValues(struct Control* control);

#endif
