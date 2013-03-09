#ifndef SCREEN_H_
#define SCREEN_H_

#define MAX_X 80
#define MAX_Y 25

/* lowvideo.c */
void ColorScreen (int color);
void SetStatusBar (int forcolor, int backcolor, char* text);
void DelimitStatusBar (int forcolor, int backcolor, int x);
void DrawBox (int x, int y, int lenx, int leny, int forcolor, int backcolor,
              int tlc, int trc, int blc, int brc, int hc, int vc,
              char* caption);
void DrawSingleBox (int x, int y, int lenx, int leny, int forcolor, 
                    int backcolor, char* caption);
void DrawDoubleBox (int x, int y, int lenx, int leny, int forcolor, 
                    int backcolor, char* caption);
void DrawText (int x, int y, char* buf, int forcolor, int backcolor);
void DrawStatusBar (int x, int y, int len, int forcolor, int backcolor);
void DrawButton (int x, int y, int len, int forcolor, int backcolor,
                 char* caption, int selected, int shadow);
void DrawSelectionButton (int x, int y, int forcolor, int backcolor,
                          int selected, char* caption);
void ShowDialog (int posx, int posy, int xlen, int ylen, char* caption,
                 int forcolor, int backcolor);
void DrawSequence (int posx, int posy, int len, char r, int forcolor,
                   int backcolor);
void DrawVScrollBox(int posx, int posy, int xlen, int ylen, int forcolor,
                    int backcolor, int barforc, int barbackc);
void DrawStatusOnBar(int barx, int posy, int barforc, int barbackc, 
                     int amofentries, int nrofentry, int ylen, char piece);
void ChangeCharColor(int x, int y, int newfor, int newback);
void InvertLine (int x1, int x2, int y);
int cdecl GetHighIntensity(void);

/* DrawScr.c */
void DrawScreen (void);

/* ScrClip.c */
void CopyScreen (int left, int top, int right, int bottom);
void PasteScreen (int xincr, int yincr);
void SetClipIndex(int index);

/* Screen.asm */
void cdecl HideCursor (void);
char cdecl ReadCursorChar(void);
void cdecl InvertChar(void);
void cdecl SetHighIntensity(int onoff); 
int  cdecl GetCursorShape(void);
void cdecl SetCursorShape(int shape);
void cdecl DOSWipeScreen(void);
int  cdecl GetScreenLines(void);
void cdecl SetScreenLines(int lines);
int  cdecl isEGA(void); 
int  cdecl ReadCursorAttr(void);
void cdecl ChangeCursorPos(int x, int y);
void cdecl PrintString(char* string);
void cdecl PrintChar(int asciichar, int repeat);
void cdecl DrawChar(int asciichar, int repeat);
void cdecl SetForColor(int color);
void cdecl SetBackColor(int color);

#endif

