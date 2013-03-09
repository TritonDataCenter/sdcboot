#!/bin/perl -w
#
# Script to generate the Makefile rules needed to generate MEM using MUSCHI.
# Modify @langs below if you add a new language.
#
# NOTE: You need Perl installed to run this script.  Perl is available from
# www.delorie.com/djgpp/  If you modify this script but don't have Perl
# installed your build will fail.
#

use strict;

# base name for NLS and binary files, giving us e.g.: MEM.EXE, MEM.EN
my($progname) = 'MEM';

# the available languages
my(@langs) = qw(DE EN ES IT NL);

my($muschi) = "\$(PERL) muschi.pl";

# path separator
my($pathsep) = '\\';

# path to directory containing translation files; relative to source directory
my($nlsdir) = '..' . $pathsep . '..' . $pathsep . 'NLS';

my($objsuffix) = '.OBJ';
my($binsuffix) = '.EXE';
my($hsuffix)   = '.H';
my($csuffix)   = '.C';

my($progsrc) = $progname . $csuffix;

# base name for program source compiled with -DMUSCHI
my($nlsname) = $progname . '_NLS';

# Generate ${nlsname}.h from the first language file; it shouldn't matter
# which language file we use as they should all have the same IDs in them.
my($nlsname_h_in) = $nlsdir . $pathsep . $progname . '.' . $langs[0];

my($nlsname_h) = ${nlsname} . ${hsuffix};
my($nlsname_obj) = ${nlsname} . ${objsuffix};

print(<<__END__);
${nlsname_h}: ${nlsname_h_in}
    $muschi -th < $nlsname_h_in > $nlsname_h

${nlsname_obj}: ${progsrc} ${nlsname_h}
    \$(CC) \$(CFLAGS) -DMUSCHI \$(OBJOUT)$nlsname_obj $progsrc

__END__

my($lang, $langprogname);
my($srcall) = '';
my($objall) = '';
my($binall) = '';
my($src, $muschiarg, $obj, $bin);
for $lang (@langs) {
    $langprogname = $progname . '_' . $lang;
    $src = $langprogname . $csuffix;
    $obj = $langprogname . $objsuffix;
    $bin = $langprogname . $binsuffix;

    print(<<__END__);
$bin: $nlsname_obj $obj prf.obj kitten.obj \$(MEMSUPT)
    \$(CC) \$(LFLAGS) \$(EXEOUT)$bin $nlsname_obj $obj prf.obj kitten.obj \$(MEMSUPT)
    \$(UPX) $bin

$obj: $src
    \$(CC) \$(CFLAGS) \$(OBJOUT)$obj $src

$src: $nlsdir$pathsep${progname}.$lang
    $muschi -tc < $nlsdir$pathsep${progname}.$lang > $src

__END__

    $srcall .= ' ' . $src;
    $objall .= ' ' . $obj;
    $binall .= ' ' . $bin;
}

print(<<__END__);
NLSSRCALL: $srcall
NLSOBJALL: $objall
NLSBINALL: $binall
__END__
