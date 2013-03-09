#ifndef EVENT_BUTTON_H_
#define EVENT_BUTTON_H_

struct EventButton {

       struct Control* control;

       int (*OnPress) (void* EventData);

       void* EventData;
};


#endif
