/* EXE file header fixer, code taken from Morten Welinder */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>			/* For broken DOS compilers, should be unistd */

static char copyright[] =
 "\r\nCWSDPMI r7 Copyright (C) 2010 CW Sandmann (cwsdpmi@earthlink.net).\r\n"
 "The stub loader is Copyright (C) 1993-1995 DJ Delorie.\r\n"
 "Permission granted to use for any purpose provided this copyright\r\n"
 "remains present and unmodified.\r\n"
 "This only applies to the stub, and not necessarily the whole program.\r\n";

/* DOS EXE header:
   MZ sign, MOD 512 length, BLK 512 length, Number Relocations, Header Size,
   Min Memory, Max Memory, Prog Memory, SP, Checksum, IP, CS, Reloc Pos */

int main(int argc, char **argv)
{
  int f;
  unsigned short us, test;
  char stub, msg[512];

  if(argc != 2) {
    printf("Usage: ehdrfix cwsdpmi.exe\n       Updates heap size in exe header\n");
    exit(1);
  }

  f = open(argv[1], O_RDWR | O_BINARY);
  if (f < 0) {
    perror(argv[1]);
    exit(1);
  }

  lseek(f, 0x200L, SEEK_SET);
  read(f, msg, 8);
  if(!memcmp(msg,"go32stub",8))
    stub = 1;
  else
    stub = 0;

  lseek(f, 0x0aL, SEEK_SET);
  read(f, &us, sizeof(us));
  read(f, &test, sizeof(us));
  if(test == 0xffff) {		/* Not set yet */
    int add,add2;
    FILE *fh;
  
    fh = fopen("control.c","r");
    if(fh) {
      char buffer[256];
      add = 4096;		/* Default _heaplen (can be trimmed) */
      add2 = 0;
      while(!fscanf(fh,"extern unsigned _stklen = %dU", &add2)) {
        if(!fgets(buffer,sizeof(buffer),fh)) break;
      }
      fclose(fh);
      if(add && add2) {
        add /= 16;			/* Kb to paragraphs */
        add2 /= 16;
      } else {
        printf("Can't find stack size in control.c!\n");
        exit(2);
      }
    } else {
      printf("Can't find control.c!\n");
      exit(3);
    }
    
    us += add2;
    lseek(f, 0x0aL, SEEK_SET);
    write(f, &us, sizeof(us));	/* Update min memory */
    us += add;
    write(f, &us, sizeof(us));	/* Update max memory */

    if(stub) {
      lseek(f, 0x02L, SEEK_SET);
      read(f, &us, sizeof(us));   /* Modulo 512 bytes */
      us = 512 - us;
      test = 0;
      lseek(f, 0x02L, SEEK_SET);
      write(f, &test, sizeof(test));	/* Update Modulo */
      lseek(f, 0x0aL, SEEK_SET);
      memset(msg, 0, sizeof(msg));
      lseek(f, 0L, SEEK_END);
      write(f, msg, us);
      printf("Padded %d bytes\n",us);
    } else {
      char *p;
      
      p = strstr(copyright,"The ");
      if(p)
        *p = 0;
    }

    lseek(f, 0x06L, SEEK_SET);
    read(f, &us, sizeof(us));   /* Number of relocations */
    lseek(f, 0x18L, SEEK_SET);
    read(f, &test, sizeof(us));   /* Location of relocations */
    test += ++us * 4;
    if(test + strlen(copyright) > 511) {
      printf("Copyright too large for header: %d.\n",test);
      copyright[511-test] = 0;
    }
    lseek(f, (long)test, SEEK_SET);
    write(f, copyright, strlen(copyright));

  } else
    printf("%s already updated.\n",argv[0]);

  return close(f);
}
