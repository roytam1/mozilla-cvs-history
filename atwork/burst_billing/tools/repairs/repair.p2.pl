#! /usr/local/bin/perl
########################
##
## This program computes the top %5 usage for a given host and ip (roug
#
# This program regenerates the top 5% for the companies for august.
# -Kevin


use POSIX qw(mktime);
use lib '/usr/local/nme/polling/lib';
use CGI;
use RRDs;
use MIME::Entity;
use Sys::Hostname;


if (Sys::Hostname::hostname() eq 'jitter.noc.home.net') 
{
  $rrd_accounts_location = "/usr/local/root/apache/cgi-bin/Burstable/";
  $report_location_root = "/usr/local/root/apache/cgi-bin/Burstable/BurstReports/";
  $rrd_location = "/usr/local/nme/polling/www/";
} else {
  $report_location_root = "./BurstReports/";
  $rrd_location = "./rrds/";
}


$current_time = time;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($current_time);
$end_time = mktime(0,0,0,$mday,$mon,$year,$wday,$yday,$isdst);
$start_time = $end_time - (24 * 60 * 60);  
$month = $mon + 1; # b/c it starts at 0

    $first_of_month = 0;

#
# NOTE: HARDCODE
#
$month = 8;

$MAX_COLLECT = (31 * 24 * 12) * .05 + 1; # this is the maximum numbers of entries reqd to know top 5th
$month_directory = $report_location_root.$month;    

# eventually replace with a DB call to find out which accounts are set to be Burst Billing
# for now, stored in a text file
$accounts_file = $rrd_accounts_location."burst_accounts.txt";
open (burst_accounts_file, "<$accounts_file");
while (defined ($line = <burst_accounts_file>))
{
    chomp($line);
    ($company_name, $ip_address, $host_name) = split(/:/, $line);
    print "processing $company_name...\n";

    $report_location = $month_directory."/".$company_name;
    
    my @new_stats;
    
    $in_filename = $report_location."/".$host_name.".".$ip_address.".in.log";
    $out_filename = $report_location."/".$host_name.".".$ip_address.".out.log";

    &create_logs($in_filename);
    &create_logs($out_filename);
}



sub create_logs
  {
    my ($filename) = @_;
    my (@old_top5, @new_top5, @junk);
    
    if (-e "$filename.all")
    {
      open(FILE, "<$filename.all") || die "Unable to read from $filename.all: $!";
      @old_top5 = <FILE>;
      close(FILE);
    }
    else
    {
       print "WARNING: can't find $filename.all\n";
       return;
    }
    
    chop(@old_top5);
    @new_top5 = sort {$b <=> $a} @old_top5; # sort all the stats
    @junk = splice(@new_top5,$MAX_COLLECT); # save all stats which are not in the top 5% to a junk array
    
    open(FILE, ">$filename") || die "Unable to write to $in_filename: $!";
    print FILE join("\n", @new_top5);
    close(FILE);
  }

