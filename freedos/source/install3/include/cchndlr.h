/*
  Control-C handler - SIGINT
*/

#ifndef CCHNDLR_H
#define CCHNDLR_H

#ifdef __cplusplus
extern "C" {
#endif


/* call to register our SIGINT handler */
void registerSIGINTHandler(void);

/* call to restore original SIGINT handler */
void unregisterSIGINTHandler(void);

/* Reregisters handler if some other process (such as unzip) de-registers it */
void reregisterSIGINTHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* CCHNDLR_H */

