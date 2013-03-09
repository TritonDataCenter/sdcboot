
                 XMGR, RDISK, and UIDE -- DOS Device Drivers
               ===============================================

1. Description
   -----------

   XMGR, RDISK, and the UIDE drivers are a set of DOS device drivers, that
   run on systems with an 80386+ CPU and using MS-DOS V5.0+ or equivalent.

   XMGR is a DOS driver that works as an XMS memory manager.   It provides
   up to 4-GB of XMS memory and is written to the V3.0 XMS Specifications.
   XMGR has direct support for V3.70+ UMBPCI by Uwe Sieber.   After UMBPCI
   enables upper-memory, XMGR can load there directly and offer both upper
   and XMS memory to a DOS system.   XMGR uses an I-O catcher with UMBPCI,
   to "catch" hard-disk or diskette I-O above 640K.   Such I-O goes thru a
   low memory area, to avoid DMA problems with UMBPCI "Shadow RAM".   XMGR
   also works with JEMM386 and V4.49/V4.95 EMM386 (MS-DOS V6.22 or V7.10).
   With an EMM driver, XMGR using its /B switch first "boots" in temporary
   space.   After the EMM driver enables upper-memory, XMGR can load there
   with no /B switch, copy all of its "boot" data, and take-over XMS work.
   Only its XMS "Handles" table remains in low memory.   With a small XMS-
   only system, XMGR can also load entirely in low memory.

   RDISK is a DOS RAM-disk driver.    It creates a "fast" disk drive using
   2 Megabytes to 2 GIGABYTES of XMS memory!   It loads as a system driver
   in CONFIG.SYS, or it can load later in AUTOEXEC.BAT or by user command.
   DOS can copy critical programs, data files, and other directories which
   need fast access (compiler TEMP files, databases, etc.) to the RAMdisk,
   where they can be read and written at memory speeds!    If loaded after
   CONFIG.SYS, RDISK files can be assigned to any free DOS drive letter by
   using its /: switch.   RDISK runs with V2.0 or V3.0 XMS managers (60-MB
   maximum with V2.0 XMS).   It uses only 656 to 752 bytes of upper-memory
   depending on the system configuration, and it can also load in 640K DOS
   memory.   RDISK is a small "load it and forget about it" driver for use
   when resizing or other features are not needed.

   UIDE is a DOS "Universal IDE" caching driver.   It intercepts "Int 13h"
   BIOS I-O requests and caches data for up to 34 BIOS disks, including A:
   and B: diskettes, and including up to 2-Terabyte disks.    UIDE accepts
   48-bit LBA or 24-bit CHS I-O calls by new or old DOS systems.   It uses
   its own UltraDMA logic for SATA or IDE hard-disks and will run up to 10
   "Legacy" or "Native PCI" IDE controllers.   All possible I-O is done in
   its cache memory for fast speed.   UIDE "calls the BIOS" for diskettes,
   SCSI, and other disk models, so it can cache ALL disks on a DOS system!
   "Int 13h" drivers can load before UIDE, and it will intercept and cache
   I-O for their disks, also.    ("ASPI" and other drivers that do not use
   Int 13h I-O are unsupported).

   UIDE also supports up to 8 CD/DVD drives, including SATA, IDE, or older
   "PIO mode" drives.   On loading, it checks up to 10 "Legacy" or "Native
   PCI" IDE controllers and runs the first 8 CD/DVD drives found.     UIDE
   does file input for SHCDX33E or other "CD-ROM Redirectors" like MSCDEX,
   and it caches all file data and directories for faster speed!   It also
   supports DOS "audio" requests and will "play back" an audio CD.   Data,
   audio playback, and "trackwriter" programs can be handled by UIDE using
   multiple drives on one system.    Audio and "trackwriting" is uncached.

   UIDE caches 5 Megabytes to 4 GIGABYTES of data!   It uses 5280 bytes of
   upper-memory for any size cache.   All its cache tables and data are in
   XMS memory.   A "stand alone" UIDE (/B switch, no caching or diskettes)
   can be used for tests/diagnostics and takes 3792 bytes of upper-memory.
   UIDE can also load in 640K DOS memory, and its /H switch can be used to
   put most of the driver in free HMA space, thus taking only 944 bytes of
   upper- or DOS memory.

   UIDE can also be called by a user driver to cache data for its read and
   write requests.   Read the file UIDE.TXT for all details.

   The UIDE2 driver is a special "variant" of UIDE.   UIDE2 uses old style
   protected-mode caching (binary-search table is in memory, not XMS!) for
   faster speed when using JEMM386 or other protected-mode schemes.   Read
   the initial comments in the UIDE.ASM source file for full details.   On
   most DOS systems, the standard UIDE driver should work O.K.

   UIDEJR ("junior" UIDE) is also available.   UIDEJR omits caching but it
   still runs up to 10 SATA/IDE controllers, 32 SATA/IDE hard-disks, and 4
   or 8 SATA/IDE/"PIO mode" CD/DVD drives (8 drives using its /U8 switch).
   The full UIDEJR for disks/CDs/DVDs uses 3168 bytes of memory, or 768 if
   its /H switch puts most driver logic in "free HMA" space.   UIDEJR will
   optimize its run-time size by omitting unneeded logic and the hard-disk
   stack when its "CD/DVD only" or "disk only" options are requested.   In
   HMA space, the CD/DVD-only UIDEJR (/N1 /H switches) takes 176 bytes and
   the disk-only UIDEJR (/N2 /H) takes 688 bytes of upper-memory!   UIDEJR
   thus directly replaces "old" XDMA/XCDROM/GCDROM drivers.    UIDEJR will
   also load in 640K DOS memory, when needed.   It usually sets a 128K XMS
   memory buffer, so "misaligned" I-O can still be done at UltraDMA speed.
   UIDEJR can play audio CDs and will do "raw" (trackwriter) CD/DVD input.

   Except as noted, all UIDE comments in this README also apply for UIDEJR
   and for UIDE2.

   The small RDISKON.COM program can "re-enable" a DOS drive used by RDISK
   if a "format" command is accidentally issued to it.   This disables the
   drive on some systems!    Entering  RDISKON L  at a DOS command-prompt,
   where L is the desired drive letter (A to Z), will re-enable the drive.

   The small CC.COM "Clear Cache" program can help verify files written by
   UIDE.   Entering  CC  at the DOS command-prompt sends a BIOS "reset" to
   all disks, making UIDE discard its cache.    Data on the disk (NOT data
   still in the cache!) can then be compared with the original output.


2. NO Warranties
   -------------

   XMGR, RDISK, and all UIDE drivers are offered at no cost, "as-is", "use
   at your own risk", and with NO warranties, not even an implied warranty
   of FITNESS for any particular purpose nor of MERCHANTABILITY!

   Driver questions or comments may be addressed to the website of Johnson
   Lam, <johnson@tmfc.net>.


3. Revision Summary
   ----------------

   16-Oct-11   UIDE /M switch deleted, search-table buffer now always 512
                 bytes.   UIDE.SYS is back to 7.5K, UIDE-S is eliminated!
                 UIDE2 improved, other drivers unchanged (re-dated only).

    7-Oct-11   All UIDE drivers updated to avoid BIOS "DPTE" ERRORS:  BAD
                 "DPTE" data for "hard disk" USB sticks!   Many Thanks to
                 Daniel Nice!   XMGR and RDISK unchanged (re-dated only).

   30-Sep-11   UIDE.ASM now assembles UIDE, UIDE2 or UIDE-S.   All handle
                 10 controllers, 34 BIOS units, 8 CD/DVD drives.

   23-Sep-11   UIDE2 /HL now takes 4608 bytes but caches up to 280-MB for
                 V7.10 MS-DOS, more on other DOS systems!

   20-Sep-11   Minor UIDEJR error fixed ("No CD" name must be "UIDEJR$").

   18-Sep-11   UIDE2 error (non-cached CD/DVD input in protected mode) is
                 corrected.   UIDE-S or UIDE2 now handle 8 CD/DVD drives!
                 UIDE-S now has a 512-byte search buffer, /M can set 256.

    9-Sep-11   UIDE2 re-added.   UIDE-S and UIDE2 handle 6 CD/DVD drives.

   22-Jul-11   UIDE /E switch added for DOS emulators (VirtualBox, etc.).

   16-Jun-11   UIDEJR error detecting multiple CD/DVD units is corrected.

    7-Jun-11   UIDE and UIDE-S /X switch deleted.   UIDE /M switch added,
                 to save HMA but avoid speed loss!   UIDE-S size reduced.

   20-May-11   UIDE-S ("short" UIDE) added, for systems with limited HMA.
                 See the initial UIDE.ASM comments, for details.

   16-May-11   More "code mod" init errors corrected in XMGR/UIDE/UIDEJR.
                 RDISK unaffected.   The author's apologies; users should
                 update NOW to the 16-May-2011 drivers!

   14-May-11   UIDE/UIDEJR disk "timeout" now 7 seconds (was 0.4 sec) for
                 "laptop" disk spin-ups.   Disk-only UIDEJR at 1504 bytes
                 (was 1600!) in memory, still 688 bytes using /H.

   10-May-11   UIDEJR sizes minimized!   UIDEJR now runs 4 CD/DVD drives,
                 new /U8 switch runs 8 when needed.

    2-May-11   UIDE corrected to flush its cache before CD/DVD input on a
                 "VDS Lock" error.   /X switch added to UIDE.

   25-Apr-11   BAD "code mods" init error corrected for UIDE, UIDEJR, and
                 RDISK (XMGR not affected)!    Users should update NOW to
                 the 25-Apr-2011 drivers!

   16-Apr-11   UIDE user-driver caching improved and re-added.   Read the
                 file UIDE.TXT for all details.

    5-Dec-10   UIDE/UIDEJR /R15 and /R63 switches added to handle old DOS
                 "games" (Thanks, Guillermo Grana Gomez!).

   28-Nov-10   Minor updates:  UIDEJR audio track-number error corrected,
                 XMGR protected-mode speed improved.   /Z switch added in
                 XMGR and UIDE.

   15-Aug-10   UIDE audio track-number error corrected (Thanks, Nagatoshi
                 Uehara!).   Faster speed for UIDE cache-table updates.

   10-Aug-10   UIDE binary-search buffer added.   Using $ in CD/DVD names
                 fixed in UIDE/UIDEJR (Thanks, Japheth!).

    4-Aug-10   Greater UIDE protected-mode speed, "protected" and "memory
                 only" caches deleted to simplify UIDE, /N4 switch added.

   13-Jul-10   Fast UIDE "memory only" cache added via /PM switch.

    4-Jul-10   README file update -- XMGR/UIDE can use "Native IDE" mode,
                 same as "Legacy"/"Compatibility", for AHCI mainboards.

   28-Jun-10   XMGR updated for AHCI, see the README sec. 7 for details.

   16-Jun-10   UIDE /E switch and external-entry logic summarily deleted.

   10-Jun-10   UIDE now ignores a "removable HARD disk", size reduced.

   27-Nov-09   UIDE /HL switch added for larger /P caches, /N3 fixed.

   22-Nov-09   UIDE uses more-optimal cache blocks, UIDE /F switch added.
                 UPX packing of UIDE/UIDEJR dropped (avoids loader woes).

   16-Nov-09   UIDE now caches 4 GIGABYTES of data, UIDE /P switch added.
                 XMGR/UIDE/UIDEJR now handle 32 BIOS disks (not 40).

   19-Oct-09   UIDEJR "VDS lock" error fixed:  CD/DVD logic now dismissed
                 only by its /N2 switch.   UIDE logic reduced.

   15-Oct-09   XMGR/UIDE /K switch deleted re: Win95/98 trouble.   Thanks
                 to Japheth for his "warning"!

    6-Oct-09   UIDE and UIDEJR /H requests HMA use "at the user's risk"!

    3-Sep-09   XMGR now does protected-mode move "calls" (as in UIDE) for
                 faster speed using RDISK, etc.   XMGR /K switch added.

    2-Sep-09   README file updated -- FreeDOS users who desire full upper
                 memory must omit UMBPCI and load JEMM386/JEMMEX only.

   23-Jun-09   RDISK now a .COM file.    RDISK /: switch, RDISKON program
                 added.   Corrected UIDE CD/DVD handling of VDS errors.

    9-Jun-09   UIDE/UIDEJR /N3 switch added for no XMS memory.   Override
                 of /D: name by UIDE$/UIDEJR$ added for no CD/DVD drives.

   15-May-09   Added the RDISK driver.

    6-May-09   Added the non-caching UIDEJR driver.   UIDE and UIDEJR now
                 run up to 10 controllers, 36 disks, 8 CD/DVD drives.

    1-May-09   Fixed XMGR "Port 92h" logic error.  Added XMGR /PA and /PN
                 switches to control use of "Port 92h" logic.

   25-Apr-09   XMGR/UIDE license and FreeDOS prohibition deleted, drivers
                 and sources are again available to all.   Corrected UIDE
                 CD/DVD "return codes".   UIDE /R switch re-added.


4. Switch Options
   --------------

   XMGR usually needs only its /B switch, if "booting" with an EMM driver.
      All XMGR switch options are as follows:

      /B     Specifies "boot" mode.   XMGR loads in temporary memory until
                JEMM386 enables upper-memory.   Without /B, XMGR will load
                stand-alone in low memory or directly in upper-memory with
                UMBPCI.   See the CONFIG.SYS examples in section 5.

      /Mn    Specifies the temporary area used to load XMGR in "boot" mode
                and used for UMBPCI upper memory I-O before DOS can post a
                "workspace" buffer.   Values are:

                    /M1 = 64K.    /M3 = 192K.   /M5 = 320K.   /M7 = 448K.
                    /M2 = 128K.   /M4 = 256K.   /M6 = 384K.   /M8 = 512K.

                Without /M, /M5 is assumed and the 320K area will be used.
                NOTE:  A DOS system often may NOT load at address 0 up and
                may put temporary data anywhere in memory!    /Mn helps in
                finding a "safe" place for XMGR to use.   /M is ignored if
                XMGR loads stand-alone.

      /Nnn   Specifies how many XMS "Handles" can be used by DOS programs.
                The value nn may be 48, 80, or 128.   If /N is omitted, 48
                "Handles" are used.   A big system doing much XMS work may
                need 80 or 128 "Handles".

      /PA    Specifies use or non-use of PS/2 Port 92h logic to handle the
      /PN       system's "A20" line.   /PA indicates "Always" use Port 92h
                logic.   /PN indicates "Never" use it and handle "A20" via
                normal keyboard-port logic.   If /P is omitted, XMGR "asks
                the BIOS" if the system has Port 92h logic.   If not, XMGR
                will use normal "A20" logic.   NOTE:  If "A20" was enabled
                by DOS before XMGR loads, XMGR does not handle it at all!

      /Tn    Specifies the BIOS requests to use in getting extended memory
                as follows:

                   /T0   Neither "E820h" nor "E801h" requests.
                   /T1   Memory-list requests only (Int 15h, AX=E820h).
                   /T2   A dual-area request only  (Int 15h, AX=E801h).
                   /T3   "E820h" requests first, then an "E801h" request.

                /T can usually be omitted, which causes /T3 to be assumed.
                In addition, XMGR always uses an old 64-MB request, to get
                extended memory for /T0, or if the requests specified with
                /T1 through /T3 are unsuccessful.   Users may need to test
                /T1 and /T2 separately, to see if their BIOS accepts them.
                A pre-1994 BIOS may not "ignore" /T1 thru /T3 properly and
                may require /T0 to be used.   For compatibility with older
                QHIMEM drivers, /T4 thru /T7 may be used and work the same
                as /T0 thru /T3.

      /W     Specifies use of the DOS "workspace" buffer, for upper-memory
                I-O if loading with UMBPCI.    If /W is omitted, or if the
                DOS system does not have proper workspace logic, XMGR will
                set its own buffer in low memory.   For PC-DOS or EDR-DOS,
                /W must be omitted!   Without UMBPCI, /W is ignored.

      /Z     See /Z for UIDE, below.

             --------------------

   RDISK uses only /S size and /: drive-letter switches:

      /Sn    Specifies a desired RAM-disk size in megabytes of XMS memory.
                Values may be any number from 2 to 2047.    /S1024 or more
                creates a 1- to 2-GIGABYTE RAM-disk!   If /S is omitted or
                invalid, a 25-MB RAM-disk is created by default.   For old
                V2.0 XMS managers (ROM-DOS etc.) only /S2 through /S60 may
                be used.   See section 5 below for more details.

      /:L    Specifies the DOS drive letter desired to access RDISK files.
                L may be any available drive letter from A to Z, e.g.  /:N
                assigns drive N: to all RDISK files.   If the drive letter
                is too high or already in use, RDISK will abort, and users
                may need "LASTDRIVE=" in CONFIG.SYS to set up more drives.
                If RDISK is loaded by CONFIG.SYS, or if /: is omitted, the
                next free drive letter will be used.

             --------------------

   All "UIDE" drivers usually need only a /D: switch to specify the device
   name used by the CD-ROM Redirector (MSCDEX, etc.).   UIDE or UIDE2 also
   need a /S switch to set their cache size.    "UIDE" switch options are:

      /A     Specifies ALTERNATE "legacy IDE" I-O addresses.     The first
                legacy controller uses 01E8h/0168h addresses, and a second
                controller uses 01F0h/0170h addresses, REVERSED from usual
                mainboard and BIOS-program designs.   /A is rarely needed.
                /A does not affect "Native PCI" controllers.

      /B     For UIDE and UIDE2, requests a basic UltraDMA driver for disk
                or CD/DVD drives (no cache nor diskettes).   This may help
                with tests and diagnostics.   The basic driver can request
                128K of XMS memory for its I-O buffer and can be loaded in
                HMA space.   UIDEJR will ignore /B.

      /D:    Specifies the desired device name, used by SHCDX33E to access
                the CD/DVD drives.   Example:  /D:CDROM1  /D:MYCDROM  etc.
                If /D: is not given, or the name after a /D: is missing or
                invalid,  UDVD1  is used by default.   If no CD/DVD drives
                are present,  UIDE$  or  UIDEJR$  overrides any /D: name.

      /E     For UIDE and UIDE2, makes the driver "call the BIOS" for each
                hard disk I-O request.   /E avoids setup trouble with some
                DOS "emulators" (VirtualBox, etc.) that do NOT emulate all
                hardware logic!   /E still provides hard-disk data caching
                (unlike /N1 that removes ALL disk support!).   UIDEJR will
                ignore /E.

                ***** NOTE *****

                Use of /E on protected-mode systems (JEMM386 etc.) may run
                VERY slow!    Many "cheap" BIOS programs have no DOS "VDS"
                logic for hard-disks, and in protected-mode, they can only
                do old "PIO mode" data transfers, not UltraDMA!   If /E is
                needed, users should run their system in real-mode (UMBPCI
                etc.) whenever possible.

      /F     For UIDE only, requests a "fast" cache using 32K cache blocks
                with a 40-MB or 50-MB cache, and 64K blocks with an 80-MB+
                cache, for more speed with fewer blocks.   Without /F, 40-
                to 1023-MB caches use 16K blocks, and 1024-MB thru 2047-MB
                caches use 32K blocks, for more cache capacity due to less
                "lost space" in a file's last cache block.   /F is ignored
                with /S2048 or more, as 2-GB+ caches must have 64K blocks.
                UIDE2 and UIDEJR will ignore /F.

      /H     Loads most of the driver in "free HMA" space.   UIDE or UIDE2
      /HL       will use 944 bytes of upper/DOS memory and UIDEJR will use
                768 bytes.   For UIDE2 only, /HL uses 4608 bytes but gives
                115-MB larger HMA caches.   /H or /HL may NOT be used with
                ROM-DOS, which has no HMA!   When /N3 is also given, /H or
                /HL will be ignored.

                ***** NOTE *****

                MS-DOS kernels have ERRORS in posting free HMA space which
                can give CRASHES!   Specifying /H is "At the user's risk"!
                No such crashes are noted for other DOS systems, but users
                must TEST a system before using /H with any "UIDE" driver!

      /N1    Requests NO handling of hard-disk drives.    For UIDEJR only,
                all disk routines plus the driver stack are dismissed, and
                UIDEJR then takes only 2032 bytes of memory, or 176 memory
                and 1856 HMA bytes when /H is given!

      /N2    Requests NO handling of CD or DVD drives.    For UIDEJR only,
                all CD/DVD routines are dismissed and the driver will take
                only 1504 bytes of memory, or 688 memory and 816 HMA bytes
                when /H is given!

      /N3    Requests no XMS memory.   /N3 requires loading in low memory,
                or the driver aborts!   With UIDE or UIDE2, /N3 causes the
                /B "basic" driver to be used.    /N3 will LOSE much speed,
                as "misaligned" or other I-O unsuited to UltraDMA must use
                BIOS logic for a hard disk or PIO-mode for a CD/DVD drive!
                UIDEJR will ignore /N3.

      /N4    See /Z below.

      /Q     Enables awaiting "data request" before starting UltraDMA disk
                transfers.   /Q is for "old" systems and must be used only
                if UIDE loads O.K. but seems unable to transfer data.   /Q
                must be OMITTED with a SATA-to-IDE adapter from Sabrent or
                others, as such cards do not emulate "data request" from a
                SATA disk!   /Q does not affect CD/DVD drives.

      /R15   Sets the driver's XMS memory at 16-MB or 64-MB.    /R15 shall
      /R63      reserve 15-MB of XMS, and /R63 shall reserve 63-MB of XMS,
                for old DOS "game" programs which require XMS memory below
                16-MB or 64-MB!   The drivers must be able to reserve this
                memory, reserve their required XMS above that, then "free"
                the 15/63-MB XMS.    If not, the driver displays "XMS init
                error" and aborts!   /R15 or /R63 require that the drivers
                load after the user's XMS manager (XMGR, HIMEMX, etc.), so
                another driver cannot take any XMS first, and the reserved
                XMS is just beyond the HMA area.   See section 7 below for
                further details.

      /Sn    For UIDE or UIDE2, specifies a user's cache size in megabytes
                of XMS memory.    Values for /S are 5, 15, 25, 40, 50, and
                any number from 80 to 4093.   /S1024 or more gives a 1- to
                4-GIGABYTE cache!   Recommended values are --

                    Below 128-MB memory:     Use /S5 /S15 /S25 or /S40

                    With 128-MB memory:      Use /S25 /S40 /S50 or /S80
                    With 256-MB memory:      Use /S80  up to /S127
                    With 512-MB memory:      Use /S160 up to /S255

                    With 1-GB memory:        Use /S320  up to /S511
                    With 2-GB memory:        Use /S640  up to /S1023
                    With 4-GB memory:        Use /S1280 up to /S3072

                Small systems may prefer /S25 or /S50 which set 1600 cache
                blocks and are more efficient.   If /S is omitted/invalid,
                an 80-MB cache is set.   Except for 25 or 50, values below
                80 are cut to 40- 15- or 5-MB.    The drivers display "XMS
                init error" and abort, when not-enough XMS memory is free!
                If so, a smaller cache must be requested.   For older V2.0
                XMS managers (ROM-DOS etc.), only /S5 to /S50 may be used.
                UIDE2 has other rules for /S values.    UIDEJR ignores /S.

      /U8    For UIDEJR only, handles up to 8 CD/DVD drives, not 4.   This
                adds 80 bytes to UIDEJR's size.   /U8 is rarely needed, as
                few PCs have over 4 CDs/DVDs.   UIDE and UIDE2 will ignore
                /U8, since they always handle up to 8 CD/DVD drives.

      /UX    Disables all CD/DVD UltraDMA, even for drives that can do it.
                PIO-mode is then used on all CD/DVD requests.   Except for
                some "unusual" drives by Sony, etc. that do not follow all
                ATAPI "rules", /UX is rarely needed.   /UX does not affect
                hard-disks.

      /Z     For XMGR, UIDE, or UIDE2, limits XMS moves to a maximum of 2K
                bytes in protected-mode, not 64K.    /Z is not needed with
                JEMM386/JEMMEX, MS-DOS EMM386, or real-mode UMBPCI.   With
                other protected-mode EMM/VCPI/DPMI drivers, systems should
                be TESTED, to find if /Z may be needed with XMGR, UIDE, or
                UIDE2 -- BAD schemes allowing NOT enough interrupts during
                XMS moves could still exist!    The old UIDE /N4 switch is
                the same as /Z and can still be given.   UIDEJR ignores /Z
                or /N4, since it calls the XMS manager to move XMS data.

             --------------------

   For all switches in each driver, a dash may replace the slash and lower
   case letters may be used if desired.


5. Setup and Configuration
   -----------------------

   XMGR and the UIDE drivers are loaded using the CONFIG.SYS file.    Your
   CONFIG.SYS should have command lines similar to the following examples:

      DEVICE=C:\DOSDVRS\XMGR.SYS /N128 /B

      DEVICEHIGH=C:\BIN\UIDE.SYS /S3072 /H /D:MYDVD     ;3 Gigabyte cache!

      DEVICEHIGH=C:\MYSYSTEM\UIDEJR.SYS /D:TOSHIBA1

      DEVICEHIGH=C:\DRIVERS\RDISK.COM /S500

   With V3.70+ UMBPCI and XMGR, a "boot" procedure is not needed!   UMBPCI
   loads first to enable upper-memory, then XMGR loads to offer it and XMS
   to DOS, then other drivers may load.    For V6.22/V7.10 MS-DOS, JEMM386
   can also be loaded, to offer extra upper-memory in the "video graphics"
   areas, or if other JEMM386 features are desired.

   NOTE:  FreeDOS and some other DOS variants will NOT "add up" the memory
   found by both UMBPCI and JEMM386, like MS-DOS does!   FreeDOS users who
   want extra upper-memory or other items must omit UMBPCI and load JEMMEX
   or HIMEMX/JEMM386 per their instructions, or load XMGR/JEMM386 as shown
   in the 3rd example below.

   An example CONFIG.SYS file using V3.70+ UMBPCI and XMGR is as follows:

      SHELL=C:\DOS\COMMAND.COM C:\DOS /E:512 /P
      DEVICE=C:\BIN\UMBPCI.SYS
      DEVICE=C:\BIN\XMGR.SYS /W
      DOS=HIGH,UMB
      DEVICE=C:\BIN\JEMM386.EXE I=B000-B7FF X=C800-EFFF NOEMS   ;Optional
      DEVICEHIGH=C:\BIN\UIDE.SYS /S500 /D:CDROM1                ;Or others
      DEVICEHIGH=C:\BIN\RDISK.COM /S250                         ;Optional
          ..
          ..  Etc.
          ..

   XMGR can be used stand-alone, for a small XMS-only system.   It must be
   the first DOS system driver to load, and it must load in LOW memory, as
   in the following example:

      SHELL=C:\DOS\COMMAND.COM C:\DOS /E:512 /P
      DEVICE=C:\BIN\XMGR.SYS
      DOS=HIGH
      DEVICE=C:\BIN\UIDE.SYS /S80 /D:MYCDROM               ;Or others
      DEVICE=C:\BIN\RDISK.COM /S20                         ;Optional
          ..
          ..  Etc.
          ..

   With JEMM386 and XMGR,  XMGR loads first in "boot" mode,  then JEMM386,
   and then XMGR finally loads in upper-memory.   JEMMEX can also be used,
   and if so, XMGR can be omitted.   An example CONFIG.SYS file which uses
   the XMGR "boot" procedure is as follows:

      SHELL=C:\DOS\COMMAND.COM C:\DOS /E:512 /P
      DEVICE=C:\BIN\XMGR.SYS /B                           ;/B for "boot"
      DOS=HIGH,UMB
      DEVICE=C:\DOS\JEMM386.EXE I=B000-B7FF NOEMS ...     ;Or JEMMEX here
      DEVICEHIGH=C:\BIN\XMGR.SYS                          ;No "boot" here!
      DEVICEHIGH=C:\BIN\UIDE.SYS /S1023 /H /D:MYDVD       ;Or others here
      DEVICEHIGH=C:\BIN\RDISK.COM /S500                   ;Optional
          ..
          ..  Etc.
          ..

   If other Int 13h DOS drivers are used,  UIDE or UIDE2 should load after
   them, to cache disk I-O for the other drivers.   After XMGR, other disk
   drivers, and UIDE or UIDE2 are loaded, other CONFIG.SYS drivers (RDISK,
   SETVER.EXE, ANSI.SYS etc.) may then load in any desired order.

   When a specific RDISK drive letter is required, RDISK can now be loaded
   by AUTOEXEC.BAT, and its /: switch can specify any "free" drive letter,
   e.g.  /:Q  assigns drive Q: for RDISK files.    Whenever RDISK is used,
   AUTOEXEC.BAT should also include commands which copy all RDISK programs
   and data up to the RAM-disk.   This is required each time DOS loads, as
   XMS memory is LOST when a system shuts down!   Such copies usually take
   little time.

   If RDISK and UIDE/UIDE2 are both loaded, users need to balance how much
   XMS memory the drivers use.   RDISK must use no more XMS than its files
   shall need.   UIDE/UIDE2 can then take most remaining XMS, for a better
   cache.   Some XMS memory must be saved for other programs that need it!
   For example, on a 4-GB PC, RDISK may use 500-MB, UIDE may use 3-GB, and
   500-MB of XMS is "free" for other programs.   These sizes can be varied
   so RDISK handles programs and "fast" data files while UIDE/UIDE2 caches
   other "ordinary" files.    Properly "balanced" use of XMS will create a
   VERY high-speed DOS system!

   Please be sure to set each hard disk's geometry correctly in your BIOS.
   Set it to "Auto", "LBA" or "LBA Assisted", but NOT to "None", "Normal",
   "CHS", "ECHS".   "User Cylinders/Heads/Sectors", "Revised ECHS" or "Bit
   Shift" should run but are NOT preferred.   If a BIOS has a setting like
   "UltraDMA" or "UDMA Capable" for a disk, enable it.

   "Laptop" power-saving items like a "drive spin-down timeout" should run
   O.K. but must be TESTED before use!    All UIDE drivers allow 7 seconds
   for a disk or CD/DVD drive to spin-up, after being idle.   More DRASTIC
   power-saving items like a "drive SHUTDOWN timeout" (may require "extra"
   logic to restart the drive!) should be DISABLED, or driver I-O requests
   may time out!

   Also, be sure to use an 80-connector cable for any UltraDMA drive using
   "mode 3" ATA-44 (44 MB/sec) or higher.   When cabling a single drive to
   an IDE channel, note that you MUST use both "ends" of the cable, NOT an
   "end" and the middle connector!   This prevents ERRORS, since an unused
   cable-end can pick up "noise", like a RADIO antenna!

   Be sure to enable all CD/DVD drive(s) through the BIOS set-up routines!
   A drive that is "disabled" may cause the BIOS to clear all its UltraDMA
   flags and force the drive into "PIO mode" zero, which is terribly SLOW!


6. Error Reporting
   ---------------

   XMGR and the UIDE drivers return normal XMS or CD/DVD error codes, when
   needed.   They are shown in the V3.0 XMS Specification or the Microsoft
   "MS-DOS CD-ROM Extensions 2.1" documentation,  from Microsoft and other
   Internet sources.

   All UIDE drivers run as "BIOS drivers" and return whatever codes a BIOS
   posts for its diskettes and hard-disks.    For a SATA or IDE hard-disk,
   the UIDE drivers can post the following error codes:

       Code 0Fh - DMA error.           CCh - Disk is FAULTED.
            20h - Controller busy.     E0h - Hard I-O error.
            AAh - Disk not ready.      FFh - XMS memory error.

   Many DOS programs display only "Disk Error" messages with NO code, thus
   disk errors may require running a diagnostic to get better information!


7. Technical Notes
   ---------------

   In all the following notes, "UIDE" applies to UIDE, UIDE2, and UIDEJR.

   The JEMM386 or JEMMEX drivers are now recommended for use with UIDE, if
   using a DOS system that needs their extra upper-memory, DPMI/VCPI logic
   etc.    Other EMM drivers are essentially "abandoned", some with still-
   uncorrected ERRORS, and those drivers are no-longer recommended!

   UIDE's /R15 and /R63 switches, for old DOS "games", are for a real-mode
   system using UMBPCI and XMGR.   Game users prefer real-mode as it gives
   more speed.   If protected-mode (JEMM386/EMM386) is desired, UIDE using
   a /R switch must load prior to the "EMM" driver, so the XMS reserved by
   UIDE is just beyond the HMA.   If using UMBPCI, XMGR, UIDE, and then an
   EMM driver, this works fine.   But, FreeDOS users, and others whose DOS
   systems permit only one XMS provider (i.e. UMBPCI cannot be used!) must
   load XMGR/HIMEMX first, UIDE second into low memory (upper-memory isn't
   yet enabled!), then JEMM386/EMM386 last.   Using JEMMEX with UIDE and a
   /R switch is unrecommended!   JEMMEX must load first and takes some XMS
   itself, which pushes the reserved XMS above its intended 16/64-MB area,
   and a few DOS "games" programs may CRASH!

   UIDE shall NOT include a "huge" AHCI driver!   Most BIOS programs still
   offer "Legacy", "Compatibility", or "Native IDE" BIOS settings for AHCI
   controllers, with which UIDE runs fine.   If a "newer" AHCI BIOS has no
   such settings, UIDE should be able to "call the BIOS" and use its logic
   to handle AHCI hard-disks.   NOTE:  Much "DOS driver" code is now being
   omitted from AHCI BIOS programs, and using UIDE for such new systems is
   not guaranteed!   UIDE should thus be TESTED before normal use on a new
   "cheap BIOS" AHCI mainboard!

   Also note that CD/DVD drives are not supported by an AHCI BIOS for file
   handling, only for "boot" CDs.   On systems whose AHCI chips can be set
   to "Legacy", "Compatibility" or "Native IDE" mode, CD/DVD drives should
   be run from AHCI ports using such modes.   On mainboards having no such
   BIOS settings, UIDE can run CD/DVD drives only on the parallel IDE port
   (80-pin cable) or on an IDE-capable "add on" card by Promise, etc. that
   UIDE can "detect" through normal PCI-bus logic.

   UIDE handles only "Legacy" or "Native PCI" IDE controllers.   RAID-only
   chipsets (Via VT6420, etc.), "port multiplier" chips, and ADMA chipsets
   are not currently supported.   AHCI is supported only by BIOS "Legacy",
   "Compatiblity" or "Native IDE" mode settings, or thru UIDE "calling the
   BIOS" as noted above.     To use UIDE, a mainboard BIOS should set SATA
   and IDE controllers to some form of "IDE" mode, not RAID/ADMA/AHCI, for
   best speed.    If no "Legacy"/"Compatibility"/"Native IDE" BIOS setting
   for disk controllers is provided, a Sabrent converter card (or similar)
   will let UIDE handle SATA hard-disks or CD/DVD drives from the parallel
   port IDE controller channel, using full UltraDMA speeds.

   Except if necessary for AHCI, it is NOT RECOMMENDED for UIDE to run any
   DOS disk using only the BIOS!   A "cheap" BIOS often omits DOS "Virtual
   DMA" logic.   If so, when an EMM driver (JEMM386 etc.) enables its "V86
   protected-mode", the BIOS can do only PIO-mode transfers and LOSES much
   speed!   If needed, get SATA-to-IDE adapters for SATA disks (as above),
   or get "Int 13h" disk drivers for SCSI or other disk models.   UIDE can
   then handle such disks at full DMA speeds!

   XMGR's "I-O Catcher", used with UMBPCI as noted below, has been changed
   to handle all hard-disks capable of 48-bit LBA I-O (it no-longer checks
   for EDD BIOS "DPTE" data).   Since AHCI BIOS programs still report "LBA
   capable" for a hard-disk, XMGR will now run O.K. with the UMBPCI driver
   on AHCI systems.   Without UMBPCI, XMGR's "I-O Catcher" is not loaded.

   XMGR loads in UMBPCI upper-memory BEFORE that memory is declared to the
   DOS system!   Memory displays using UMBPCI may not list XMGR, since its
   memory is not part of the DOS memory lists.   Such memory displays will
   begin with a block having a 00A7h offset, or greater if using 80 or 128
   XMS "Handles".   The upper-memory skipped by this offset contains XMGR.

   The UMBPCI upper-memory manager uses system "Shadow RAM" that CANNOT do
   DMA!   Newer BIOS programs may use UltraDMA to load programs into upper
   memory.   If this is UMBPCI "Shadow RAM", a CRASH will occur!   To stop
   this, and handle new BIOS programs, users should follow these two RULES
   for running UMBPCI together with XMGR and UIDE:

     A) The loading "order" for V3.70+ UMBPCI and XMGR, shown in section 5
        above, MUST be used!    This lets the XMGR "I-O Catcher" intercept
        and process upper memory disk I-O, until the UIDE driver loads and
        takes-over disk UltraDMA.   Older UMBPCI versions, or other UMBPCI
        loading schemes, are NOT recommended!

     B) When CHS I-O is done (MS-DOS V6.22 or older), every disk MUST have
        valid CHS parameters!   If not, UIDE and the "I-O Catcher" let the
        BIOS handle CHS I-O.   If BIOS UltraDMA is not disabled, a similar
        "Shadow RAM" CRASH will occur!

   Some "CD-ROM boot" programs handle the CD/DVD as a "fake" hard disk and
   provide incorrect EDD BIOS data for it!   In scanning for disks to use,
   UIDE may display "EDD BIOS error!  Unit ignored.", then go on searching
   for more UltraDMA disks.   Users who did NOT "boot" from CD/DVD need to
   see which disk was passed-over and why.   Users who DID "boot" from CD/
   DVD, where all SATA/UltraDMA disks were found, may IGNORE this message!
   It is caused by an ERROR in the "CD-ROM boot" program, NOT by a problem
   with UIDE or its SATA/UltraDMA disks!

   Some BIOS programs do not "configure" a mainboard controller if no user
   drives are found!     An unconfigured controller causes UIDE to display
   "BAD controller", then it goes on looking for others to use.    If this
   message is displayed, users should verify that each SATA/UltraDMA drive
   was made "active" thru the BIOS set-up logic.   If so, "BAD controller"
   says a chip was not set to both "Bus Master" and "I-O Space" modes, and
   the BIOS should be UPDATED!

