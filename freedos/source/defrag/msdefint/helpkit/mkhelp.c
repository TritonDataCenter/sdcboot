#include <stdio.h>
#include "mkhelp.h"

static void ShowHelp(void);

int main(int argc, char** argv)
{
  if (argc != 3)
  {
     ShowHelp();
     return 0;
  }

  return CreateHelpFile(argv[1], argv[2]);
}

static void ShowHelp(void)
{
   printf("\nMkhelp " VERSION "\n"
          "Help utility to combine text files in one large help file.\n"
          "\n"
          "Syntax:\n"
          "\tMkhelp <listfile> <helpfile>\n"
          "\n"
          "listfile: file containing a list of the files that have to be included\n"
          "helpfile: file to contain the output\n");
}
