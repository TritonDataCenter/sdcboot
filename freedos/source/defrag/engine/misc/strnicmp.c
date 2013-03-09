/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
#include <ctype.h>

int
strnicmp(const char *s1, const char *s2, size_t n)
{

  if (n == 0)
    return 0;
  do {
    if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2++))
      return (int)tolower((unsigned char)*s1) - (int)tolower((unsigned char)*--s2);
    if (*s1++ == 0)
      break;
  } while (--n != 0);
  return 0;
}
