/* +++Date last modified: 24-dec-1999 by Imre Leber                    */
/*         added DOSemu and FreeDOS tests.                             */
/*
**  OS_ID.C
**
**  Based upon public domain works by David Gibbs & Stephen Lindholm
*/

#ifdef test
#undef test
#endif

#define  OS_ID_MAIN
#include "OS_ID.h"
#include "dosemu.h"
#include <dos.h>

struct i_os_ver id_os_ver[TOT_OS];
int id_os_type;
int id_os = -1;

const char *id_os_name[TOT_OS] = {
      "FreeDOS",
      "DOS",
      "OS/2 DOS",
      "DESQview",
      "Windows Std",
      "Windows 386",
      "Linux DOSEMU"
      };

/*
**  get_os() - Determine OS in use
*/

int get_os (void)
{
      union REGS t_regs;
      long osmajor, osminor;
      unsigned status;

      if (id_os == -1)
      {
         id_os_type = 0;
         id_os = 0;

         /* test for DOS or OS/2 */

         t_regs.h.ah = 0x30;
         int86(0x21, &t_regs, &t_regs);
         osmajor = t_regs.h.al;
         osminor = t_regs.h.ah;

         if (osmajor < 10)
         {
            id_os_ver[DOS].maj = osmajor;
            id_os_ver[DOS].min = osminor;

            /* Look for FreeDOS. */
            if (t_regs.h.bh == 0xfd)
            {
               id_os_ver[FreeDOS].maj = 0;
               id_os_ver[FreeDOS].min = 0;

               id_os_type = is_FreeDOS;
            }
            else
               id_os_type = is_DOS;
         }
         else
         {
            /* OS/2 v1.x DOS Box returns 0x0A */

            id_os_type = id_os_type | is_OS2;

            /* OS/2 v2.x DOS Box returns 0x14 */

            id_os_ver[OS2].maj = osmajor/10;
            id_os_ver[OS2].min = osminor;
         }

         /* test for Windows */

         t_regs.x.ax = 0x1600;         /* check enhanced mode operation    */
         int86(0x2F, &t_regs, &t_regs);
         status = t_regs.h.al;

         if ((0x00 == status) || (0x80 == status))
         {
            /*
            ** Can't trust it...
            **  let's check if 3.1 is running in standard mode or what?
            */

            t_regs.x.ax = 0x160A;
            int86( 0x2F, &t_regs, &t_regs );
            if (0 == t_regs.x.ax)
            {
                  id_os_ver[WINS].maj = t_regs.h.bh;
                  id_os_ver[WINS].min = t_regs.h.bl;
                  id_os_type = id_os_type | is_WINS;
            }
         }
         else if ((0x01 == status) || (0xff == status))
         {
            id_os_ver[WINS].maj = 2;
            id_os_ver[WINS].min = 1;
            id_os_type = id_os_type | is_WINS;
         }
         else
         {
            id_os_ver[WINS].maj = t_regs.h.al;
            id_os_ver[WINS].min = t_regs.h.ah;
            id_os_type = id_os_type | is_WINS;
         }

         /* Test for DESQview */

         t_regs.x.cx = 0x4445;                /* load incorrect date */
         t_regs.x.dx = 0x5351;
         t_regs.x.ax = 0x2B01;                /*  DV set up call     */

         intdos(&t_regs, &t_regs);
         if (t_regs.h.al != 0xFF)
         {
            id_os_type = id_os_type | is_DESQVIEW;
            id_os_ver[DESQVIEW].maj = t_regs.h.bh;
            id_os_ver[DESQVIEW].min = t_regs.h.bl;
         }

         /* Test for linux DOSEMU. */
         if ((osmajor = is_dosemu()) != 0)
         {
            id_os_ver[DOSEMU].maj = osmajor;
            id_os_ver[DOSEMU].min = 0;
            id_os_type = id_os_type | is_DOSEMU;
         }

         if (id_os_type & is_FreeDOS)
            id_os = FreeDOS;
         if (id_os_type & is_DOS)
            id_os = DOS;
         if (id_os_type & is_WINS)
            id_os = WINS;
         if (id_os_type & is_WIN3)
            id_os = WIN3;
         if (id_os_type & is_DESQVIEW)
            id_os = DESQVIEW;
         if (id_os_type & is_OS2)
            id_os = OS2;
         if (id_os_type & is_DOSEMU)
            id_os = DOSEMU;
      }

      return(id_os);
}

#ifdef TEST

#include <stdio.h>

int main(void)
{
      int ostype = get_os();

      printf("%s version %d.%d\n",
            id_os_name[ostype],
            id_os_ver[ostype].maj,
            id_os_ver[ostype].min);

      return(0);
}

#endif /* TEST */
