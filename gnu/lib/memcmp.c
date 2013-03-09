/* memcmp.c -- compare memory.
   Return:
   <0 if S1 < S2,
   0 if strings are identical,
   >0 if S1 > S2.
   Stops looking after N characters.  Doesn't stop at nulls.
   In the public domain.
   By David MacKenzie <djm@ai.mit.edu>. */

int
memcmp (s1, s2, n)
     register char *s1, *s2;
     register unsigned n;
{
  register int diff;

  while (n--)
    {
      diff = *s1++ - *s2++;
      if (diff)
	return diff;
    }
  return 0;
}
