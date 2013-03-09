#ifndef _CONTROLS_H_
#define _CONTROLS_H_

#include "..\winman\cmdbtn.h"
#include "..\winman\frame.h"
#include "..\winman\timlabel.h"
#include "..\winman\slctbtn.h"
#include "..\winman\vscrctrl.h"
#include "..\winman\lolstbx.h"
#include "..\winman\drvsltbx.h"
#include "..\winman\loview.h"

struct Control CreateHLine(int posx, int posy, int len,
                           int forcolor, int backcolor);

struct Control CreateCommandButton(struct CommandButton* button, 
                                   int forcolor, int backcolor, 
                                   int posx, int posy,
                                   int Default, int BeforDefault,
                                   int MustBeTab);

struct Control CreateFrame(struct Frame* frame, int forcolor, int backcolor,
                           int posx, int posy);

struct Control CreateLabel(char* caption, int forcolor, int backcolor,
                           int posx, int posy);

struct Control CreateTimeLabel(struct TimeLabel* timelabel, 
                               int forcolor, int backcolor,
                               int posx, int posy);

struct Control CreateSelectionButton(struct SelectButton* button, 
                                     int forcolor, int backcolor, 
                                     int posx, int posy);

struct Control CreateVScrollBox(struct VerticalScrollControl* box, 
                                int posx, int posy, 
                                int forcolor, int backcolor);

struct Control CreateLowListBox(struct LowListBox* box,
                                struct VerticalScrollControl* VBox,
                                int posx, int posy,
                                int forcolor, int backcolor);

struct Control CreateDriveSelectionBox(struct DriveSelectionBox* box,
                                       struct LowListBox* LBox,
                                       struct VerticalScrollControl* VBox,
                                       int posx, int posy,
                                       int forcolor, int backcolor);

struct Control CreateWindowCloser(int forcolor, int backcolor,
	                          int posx, int posy);


struct Control CreateLowView(struct LowView* view,
                             struct VerticalScrollControl* VBox,
                             int posx, int posy,
                             int forcolor, int backcolor);


struct Control CreateHelpView(struct LowView* LView,
                              struct VerticalScrollControl* VBox,
                              int posx, int posy,
                              int forcolor, int backcolor);

#endif
