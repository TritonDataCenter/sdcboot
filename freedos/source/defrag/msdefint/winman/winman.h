#ifndef WINMAN_H_
#define WINMAN_H_

void OpenWindow(struct Window* winpars);
int  ControlWindow(struct Window* winpars);
void CloseWindow(void);
void CheckExternalEvent(int event);

#endif
