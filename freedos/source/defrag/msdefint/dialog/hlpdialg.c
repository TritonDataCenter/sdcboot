/*  Help dialog

    Copyright (c) Imre Leber 2000.
    Modified by Joe Cosentino 2000.

*/

#include <stdlib.h>
#include "..\screen\screen.h"
#include "..\dialog\dialog.h"
#include "..\winman\control.h"
#include "..\winman\loview.h"
#include "..\winman\controls.h"
#include "..\winman\window.h"
#include "..\winman\winman.h"
#include "..\winman\vscrctrl.h"

#include "..\helpsys\idxstack.h"
#include "..\helpsys\hlpfncs.h"

#include "..\..\misc\bool.h"

#define DIALOG_X_LEN 76
#define DIALOG_Y_LEN 20

#define DIALOG_X      ((MAX_X / 2) - (DIALOG_X_LEN / 2))
#define DIALOG_Y      ((MAX_Y / 2) - (DIALOG_Y_LEN / 2)) + 1
#define HELP_X        DIALOG_X + 1
#define HELP_Y        DIALOG_Y + 1
#define HELP_X_LEN    DIALOG_X_LEN - 3
#define HELP_Y_LEN    DIALOG_Y_LEN - 1
#define BUTTON_LENGTH 10
#define BUTTON_X      DIALOG_X + (DIALOG_X_LEN / 2) - (BUTTON_LENGTH / 2)
#define BUTTON_Y      DIALOG_Y + 17

#define AMOFCONTROLS 2

static struct Control controls[AMOFCONTROLS];

static struct LowView view = {0, NULL, NULL, NULL, NULL};
static struct VerticalScrollControl VBox =
              {HELP_X_LEN, HELP_Y_LEN,       /* xlen, ylen         */
               0,0,0,                        /* BAR: Y1, Y2, X     */
               DIALOGBACKCOLOR,              /* Border back color. */
               BLUE,                         /* Border for color.  */
               0,                            /* Answers events.    */
               0,                            /* SavedEntry         */
               NULL,                         /* ControlData        */
               NULL,                         /* OneUp              */
               NULL,                         /* OneDown            */
               NULL,                         /* OnClick            */
               NULL,                         /* EntryChosen        */
               NULL,                         /* HandleEvent        */
               NULL,                         /* DrawControl        */
               NULL,                         /* OnEntering         */
               NULL,                         /* OnLeaving          */
               NULL};                        /* AdjustScrollBar    */

static struct Window HelpDialog = {DIALOG_X, DIALOG_Y, DIALOG_X_LEN, DIALOG_Y_LEN, DIALOGBACKCOLOR, DIALOGFORCOLOR, " Help system ", controls, AMOFCONTROLS};

static void Initialize(void)
{
    static int Initialized = FALSE;

    if (!Initialized)
        Initialized = TRUE;
    else
        return;

    controls[0] = CreateHelpView(&view, &VBox, HELP_X, HELP_Y, DIALOGFORCOLOR, DIALOGBACKCOLOR);
    controls[1] = CreateWindowCloser(DIALOGFORCOLOR, DIALOGBACKCOLOR,
                                     DIALOG_X+2, DIALOG_Y);

} 

void ShowHelpSystem()
{
    Initialize();

    SelectHelpPage(TopHelpIndex());

    OpenWindow(&HelpDialog);
    ControlWindow(&HelpDialog);
    CloseWindow();
    
    view.top = 0; /* Fix top since we only initialise the help system once! */
}
