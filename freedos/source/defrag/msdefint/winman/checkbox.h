#ifndef CHECKBOX_H_
#define CHECKBOX_H_

struct CheckBox {

       char* caption;          /* Caption of the button.        */

       int   Checked;          /* Wether the button is pressed. */

       int   ActiveColor;      /* Color when activated.         */

       int   PushLength;       /* Length of pushable area.      */
};

#endif
