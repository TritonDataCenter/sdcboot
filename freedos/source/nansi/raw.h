/*--------------------------------------------------------------------------
 Include file for users of raw.c.
 For details, see raw.c.
 $Log:	raw.h $
 * Revision 1.2  90/07/14  09:00:55  dan_kegel
 * Added raw_set_stdio(), which should make it easy to use RAW mode.
 * 
 * Revision 1.1  90/07/09  23:11:39  dan_kegel
 * Initial revision
 * 
--------------------------------------------------------------------------*/

/* Call this routine with raw=1 at the start of your program;
 * call it with raw=0 when your program terminates.
 * If raw=1, sets stdin and stdout to RAW mode, disables break checking,
 * and turns off line buffering on stdin.
 * If raw=0, sets stdin and stdout back to non-RAW mode, and restores break
 * checking to its previous state.
 */
void raw_set_stdio(int raw);

/* A replacement for kbhit().
 * Check for char on stdin.  Returns nonzero if one is waiting, 0 if not.
 * If stdin is RAW, and break checking is off, then this won't kill
 * your program when the user types ^C (unlike the kbhit() that comes
 * with MSC).
 */
int raw_kbhit(void);

/* Building-block routines.  See raw.c for documentation. */
int raw_get(int fileno);
void raw_set(int fileno, int raw);
int break_get(void);
void break_set(int check);
