#!/usr/local/bin/perl
########################
##
## This program fixed a bug where the 8/31 data was actually put
## inside of september.  We therefore had to append all the
## September data to the end of the August data.
## repair.p2 recomputed the top 5% file
## repair.p3 regenerated the monthly 95/5 report email.
##

$rrd_accounts_location = "/usr/local/root/apache/cgi-bin/Burstable/";
$report_location_root = "/usr/local/root/apache/cgi-bin/Burstable/BurstReports/";

$month = 8;

$month_directory = $report_location_root.$month;    
$append_month_directory = "$report_location_root$month.append";


# eventually replace with a DB call to find out which accounts are set to be Burst Billing
# for now, stored in a text file
$accounts_file = $rrd_accounts_location."burst_accounts.txt";
open (burst_accounts_file, "<$accounts_file");
while (defined ($line = <burst_accounts_file>))
{
    chomp($line);
    ($company_name, $ip_address, $host_name) = split(/:/, $line);
    print "processing $company_name...\n";

    $report_location = "$month_directory/$company_name";
    $append_report_location = "$append_month_directory/$company_name";

    $in_filename = $host_name.".".$ip_address.".in.log.all";
    $out_filename = $host_name.".".$ip_address.".out.log.all";

    `cat $append_report_location/$in_filename >> $report_location/$in_filename`;
    `cat $append_report_location/$out_filename >> $report_location/$out_filename`;
}
