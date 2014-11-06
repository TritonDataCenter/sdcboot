<!--
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

<!--
    Copyright (c) 2014, Joyent, Inc.
-->

# sdcboot

This repository is part of the Joyent SmartDataCenter project (SDC).  For
contribution guidelines, issues, and general documentation, visit the main
[SDC](http://github.com/joyent/sdc) project page.

Note that most of the contents of this repository are covered by their
own separate licenses and copyright statements.

# Development

This repository contains the following:

- FreeDOS 1.1 with all source and tools
- GNU Textutils compiled for FreeDOS, with all source and tools
- A submodule containing the syslinux repository, for memdisk
- A submodule containing a program for retrieving arguments to memdisk
- A submodule for iPXE, for non-DOS network booting
- A replacement for the BIOS int 0x14 service that actually works
- Glue for building the FreeDOS-related portions of the SDC boot system

The repository serves three main purposes: first, it contains the
original sources for all DOS-related binaries.  Since we don't have any
ability to build them, they aren't really useful, but we still need them
due to licensing requirements.  Second, this repository assembles a
FreeDOS boot environment usable on a USB key for Firmware Diagnostics
and Upgrade Mode.  This is not (currently) used by the SmartOS
distribution.  Finally, it builds an iPXE binary for use when netbooting
compute nodes.

Note that there are a number of places where we use binaries checked in
rather than building them ourselves.  These are:

- memdisk (could be built from the syslinux repo)
- getargs.com (could be built from the memdisk_getargs repo)
- most of FreeDOS (could be built from the sources)
- the boot sector (could be built from FreeDOS sources, with effort)
- the GNU utilities for DOS (could be built from their sources)

In most cases, we do this because building these would require tools we
do not have or that would be difficult or time-consuming to port,
notably OpenWatcom.  In the case of syslinux, we avoid building because
the single tool we want is tied up within a giant hairball that has
deeply embedded assumptions that it is building on GNU/Linux.  We avoid
building small assembly programs like the boot block because most DOS
assembly programs tend to be written for NASM rather than the AT&T or
GNU assemblers that are available to us.  Each and every one of these
could be fixed simply by doing work, but it's unlikely that most or any
of these will need to be modified, and so it seems not worth the effort.
If we do later need to change any of these before we finally rid
ourselves of the need for them, we can revisit then.  In any case, for
those who so desire, all of the source is included here or in
submodules.  Changes to build from source would generally be welcome.

# Testing

No tests are available.
