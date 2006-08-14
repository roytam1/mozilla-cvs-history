#!/usr/bin/perl -w
#############################################################################
# $Id$
#
# BB_main.pl
# 
# This is the main control program for the Burst Billing process
# It:
#         1) Checks the directory setup and permissions.
#         2) Archives the log file for the current project.
#         3) Launches the Burst Billing program
#         4) Runs the monitor program.
#
# I broke the burst billing and monitor programs out into seperate programs.
# This is to handle the event where the burst billing program die's, aborts,
# crashes, or whatever - so the main program can run the monitor program afterwards.
# Yeah yeah, the main program might crash too, but what can I do...
# 
# History:
#         Kevin J. McCarthy   [10/17/2000]   Original Coding
#
#############################################################################

use strict;

BEGIN {require '../inc/BB_include.pl';}
require "$INC_DIR/BB_lib.pl";


##############
# Begin Code #
##############

&init_program();

&run_burst_billing();

&run_monitor();

&cleanup_program();



###################
# Local Functions #
###################

########################################
# init_program
# 
# Does program initializations, such as:
# - Checking directory permissions
# - Archiving the log file
# - Opening the new log file
########################################
sub init_program
{
   &check_if_running();

   &check_directories();

   &check_files();

   &archive_log_file();

   &open_log_file();

   &write_log_file($START_STAMP, "BB_main.pl started");
}


##########################################################################
# check_if_running
# 
# Checks to see if another copy of the program is running.  This is
# indicated by the presence of the $IN_PROGRESS_FILE
# If it is, the program aborts.
# 
# If it isn't:
# 1) The $IN_PROGRESS_FILE is created.
# 2) An END routine which removes the $IN_PROGRESS_FILE is evaled.  I need
#    to eval this AFTER the check for the file, otherwise I will remove
#    the $IN_PROGRESS_FILE for the other program!
# Note that this function is not used for contention issues - I'm
# fully of aware of the test-and-set race condition.
##########################################################################
sub check_if_running
{
   my ($ip_fh);

   if ( -f $IN_PROGRESS_FILE )
   {
      print STDERR "Another copy of BB_main.pl is currently running\n",
                   "If this is not the case, please remove the ",
                   "$IN_PROGRESS_FILE file and rerun the program.\n",
                   "Aboring program\n";
      exit($ERROR_CRITICAL);
   }

   $ip_fh = new IO::File ">$IN_PROGRESS_FILE";
   $ip_fh->close();

   eval 'END { &remove_running_file(); }';
}


################################################################
# check_directories
# 
# Basic sanity check, to make sure all the directories exist and
# are writable by this process
################################################################
sub check_directories
{
   my ($directory);

   foreach $directory ( $BILLING_HOME, $REPORT_HOME, $LOG_DIR, $LOG_ARCHIVE_DIR,
                        $RRD_DIR, $BIN_DIR, $INC_DIR )
   {
      if ( ! -d $directory )
      {
         print STDERR "ERROR: $directory is not a directory\n",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
   }
   foreach $directory ( $BILLING_HOME, $REPORT_HOME, $LOG_DIR,
                        $LOG_ARCHIVE_DIR )
   {
      if ( ! -w $directory )
      {
         print STDERR "ERROR: $directory is not writable\n",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
   }
}


################################################################
# check_files
# 
# Basic sanity check, to make sure all the programs and files exist
# before launching sub-process
################################################################
sub check_files
{
   my ($file);

   foreach $file ( $BURST_ACCOUNTS_FILE )
   {
      if ( ! -e $file )
      {
         print STDERR "ERROR: $file does not exist\n.",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
      elsif ( ! -r $file )
      {
         print STDERR "ERROR: $file is not readable.\n",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
   }

   foreach $file ( $BILLING_MODULE, $MONITOR_MODULE )
   {
      if ( ! -e "$BIN_DIR/$file" )
      {
         print STDERR "ERROR: $BIN_DIR/$file does not exist.\n",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
      if ( (! -r "$BIN_DIR/$file") || (! -x "$BIN_DIR/$file") )
      {
         print STDERR "ERROR: $BIN_DIR/$file is not runnable.\n",
                      "Aborting program\n";
         exit($ERROR_CRITICAL);
      }
   }
}


##############################################################
# archive_log_file
# 
# This copies the previous log file into the archive directory
# and then cleans up the archive directory.
##############################################################
sub archive_log_file
{
   if ( &archive_file($LOG_DIR, $LOG_FILE, $LOG_ARCHIVE_DIR, $LOG_FILE, 0) != $OKAY )
   {
      print STDERR "Error archiving log file\n";
   }

   &purge_old_archives($LOG_ARCHIVE_DIR, $MAX_LOG_ARCHIVE_FILES, $LOG_FILE);
}


#####################################################################
# run_boss_loader
# 
# This function executes the burst_billing program 
# programs, checks the return codes, and handles errors.
#####################################################################
sub run_burst_billing
{
   my ($retcode);

   &write_log_file($LOG_INFO, "BB_main.pl starting to run $BILLING_MODULE");
   &close_log_file();

   $retcode = system("$BIN_DIR/$BILLING_MODULE");
   $retcode >>= 8;

   &open_log_file();
   &write_log_file($LOG_INFO, "$BILLING_MODULE returned exit code $retcode");

   if ( ($retcode != $OKAY) &&
        ($retcode != $ERROR_WARNING) )
   {
      &write_log_file($LOG_ERROR, "$BILLING_MODULE had some sort of error");
   }

   return($retcode);
}


#########################################################################
# run_monitor
# 
# Runs the monitor module.
#########################################################################
sub run_monitor
{
   my ($retcode);

   &write_log_file($LOG_INFO, "BB_main.pl starting to run $MONITOR_MODULE");
   &close_log_file();

   $retcode = system("$BIN_DIR/$MONITOR_MODULE");
   $retcode >>= 8;

   &open_log_file();
   &write_log_file($LOG_INFO, "$MONITOR_MODULE returned exit code $retcode");

   if ( ($retcode != $OKAY) &&
        ($retcode != $ERROR_WARNING) )
   {
      &write_log_file($LOG_ERROR, "$MONITOR_MODULE had some sort of error");
   }

   return($retcode);
}


#####################################################
# cleanup_program
# 
# Perform final cleanup before the program terminates
#####################################################
sub cleanup_program
{
   &write_log_file($END_STAMP, "BOSS_main.pl ended");

   &close_log_file();
}


##########################################################################
# remove_running_file
# 
# Removes the $IN_PROGRESS_FILE, just before the program exits.  Note that
# this function is called by the END{} routine, which runs just as the
# perl interpreter exits.  In this way, I don't have to worry about
# something die()ing and not cleaning up this file.
##########################################################################
sub remove_running_file
{
   unlink $IN_PROGRESS_FILE;
}
