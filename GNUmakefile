#
# Copyright (c) 2013, Joyent, Inc. All rights reserved.
#

NAME =	freedos

TOP :=	$(shell pwd)
ROOT =	$(TOP)/proto

FREEDOS_ROOT =	$(ROOT)/dos
BOOT_ROOT =	$(ROOT)/boot

RELEASE_TARBALL =	$(NAME)-pkg-$(STAMP).tar.gz
CLEAN_FILES += \
	$(ROOT) \
	$(NAME)-pkg-*.tar.gz \
	src/*.o \
	src/*.elf \
	src/*.com

CC =		/opt/local/bin/gcc
LD =		/usr/bin/ld
MAPFILE =	mapfile-dos
OBJCOPY =	/opt/local/bin/objcopy
RM =		/usr/bin/rm
INS =		/usr/sbin/install
FILEMODE =	644
DIRMODE =	755

INS.file =	$(RM) $@; $(INS) -s -m $(FILEMODE) -f $(@D) $<
INS.dir =	$(INS) -s -d -m $(DIRMODE) $@

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
	kernel.sys \
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

JOYENT_BINS = \
	int14.com

BOOT_BINS = \
	memdisk

FILES = \
	autoexec.bat

ROOT_FILES =		$(FILES:%=$(ROOT)/%)
ROOT_JOYENT_BINS =	$(JOYENT_BINS:%=$(FREEDOS_ROOT)/joyent/%)
ROOT_FREEDOS_BINS =	$(FREEDOS_BINS:%=$(FREEDOS_ROOT)/freedos/%)
ROOT_GNU_BINS =		$(GNU_BINS:%=$(FREEDOS_ROOT)/gnu/%)
ROOT_BOOT_BINS =	$(BOOT_BINS:%=$(BOOT_ROOT)/%)

ROOT_FREEDOS =	$(ROOT_JOYENT_BINS) $(ROOT_FREEDOS_BINS) $(ROOT_GNU_BINS)
ROOT_BOOT =	$(ROOT_BOOT_BINS)

include ./tools/mk/Makefile.defs

$(FREEDOS_ROOT)/joyent/%.com :	FILEMODE = 755
$(FREEDOS_ROOT)/freedos/%.com :	FILEMODE = 755
$(FREEDOS_ROOT)/freedos/%.exe :	FILEMODE = 755
$(FREEDOS_ROOT)/gnu/%.exe :	FILEMODE = 755
$(BOOT_ROOT)/memdisk :		FILEMODE = 755
$(ROOT)/autoexec.bat :		FILEMODE = 755

.PHONY: all
all: $(ROOT_FREEDOS) $(ROOT_BOOT) $(ROOT_FILES)

$(ROOT):
	$(INS.dir)

$(FREEDOS_ROOT): $(ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/joyent: $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/joyent/%: src/% $(FREEDOS_ROOT)/joyent
	$(INS.file)

$(FREEDOS_ROOT)/freedos: $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/freedos/%: freedos/bin/% $(FREEDOS_ROOT)/freedos
	$(INS.file)

$(FREEDOS_ROOT)/freedos/%: memdisk/% $(FREEDOS_ROOT)/freedos
	$(INS.file)

$(FREEDOS_ROOT)/gnu: $(FREEDOS_ROOT)
	$(INS.dir)

$(FREEDOS_ROOT)/gnu/%: gnu/out/% $(FREEDOS_ROOT)/gnu
	$(INS.file)

$(BOOT_ROOT): $(ROOT)
	$(INS.dir)

$(BOOT_ROOT)/%: memdisk/% $(BOOT_ROOT)
	$(INS.file)

$(ROOT)/%: src/% $(ROOT)
	$(INS.file)

.PRECIOUS: src/%.elf

src/%.o: src/%.S
	$(CC) -march=i386 -o $@ -c $<

src/%.elf: src/%.o
	$(LD) -dn -M $(MAPFILE) -o $@ $<

src/%.com: src/%.elf
	$(OBJCOPY) -O binary $< $@

.PHONY: test
test:

.PHONY: pkg
pkg: all

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

$(BITS_DIR)/$(NAME)/$(RELEASE_TARBALL): $(RELEASE_TARBALL) $(BITS_DIR)/$(NAME)
	$(INS.file)

$(BITS_DIR)/$(NAME):
	$(INS.dir)

include ./tools/mk/Makefile.targ
