
package Moz::Prefs;

require 5.004;
require Exporter;

# Package that attempts to read a file from the Preferences folder,
# and get build settings out of it

use strict;

use Exporter;
use File::Path;

use Mac::Files;

use vars qw(@ISA @EXPORT);

@ISA		  = qw(Exporter);
@EXPORT 	= qw(ReadMozUserPrefs);


#-------------------------------------------------------------------------------
#
# GetPrefsFolder
#
#-------------------------------------------------------------------------------

sub GetPrefsFolder()
{
  my($prefs_folder) = FindFolder(kOnSystemDisk, kPreferencesFolderType, 1);
  return $prefs_folder.":Mozilla build prefs";
}


#-------------------------------------------------------------------------------
#
# SetArrayValue
#
#-------------------------------------------------------------------------------
sub SetArrayValue($$$)
{
  my($array_ref, $index1, $index2) = @_;

  my($index);
  foreach $index (@$array_ref)
  {
    if ($index->[0] eq $index1)
    {
      $index->[1] = $index2;
      return 1;
    }
  }
  
  return 0;
}


#-------------------------------------------------------------------------------
#
# WriteDefaultPrefsFile
#
#-------------------------------------------------------------------------------

sub WriteDefaultPrefsFile($)
{
  my($file_path) = @_;

  my($file_contents);
  $file_contents = <<'EOS';
% You can use this file to customize the Mozilla build system.
% The following kinds of lines are allowable:
%   Comment lines, which start with a '%' in the first column
%   Lines which modify the default build settings. For the list of flags,
%   see MozBuildFlags.pm. Examples are:
%
%    build       pull            0                    % don't pull
%    options     mng             1                    % turn mng on
%   
%   Line containing the special 'buildfrom' flag, which specifies
%   where to start the build. Example:
%   
%    buildfrom   nglayout                % where to start the build
%
%   Lines which specify the location of the files used to store paths
%   to the CodeWarrior IDE, and the MacCVS Pro session file. Note quoting
%   of paths containing whitespace. Examples:
%
%    filepath   idepath         ::codewarrior.txt
%    filepath   sessionpath     ":Some folder:MacCVS session path.txt"
%
%   Lines which modify the build settings like %main::DEBUG.
%   Any lines which do not match either of the above are assumed
%   to set variables on $main::. Examples:
%   
%    MOZILLA_OFFICIAL    1
%
EOS

  $file_contents =~ s/%/#/g;
   
  local(*PREFS_FILE);

  open(PREFS_FILE, "> $file_path") || die "Could not write default prefs file\n";
  print PREFS_FILE ($file_contents);
  close(PREFS_FILE);

  MacPerl::SetFileInfo("McPL", "TEXT", $file_path);
}


#-------------------------------------------------------------------------------
#
# HandlePrefSet
#
#-------------------------------------------------------------------------------
sub HandlePrefSet($$$$)
{
  my($flags, $name, $value, $desc) = @_;

  if (SetArrayValue($flags, $name, $value)) {
    print "Prefs set $desc flag '$name' to '$value'\n";
  } else {
    die "$desc setting '$name' is not a valid option\n";
  }

}


#-------------------------------------------------------------------------------
#
# HandleBuildFromPref
#
#-------------------------------------------------------------------------------
sub HandleBuildFromPref($$)
{
  my($build_array, $name) = @_;

  my($setting) = 0;
  my($index);
  foreach $index (@$build_array)
  {
    if ($index->[0] eq $name) {
      $setting = 1;
    }
    
    $index->[1] = $setting;    
  }

  if ($setting == 1) {
    print "Building from $name onwards, as specified by prefs\n";
  } else {
    printf "Failed to find buildfrom setting '$name'\n";
  }
}


#-------------------------------------------------------------------------------
#
# ReadPrefsFile
#
#-------------------------------------------------------------------------------

sub ReadPrefsFile($$$$$)
{
  my($file_path, $build_flags, $options_flags, $filepath_flags, $create_if_missing) = @_;
  
  local(*PREFS_FILE);
  
  if (open(PREFS_FILE, "< $file_path"))
  {
    print "Reading build prefs from '$file_path'\n";
  
    while (<PREFS_FILE>)
    {
      my($line) = $_;
      chomp($line);
      
      if ($line =~ /^\#/ || $line =~ /^\s*$/) {    # ignore comments and empty lines
        next;
      }
            
      if (($line =~ /^\s*([^#\s]+)\s+([^#\s]+)\s+\"(.+)\"(\s+#.+)?/) ||
          ($line =~ /^\s*([^#\s]+)\s+([^#\s]+)\s+\'(.+)\'(\s+#.+)?/) ||
          ($line =~ /^\s*([^#\s]+)\s+([^#\s]+)\s+([^#\s]+)(\s+#.+)?/))
      {
        my($array_name) = $1;      
        my($option_name) = $2;
        my($option_value) = $3;      
      
        # print "Read '$array_name' '$option_name' '$option_value'\n";
        
        if ($array_name eq "build")
        {
          HandlePrefSet($build_flags, $option_name, $option_value, "Build");
        }
        elsif ($array_name eq "options")
        {
          HandlePrefSet($options_flags, $option_name, $option_value, "Options");
        }
        elsif ($array_name eq "filepath" && $option_name && $option_value)
        {
          HandlePrefSet($filepath_flags, $option_name, $option_value, "Filepath");
        }
        else
        {
          print "Unknown pref option at $line\n";
        }
      }
      elsif ($line =~ /^\s*buildfrom\s+([^#\s]+)(\s+#.+)?/)
      {
        my($build_start) = $1;
        HandleBuildFromPref($build_flags, $build_start);
      }
      elsif ($line =~ /^\s*([^#\s]+)\s+([^#\s]+)(\s+#.+)?/)
      {
        my($build_var) = $1;
        my($var_setting) = $2;
        
        print "Setting \$main::$build_var to $var_setting\n";
        eval "\$main::$build_var = \"$var_setting\"";
      }
      else
      {
        print "Unrecognized input line at $line\n";
      }
      
    }
    
    close(PREFS_FILE);  
  }
  elsif ($create_if_missing)
  {
    print "No prefs file found at $file_path; using defaults\n";
    
    my($folder_path) = $file_path;
    $folder_path =~ s/[^:]+$//;
    mkpath($folder_path);
    WriteDefaultPrefsFile($file_path);
  }
}


#-------------------------------------------------------------------------------
#
# ReadMozUserPrefs
#
#-------------------------------------------------------------------------------

sub ReadMozUserPrefs($$$$)
{
  my($prefs_file_name, $build_flags, $options_flags, $filepath_flags) = @_;
  
  if ($prefs_file_name eq "") { return; }
  
  # if local prefs exist, just use those. Othewise, look in the prefs folder
  if (-e $prefs_file_name)
  {
    # read local prefs
    ReadPrefsFile($prefs_file_name, $build_flags, $options_flags, $filepath_flags, 0);
  }
  else
  {
    # first read prefs folder prefs
    my($prefs_path) = GetPrefsFolder();
    $prefs_path .= ":$prefs_file_name";

    ReadPrefsFile($prefs_path, $build_flags, $options_flags, $filepath_flags, 1);
  }
}

1;
