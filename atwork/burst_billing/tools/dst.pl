#!/usr/bin/perl
#
# Daylight savings time test!
#
use strict;
use POSIX qw(mktime);
use Time::Local;

my ($current_time, $posix_time, $time_local);
my ($Ssec,$Smin,$Shour,$Smday,$Smon,$Syear,$Swday,$Syday,$Sisdst);

my ($Esec,$Emin,$Ehour,$Emday,$Emon,$Eyear,$Ewday,$Eyday,$Eisdst);
my ($s_time, $e_time);


$Esec = 0;
$Emin = 0;
$Ehour = 0;
$Emday = 3;
$Emon = 3;
$Eyear = 100;

$Ssec = 0;
$Smin = 0;
$Shour = 0;
$Smday = 2;
$Smon = 3;
$Syear = 100;


#
# Testing the timelocal routine
#
print "Timelocal()\n";
$e_time = timelocal($Esec,$Emin,$Ehour,$Emday,$Emon,$Eyear);
($Esec,$Emin,$Ehour,$Emday,$Emon,$Eyear,$Ewday,$Eyday,$Eisdst) = localtime($e_time);
print "$Esec $Emin $Ehour $Emday $Emon $Eyear $Ewday $Eyday $Eisdst\n";


$s_time = timelocal($Ssec,$Smin,$Shour,$Smday,$Smon,$Syear);
($Ssec,$Smin,$Shour,$Smday,$Smon,$Syear,$Swday,$Syday,$Sisdst) = localtime($s_time);
print "$Ssec $Smin $Shour $Smday $Smon $Syear $Swday $Syday $Sisdst\n";

print "Start: $s_time   End: $e_time\n";
print "Difference: ", ($e_time - $s_time) / (60 * 60), " hours\n";

exit;

__END__


$current_time = time;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($current_time);

print "$sec $min $hour $mday $mon $year $wday $yday $isdst\n";

exit;



$posix_time = mktime($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst);
$time_local = timelocal($sec,$min,$hour,$mday,$mon,$year);


print "current_time: $current_time\npoxixtime: $posix_time\n";
print "time_local: $time_local\n";
