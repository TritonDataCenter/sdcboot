
 1. About

 This directory contains some JLM samples:

 GENERIC:  protected-mode TSR sample in ASM (Masm/JWasm/PoAsm).
 HELLO:    "hello world" JLM in ASM (Masm/JWasm/PoAsm)
 HELLO2:   "hello world" JLM in C (MS VC, Open Watcom)
 IOTRAP:   sample how to trap IO port access.
 JKEYBGR:  German keyboard driver.

 JCLOCK:   "clock" application in FASM syntax. (c) Alexey Voskov,
           distributed with his kind permission.
 XDMA32:   UltraDMA HD driver (GNU GPL 2).
 XCDROM32: UltraDMA DVD/CD-ROM driver (GNU GPL 2).

 The first 5 samples are Public Domain.


 2. Tools which have been tested

 tools             version   result
 -----------------------------------------
 assembler:
 Masm               6.15      ok
 JWasm              1.95      ok
 Fasm               1.67      ok
 PoAsm              1.00.35   ok

 C compiler:
 MS VC              2/5/6     ok
 Borland C++        5.5       ok
 Ladsoft CC386      3.8.1.18  ok
 Digital Mars C++   8.49      ok
 Pelles C           4.50.16   ok
 Open Watcom WCC386 1.7       failed
 
 COFF linker:
 PoLink             4.50      ok
 Open Watcom WLink  1.6       ok
 MS Link            6.00      ok
 ALink              1.5       failed
 Borland TLink32    2.0.68    failed

 The reason for the linker failures is that these linkers have
 no option to define the module's entry point. Usually this is
 easy to fix or at least to cirumvent.
 
 PoAsm and PoLink are included in Pelles C.


 3. History

 v5.75: bugfix in XDMA32.
 v5.73: Hello2w sample added (Open Watcom specific).
        GENERIC sample added.
        IOTRAP sample added.
        JClock2 sample got a DDB with ID, which makes it unloadable.
 v5.69: usage of new exports makes XDMA32/XCDROM32
        incompatible with previous versions of JLoad.
 v5.68: XDMA32 added.
        bugfixes in XCDROM32.
        XCDROM32 now supports native controllers (SATA).
 v5.62: PoAsm now supported to create JLMs.
 v5.61: no changes
 v5.60: initial
 