#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;

my $scriptpath = dirname($0);
my $outfile = $ARGV[0];
my $mergebase = $ARGV[1];

$mergebase or $mergebase = "";

mkdir dirname($outfile);

chomp;

my ($major, $tag, $pretyp) = ($2, $1, $3);

my $rel = defined($pretyp) ? $pretyp le "b" ? "ALPHA" : "BETA" : "FINAL";

my $prefix = "CRAWL";

open OUT, ">", "$outfile" or die $!;
print OUT <<__eof__;
#define ${prefix}_VERSION_MAJOR "$major"
#define ${prefix}_VERSION_RELEASE VER_$rel
#define ${prefix}_VERSION_SHORT "$tag"
#define ${prefix}_VERSION_LONG "$major"
__eof__
close OUT or die $!;
