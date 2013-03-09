#define INCL_NOPM
#define INCL_DOSPROCESS
#include <os2.h>

void sleep(int x)
{
  DosSleep(1000L * x);
}
