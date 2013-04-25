#
# Copyright (c) 2013, Joyent, Inc. All rights reserved.
#

NAME =	sdcboot

TOP :=	$(shell pwd)
ROOT =	$(TOP)/proto

FREEDOS_ROOT =	$(ROOT)/dos
BOOT_ROOT =	$(ROOT)/boot

RELEASE_TARBALL =	$(NAME)-$(STAMP).tgz
CLEAN_FILES += \
	$(ROOT) \
	$(NAME)-pkg-*.tar.gz \
	src/*.o \
	src/*.elf \
	src/*.com \
	$(MTOOLS_DIR)

#
# We would like to use mkfs_pcfs here, but it creates a filesystem that can
# boot but cannot be read.  Bug in FreeDOS?  Bug in mkfs_pcfs?  It's not
# clear.  So instead we will build and use mtools for this purpose.
#
MTOOLS_VER =	4.0.18
MTOOLS_TARBALL =	tools/mtools-$(MTOOLS_VER).tar.bz2
MTOOLS_DIR =	tools/mtools-$(MTOOLS_VER)
MFORMAT =	$(MTOOLS_DIR)/mformat
MCOPY =		$(MTOOLS_DIR)/mcopy
MTOOLS =	$(MFORMAT) $(MCOPY)
MTOOLS_BUILD_ENV = \
		LIBS="-liconv" \
		LDFLAGS="-L/opt/local/lib -R/opt/local/lib" \
		CPPFLAGS="-I/opt/local/include"

MFORMAT.bootflop = \
	$(MFORMAT) -t 80 -h 2 -n 18 -v $(FLOPPY_LABEL) \
	-f 1440 -B $(FLOPPY_BOOTSECT) -i $@ ::

#
# ipxe assumes GNU without using prefixed commands.
#
IPXE_ENV = \
	AS=/opt/local/bin/as \
	LD=/opt/local/bin/ld \
	AWK=/usr/bin/nawk \
	GREP=/usr/xpg4/bin/grep \
	V=1

MAPFILE =	mapfile-dos
CC =		/opt/local/bin/gcc
LD =		/usr/bin/ld
TAR =		/opt/local/bin/tar
MKDIR =		/usr/bin/mkdir
MKFILE =	/usr/sbin/mkfile
CP =		/usr/bin/cp
OBJCOPY =	/opt/local/bin/objcopy
CHMOD =		/usr/bin/chmod
RM =		/usr/bin/rm -f
INS =		/usr/sbin/install

FILEMODE =	644
DIRMODE =	755

INS.file =	$(RM) $@; $(INS) -s -m $(FILEMODE) -f $(@D) $<
INS.dir =	$(INS) -s -d -m $(DIRMODE) $@

FLOPPY_IMAGE =	$(BOOT_ROOT)/freedos.img
FLOPPY_BOOTSECT =	src/bootsect.bin
FLOPPY_LABEL =	FREEDOS

FLOPPY_FILES = \
	freedos/bin/kernel.sys \
	src/fdconfig.sys

FREEDOS_BINS = \
	append.exe \
	assign.com \
	attrib.com \
	chkdsk.exe \
	choice.exe \
	command.com \
	comp.com \
	cpulevel.com \
	cwsdpmi.exe \
	debug.com \
	deltree.com \
	devload.com \
	diskcomp.com \
	diskcopy.exe \
	edlin16.exe \
	edlin32.exe \
	exe2bin.com \
	fart.exe \
	fc.exe \
	fdapm.com \
	fdisk.exe \
	fdisk.ini \
	fdisk131.exe \
	fdiskpt.ini \
	find.com \
	format.exe \
	getargs.com \
	himemx.exe \
	jemfbhlp.exe \
	jemm386.exe \
	jemmex.exe \
	join.exe \
	label.exe \
	mem.exe \
	mode.com \
	more.exe \
	move.exe \
	mset.com \
	oscheck.com \
	rdisk.com \
	recover.exe \
	replace.exe \
	share.com \
	sort.com \
	subst.exe \
	swsubst.exe \
	sys.com \
	tree.com \
	unzip.exe \
	whichfat.com \
	xcopy.exe \
	zip.exe

GNU_BINS = \
	cat.exe \
	comm.exe \
	csplit.exe \
	cut.exe \
	expand.exe \
	fold.exe \
	grep.exe \
	head.exe \
	join.exe \
	nl.exe \
	paste.exe \
	pr.exe \
	sort.exe \
	split.exe \
	sum.exe \
	tac.exe \
	tail.exe \
	tr.exe \
	unexpand.exe \
	uniq.exe \
	wc.exe

MISC_BINS = \
	sed.exe

JOYENT_BINS = \
	int14.com

BOOT_BINS = \
	default.ipxe \
	ipxe.lkrn \
	memdisk

DOS_BINS = \
	autoexec.bat

SUBMODULE_DIRS = \
	ipxe \
	memdisk_getargs \
	syslinux

ROOT_JOYENT_BINS =	$(JOYENT_BINS:%=$(FREEDOS_ROOT)/joyent/%)
ROOT_FREEDOS_BINS =	\
	$(FREEDOS_BINS:%=$(FREEDOS_ROOT)/freedos/%) \
	$(MISC_BINS:%=$(FREEDOS_ROOT)/freedos/%)
ROOT_DOS_BINS =		$(DOS_BINS:%=$(FREEDOS_ROOT)/%)
ROOT_GNU_BINS =		$(GNU_BINS:%=$(FREEDOS_ROOT)/gnu/%)
ROOT_BOOT_BINS =	$(BOOT_BINS:%=$(BOOT_ROOT)/%)
SUBMODULES =		$(SUBMODULE_DIRS:%=%/.git)

ROOT_FREEDOS = \
	$(ROOT_DOS_BINS) \
	$(ROOT_FREEDOS_BINS) \
	$(ROOT_GNU_BINS) \
	$(ROOT_JOYENT_BINS)

ROOT_BOOT =	$(ROOT_BOOT_BINS)

include ./tools/mk/Makefile.defs

$(FREEDOS_ROOT)/autoexec.bat :	FILEMODE = 755
$(FREEDOS_ROOT)/joyent/%.com :	FILEMODE = 755
$(FREEDOS_ROOT)/freedos/%.com :	FILEMODE = 755
$(FREEDOS_ROOT)/freedos/%.exe :	FILEMODE = 755
$(FREEDOS_ROOT)/gnu/%.exe :	FILEMODE = 755
$(BOOT_ROOT)/memdisk :		FILEMODE = 755
$(BOOT_ROOT)/ipxe.lkrn :	FILEMODE = 755
$(BOOT_ROOT)/default.ipxe :	FILEMODE = 644

.PHONY: all
all: $(SUBMODULES) $(ROOT_FREEDOS) $(ROOT_BOOT) $(FLOPPY_IMAGE)

%/.git:
	git submodule update --init $*

$(ROOT):
	$(INS.dir)

$(FREEDOS_ROOT): | $(ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/%: src/% | $(FREEDOS_ROOT)
	$(INS.file)

$(FREEDOS_ROOT)/joyent: | $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/joyent/%: src/% | $(FREEDOS_ROOT)/joyent
	$(INS.file)

$(FREEDOS_ROOT)/freedos: | $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/freedos/%: freedos/bin/% | $(FREEDOS_ROOT)/freedos
	$(INS.file)

$(FREEDOS_ROOT)/freedos/%: misc/% | $(FREEDOS_ROOT)/freedos
	$(INS.file)

$(FREEDOS_ROOT)/freedos/%: memdisk/% | $(FREEDOS_ROOT)/freedos
	$(INS.file)

$(FREEDOS_ROOT)/gnu: | $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/gnu/%: gnu/out/% | $(FREEDOS_ROOT)/gnu
	$(INS.file)

$(BOOT_ROOT): | $(ROOT)
	$(INS.dir)

$(BOOT_ROOT)/%: memdisk/% | $(BOOT_ROOT)
	$(INS.file)

$(BOOT_ROOT)/%: ipxe/src/bin/% | $(BOOT_ROOT)
	$(INS.file)

$(BOOT_ROOT)/%: src/% | $(BOOT_ROOT)
	$(INS.file)

$(FLOPPY_IMAGE): $(FLOPPY_BOOTSECT) $(FLOPPY_FILES) $(MTOOLS) | $(BOOT_ROOT)
	$(MKFILE) 1440k $(FLOPPY_IMAGE)
	$(MFORMAT.bootflop)
	$(MCOPY) -i $@ $(FLOPPY_FILES) ::
	$(CHMOD) $(FILEMODE) $@

.ONESHELL:
$(MTOOLS): $(MTOOLS_TARBALL)
	$(TAR) x -C tools -j -f $<
	cd $(MTOOLS_DIR)
	$(MTOOLS_BUILD_ENV) ./configure
	$(MAKE)

.PRECIOUS: src/%.elf

src/%.o: src/%.S
	$(CC) -march=i386 -o $@ -c $<

src/%.elf: src/%.o
	$(LD) -dn -M $(MAPFILE) -o $@ $<

src/%.com: src/%.elf
	$(OBJCOPY) -O binary $< $@

ipxe/src/bin/%: ipxe/.git
	(cd ipxe/src && $(MAKE) bin/$(@F) $(IPXE_ENV))

.PHONY: test
test:

.PHONY: pkg
pkg: all

clean:: ipxe.clean

ipxe.clean:
	(cd ipxe/src && $(MAKE) clean $(IPXE_ENV))

release: $(RELEASE_TARBALL)

$(RELEASE_TARBALL): pkg
	(cd $(ROOT); $(TAR) czf $(TOP)/$(RELEASE_TARBALL) .)

publish: prepublish $(BITS_DIR)/$(NAME)/$(RELEASE_TARBALL)

.PHONY: prepublish
prepublish:
	@if [[ -z "$(BITS_DIR)" ]]; then \
		echo "error: 'BITS_DIR' must be set for 'publish' target"; \
		exit 1; \
	fi
	@if [[ ! -d "$(BITS_DIR)" ]]; then \
		echo "error: $(BITS_DIR) is not a directory"; \
		exit 1; \
	fi

$(BITS_DIR)/$(NAME)/$(RELEASE_TARBALL): $(RELEASE_TARBALL) | $(BITS_DIR)/$(NAME)
	$(INS.file)

$(BITS_DIR)/$(NAME):
	$(INS.dir)

include ./tools/mk/Makefile.targ
