#!/usr/bin/perl -w
#####################################################################
# $Id$
#
# BB_lib.pl
# 
# This file contains functions shared amongst the programs
# 
# History:
#         Kevin J. McCarthy  [10/17/2000]   Original Coding
#####################################################################

use strict;

use DirHandle;
use IO::File;
use File::Copy;

BEGIN {require '../inc/BB_include.pl';}


###########
# Globals #
###########
my $LOG_FH;


##################################################################
# get_time_stamp
# 
# This returns the current timestamp, in the logfile format.
##################################################################
sub get_time_stamp
{
   my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();

   $mon += 1;
   $year += 1900;

   $mon  = "0" . $mon  if ( $mon < 10 );
   $mday = "0" . $mday if ( $mday < 10 );
   $hour = "0" . $hour if ( $hour < 10 );
   $min  = "0" . $min  if ( $min < 10 );
   $sec  = "0" . $sec  if ( $sec < 10 );

   return "$year$mon$mday-$hour$min$sec";
}


######################################################
# get_file_timestamp
# 
# This returns the 'modification' timestamp of a file,
# used for file archving.
######################################################
sub get_file_timestamp
{
   my ($filename) = @_;

   my ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size);
   my ($atime, $mtime, $ctime, $blksize, $blocks);
   my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst);

   ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size,
    $atime, $mtime, $ctime, $blksize, $blocks) = stat $filename;

   ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($mtime);

   $mon += 1;
   $year += 1900;

   $mon  = "0" . $mon  if ( $mon < 10 );
   $mday = "0" . $mday if ( $mday < 10 );
   $hour = "0" . $hour if ( $hour < 10 );
   $min  = "0" . $min  if ( $min < 10 );
   $sec  = "0" . $sec  if ( $sec < 10 );

   return "$year$mon$mday-$hour$min$sec";
}


###############################################################
# archive_file
# 
# This routine moves a file into a backup directory.
# 
# Arguments:
#   $dir              => source directory to archive from
#   $filename         => file to archive
#   $archive_dir      => destination directory to archive to
#   $archive_filename => destination filename (wihout datestamp -
#                        that is appended automatically)
#   $copy             => optional parameter.  If == 1 then the files
#                        are copied, not moved to $archive_dir.
# Returns: $OKAY on success
###############################################################
sub archive_file
{
   my ($dir, $filename, $archive_dir, $archive_filename, $copy) = @_;

   my ($full_filename, $timestamp, $full_archive_filename);


   $full_filename = "$dir/$filename";
##   $timestamp = &get_time_stamp();
   $timestamp = &get_file_timestamp($full_filename);

   $full_archive_filename = "$archive_dir/$archive_filename.$timestamp";

   if ( -f $full_filename )
   {
      if ( defined($copy) && $copy )
      {
         if ( ! copy($full_filename, $full_archive_filename) )
         {
            return($ERROR_WARNING);
         }
      }
      else
      {
         if ( ! move($full_filename, $full_archive_filename) )
         {
            return($ERROR_WARNING);
         }
      }
   }

   return($OKAY);
}


###########################################################################
# purge_old_archives
# 
# This routine deletes old archive files - it sorts the files in the
# directory and deletes the oldest files until there are at most $max_files
# in the directory
###########################################################################
sub purge_old_archives
{
   my ($archive_dir, $max_files, $prefix) = @_;

   my ($dirhandle, @allfiles, @delfiles);
   local($_);

   $dirhandle = new DirHandle $archive_dir;
   if ( ! $dirhandle )
   {
      return ($ERROR_WARNING);
   }

   @allfiles = grep /^\Q$prefix\E/,
                    reverse sort $dirhandle->read();
   $dirhandle->close();

   @delfiles = splice @allfiles, $max_files;
   unlink map {"$archive_dir/$_"}  @delfiles;
}


###############################
# open_log_file
# 
# Opens the log file for append
###############################
sub open_log_file
{
   $LOG_FH = new IO::File ">>$LOG_DIR/$LOG_FILE";
   if ( ! $LOG_FH )
   {
      print STDERR "Error opening log file\n";
      exit($ERROR_CRITICAL);
   }
}


################
# close_log_file
################
sub close_log_file
{
   $LOG_FH->close();
}


##################################
# write_log_file
# 
# Writes a message to the log file
##################################
sub write_log_file
{
   my ($code, $desc) = @_;

   my ($timestamp);

   $timestamp = &get_time_stamp();

   if ( $code eq $START_STAMP )
   {
      print $LOG_FH "\n$code<$timestamp>: $desc\n";
   }
   elsif ( $code eq $END_STAMP )
   {
      print $LOG_FH "$code<$timestamp>: $desc\n\n";
   }
   else
   {
      print $LOG_FH "\t$code<$timestamp>: $desc\n";
   }
}

1;
