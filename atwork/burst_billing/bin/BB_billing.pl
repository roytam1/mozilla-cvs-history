#!/usr/bin/perl -w
#############################################################################
# $Id$
#
# BB_billing.pl
# 
# This is the program that grabs the data from the RRDs.  On the first of the
# month, it computes the 95/5 stat and emails it out.
# The program is pretty simple, it's just more complicated trying to wrap
# it in an egg carton so it's robust.
# 
# History:
#         Kevin J. McCarthy   [10/17/2000]   Original Coding
#
#############################################################################

use strict;

use MIME::Entity;
use Time::Local;
use DirHandle;

# use lib '/usr/local/nme/polling/lib';
# use RRDs;

BEGIN {require '../inc/BB_include.pl';}
require "$INC_DIR/BB_lib.pl";


##############
# Begin Code #
##############

my ($circuit_list, $global_monitor_time_list);

($circuit_list, $global_monitor_time_list) = &init_program();

&run_burst_billing($circuit_list, $global_monitor_time_list);

&cleanup_program();

exit($OKAY);



###################
# Local Functions #
###################


 ###                                ###
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ##      Program Initialization      ##
 ##        & Cleanup Routines        ##
 ##                                  ##
 ##                                  ##
 ###                                ###

########################################
# init_program
# 
# Does program initializations, such as:
# - Opening the log file
# - Checking if the program needs to run
########################################
sub init_program
{
   my ($global_monitor_time_list, $circuit_list);

   &open_log_file();

   &write_log_file($START_STAMP, "BB_billing.pl started");

   $global_monitor_time_list = &get_global_monitor_time_list();

   $circuit_list = &query_circuit_list();

   return($circuit_list, $global_monitor_time_list);
}


########################################################################
# get_global_monitor_time_list
# 
# This checks if the program needs to run today.
# It does this by looking at the overall $LAST_RUN_FILE, and calculating
# the number of days to run.  If there are any days to run, it returns.
# Otherwise, it gracefully exits the program.
########################################################################
sub get_global_monitor_time_list
{
   my ($monitor_time_list);

   &write_log_file($LOG_INFO, "Calculating days we need to obtain data for");

   $monitor_time_list = &calculate_monitor_times("$REPORT_HOME/$LAST_RUN_FILE");

   if ( ! scalar(@{$monitor_time_list}) )
   {
      &write_log_file($LOG_INFO, "Data already captured for today.  Exiting program.");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($OKAY);
   }

   return($monitor_time_list);
}


######################################################
# query_circuit_list
# 
# TODO: query from WBN and implement cache file.
# For now, just suck in the values from the text file.
######################################################
sub query_circuit_list
{
   my (@circuit_list);
   my ($circuit_fh, $circuit_line);
   my ($company_name, $ip_address, $port_name, $circuit_id);

   &write_log_file($LOG_INFO, "Querying the circuits to monitor");

   $circuit_fh = new IO::File "<$BURST_ACCOUNTS_FILE";
   if ( ! $circuit_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $BURST_ACCOUNTS_FILE.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }
      
   while ( defined($circuit_line = <$circuit_fh>) )
   {
      chomp($circuit_line);
      ($company_name, $ip_address, $port_name, $circuit_id) =
            split(/:/, $circuit_line);
      push (@circuit_list,
            {$CIRCUIT_COMPANY_NAME => $company_name,
             $CIRCUIT_IP_ADDRESS   => $ip_address,
             $CIRCUIT_PORT_NAME    => $port_name,
             $CIRCUIT_ID           => $circuit_id});
   }

   $circuit_fh->close();

   return(\@circuit_list);
}


#######################################################
# cleanup_program
# 
# Does any cleanup needed before the program shuts down
# (Currently this is just closing the log file
#######################################################
sub cleanup_program
{
   &write_log_file($END_STAMP, "BB_billing.pl ended");
   &close_log_file();
}


 ###                                ###
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ##    Billing Data Acquisition      ##
 ##           Routines               ##
 ##                                  ##
 ##                                  ##
 ###                                ###

#######################################################################
# run_burst_billing
# 
# Loops through each of the days in the monitor_time_list.
# For each day, it loos through and grabs the data for each circuit
# in the circuit_list.  It checks to be sure the circuit hasn't already
# run first...
#######################################################################
sub run_burst_billing
{
   my ($circuit_list, $global_monitor_time_list) = @_;

   my ($day_entry);
   my ($circuit_entry);

   &write_log_file($LOG_INFO, "Obtaining data from RRDs");

   foreach $day_entry ( @{$global_monitor_time_list} )
   {
      &write_log_file($LOG_INFO, "Polling data for " . $day_entry->{$MTL_YEAR} . "/" .
                      $day_entry->{$MTL_MONTH} . "/" . $day_entry->{$MTL_MDAY});

      &create_month_directory($day_entry);

      foreach $circuit_entry ( @{$circuit_list} )
      {
         &write_log_file($LOG_NOTE, "Checking if circuit " .
                                    $circuit_entry->{$CIRCUIT_ID} .
                                    " has run for this day");

         &create_circuit_directory($day_entry, $circuit_entry);
         &update_circuit_header_file($day_entry, $circuit_entry);

         if ( ! &circuit_has_run($day_entry, $circuit_entry) )
         {
            &write_log_file($LOG_INFO, "Obtaining data for circuit " .
                                       $circuit_entry->{$CIRCUIT_ID});
            &record_circuit_data($day_entry, $circuit_entry);
            &write_last_run_day_file(&get_circuit_directory($day_entry, $circuit_entry),
                                     $day_entry);
         }
         else
         {
            &write_log_file($LOG_NOTE, "Circuit already ran - skipping");
         }
      }

      if ( $day_entry->{$MTL_FIRST_OF_MONTH} == 1 )
      {
         &write_log_file($LOG_INFO, "Running first of month report");
         &run_first_of_month_report($day_entry);
      }

      &write_last_run_day_file($REPORT_HOME, $day_entry);
   }
}


#########################################################################
# circuit_has_run
# 
# This function checks to see if a circuit has already collected data for
# the day passed in.
# 
# It does this by calling calculate_monitor_times for the circuit.
# This returns a list of days the circuit needs to run for.  If the
# $day_entry is in the list returned, then the circuit has not collected
# data for that day yet.
# 
# Yes, this could be done more efficiently, but I'm using existing
# functions - which already account for missing last run files, etc...
#
# Returns: 1 if the circuit has already run
#          0 if the circuit has not run yet
#########################################################################
sub circuit_has_run
{
   my ($day_entry, $circuit_entry) = @_;

   my ($circuit_directory, $circuit_monitor_time_list);
   my ($monitor_entry);

   $circuit_directory = &get_circuit_directory($day_entry, $circuit_entry);

   $circuit_monitor_time_list = 
         &calculate_monitor_times("$circuit_directory/$LAST_RUN_FILE");

   foreach $monitor_entry ( @{$circuit_monitor_time_list} )
   {
      if ( ($monitor_entry->{$MTL_YEAR} == $day_entry->{$MTL_YEAR}) &&
           ($monitor_entry->{$MTL_MONTH} == $day_entry->{$MTL_MONTH}) &&
           ($monitor_entry->{$MTL_MDAY} == $day_entry->{$MTL_MDAY}) )
      {
         return(0);
      }
   }

   return(1);
}


#############################################################
# record_circuit_data
# 
# This routine does the work.  It grabs the data from the RRD
# and appends it to the circuits data files.
#############################################################
sub record_circuit_data
{
   my ($day_entry, $circuit_entry) = @_;

   my ($in_data, $out_data);

   ($in_data, $out_data) = &fetch_rrd_data($day_entry, $circuit_entry);
   if ( $in_data && $out_data )
   {
      &record_rrd_data($day_entry, $circuit_entry, $in_data, $out_data);
   }
}


########################################################
# fetch_rrd_data
# 
# Retrieves the in and out data from the RRD, and checks
# it for errors.
########################################################
sub fetch_rrd_data
{
   my ($day_entry, $circuit_entry) = @_;

   my ($rrd_filename);
   my ($start, $step, $names, $data);
   my ($total_data_count, $zero_data_in_count, $nan_data_in_count);
   my ($zero_data_out_count, $nan_data_out_count);
   my ($data_estimate_count);
   my (@in_data, @out_data);

   $rrd_filename = "$RRD_DIR/" .
                   $circuit_entry->{$CIRCUIT_PORT_NAME} .
                   "." .
                   $circuit_entry->{$CIRCUIT_IP_ADDRESS} .
                   ".int.rrd";

   if ( ! -f $rrd_filename )
   {
      &write_log_file($LOG_WARNING, "The RRD for circuit " .
            $circuit_entry->{$CIRCUIT_ID} . ": <$rrd_filename> does NOT EXIST.  Skipping this circuit.");
      return(undef, undef);
   }

   ($start,$step,$names,$data) = RRDs::fetch($rrd_filename, "MAX",
                                             "--start", $day_entry->{$MTL_START_EPOCH},
                                             "--end", $day_entry->{$MTL_END_EPOCH});

   if ( $step <= 0 )
   {
      &write_log_file($LOG_WARNING, "The RRD returned a step size of $step.  That is majorly bad.  I'm going to record the data, but I'm assuming it has a step size of $MIN_RRD_STEP_SIZE.");
      $step = $MIN_RRD_STEP_SIZE;
   }

   $total_data_count = 0;
   $zero_data_in_count = 0;
   $zero_data_out_count = 0;
   $nan_data_in_count = 0;
   $nan_data_out_count = 0;

   foreach my $data_line ( @{$data} )
   {
      #
      # The RRD returns INCLUSIVE endpoints... so for example, if I request
      # range 1000 - 2000, the RRD may return values from 950 - 2050!
      # Therefore I need to manually exclude values outside the range I want
      #
      if ( ($start >= $day_entry->{$MTL_START_EPOCH}) &&
           ($start <= $day_entry->{$MTL_END_EPOCH}) )
      {
         $total_data_count++;

         #
         # TODO: record the epoch time with the data
         #
         if ($data_line->[0] eq "NaN")
         { 
            $nan_data_in_count++;
            push(@in_data, -1);
         }
         else
         {
            if ( $data_line->[0] == 0 )
            { 
               $zero_data_in_count++;
            }
            push(@in_data, $data_line->[0]); 
         }

         if ($data_line->[1] eq "NaN")
         { 
            $nan_data_out_count++;
            push(@out_data, -1);
         }
         else
         {
            if ( $data_line->[1] == 0 )
            { 
               $zero_data_out_count++;
            }
            push(@out_data, $data_line->[1]); 
         }
      #
      # TODO: Pad for step size > 500
      }

      $start += $step;
   }

   &write_log_file($LOG_NOTE, "Circuit returned $total_data_count datapoints with a step size of $step.");
   &write_log_file($LOG_NOTE, "Zero data count:  $zero_data_in_count in, $zero_data_out_count out.");
   &write_log_file($LOG_NOTE, "NaN data count: $nan_data_in_count in, $nan_data_out_count out");

   #
   # Check to make sure the datapoint count is in the ballpark.
   # 
   $data_estimate_count = ($day_entry->{$MTL_END_EPOCH} - $day_entry->{$MTL_START_EPOCH}
                          + 1) / $step;
   if ( ($total_data_count < ($data_estimate_count - 1)) ||
        ($total_data_count > ($data_estimate_count + 1)) )
   {
      &write_log_file($LOG_WARNING, "Circuit should have returned about $data_estimate_count datapoints.  $total_data_count is off from this figure");
   }

   #
   # Check for excessive 0 or NaN's
   #
   if ( $total_data_count > 0 )
   {
      if ( (($zero_data_in_count * 100) / $total_data_count) > $MAX_ZERO_DATA_PERCENT )
      {
         &write_log_file($LOG_WARNING,
                         "Greater than $MAX_ZERO_DATA_PERCENT\% 0 IN data.");
      }
      if ( (($zero_data_out_count * 100) / $total_data_count) > $MAX_ZERO_DATA_PERCENT )
      {
         &write_log_file($LOG_WARNING,
                         "Greater than $MAX_ZERO_DATA_PERCENT\% 0 OUT data.");
      }

      if ( (($nan_data_in_count * 100) / $total_data_count) > $MAX_NAN_DATA_PERCENT )
      {
         &write_log_file($LOG_WARNING,
                         "Greater than $MAX_NAN_DATA_PERCENT\% NaN IN data.");
      }
      if ( (($nan_data_out_count * 100) / $total_data_count) > $MAX_NAN_DATA_PERCENT )
      {
         &write_log_file($LOG_WARNING,
                         "Greater than $MAX_NAN_DATA_PERCENT\% NaN OUT data.");
      }
   }

   return(\@in_data, \@out_data);
}


#################################################
# record_rrd_data
# 
# Appends the RRD data to the circuit data files.
#################################################
sub record_rrd_data
{
   my ($day_entry, $circuit_entry, $in_data, $out_data) = @_;

   my ($circuit_directory, $data_fh, $data_point, $retval);

   $circuit_directory = &get_circuit_directory($day_entry, $circuit_entry);

   $data_fh = new IO::File ">>$circuit_directory/$IN_DATA_FILE";
   if ( ! $data_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $circuit_directory/$IN_DATA_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   foreach $data_point ( @{$in_data} )
   {
      $retval = print $data_fh "$data_point\n";
      if ( $retval != 1 )
      {
         &write_log_file($LOG_ERROR, "Error writing to $circuit_directory/$IN_DATA_FILE: $!.  Program aborting");
         &write_log_file($END_STAMP, "BB_billing.pl ended");
         &close_log_file();
         exit($ERROR_CRITICAL);
      }
   }

   $retval = $data_fh->close();
   if ( ! $retval )
   {
      &write_log_file($LOG_ERROR, "Error closing $circuit_directory/$IN_DATA_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }


   $data_fh = new IO::File ">>$circuit_directory/$OUT_DATA_FILE";
   if ( ! $data_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $circuit_directory/$OUT_DATA_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   foreach $data_point ( @{$out_data} )
   {
      $retval = print $data_fh "$data_point\n";
      if ( $retval != 1 )
      {
         &write_log_file($LOG_ERROR, "Error writing to $circuit_directory/$OUT_DATA_FILE: $!.  Program aborting");
         &write_log_file($END_STAMP, "BB_billing.pl ended");
         &close_log_file();
         exit($ERROR_CRITICAL);
      }
   }

   $retval = $data_fh->close();
   if ( ! $retval )
   {
      &write_log_file($LOG_ERROR, "Error closing $circuit_directory/$OUT_DATA_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }
}


 ###                                ###
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ##    First Of Month 95/5 Report    ##
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ###                                ###

##########################################################################
# run_first_of_month_report
# 
# On the first of the month, a report is sent out to finance with the 95/5
# information for each circuit.
# 
# Because a circuit can be de-provisioned mid-month, we can't use the
# circuit list.  Instead we scan the directory for the circuits to bill
# for.
#
# We use the header file in each directory to get information about the
# circuit.
##########################################################################
sub run_first_of_month_report
{
   my ($day_entry) = @_;

   my ($report_directory, $report_fh, $retval);
   my (@circuit_directories, $circuit_directory, $circuit_entry);
   my ($data_point, $in_out);

   $report_directory = "$REPORT_HOME/" .  $day_entry->{$MTL_YEAR} .
                       "/" . $day_entry->{$MTL_MONTH};

   $report_fh = new IO::File ">$report_directory/$REPORT_FILE";
   if ( ! $report_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $report_directory/$REPORT_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   $retval = print $report_fh "Company\tWBN Order #\tPeak Bits\tPeak Bytes\tIn/Out\n";
   if ( $retval != 1 )
   {
      &write_log_file($LOG_ERROR, "Error writing to $report_directory/$REPORT_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   @circuit_directories = &get_circuit_directories($report_directory);

   foreach $circuit_directory ( @circuit_directories )
   {
      $circuit_entry = &read_circuit_header_file($circuit_directory);
      if ( ! $circuit_entry )
      {
         &write_log_file($LOG_WARNING, "No circuit header file found in $circuit_directory.  Skipping this directory for the monthly 95/5 report");
         next;
      }

      ($data_point, $in_out) = &get_circuit_95_5($circuit_directory);

      $retval = print $report_fh
                      $circuit_entry->{$CIRCUIT_COMPANY_NAME}, "\t",
                      $circuit_entry->{$CIRCUIT_ID}, "\t",
                      ($data_point * 8), "\t",
                      $data_point, "\t",
                      $in_out, "\n";
      if ( $retval != 1 )
      {
         &write_log_file($LOG_ERROR, "Error writing to $report_directory/$REPORT_FILE: $!.  Program aborting");
         &write_log_file($END_STAMP, "BB_billing.pl ended");
         &close_log_file();
         exit($ERROR_CRITICAL);
      }
   }

   $retval = $report_fh->close();
   if ( ! $retval )
   {
      &write_log_file($LOG_ERROR, "Error closing $report_directory/$REPORT_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   &send_report_email("$report_directory/$REPORT_FILE");
}


#########################################################################
# get_circuit_directories
# 
# Returns an array of all the subdirectories in the directory passed in.
#########################################################################
sub get_circuit_directories
{
   my ($report_directory) = @_;

   my ($dirhandle, @dir_entries, $dir_entry);
   my (@subdirs);

   $dirhandle = new DirHandle $report_directory;
   if ( ! $dirhandle )
   {
      &write_log_file($LOG_ERROR, "Error reading directory $report_directory: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   @dir_entries = sort $dirhandle->read();
   $dirhandle->close();

   foreach $dir_entry ( @dir_entries )
   {
      next if ( $dir_entry =~ /^\.\.?$/ );
      next if ( ! -d "$report_directory/$dir_entry" );
      push(@subdirs, "$report_directory/$dir_entry");
   }

   return @subdirs;
}


#######################################################################
# get_circuit_95_5
# 
# Reads in the circuits in and out data files, calculates the 95/5, and
# returns the greater of the two.
#######################################################################
sub get_circuit_95_5
{
   my ($circuit_directory) = @_;

   my ($in_95_5, $out_95_5);

   $in_95_5 = &get_file_95_5("$circuit_directory/$IN_DATA_FILE");
   $out_95_5 = &get_file_95_5("$circuit_directory/$OUT_DATA_FILE");

   if ( $in_95_5 > $out_95_5 )
   {
      return($in_95_5, "In");
   }
   else
   {
      return($out_95_5, "Out");
   }
}


############################################################
# get_file_95_5
# 
# Reads in the circuit data, then calculates and returns the
# 95/5 percent.
############################################################
sub get_file_95_5
{
   my ($datafile_name) = @_;

   my ($datafile_fh);
   my ($dataline, $datapoint, $epoch_time);
   my (@datapoints, $fifth_percent);

   if ( ! -e $datafile_name )
   {
      &write_log_file($LOG_WARNING, "The datafile $datafile_name does not exist.  This file won't be used in 95/5 calculations.  Returning 0 for the 95/5 for this datafile");
      return(0);
   }

   $datafile_fh = new IO::File "<$datafile_name";
   if ( ! $datafile_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $datafile_name.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   while ( defined($dataline = <$datafile_fh>) )
   {
      chomp($dataline);
      ($datapoint, $epoch_time) = split(/\t/, $dataline);
      push(@datapoints, $datapoint);
   }
   $datafile_fh->close();

   @datapoints = sort {$b <=> $a} @datapoints;

   #
   # This works because the array is zero based,
   # but we're calculating based on the number of entries.  Therefore, this
   # is actually the 5th percent plus 1
   #
   $fifth_percent = scalar(@datapoints) * 0.05;

   return($datapoints[$fifth_percent]);
}


###############################
# send_report_email
# 
# Sends out the 95/5 email out.
###############################
sub send_report_email
{
   my ($report_filename) = @_;

   my ($mime_mail, $mail_fh);

   $mime_mail =  new MIME::Entity;

   $mime_mail->build('Type'    => "multipart/mixed",
                     'From'    => $MAIL_FROM,
                     'To'      => $REPORT_MAIL_TO,
                     'Subject' => $report_filename);

   $mime_mail->attach('Path'     => "$report_filename",
                      'Type'     => "application/postscript",
                      'Encoding' => "base64");

   $mime_mail->attach('Data' => "Attached configuration file: $report_filename\n");

   $mail_fh = new IO::File "| /usr/lib/sendmail -t -i";
   if ( ! $mail_fh )
   {
      &write_log_file($LOG_ERROR, "Error invoking sendmail.  Could not send the 95/5 report out.  It exists in $report_filename though, so the program is continuing.");
      return;
   }

   $mime_mail->print($mail_fh);
   $mail_fh->close();
}


 ###                                ###
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ##    Last Run File Functions       ##
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ###                                ###

###############################################################################
# calculate_monitor_times
# 
# Returns an array of information for each day that needs to be monitored.
# The last monitored day is read from the $last_run_filename.
# For each day after that, up to and including yesterday, an entry in the
# array is created:
#    [ {$MTL_START_EPOCH    => $start_epoch_time,
#       $MTL_END_EPOCH      => $end_epoch_time,
#       $MTL_FIRST_OF_MONTH => $first_of_month_flag,
#       $MTL_YEAR           => $year, 
#       $MTL_MONTH          => $month,
#       $MTL_MDAY           => $day
#      },
#      ...
#    ]
# 
# We return a day at a time so we can handle the first of the
# month if it happens in the middle of the range of times that
# need to be monitored.
#
# The first_of_month flag logic is tricky!  We want to set this
# flag for the LAST day of the month.  So the first_of_month flag
# is set based on day 'x + 1' when we are writing the data for
# day 'x'.
###############################################################################
sub calculate_monitor_times
{
   my ($last_run_filename) = @_;

   my (@monitor_time_list);
   my ($last_run_year, $last_run_month, $last_run_day);
   my ($year, $month, $mday);
   my ($start_range_epoch, $end_range_epoch);
   my ($first_of_month, $counter);

   ($last_run_year, $last_run_month, $last_run_day) =
         &read_last_run_day_file($last_run_filename);

   #
   # We're going to start monitoring from yesterday.
   # However, record the first_of_month flag for today.
   #
   ($year, $month, $mday) = &get_today();
   $first_of_month = ($mday == 1) ? 1: 0;

   #
   # Here's the last day we'll monitor.  Now we work backwards
   # in the while loop.
   #
   ($year, $month, $mday) = &get_previous_day($year, $month, $mday);

   $counter = 1;

   while ( ( ($year  != $last_run_year)  ||
             ($month != $last_run_month) ||
             ($mday  != $last_run_day)   )
           &&
           ($counter <= $MAX_RECOVER_DAYS)
         )
   {
      ($start_range_epoch, $end_range_epoch) = 
            &get_day_epoch_range($year, $month, $mday);

      unshift(@monitor_time_list,
              {$MTL_START_EPOCH    => $start_range_epoch,
               $MTL_END_EPOCH      => $end_range_epoch,
               $MTL_FIRST_OF_MONTH => $first_of_month,
               $MTL_YEAR           => $year, 
               $MTL_MONTH          => $month,
               $MTL_MDAY           => $mday});

      #
      # NOTE: tricky tricky.  We are setting the first_of_month flag
      #       on the current day, before we grab the previous day's data
      #       below.
      #
      $first_of_month = ($mday == 1) ? 1 : 0;

      ($year, $month, $mday) = &get_previous_day($year, $month, $mday);

      $counter++;
   }

   if ( ($counter > $MAX_RECOVER_DAYS)
        &&
        ( ($year   != $last_run_year)  ||
          ($month  != $last_run_month) ||
          ($mday   != $last_run_day)   )
      )
   {
      &write_log_file($LOG_WARNING, "Last run day in $last_run_filename is $last_run_year-$last_run_month-$last_run_day.  Can't recover more than $MAX_RECOVER_DAYS days.  Data will be lost!");
   }

   return(\@monitor_time_list);
}


###########################################################################
# read_last_run_day_file
# 
# Reads in the last run date from the filename passed in.
# If the file doesn't exist, it returns the day before yesterday - assuming
# we have to run yesterday's data still.
###########################################################################
sub read_last_run_day_file
{
   my ($last_run_filename) = @_;

   my ($input_line);
   my ($last_run_year, $last_run_month, $last_run_day);
   my ($last_run_fh);

   if ( ! -e $last_run_filename )
   {
      return(&get_previous_day(&get_previous_day(&get_today())));
   }

   $last_run_fh = new IO::File "<$last_run_filename";
   if ( ! $last_run_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $last_run_filename.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   $input_line = <$last_run_fh>;
   chomp($input_line);

   if ( $input_line eq "" )
   {
      &write_log_file($LOG_INFO, "$last_run_filename is empty.  Assuming we start from yesterday.");
      return(&get_previous_day(&get_previous_day(&get_today())));
   }

   ($last_run_year, $last_run_month, $last_run_day) = split(/\t/, $input_line);
   if ( ($last_run_year !~ /^\d+$/)  ||
        ($last_run_month !~ /^\d+$/) ||
        ($last_run_day !~ /^\d+$/)   )
   {
      &write_log_file($LOG_ERROR, "$last_run_filename contains garbled data.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   return ($last_run_year, $last_run_month, $last_run_day);
}


#######################################################
# write_last_run_day_file
# 
# Writes out the lastrun day information to a directory
#######################################################
sub write_last_run_day_file
{
   my ($lastrun_directory, $day_entry) = @_;

   my ($lastrun_fh, $retval);

   $lastrun_fh = new IO::File ">$lastrun_directory/$LAST_RUN_FILE";
   if ( ! $lastrun_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $lastrun_directory/$LAST_RUN_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   $retval = print $lastrun_fh $day_entry->{$MTL_YEAR}, "\t",
                               $day_entry->{$MTL_MONTH}, "\t",
                               $day_entry->{$MTL_MDAY}, "\n";
   if ( $retval != 1 )
   {
      &write_log_file($LOG_ERROR, "Error writing to $lastrun_directory/$LAST_RUN_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   $retval = $lastrun_fh->close();
   if ( ! $retval )
   {
      &write_log_file($LOG_ERROR, "Error closing $lastrun_directory/$LAST_RUN_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }
}


##############################################
# get_today
# 
# Returns today's year, month(base 1), and day
##############################################
sub get_today
{
   my ($current_time_epoch);
   my ($sec,$min,$hour,$mday,$mon_b0,$year_b1900,$wday,$yday,$isdst);

   $current_time_epoch = time;
   ($sec,$min,$hour,$mday,$mon_b0,$year_b1900,$wday,$yday,$isdst) =
         localtime($current_time_epoch);

   return($year_b1900 + 1900, $mon_b0 + 1, $mday);
}


#####################################################################
# get_previous_day
# 
# Passed in year(base 1), month(base 1), and day, it returns
# the previous day's year(base 1), month(base 1), and day.
# Its return values are in the same order as the parameters, so that
# calls can be nested.
#####################################################################
sub get_previous_day
{
   my ($current_year_b0, $current_mon_b1, $current_mday) = @_;

   my ($midnight_epoch, $yesterday_epoch);
   my ($sec,$min,$hour,$mday,$mon_b0,$year_b1900,$wday,$yday,$isdst);

   $midnight_epoch = timelocal(0, 0, 0, $current_mday, $current_mon_b1 - 1,
                               $current_year_b0 - 1900);

   $yesterday_epoch = $midnight_epoch - 1;

   ($sec,$min,$hour,$mday,$mon_b0,$year_b1900,$wday,$yday,$isdst) =
         localtime($yesterday_epoch);

   return($year_b1900 + 1900, $mon_b0 + 1, $mday);
}


####################################################
# get_day_epoch_range
# 
# Passed in year, month(base 1), and day, it returns
# the start and ending epoch range for that day.
####################################################
sub get_day_epoch_range
{
   my ($current_year_b0, $current_mon_b1, $current_mday) = @_;

   my ($start_range_epoch, $end_range_epoch);

   $start_range_epoch = timelocal(0, 0, 0, $current_mday, $current_mon_b1 - 1,
                                 $current_year_b0 - 1900);

   $end_range_epoch = timelocal(59, 59, 23, $current_mday, $current_mon_b1 - 1,
                                $current_year_b0 - 1900);

   return($start_range_epoch, $end_range_epoch);
}


 ###                                ###
 ##                                  ##
 ##                                  ##
 ##                                  ##
 ##      Circuit Directory /         ##
 ##    Header File Functions         ##
 ##                                  ##
 ##                                  ##
 ###                                ###

#####################################################################
# create_month_directory
# 
# Creates the month directory for a given date, if it doesn't exist
#####################################################################
sub create_month_directory
{
   my ($day_entry) = @_;

   my ($directory, $subdir);

   $directory = $REPORT_HOME;
   foreach $subdir ( $day_entry->{$MTL_YEAR},
                     $day_entry->{$MTL_MONTH} )
   {
      $directory .= "/$subdir";
      if ( ! -d $directory )
      {
         if ( ! mkdir($directory, 0755) )
         {
            &write_log_file($LOG_ERROR, "Error creating directory $directory: $!.  Program aborting");
            &write_log_file($END_STAMP, "BB_billing.pl ended");
            &close_log_file();
            exit($ERROR_CRITICAL);
         }
      }
   }
}


#############################################################
# get_circuit_directory
# 
# Returns the directory for the circuit data for a given date
#############################################################
sub get_circuit_directory
{
   my ($day_entry, $circuit_entry) = @_;

   return "$REPORT_HOME/" . $day_entry->{$MTL_YEAR} .
          "/" . $day_entry->{$MTL_MONTH} .
          "/" . $circuit_entry->{$CIRCUIT_ID};
}


#####################################################################
# create_circuit_directory
# 
# Creates the circuit directory for a given date, if it doesn't exist
#####################################################################
sub create_circuit_directory
{
   my ($day_entry, $circuit_entry) = @_;

   my ($directory, $subdir);

   $directory = $REPORT_HOME;
   foreach $subdir ( $day_entry->{$MTL_YEAR},
                     $day_entry->{$MTL_MONTH},
                     $circuit_entry->{$CIRCUIT_ID} )
   {
      $directory .= "/$subdir";
      if ( ! -d $directory )
      {
         if ( ! mkdir($directory, 0755) )
         {
            &write_log_file($LOG_ERROR, "Error creating directory $directory: $!.  Program aborting");
            &write_log_file($END_STAMP, "BB_billing.pl ended");
            &close_log_file();
            exit($ERROR_CRITICAL);
         }
      }
   }
}


##################################################################
# create_circuit_header_file
# 
# Creates the header file for a circuit.  This records information
# about the circuit for the end-of-month reporting.
##################################################################
sub create_circuit_header_file
{
   my ($day_entry, $circuit_entry) = @_;

   my ($circuit_directory, $data_key, $header_fh, $retval);

   $circuit_directory = &get_circuit_directory($day_entry, $circuit_entry);

   $header_fh = new IO::File ">$circuit_directory/$CIRCUIT_HEADER_FILE";
   if ( ! $header_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $circuit_directory/$CIRCUIT_HEADER_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   foreach $data_key ( $CIRCUIT_COMPANY_NAME,
                       $CIRCUIT_IP_ADDRESS,
                       $CIRCUIT_PORT_NAME,
                       $CIRCUIT_ID )
   {
      $retval = print $header_fh $circuit_entry->{$data_key}, "\n";
      if ( $retval != 1 )
      {
         &write_log_file($LOG_ERROR, "Error writing to $circuit_directory/$CIRCUIT_HEADER_FILE: $!.  Program aborting");
         &write_log_file($END_STAMP, "BB_billing.pl ended");
         &close_log_file();
         exit($ERROR_CRITICAL);
      }
   }

   $retval = $header_fh->close();
   if ( ! $retval )
   {
      &write_log_file($LOG_ERROR, "Error closing $circuit_directory/$CIRCUIT_HEADER_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }
}


#################################################
# read_circuit_header_file
# 
# Reads the header file information for a circuit
#################################################
sub read_circuit_header_file
{
   my ($circuit_directory) = @_;

   my ($header_fh, $data_key, %circuit_entry);
   my ($circuit_line);

   if ( ! -e "$circuit_directory/$CIRCUIT_HEADER_FILE" )
   {
      return undef;
   }

   $header_fh = new IO::File "<$circuit_directory/$CIRCUIT_HEADER_FILE";
   if ( ! $header_fh )
   {
      &write_log_file($LOG_ERROR, "Error opening $circuit_directory/$CIRCUIT_HEADER_FILE: $!.  Program aborting");
      &write_log_file($END_STAMP, "BB_billing.pl ended");
      &close_log_file();
      exit($ERROR_CRITICAL);
   }

   foreach $data_key ( $CIRCUIT_COMPANY_NAME,
                       $CIRCUIT_IP_ADDRESS,
                       $CIRCUIT_PORT_NAME,
                       $CIRCUIT_ID )
   {
      $circuit_line = <$header_fh>;
      chomp($circuit_line);
      if ( ! $circuit_line )
      {
         &write_log_file($LOG_WARNING, "$circuit_directory/$CIRCUIT_HEADER_FILE has corrupted data.");
      }

      $circuit_entry{$data_key} = $circuit_line;
   }

   $header_fh->close();

   return(\%circuit_entry);
}


#################################################
# update_circuit_header_file
# 
# This checks if the circuit file exists.
# If it doesn't exist, it creates a header file.
# If it exists, it checks to see whether the information 
# in the header file has changed.  If it changed, it
# updates the header file.
#################################################
sub update_circuit_header_file
{
   my ($day_entry, $circuit_entry) = @_;

   my ($circuit_directory, $old_circuit_entry);
   my ($data_key);

   $circuit_directory = &get_circuit_directory($day_entry, $circuit_entry);

   $old_circuit_entry = &read_circuit_header_file($circuit_directory);
   if ( ! $old_circuit_entry )
   {
      &create_circuit_header_file($day_entry, $circuit_entry);
   }
   else
   {
      foreach $data_key ( $CIRCUIT_COMPANY_NAME,
                          $CIRCUIT_IP_ADDRESS,
                          $CIRCUIT_PORT_NAME,
                          $CIRCUIT_ID )
      {
         if ( $old_circuit_entry->{$data_key} ne $circuit_entry->{$data_key} )
         {
            &create_circuit_header_file($day_entry, $circuit_entry);
            return;
         }
      }
   }
}
