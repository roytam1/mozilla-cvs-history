#!/usr/local/bin/perl -w
#
# converts the V1.0 directory structure layout to the V1.1 layout
#

use strict;
use DirHandle;

my $ROOT_DIR = "/usr/local/root/apache/cgi-bin/Burstable";

my $OLD_DIR = "$ROOT_DIR/BurstReports";

my $NEW_DIR = "$ROOT_DIR/TEST_ENV/data";

my $circuit_file = "$ROOT_DIR/burst_accounts.txt";

&main_program();


sub main_program
{
   my ($month_dir_list, $month);
   my ($conversion_list);

   $conversion_list = &get_circuit_info_list();

   $month_dir_list = &get_subdirs($OLD_DIR);

   foreach $month ( @{$month_dir_list} )
   {
      print "Converting month $month\n";
      &convert_month($month, $conversion_list);
   }
}


sub convert_month
{
   my ($month, $conversion_list) = @_;

   my ($circuit_dir_list, $circuit);

   $circuit_dir_list = &get_subdirs("$OLD_DIR/$month");

   foreach $circuit ( @{$circuit_dir_list} )
   {
      print "\tConverting circuit $circuit\n";
      &convert_circuit($circuit, $month, $conversion_list);
   }
}

sub convert_circuit
{
   my ($circuit, $month, $conversion_list) = @_;

   my ($circuit_files, $circuit_file);

   $circuit_files = &get_circuit_files("$OLD_DIR/$month/$circuit");
   foreach $circuit_file ( @{$circuit_files} )
   {
      print "\t\tConverting data file $circuit_file\n";
      &copy_circuit_file($circuit, $month, $conversion_list, $circuit_file);
   }
}


sub copy_circuit_file
{
   my ($circuit, $month, $conversion_list, $circuit_file) = @_;

   my ($host, $ip, $type);
   my ($company_name, $circuit_id);

   if ( $circuit_file =~ /^(.*)\.(\d+\.\d+\.\d+\.\d+).(in|out).log.all$/ )
   {
      print "\t\tHost: $host   IP: $ip   Type: $type\n";
      $host = $1;
      $ip = $2;
      $type = $3;
      ($company_name, $circuit_id) = &get_circuit_info($host, $ip, $conversion_list);
      print "\t\tLookup found: Company Name: $company_name  Circuit ID: $circuit_id\n";

      &create_circuit_directory($month, $circuit_id);
###      `cp $OLD_DIR/$month/$company_name/$circuit_file $NEW_DIR/2000/$month/$circuit_id/$type.log`;
      print "Would run: cp $OLD_DIR/$month/$company_name/$circuit_file $NEW_DIR/2000/$month/$circuit_id/$type.log\n";
   }
   else
   {
      print "Can't parse circuit_file $circuit_file\n";
   }
}


sub get_circuit_info_list
{
   my (@circuit_list, $circuit_fh, $circuit_line);
   my (@circuit_entry);

   $circuit_fh = new IO::File "<$circuit_file";
   if ( ! $circuit_fh )
   {
      die "Error opening $circuit_file: $!\n";
   }
   while ( defined($circuit_line = <$circuit_fh>) )
   {
      chomp($circuit_line);

      @circuit_entry = split(/:/, $circuit_line);
      push(@circuit_list, [@circuit_entry]);
   }

   $circuit_fh->close();

   return(\@circuit_list);
}

sub get_circuit_info
{
   my ($host, $ip, $conversion_list) = @_;

   my ($circuit_entry);

   foreach $circuit_entry ( @{$circuit_entry} )
   {
      if ( ($circuit_entry->[1] eq $ip) &&
           ($circuit_entry->[2] eq $host) )
      {
         return ($circuit_entry->[0], $circuit_entry->[3]);
      }
   }

   print "Circuit info not found. for $host / $ip\n";
   return ("Unknown", "$host-$ip");
}


sub get_subdirs
{
   my ($dir) = @_;

   my ($dirhandle, @direntries, $direntry);
   my (@subdirs);

   $dirhandle = new DirHandle $dir;
   if ( ! $dirhandle )
   {
      die "Error opening directory $dir: $!\n";
   }
   
   @direntries = $dirhandle->read();
   $dirhandle->close();

   foreach $direntry ( @direntries )
   {
      next if ( $direntry =~ /^\.\.?$/ );
      next if ( ! -d "$dir/$direntry" );
      push(@subdirs, $direntry);
   }

   return \@subdirs;
}


sub get_circuit_files
{
   my ($dir) = @_;

   my ($dirhandle, @direntries, $direntry);
   my (@files);

   $dirhandle = new DirHandle $dir;
   if ( ! $dirhandle )
   {
      die "Error opening directory $dir: $!\n";
   }
   
   @direntries = sort $dirhandle->read();
   $dirhandle->close();

   foreach $direntry ( @direntries )
   {
      next if ( $direntry !~ /log\.all$/ );
      push(@files, $direntry);
   }

   return \@files;
}


sub create_circuit_directory
{
   my ($month, $circuit_id) = @_;

   my ($directory, $subdir);

   $directory = $NEW_DIR;
   foreach $subdir ( "2000", $month, $circuit_id )
   {
      $directory .= "/$subdir";
      if ( ! -d $directory )
      {
         if ( ! mkdir($directory, 0755) )
         {
            die "Error creating directory $directory: $!.  Program aborting\n";
         }
      }
   }
}
