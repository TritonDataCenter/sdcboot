/* Copyright (C) 2000 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef GO32
#include <unistd.h>
#else
#include <io.h>
#endif

#include "gotypes.h"
#include "control.h"

char view_only = 0;
int size_of_stubinfo = 0;
char *client_stub_info;
char buffer[30000];
int f;

void find_info(char *filename)
{
  int i,size;
  
  f = open(filename, O_RDWR | O_BINARY);
  if (f < 0) {
    f = open(filename, O_RDONLY | O_BINARY);
    if (f < 0) {
      perror(filename);
      exit(1);
    }
    if(!view_only) {
      view_only = 1; 
      printf("%s is read only, you can only view:\n",filename);
    }
  }

  size = read(f, buffer, sizeof(buffer));

  client_stub_info = NULL;
  for(i=0; i<size && !client_stub_info; i++)
    if(buffer[i] == 'C' && !strcmp(buffer+i+1,"WSPBLK")) {
      client_stub_info = (buffer+i);
      size_of_stubinfo = sizeof(CWSDPMI_pblk);
    }

  if(!client_stub_info) {
    printf("Parameter block magic not found in %s!\n",filename);
    exit(2);
  }

  lseek(f, i-1, SEEK_SET);	/* Ready to update */
  return;
}

void store_info(void)
{
  write(f, client_stub_info, size_of_stubinfo);
  *(short *)(buffer+0x0c) += *(short *)(buffer+0x0a);
  lseek(f, 0x0c, SEEK_SET);	/* Ready to update */
  write(f, buffer+0x0c, 2);
}

char *pose_question(char *question, char *default_answer)
{
  static char response[200];
  printf("%s ? [%s] ", question, default_answer);
  fflush(stdout);
  gets(response);
  if (response[0] == '\0')
    return 0;
  return response;
}

typedef void (*PerFunc)(void *address_of_field, char *buffer);

void str_v2s(void *addr, char *buf, int len)
{
  if (*(char *)addr == 0)
    strcpy(buf, "\"\"");
  else
  {
    buf[len] = 0;
    strncpy(buf, (char *)addr, len);
  }
}

void str_s2v(void *addr, char *buf, int len)
{
  if (strcmp(buf, "\"\"") == 0)
    *(char *)addr = 0;
  else
  {
    ((char *)addr)[len-1] = 0;
    strncpy((char *)addr, buf, len);
  }
}

void str_v2s48(void *addr, char *buf)
{
  str_v2s(addr, buf, 48);
  if (strcmp(buf, "\"\"") == 0)
    strcpy(buf,"*Disabled*");
}

void str_s2v48(void *addr, char *buf)
{
  str_s2v(addr, buf, 48);
}

void num_v2s(void *addr, char *buf)
{
  unsigned v = *(unsigned short *)addr;
  sprintf(buf, "%u", v);
}

void num_s2v(void *addr, char *buf)
{
  unsigned r = 0;
  sscanf(buf, "%i", &r);
  *(unsigned short *)addr = r;
}

void num_vlp2s(void *addr, char *buf)
{
  unsigned long v = *(unsigned long *)addr;
  if(v%256)
    sprintf(buf, "%luKb", v*4);
  else
    sprintf(buf, "%uMb", (unsigned)(v/256));
}

void num_s2vlp(void *addr, char *buf)
{
  unsigned long r = 0;
  char s = 0;
  sscanf(buf, "%li%c", &r, &s);
  switch (s)
  {
    case 'k':
    case 'K':
      r *= 1024L;
      break;
    case 'm':
    case 'M':
      r *= 1048576L;
      break;
  }
  *(unsigned long *)addr = (r+4095L)/4096L;
}

void num_vpt2s(void *addr, char *buf)
{
  unsigned long v = *(unsigned short *)addr;
  num_vlp2s((void *)&v, buf);
}

void num_s2vpt(void *addr, char *buf)
{
  unsigned long r;
  num_s2vlp((void *)&r, buf);
  *(unsigned short *)addr = (unsigned short)r;
}

#define Ofs(n) ((int)&(((CWSDPMI_pblk *)0)->n))

struct {
  char *short_name;
  char *long_name;
  int offset_of_field;
  PerFunc val2string;
  PerFunc string2val;
} per_field[] = {
  {
    "swapfile",
    "Full name of paging file (\"\" to disable)",
    Ofs(swapname),
    str_v2s48, str_s2v48
  },
  {
    "pagetable",
    "Number of page tables to initially allocate (0=auto)",
    Ofs(pagedir),
    num_v2s, num_s2v
  },
  {
    "minappmem",
    "Minimum application memory desired before 640K paging",
    Ofs(minapp),
    num_vpt2s, num_s2vpt
  },
  {
    "savepara",
    "Paragraphs of DOS memory to reserve when 640K paging",
    Ofs(savepar),
    num_v2s, num_s2v
  },
  {
    "heap",
    "Paragraphs of memory for extra CWSDPMI internal heap",
    -2,
    num_v2s, num_s2v
  },
  {
    "maxswap",
    "Maximum size of swap file",
    Ofs(maxdblock),
    num_vlp2s, num_s2vlp
  },
  {
    "flags",
    "Value of run option flags",
    Ofs(flags),
    num_v2s, num_s2v
  }
};

#define NUM_FIELDS (sizeof(per_field) / sizeof(per_field[0]))

#define HFORMAT "%-18s %s\n"

void give_help(void)
{
  int i;
  fprintf(stderr, "Usage: cwsparam [-v] [-h] [cwsdpmi.exe] [field=value . . . ]\n");
  fprintf(stderr, "-h = give help   -v = view info  field=value means set w/o prompt\n");
  fprintf(stderr, HFORMAT, "-field-", "-description-");

  for (i=0; i < NUM_FIELDS; i++)
    fprintf(stderr, HFORMAT, per_field[i].short_name, per_field[i].long_name);
  exit(1);
}

int main(int argc, char **argv)
{
  int i;
  char need_to_save;

  if (argc > 1 && strcmp(argv[1], "-v") == 0)
  {
    view_only = 1;
    argc--;
    argv++;
  }

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    give_help();

  if (argc < 2 || strchr(argv[1],'=') != NULL)
    find_info("cwsdpmi.exe");
  else {
    find_info(argv[1]);
    argc--;
    argv++;
  }
  /* This hack is for the heap which isn't in the stubinfo struct */
  per_field[4].offset_of_field = (int)(buffer+0x0c - client_stub_info);
  *(short *)(buffer+0x0c) -= *(short *)(buffer+0x0a);

  if (view_only)
  {
    char buf[100];
    fprintf(stderr, HFORMAT, "-value-", "-field description-");
    for (i=0; i<NUM_FIELDS; i++)
    {
      if (per_field[i].offset_of_field < size_of_stubinfo)
      {
        per_field[i].val2string(client_stub_info + per_field[i].offset_of_field, buf);
        fprintf(stderr, HFORMAT, buf, per_field[i].long_name);
      }
    }
    exit(0);
  }

  if (argc > 1)
  {
    int field;
    char got, got_any = 0;
    char fname[100], fval[100];
    for (i=1; i < argc; i++)
    {
      fname[0] = 0;
      fval[0] = 0;
      sscanf(argv[i], "%[^=]=%s", fname, fval);
      got = 0;
      for (field=0; field<NUM_FIELDS; field++)
      {
        if (strcmp(per_field[field].short_name, fname) == 0)
        {
          got = 1;
          got_any = 1;
          if (per_field[field].offset_of_field < size_of_stubinfo)
          {
            per_field[field].string2val(client_stub_info + per_field[field].offset_of_field, fval);
          }
          else
            fprintf(stderr, "Warning: This image does not support field %s\n", fname);
        }
      }
      if (!got)
      {
        fprintf(stderr, "Error: %s is not a valid field name.\n", fname);
        give_help();
      }
    }
    if (got_any)
      store_info();
    close(f);
    return 0;
  }

  need_to_save = 0;
  for (i=0; i<NUM_FIELDS; i++)
  {
    char buf[100], *resp;
    if (per_field[i].offset_of_field < size_of_stubinfo)
    {
      per_field[i].val2string(client_stub_info + per_field[i].offset_of_field, buf);
      if ((resp = pose_question(per_field[i].long_name, buf)) != 0)
      {
        per_field[i].string2val(client_stub_info + per_field[i].offset_of_field, resp);
        need_to_save = 1;
      }
    }
  }
  if (need_to_save)
    store_info();
  close(f);

  return 0;
}
