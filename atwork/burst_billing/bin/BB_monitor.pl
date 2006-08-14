#!/usr/bin/perl
#############################################################################
# $Id$
#
# BB_monitor.pl
# 
# History:
#         Kevin J. McCarthy   [10/17/2000]   Original Coding
#
#############################################################################

#
# Not using strict so I can play namespace games
# Not using warnings because the Mail and Net modules suck.
#

use IO::File;
#
# I'm not using Mail::Send becuase the current implementation has a bug
# with multiple recipients - sent a patch to Graham.
#
use Mail::Mailer;
#
# Have to 'use' this here so I can overwrite the 
#   $Net::SMTP::NetConfig{'smtp_hosts'} = [$MAIL_HOST];
#
use Net::SMTP;

BEGIN {require '../inc/BB_include.pl';}
require "$INC_DIR/BB_lib.pl";


###########
# Globals #
###########
my $errors = 0;
my $warnings = 0;


##############
# Begin Code #
##############

&init_program();

&scan_log_file();

# if ( $errors || $warnings )
&send_email();

exit($OKAY);


###################
# Local Functions #
###################


#####################################################################
# init_program
# 
# Sets the smtp server to use and the mail from address.
# Closes STDOUT and STDERR to prevent junk messages from STMP modules
#####################################################################
sub init_program
{
   #
   # Set the SMTP server to use.
   # This is cheating, but it works.
   #
   $Net::SMTP::NetConfig{'smtp_hosts'} = $MAIL_HOSTS;

   #
   # Set the from address
   #
   $ENV{'MAILADDRESS'} = $MAIL_FROM;

   #
   # Unfortunately, the SMTP modules are noisy in the case of errors.
   #
   close(STDOUT);
   close(STDERR);
}


###########################################################
# scan_log_file
# 
# Records the number of errors and warnings in the log file
###########################################################
sub scan_log_file
{
   my ($log_fh, $log_line);

   $log_fh = new IO::File "<$LOG_DIR/$LOG_FILE";
   return if ( ! $log_fh );

   while ( defined($log_line = <$log_fh>) )
   {
      $errors++ if ( $log_line =~ /^\s*$LOG_ERROR/o );
      $warnings++ if ( $log_line =~ /^\s*$LOG_WARNING/o );
   }

   $log_fh->close();
}


#############################
# send_email
# 
# Sends out the summary email
#############################
sub send_email
{
   my ($mailer, $timestamp);
   my ($log_fh, $log_line);

   $timestamp = &get_time_stamp();

   $mailer = new Mail::Mailer('smtp');
   if ( ! $mailer )
   {
      exit($ERROR_CRITICAL);
   }

   eval {$mailer->open({
             'To' => $ERROR_MAIL_TO,
             'Subject' => "Burst Billing summary for $timestamp",
             });
        };
   if ( $@ )
   {
      exit($ERROR_CRITICAL);
   }

   print $mailer "Burst Billing summary for $timestamp\n\n";
   print $mailer "ERRORS: $errors\n";
   print $mailer "WARNINGS: $warnings\n";

   print $mailer "Contents of log file:\n\n";

   $log_fh = new IO::File "<$LOG_DIR/$LOG_FILE";
   if ( $log_fh )
   {
      while ( defined($log_line = <$log_fh>) )
      {
         if ( ($log_line =~ /^\s*$START_STAMP/o)   ||
              ($log_line =~ /^\s*$END_STAMP/o)     ||
              ($log_line =~ /^\s*$LOG_INFO/o)      ||
              ($log_line =~ /^\s*$LOG_WARNING/o)   ||
              ($log_line =~ /^\s*$LOG_ERROR/o)     ||
              ($log_line eq "\n")                  )
         {
            print $mailer $log_line;
         }
      }
      $log_fh->close();
   }

   $mailer->close();
}
