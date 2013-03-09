#!/bin/perl -w

use strict;

use FileHandle;

# Modify this if you want to generate different output.  For example,
# you may wish to put the strings into an array instead of separate
# globals, or perhaps generate the output for a different
# (programming) language.

# This defines the format strings used for generating the output into each
# file.  The 'string' format is used each time we read a string with its set
# and message numbers from the input file; $1 refers to the set number, $2
# refers to the message number, and $3 refers to the text of the string.  The
# 'comment' format is used when we read a comment (a line starting with a
# hash) from the input file; $1 refers to the text of the comment without the
# leading hash.  The 'blank' format is used for each blank line (consisting
# only of zero or more whitespace characters) that we read; there are no
# variables you can substitute here.

my($output_formats) = {'h' => {'string' =>  'extern char *muschi_$1_$2;\n',
			       'comment' => '/*$1 */\n',
			       'blank' =>   '\n'},
		       'c' => {'string' =>  'char *muschi_$1_$2 = \"$3\";\n',
			       'comment' => '/*               $1 */\n',
			       'blank' =>   '\n'},
		   };

my($arg) = pop(@ARGV);
my($type);
if (defined($arg) && $arg =~ /^-t(.*)$/) {
    $type = $1;
}
if (!defined($arg) || !defined($type) || !exists($output_formats->{$type})) {
    print(<<'__END__');
Usage: muschi.pl -t<type>
where type must match one of the keys in $output_formats.
Supply an NLS file on standard input.
__END__
    exit(1);
}

my($format);
while (<>) {
    # Work out which format to use and set up the $1, $2, ...
    if (/^\s*#(.*)$/) {
	$format = 'comment';
    } elsif (/^(\d+)\.(\d+):(.*)$/) {
	$format = 'string';
    } elsif (/^\s*$/) {
	$format = 'blank';
    } else {
	die("Could not interpret input line: $_");
    }

    # Write the output to stdout
    eval('print("' . $output_formats->{$type}->{$format} . '")');
}
