

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "4all.h"

void pascal strout(char *, unsigned int);
void pascal intout(char *, unsigned int);
void pascal bytout(char *, unsigned char);

void ini_dump(void) {
   unsigned int keycnt;
   unsigned int *keyptr, *subptr;

   qputs("D to dump INI file, any other key to continue ...");
   if (toupper(GetKeystroke(NO_ECHO | ECHO_CRLF)) != 'D')
      return;
   intout("*   INI data addr", *(unsigned int *)gpIniptr);
   intout("*  ptr to strings", (unsigned int)gpIniptr->StrData);
   intout("*     max strings", gpIniptr->StrMax);
   intout("*  actual strings", gpIniptr->StrUsed);
   intout("*     ptr to keys", (unsigned int)gpIniptr->Keys);
   intout("*        max keys", gpIniptr->KeyMax);
   intout("*     actual keys", gpIniptr->KeyUsed);
   intout("*    section bits", gpIniptr->SecFlag);
   strout("       4StartPath", gpIniptr->FSPath);
   strout("          LogName", gpIniptr->LogName);
   strout("      NextINIFile", gpIniptr->NextININame);
   strout("*   prim INI file", gpIniptr->PrimaryININame);
   intout("            Alias", gpIniptr->AliasSize);
   intout("       BeepLength", gpIniptr->BeepDur);
   intout("         BeepFreq", gpIniptr->BeepFreq);
   intout("       CursorOver", gpIniptr->CursO);
   intout("        CursorIns", gpIniptr->CursI);
   intout("   DescriptionMax", gpIniptr->DescriptMax);
   intout("        DirColors", gpIniptr->DirColor);
   intout("      Environment", gpIniptr->EnvSize);
   intout("          HistMin", gpIniptr->HistMin);
   intout("          History", gpIniptr->HistorySize);
   intout("     HistWinColor", gpIniptr->HWColor);
   intout("    HistWinHeight", gpIniptr->HWHeight);
   intout("      HistWinLeft", gpIniptr->HWLeft);
GetKeystroke(NO_ECHO);
   intout("       HistWinTop", gpIniptr->HWTop);
   intout("     HistWinWidth", gpIniptr->HWWidth);
   intout("       ListColors", gpIniptr->ListColor);
   intout("       ScreenRows", gpIniptr->Rows);
   intout("        StdColors", gpIniptr->StdColor);
   intout("      debug flags", gpIniptr->INIDebug);
   intout("*     shell level", gpIniptr->ShellLevel);
   intout("*    shell number", gpIniptr->ShellNum);

   bytout("             AmPm", gpIniptr->TimeFmt);
   bytout("        BatchEcho", gpIniptr->BatEcho);
   bytout("         EditMode", gpIniptr->EditMode);
   bytout("          Inherit", gpIniptr->Inherit);
   bytout("        LineInput", gpIniptr->LineIn);
   bytout("        NoClobber", gpIniptr->NoClobber);
   bytout("     PauseOnError", gpIniptr->PauseErr);
   bytout("        UpperCase", gpIniptr->Upper);
   bytout("       CommandSep", gpIniptr->CmdSep);
   bytout("       EscapeChar", gpIniptr->EscChr);
   bytout("    ParameterChar", gpIniptr->ParamChr);
   bytout("       boot drive", gpIniptr->BootDrive);
   bytout("     logging flag", gpIniptr->LogOn);
   bytout("       SwitchChar", gpIniptr->SwChr);
GetKeystroke(NO_ECHO);

   strout("     AutoExecPath", gpIniptr->AEPath);
   strout("      HelpOptions", gpIniptr->HOptions);
   strout("         HelpPath", gpIniptr->HPath);
   strout("         Swapping", gpIniptr->Swap);
   intout("*       alias loc", (unsigned int)gpIniptr->AliasLoc);
   intout("*     history loc", (unsigned int)gpIniptr->HistLoc);
   intout("          EnvFree", gpIniptr->EnvFree);
   intout("        StackSize", gpIniptr->StackSize);
   intout("*  local mast env", gpIniptr->EnvSeg);
   intout("*   transient seg", gpIniptr->HighSeg);
   intout("* global mast env", gpIniptr->MastSeg);
GetKeystroke(NO_ECHO);

   bytout("      ChangeTitle", gpIniptr->ChangeTitle);
   bytout("           CopyEA", gpIniptr->CopyEA);
   bytout("         CritFail", gpIniptr->CritFail);
   bytout("        DiskReset", gpIniptr->DiskReset);
   bytout("           DRSets", gpIniptr->DRSets);
   bytout("        DVCleanup", gpIniptr->DVCleanup);
   bytout("         FineSwap", gpIniptr->FineSwap);
   bytout("        FullINT2E", gpIniptr->FullINT2E);
   bytout("    MessageServer", gpIniptr->MsgServer);
   bytout("     NetwareNames", gpIniptr->NWNames);
   bytout("       SwapReopen", gpIniptr->Reopen);
   bytout("           Reduce", gpIniptr->Reduce);
   bytout("       ReserveTPA", gpIniptr->ReserveTPA);
   bytout("   UMBEnvironment", gpIniptr->UMBEnv);
   bytout("          UMBLoad", gpIniptr->UMBLd);
   bytout("   UniqueSwapName", gpIniptr->USwap);

   bytout("             ANSI", gpIniptr->ANSI);
   bytout("*       Win3 mode", gpIniptr->WinMode);
   bytout("*         DV flag", gpIniptr->DVMode);
   bytout("*     swap method", gpIniptr->SwapMeth);

   intout("*    INI signature", gpIniptr->INISig);
GetKeystroke(NO_ECHO);

   keyptr = gpIniptr->Keys;
   if ((keycnt = gpIniptr->KeyUsed) == 0) {
      printf("\n  No key substitutions\n");
   } else {
      printf("\n  %u key substitution(s):\n", keycnt);
      for (subptr = keyptr + keycnt; keycnt > 0; keycnt--) {
         printf("    type %u, key %u, substitute %u\n", (*keyptr >> 14),
            ((*keyptr++) & 0x3FFF), *subptr++);
      }
   }
GetKeystroke(NO_ECHO);
}

void pascal strout(char *name, unsigned int strloc) {
   printf("%s = %s\n", name, (strloc == 0xFFFF ? "NULL" : &gpIniptr->StrData[strloc]));
}

void pascal intout(char *name, unsigned int intval) {
  printf("%s = %d\n", name, intval);
}

void pascal bytout(char *name, unsigned char byteval){
  printf("%s = %d\n", name, (unsigned int)byteval);
}

