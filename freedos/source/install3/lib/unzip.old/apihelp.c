/* apihelp.c */

#ifdef API_DOC

#define UNZIP_INTERNAL
#include "unzip.h"
#include "version.h"


APIDocStruct APIDoc[] = {
    {
        "UZPMAIN"  , "UzpMain"  ,
        "int UzpMain(int argc, char *argv[]);",
        "Provide a direct entry point to the command line interface.\n\n"
        "\t\tThis is used by the UnZip stub but you can use it in your\n"
        "\t\town program as well.  Output is sent to STDOUT.\n"
        "\t\t0 on return indicates success.\n\n"
        "  Example:\t/* Extract 'test.zip' silently, junking paths. */\n"
        "\t\tchar *argv[] = { \"-q\", \"-j\", \"test.zip\" };\n"
        "\t\tint argc = 3;\n"
        "\t\tif (UzpUnZip(argc,argv))\n"
        "\t\t  printf(\"error: unzip failed\\n\");\n\n"
        "\t\tSee unzipapi.h for details.\n"
    },
  
    {
        "UZPUNZIPTOMEMORY", "UzpUnzipToMemory",
        "int UzpUnzipToMemory(char *zip, char *file, UzpBuffer *retstr);",
        "Pass the name of the zip file and the name of the file\n"
        "\t\tyou wish to extract.  UzpUnzipToMemory will create a\n"
        "\t\tbuffer and return it in *retstr;  0 on return indicates\n"
        "\t\tfailure.\n\n"
        "\t\tSee unzipapi.h for details.\n"
    },
  
    {
        "UZPFILETREE", "UzpFileTree",
        "int UzpFileTree(char *name, cbList(callBack),\n"
        "\t\t\tchar *cpInclude[], char *cpExclude[]);",
        "Pass the name of the zip file, a callback function, an\n"
        "\t\tinclude and exclude file list. UzpUnzipToMemory calls\n"
        "\t\tthe callback for each valid file found in the zip file.\n"
        "\t\t0 on return indicates failure.\n\n"
        "\t\tSee unzipapi.h for details.\n"
    },
  
    { 0 }
};



int function_help(__G__ doc, fname)
    __GDEF
    APIDocStruct *doc;
    char *fname;
{
    strcpy(slide, fname);
    strupr(slide);
    while (doc->compare && strcmp(doc->compare,slide))
        doc++;
    if (!doc->compare)
        return 0;
    else
        Info(slide, 0, ((char *)slide,
          "  Function:\t%s\n\n  Syntax:\t%s\n\n  Purpose:\t%s",
          doc->function, doc->syntax, doc->purpose));

    return 1;
}  



void APIhelp(__G__ argc, argv)
    __GDEF
    int argc;
    char **argv;
{
    if (argc > 1) {
        struct APIDocStruct *doc;

        if (function_help(__G__ APIDoc, argv[1]))
            return;
#ifdef SYSTEM_API_DETAILS
        if (function_help(__G__ SYSTEM_API_DETAILS, argv[1]))
            return;
#endif
        Info(slide, 0, ((char *)slide,
          "%s is not a documented command.\n\n",argv[1]));
    }

    Info(slide, 0, ((char *)slide,"\
This API provides a number of external C and REXX functions for handling\n\
zipfiles in OS/2.  Programmers are encouraged to expand this API.\n\
\n\
C functions: -- See unzipapi.h for details\n\
  int UzpMain(int argc, char *argv[]);\n\
  int UzpAltMain(int argc, char *argv[], struct _UzpInit *UzpInit);\n\
  int UzpUnzipToMemory(char *zip, char *file, UzpBuffer *retstr);\n\
  int UzpFileTree(char *name, cbList(callBack),\n\
		  char *cpInclude[], char *cpExclude[]);\n\n"));

#ifdef SYSTEM_API_BRIEF
    Info(slide, 0, ((char *)slide, SYSTEM_API_BRIEF));
#endif

    Info(slide, 0, ((char *)slide,
      "\nFor more information, type 'unzip -A <function-name>'"));
}

#endif /* API_DOC */
