=head1 NAME

B<Moz> - routines for automating CodeWarrior builds, and some extra-curricular
activities related to building Mozilla

=head1 SYNOPSIS

    use Moz;

    OpenErrorLog(":::BuildLog");
    StopForErrors();

		$Moz::QUIET = 1;
		InstallFromManifest(":projects:MANIFEST", $dist_dir);

    BuildProjectClean(":projects:SomeProject.mcp", "SomeTarget");
    MakeAlias(":projects:SomeProject.shlb", $dist_dir);

    DontStopForErrors();

    BuildProject(":projects:SomeOtherProject.mcp", "SomeTarget");

=head1 DESCRIPTION

B<Moz> comprises the routines needed to slap CodeWarrior around, force it
to build a sequence of projects, report the results, and a few other things.
This module should only contain functions that are generic to any build,
not just the Mozilla build.

=cut


package			Moz::Moz;
require			Exporter;

use Cwd;

use File::Copy;
use File::Path;
use File::Basename;

use Mac::Types;
use Mac::Events;
use Mac::Processes;

use ExtUtils::Manifest 'maniread';

use Moz::CodeWarriorLib;

@ISA				= qw(Exporter);

@EXPORT			= qw( LaunchCodeWarrior
                  GetCodeWarriorRelativePath
                  current_directory
                  full_path_to
                  DoBuildProject
                  ImportXMLProject
                  ExportProjectToXML
                  OpenErrorLog
                  MakeAlias
                  GetFileModDate
                  StopForErrors
                  DontStopForErrors
                  InstallFromManifest
                  InstallResources
                  RedirectOutputToFile
                  Delay
                  ActivateApplication
                  IsProcessRunning);

@EXPORT_OK	= qw(CloseErrorLog QUIET);


sub current_directory()
	{
		my $current_directory = cwd();
		chop($current_directory) if ( $current_directory =~ m/:$/ );
		return $current_directory;
	}

sub full_path_to($)
	{
		my ($path) = @_;
		if ( $path =~ m/^[^:]+$/ )
			{
				$path = ":" . $path;
			}

		if ( $path =~ m/^:/ )
			{
				$path = current_directory() . $path;
			}

		return $path;
	}

$logging								= 0;
$recent_errors_file			= "";
$stop_on_1st_error			= 1;
$QUIET									= 0;



=head2 Logging all the errors and warnings - C<OpenErrorLog($log_file)>, C<CloseErrorLog()>

The warnings and errors generated in the course of building projects can be logged to a file.
Tinderbox uses this facility to show why a remote build failed.

Logging is off by default.
  Start logging at any point in your build process with C<OpenErrorLog($log_file)>.
  Stop with C<CloseErrorLog()>.
  You never need to close the log explicitly, unless you want to just log a couple of projects in the middle of a big list.
  C<CloseErrorLog()> is not exported by default. 

=cut

sub CloseErrorLog()
	{
		if ( $logging )
			{
				close(ERROR_LOG);
				$logging = 0;
				StopForErrors() if $stop_on_1st_error;
			}
	}



sub OpenErrorLog($)
	{
		my ($log_file) = @_;

		CloseErrorLog();
		if ( $log_file )
			{
				$log_file = full_path_to($log_file);
 
				open(ERROR_LOG, ">$log_file") || die "Error: Can't open $log_file\n";
				MacPerl::SetFileInfo("CWIE", "TEXT", $log_file);

				$log_file =~ m/.+:(.+)/;
				$recent_errors_file = full_path_to("$1.part");
				$logging = 1;
			}
	}


=head2 Stopping before it's too late - C<StopForErrors()>, C<DontStopForErrors()>

When building a long list of projects, you decide whether to continue building subsequent projects when one fails.
  By default, your build script will C<die> after the first project that generates an error while building.
  Change this behavior with C<DontStopForErrors()>.
  Re-enable it with C<StopForErrors()>.

=cut

sub StopForErrors()
	{
		$stop_on_1st_error = 1;
		
			# Can't stop for errors unless we notice them.
			# Can't notice them unless we are logging.
			# If the user didn't explicitly request logging, log to a temporary file.

		if ( ! $recent_errors_file )
			{
				OpenErrorLog("${TMPDIR}BuildResults");
			}
	}

sub DontStopForErrors()
	{
		$stop_on_1st_error = 0;		
	}

sub log_message($)
	{
		if ( $logging )
			{
				my ($message) = @_;
				print ERROR_LOG $message;
			}
	}

sub log_message_with_time($)
	{
		if ( $logging )
			{
				my ($message) = @_;
				my $time_stamp = localtime();
				log_message("$message ($time_stamp)\n");
			}
	}

sub log_recent_errors($)
	{
		my ($project_name) = @_;
		my $found_errors = 0;
	
		if ( $logging )
			{
				open(RECENT_ERRORS, "<$recent_errors_file");

				while( <RECENT_ERRORS> )
					{
						if ( /^Error/ || /^Couldn�t find project file/ || /^Link Error/ )
							{
#								if (!$found_errors)
#									print $_;
								$found_errors = 1;
							}
						print ERROR_LOG $_;
					}

				close(RECENT_ERRORS);
				unlink("$recent_errors_file");
			}
		
		if ( $stop_on_1st_error && $found_errors )
			{
				print ERROR_LOG "### Build failed.\n";
				die "### Errors encountered building \"$project_name\".\n";
			}
	}
	
sub DoBuildProject($$$)
	{
		my ($project_path, $target_name, $clean_build) = @_;
		$project_path = full_path_to($project_path);

#		$project_path =~ m/.+:(.+)/;
#		my $project_name = $1;

		log_message_with_time("### Building \"$project_path\"");

			# Check that the given project exists
		if (! -e $project_path)
			{
				print ERROR_LOG "### Build failed.\n";
				die "### Can't find project file \"$project_path\".\n";
			}
		
		print "Building \"$project_path\[$target_name\]\"\n";
		
		$had_errors = Moz::CodeWarriorLib::build_project(
			$project_path, $target_name, $recent_errors_file, $clean_build
		);
		WaitNextEvent();

#		$had_errors =
#MacPerl::DoAppleScript(<<END_OF_APPLESCRIPT);
#	tell (load script file "$CodeWarriorLib") to BuildProject("$project_path", "$project_name", "$target_name", "$recent_errors_file", $clean_build)
#END_OF_APPLESCRIPT

			# Append any errors to the globally accumulated log file
#		if ( $had_errors ) # Removed this test, because we want warnings, too.  -- jrm
			{
				log_recent_errors($project_path);
			}
	}


sub ImportXMLProject($$)
{
    my ($xml_path, $project_path) = @_;

#    my ($codewarrior_ide_name) = Moz::CodeWarriorLib::getCodeWarriorIDEName();
#    my $ascript = <<EOS;
#      tell application "$codewarrior_ide_name"
#        make new (project document) as ("$project_path") with data ("$xml_path")
#      end tell
#EOS
#    print $ascript."\n";
#    my($result) = MacPerl::DoAppleScript($ascript);
#    unless ($result) { die "Error: ImportXMLProject AppleScript failed $^E $result\n";  }
#    

    my($import_error) = Moz::CodeWarriorLib::import_project($xml_path, $project_path);
    if ($import_error ne "") {
      die "Error: ImportXMLProject failed with error $import_error\n";
    }
}

sub ExportProjectToXML($$)
{
    my ($project_path, $xml_path) = @_;

    my (@suffix_list) = (".mcp");
    my ($project_name, $project_dir, $suffix) = fileparse($project_path, @suffix_list);
    if ($suffix eq "") { die "Project: $project_path doesn't look like a project file.\n"; }
    
    if (-e $xml_path) {
        print "$xml_path exists - not exporting $project_path\n";
    }
    else {
        print "Exporting $project_path to $xml_path\n";
        my($export_error) = Moz::CodeWarriorLib::export_project($project_path, $xml_path);
        if ($export_error ne "") {
            die "Error: export_project failed with error '$export_error'\n";
        }
        
        if (! -e $xml_path) {
          die "Error: XML export to $xml_path failed\n";
        }
    }
}


=head2 Miscellaneous

C<MakeAlias($old_file, $new_file)> functions like C<symlink()>, except with better argument defaulting and more explicit error messages.

=cut

sub MakeAlias($$)
	{
		my ($old_file, $new_file) = @_;

			# if the directory to hold $new_file doesn't exist, create it
		if ( ($new_file =~ m/(.+:)/) && !-d $1 )
			{
				mkpath($1);
			}

			# if a leaf name wasn't specified for $new_file, use the leaf from $old_file
		if ( ($new_file =~ m/:$/) && ($old_file =~ m/.+:(.+)/) )
			{
				$new_file .= $1;
			}

		my $message = "Can't create a Finder alias (at \"$new_file\")\n for \"$old_file\"; because ";

		die "Error: $message \"$old_file\" doesn't exist.\n" unless -e $old_file;
		die "Error: $message I won't replace an existing (non-alias) file with an alias.\n" if ( -e $new_file && ! -l $new_file );

			# now: $old_file exists; $new_file doesn't (or else, is an alias already)

		if ( -l $new_file )
			{
					# ...then see if it already points to $old_file
				my $current_target	= full_path_to(readlink($new_file));
				my $new_target			= full_path_to($old_file);

				return if ( $current_target eq $new_target );
					# if the desired alias already exists and points to the right thing, then we're done
				
				unlink $new_file;
			}
		
		symlink($old_file, $new_file) || die "Error: $message symlink returned an unexpected error.\n";
	}
	
	
=pod

C<InstallFromManifest()>

=cut

sub InstallFromManifest($;$$)
	{
		my ($manifest_file, $dest_dir, $flat) = @_;
		
		$flat = 0 unless defined($flat); # if $flat, all rel. paths in MANIFEST get aliased to the root of $dest_dir

		$dest_dir ||= ":";

		$manifest_file =~ m/(.+):/;
		my $source_dir =  $1;

		chop($dest_dir) if $dest_dir =~ m/:$/;

		#Mac::Events->import();
		WaitNextEvent();
		if ($flat)
		{
			print "Doing manifest on \"$manifest_file\" FLAT\n" unless $QUIET;
		}
		else
		{
			print "Doing manifest on \"$manifest_file\"\n" unless $QUIET;
		}
		
		my $read = maniread(full_path_to($manifest_file));
		foreach $file (keys %$read)
			{
				next unless $file;

				$subdir = ":";
				if (!$flat && ($file =~ /:.+:/ ))
					{
						$subdir = $&;
					}

				$file = ":$file" unless $file =~ m/^:/;
				MakeAlias("$source_dir$file", "$dest_dir$subdir");
			}
	}


=pod

C<InstallResources()>

=cut

# parameters are path to MANIFEST file, destination dir, true (to make copies) or false (to make aliases)
sub InstallResources($;$;$)
	{
		my ($manifest_file, $dest_dir, $copy_files) = @_;

		$dest_dir ||= ":";
		mkpath($dest_dir) if !-d $dest_dir;

		$manifest_file =~ m/(.+):/;
		my $source_dir =  $1;

		chop($dest_dir) if $dest_dir =~ m/:$/;

		WaitNextEvent();
		print "Installing resources from \"$manifest_file\"\n" unless $QUIET;
		
		my $read = maniread(full_path_to($manifest_file));
		foreach $file (keys %$read)
			{
				next unless $file;
				
				if ($copy_files)
					{
						copy("$source_dir:$file", "$dest_dir:$file");
					}
				else
					{
						MakeAlias("$source_dir:$file", "$dest_dir:$file");
					}
			}
	}


#//--------------------------------------------------------------------------------------------------
#// Delay
#//--------------------------------------------------------------------------------------------------
sub Delay($)		
{
  my ($delay_seconds) = @_;

  $now = time;
  
  $exit_time = $now + $delay_seconds;

  while ($exit_time > $now) {
     $now = time;
  }
	
}

#//--------------------------------------------------------------------------------------------------
#// GetFileModDate
#//--------------------------------------------------------------------------------------------------
sub GetFileModDate($)
{
    my($filePath)=@_;
    my($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks) = stat($filePath);
    return $mtime;
}


#//--------------------------------------------------------------------------------------------------
#// LaunchCodeWarrior
#//--------------------------------------------------------------------------------------------------
sub LaunchCodeWarrior($)
{
  my($idepath_file) = @_;   # full path to IDE location file
  my($cur_dir) = cwd();
  
  # this both launches and writes the IDE path file
  Moz::CodeWarriorLib::activate($idepath_file);
  
  chdir($cur_dir);
}

#//--------------------------------------------------------------------------------------------------
#// GetCodeWarriorRelativePath
#//--------------------------------------------------------------------------------------------------
sub GetCodeWarriorRelativePath($)
{
  my($rel_path) = @_;
  return Moz::CodeWarriorLib::getCodeWarriorPath($rel_path);
}


#//--------------------------------------------------------------------------------------------------
#// RedirectOutputToFile
#//--------------------------------------------------------------------------------------------------
sub RedirectOutputToFile($)
{
    my($log_file) = @_;
    
    # ensure that folders in the path exist
    my($logdir) = "";
    my($logfile) = $log_file;
    
    if ($log_file =~ /(.+?:)([^:]+)$/)       # ? for non-greedy match
    {
        $logdir = $1;
        $logfile = $2;
        
        mkpath($logdir);
    }

    print "Output is now being redirected to the file '$log_file'\n";
    
    open(STDOUT, "> $log_file") || die "Can't redirect stdout";
    open(STDERR, ">&STDOUT") || die "Can't dup stdout";
    select(STDERR); $| = 1;     # make unbuffered
    select(STDOUT); $| = 1;     # make unbuffered
    
    MacPerl::SetFileInfo("CWIE", "TEXT", $log_file);
}

#//--------------------------------------------------------------------------------------------------
#// ActivateApplication
#//--------------------------------------------------------------------------------------------------

sub ActivateApplication($)
{
	my ($appSignature) = @_;
	my ($psi, $found);
	my ($appPSN);
	
	$found = 0;
		
	foreach $psi (values(%Process))
	{
		if ($psi->processSignature() eq $appSignature)
		{
			$appPSN = $psi->processNumber();
			$found = 1;
            last;
		}
	}

	if ($found == 0 || SameProcess($appPSN, GetFrontProcess()))
	{
		return;
	}

	SetFrontProcess($appPSN);

	while (GetFrontProcess() != $appPSN)
	{
		WaitNextEvent();
	}
}

#//--------------------------------------------------------------------------------------------------
#// IsProcessRunning
#//--------------------------------------------------------------------------------------------------

sub IsProcessRunning($)
{
    my($processName, $psn, $psi) = @_;
    while ( ($psn, $psi) = each(%Process) ) {
     if ($psi->processName eq $processName) { return 1; }
    }
    return 0;
}


1;

=head1 AUTHORS

Scott Collins <scc@netscape.com>, Simon Fraser <sfraser@netscape.com>, Chris Yeh <cyeh@netscape.com>

=head1 SEE ALSO

BuildMozillaDebug.pl (et al), BuildList.pm, CodeWarriorLib (an AppleScript library)

=head1 COPYRIGHT

The contents of this file are subject to the Netscape Public
License Version 1.1 (the "License"); you may not use this file
except in compliance with the License. You may obtain a copy of
the License at http://www.mozilla.org/NPL/

Software distributed under the License is distributed on an "AS
IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
implied. See the License for the specific language governing
rights and limitations under the License.

The Original Code is Mozilla Communicator client code, released
March 31, 1998.

The Initial Developer of the Original Code is Netscape
Communications Corporation. Portions created by Netscape are
Copyright (C) 1998-1999 Netscape Communications Corporation. All
Rights Reserved.

Contributor(s): 

=cut
