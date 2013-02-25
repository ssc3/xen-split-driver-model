#!/usr/bin/perl

use warnings;

my $count =0;
my $val_from_disk = 0;
my $val_from_read = 0;

#usage: perl checker.pl sectors.pid.txt read.pid

my $sector_filename = $ARGV[0];
my $read_filename = $ARGV[1];

open (SECTORFILE, "$sector_filename") || die ("Cannot open file $!");
while(<SECTORFILE>)
{
	chomp;
	
	system("gcc -D_GNU_SOURCE -pthread -g -o test2 test2.c");
	$val_from_disk = `./test2 $_ disk1.img`;
	print "\n$val_from_disk\n";

	$val_from_read = `./test2 $count $read_filename`;
	print "\n$val_from_read\n";
	if ($val_from_disk ne $val_from_read)
	{
		print "Failed Checker test at count = $count \n";
		exit(0);
	}
 
	$count++;
}	
print "Passed Checker test :) \n";

#system("rm read* sectors*");

