#ifndef SELECT_BUTTONS_H_
#define SELECT_BUTTONS_H_

struct SelectButton {

       char*  Caption;         /* The captions on the buttons.      */
       
       int    selected;        /* is the button selected.           */

       int    HighColor;       /* Highlight color of the captions.  */

       int    PushLength;      /* Length of the pushable area.      */

       int    position;        /* Position of the button in the     */
                               /* rest of the group.                */
       
       struct Control* others; /* Pointer to the group to 
                                          which the button belongs. */   

       int    AmInGroup;       /* Amount of buttons in the group.    */  

       void   (*ClearSelection) (struct Control* _this); /* Pointer to 
                                                           a function to
                                                           indicate the
                                                           selection of an
                                                           other button.    */
};


#endif
