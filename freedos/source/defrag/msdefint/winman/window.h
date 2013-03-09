#ifndef WINDOW_H_
#define WINDOW_H_

struct Window {
        int x;                     /* Top left corner.     */
        int y;

        int xlen;                  /* Bottom right corner. */
        int ylen;

        int SurfaceColor;          /* Surface color for the window.   */
        int FrameColor;            /* Color of the surrounding frame. */

        char* caption;             /* Window caption. */ 

        struct Control* controls;  /* Pointer to array of controls. */

        int    AmofControls;
};

#endif
