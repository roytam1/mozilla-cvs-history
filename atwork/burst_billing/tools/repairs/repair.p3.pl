#!/usr/local/bin/perl
########################
#
# This program generates the monthly report for August (manually!)
#


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

#
# NOTE: HARDCODED
#
$month = 8;
$first_of_month = 1;

my $result; #running result for entry into Excel file

## REMOVE THIS LINE FOR TESTING AND LAUNCH
#$first_of_month = 1;

$result = "COMPANY\tWBN ORDER #\tPeak Bits\tPeak Bytes\tIN/OUT\n";


$month_directory = $report_location_root.$month;    

# eventually replace with a DB call to find out which accounts are set to be Burst Billing
# for now, stored in a text file
$accounts_file = $rrd_accounts_location."burst_accounts.txt";
open (burst_accounts_file, "<$accounts_file");
while (defined ($line = <burst_accounts_file>))
  {
    chomp($line);
    ($company_name, $ip_address, $host_name, $billing_id) = split(/:/, $line);
    print "processing $company_name...\n";

    $report_location = $month_directory."/".$company_name;
    
    $in_filename = $report_location."/".$host_name.".".$ip_address.".in.log";
    $out_filename = $report_location."/".$host_name.".".$ip_address.".out.log";
    
    if ($first_of_month)
    {
      #print "doing first of month calculations for $company_name\n";
      $in5 = &get5percent("in",$in_filename);
      $out5 = &get5percent("out",$out_filename);
      if ($in5 > $out5) {
	$new_result = "$company_name\t$billing_id\t".
                      ($in5 * 8).
                      "\t$in5\tIn\n";
	$result .= $new_result;
      } else {
	$new_result = "$company_name\t$billing_id\t".
                      ($out5 * 8).
                      "\t$out5\tOut\n";
	$result .= $new_result;
      }
    }
  }



if ($first_of_month)
  { 
    $xls_location = $report_location_root.$month."/results.xls";
    
    open(XLSFILE,">$xls_location") || die "Unable to write to $filename: $!";
    print XLSFILE "$result";
    close(XLSFILE);
    print "Sending Report...\n";
    sendemail("burstbilling\@excitehome.net","$report_location_root$month/results.xls");
#    sendemail("mccarthy\@excitehome.net","$report_location_root$month/results.xls");
  }     

sub get5percent 
  {
    my ($log_type,$filename) = @_;
    my $percentresult;	

    open (CRAPFILE, "$filename.all") || die "Unable to open $filename: $!";
    @stats = <CRAPFILE>;	
    close(CRAPFILE);	    

    chop(@stats);
    @sorted_stats = sort {$b <=> $a} @stats;

    $number_of_entries = scalar(@sorted_stats); 
    $fifth_percent = $number_of_entries * .05;

    $percentresult = $sorted_stats[$fifth_percent];
    
    return $percentresult;	
  }


sub sendemail
  {
    my ($toaddr,$file) = @_;
    my $return;
    my $fromaddr = "burstbilling\@excitehome.net";
    my $msgbody = "Attached configuration file:  $file\n";
    my $msg_mime_type = "multipart/mixed";
    my $attach_mime_type = "application/postscript";
    my $encoding_type = "base64";
    #print LOG "E-mailing config $file to $toaddr\n";
    my $mime_mail = new MIME::Entity;
    $mime_mail->build(
		      Type    => "$msg_mime_type",
		      From    => "$fromaddr",
		      To      => "$toaddr",
		      Subject => "$file");
    attach $mime_mail 
      Path     => "$file",
	Type     => "$attach_mime_type",
	  Encoding => "$encoding_type";
    attach $mime_mail Data => "$msgbody";
    open MAIL, "| /usr/lib/sendmail -t -i" or $return = 
      "No response from sendmail\n";
    $mime_mail->print(\*MAIL);
    close MAIL;
    #if ($return) { #print LOG $return; }
    return();
  }

