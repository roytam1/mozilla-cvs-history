package MozBuild::Util;

use strict;

use File::Path;
use IO::Handle;
use IO::Select;
use IPC::Open3;
use POSIX qw(:sys_wait_h);
use Cwd;

use base qw(Exporter);

our @EXPORT_OK = qw(RunShellCommand MkdirWithPath HashFile);

my $DEFAULT_EXEC_TIMEOUT = 600;
my $EXEC_IO_READINCR = 1000;

# RunShellCommand is a safe, performant way that handles all the gotchas of
# spawning a simple shell command. It's meant to replace backticks and open()s,
# while providing flexibility to handle stdout and stderr differently, provide
# timeouts to stop runaway programs, and provide easy-to-obtain information
# about return values, signals, output, and the like.
#
# Arguments:
#    command - string: the command to run
#    args - optional array ref: list of arguments to an array
#    logfile - optional string: logfile to copy output to
#    timeout - optional int: timeout value, in seconds, to wait for a command;
#     defaults to ten minutes; set to 0 for no timeout
#    redirectStderr - bool, default true: redirect child's stderr onto stdout
#     stream?
#    appendLogfile - bool, default true: append to the logfile (as opposed to
#     overwriting it?)
#    printOutputImmedaitely - bool, default false: print the output here to
#     whatever is currently defined as *STDOUT?
#    background - bool, default false: spawn a command and return all the info
#     the caller needs to handle the program; assumes the caller takes
#     complete responsibility for waitpid(), handling of the stdout/stderr
#     IO::Handles, etc.
#

sub RunShellCommand {
    my %args = @_;

    my $shellCommand = $args{'command'};
    die 'ASSERT: RunShellCommand(): Empty command.' 
     if (not(defined($shellCommand)) || $shellCommand =~ /^\s+$/);
    my $commandArgs = $args{'args'};
    die 'ASSERT: RunShellCommand(): commandArgs not an array ref.' 
     if (defined($commandArgs) && ref($commandArgs) ne 'ARRAY');

    my $logfile = $args{'logfile'};

    # Optional
    my $timeout = exists($args{'timeout'}) ?
     int($args{'timeout'}) : $DEFAULT_EXEC_TIMEOUT;
    my $redirectStderr = exists($args{'redirectStderr'}) ?
     $args{'redirectStderr'} : 1;
    my $appendLogfile = exists($args{'appendLog'}) ? $args{'appendLog'} : 1;
    my $printOutputImmediately = exists($args{'output'}) ? $args{'output'} : 0;
    my $background = exists($args{'bg'}) ? $args{'bg'} : 0;
    my $changeToDir = exists($args{'dir'}) ?  $args{'dir'} : undef;

    # This is a compatibility check for the old calling convention;
    if ($shellCommand =~ /\s/) {
        $shellCommand =~ s/^\s+//;
        $shellCommand =~ s/\s+$//;

        die "ASSERT: old RunShellCommand() calling convention detected\n" 
         if ($shellCommand =~ /\s+/);
    }

    # Glob the command together to check for 2>&1 constructs...
    my $entireCommand = $shellCommand . 
     (defined($commandArgs) ? join (' ', @{$commandArgs}) : '');
   
    # If we see 2>&1 in the command, set redirectStderr (overriding the option
    # itself, and remove the construct from the command and arguments.
    if ($entireCommand =~ /2\>\&1/) {
        $redirectStderr = 1;
        $shellCommand =~ s/2\>\&1//g;
        if (defined($commandArgs)) {
            for (my $i = 0; $i < scalar(@{$commandArgs}); $i++) {
                $commandArgs->[$i] =~ s/2\>\&1//g;
            }
        }
    }

    local $_;
 
    chomp($shellCommand);

    my $cwd = getcwd();
    my $exitValue = undef;
    my $signalNum = undef;
    my $sigName = undef;
    my $dumpedCore = undef;
    my $childEndedTime = undef;
    my $timedOut = 0;
    my $output = '';
    my $childPid = 0;
    my $childStartedTime = 0;

    if (defined($changeToDir)) {
        chdir($changeToDir) or die "RunShellCommand(): failed to chdir() to "
         . "$changeToDir\n";
    }

    eval {
        local $SIG{'ALRM'} = sub { die "alarm\n" };
        local $SIG{'PIPE'} = sub { die "pipe\n" };
  
        my @execCommand = ($shellCommand);
        push(@execCommand, @{$commandArgs}) if (defined($commandArgs) && 
         scalar(@{$commandArgs} > 0));
    
        my $childIn = new IO::Handle();
        my $childOut = new IO::Handle();
        my $childErr = new IO::Handle();

        alarm($timeout);
        $childStartedTime = localtime();

        $childPid = open3($childIn, $childOut, $childErr, @execCommand);
        $childIn->close();

        if ($args{'background'}) {
            alarm(0);

            chdir($cwd) if (defined($changeToDir));
            return { startTime => $childStartedTime,
                     endTime => undef,
                     timedOut => $timedOut,
                     exitValue => $exitValue,
                     sigName => $signalNum,
                     output => undef,
                     dumpedCore => $dumpedCore,
                     pid => $childPid,
                     stdout => $childOut,
                     stderr => $childErr };
        }

        if (defined($logfile)) {
            my $openArg = $appendLogfile ? '>>' : '>';
            open(LOGFILE, $openArg . $logfile) or 
             die 'Could not ' . $appendLogfile ? 'append' : 'open' . 
              " logfile $logfile: $!";
            LOGFILE->autoflush(1);
        }

        my $childSelect = new IO::Select();
        $childSelect->add($childErr);
        $childSelect->add($childOut);
    
        # Should be safe to call can_read() in blocking mode, since,
        # IF NOTHING ELSE, the alarm() we set will catch a program that
        # fails to finish executing within the timeout period.

        while (my @ready = $childSelect->can_read()) {
            foreach my $fh (@ready) {
                my $line = undef;
                my $rv = $fh->sysread($line, $EXEC_IO_READINCR);

                # Check for read()ing nothing, and getting errors...
                next if ($rv == 0);
                if (not defined($rv)) {
                    warn "sysread() failed with: $!\n";
                    next;
                }

                # This check is down here instead of up above because if we're
                # throwing away stderr, we want to empty out the buffer, so
                # the pipe isn't constantly readable. So, sysread() stderr,
                # alas, only to throw it away.
                next if (not($redirectStderr) && ($fh == $childErr));

                $output .= $line;
                print STDOUT $line if ($printOutputImmediately);
                print LOGFILE $line if (defined($logfile));
            }

            if (waitpid($childPid, WNOHANG) > 0) {
                alarm(0);
                $childEndedTime = localtime();
                $exitValue = WEXITSTATUS($?);
                $signalNum = WIFSIGNALED($?) && WTERMSIG($?);
                $dumpedCore = WIFSIGNALED($?) && WCOREDUMP($?);
                $childSelect->remove($childErr);
                $childSelect->remove($childOut);

            }
        }

        die 'ASSERT: RunShellCommand(): stdout handle not empty'
         if ($childOut->sysread(undef, $EXEC_IO_READINCR) != 0);
        die 'ASSERT: RunShellCommand(): stderr handle not empty;'
         if ($childErr->sysread(undef, $EXEC_IO_READINCR) != 0);
    };

    if (defined($logfile)) {
        close(LOGFILE) or die "Could not close logfile $logfile: $!";
    }

    if ($@) {
        if ($@ eq "alarm\n") {
            $timedOut = 1;
            kill(9, $childPid) or die "Could not kill timed-out $childPid: $!";
            warn "Shell command $shellCommand timed out, PID $childPid killed: $@\n";
        } else {
            warn "Error running $shellCommand: $@\n";
            $output = $@;
        }
    }

    chdir($cwd) if (defined($changeToDir));

    return { startTime => $childStartedTime,
             endTime => $childEndedTime,
             timedOut => $timedOut,
             exitValue => $exitValue,
             sigName => $sigName,
             output => $output,
             dumpedCore => $dumpedCore
           };
}

## This is a wrapper function to get easy true/false return values from a
## mkpath()-like function. mkpath() *actually* returns the list of directories
## it created in the pursuit of your request, and keeps its actual success 
## status in $@. 

sub MkdirWithPath {
    my %args = @_;

    my $dirToCreate = $args{'dir'};
    my $printProgress = $args{'printProgress'};
    my $dirMask = $args{'dirMask'};

    die "ASSERT: MkdirWithPath() needs an arg" if not defined($dirToCreate);

    ## Defaults based on what mkpath does...
    $printProgress = defined($printProgress) ? $printProgress : 0;
    $dirMask = defined($dirMask) ? $dirMask : 0777;

    eval { mkpath($dirToCreate, $printProgress, $dirMask) };
    return defined($@);
}

sub HashFile {
   my %args = @_;
   die "ASSERT: null file to hash\n" if (not defined($args{'file'}));

   my $fileToHash = $args{'file'};
   my $hashFunction = $args{'type'} || 'md5';
   my $dumpOutput = $args{'output'} || 0;
   my $ignoreErrors = $args{'ignoreErrors'} || 0;

   die "ASSERT: unknown hashFunction; use 'md5' or 'sha1': $hashFunction\n" if 
    ($hashFunction ne 'md5' && $hashFunction ne 'sha1');

   if (not(-f $fileToHash) || not(-r $fileToHash)) {
      if ($ignoreErrors) {
         return '';
      } else {
         die "ASSERT: unusable/unreadable file to hash\n"; 
      }
   }

   # We use openssl because that's pretty much guaranteed to be on all the
   # platforms we want; md5sum and sha1sum aren't.
   my $rv = RunShellCommand(command => "openssl dgst -$hashFunction " .
                                       "\"$fileToHash\"",
                            output => $dumpOutput);
   
   if ($rv->{'exitValue'} != 0) {
      if ($ignoreErrors) {
         return '';      
      } else {
         die "MozUtil::HashFile(): hash call failed: $rv->{'exitValue'}\n";
      }
   }

   my $hashValue = $rv->{'output'};
   chomp($hashValue);

   # Expects input like MD5(mozconfig)= d7433cc4204b4f3c65d836fe483fa575
   # Removes everything up to and including the "= "
   $hashValue =~ s/^.+\s+(\w+)$/$1/;
   return $hashValue;
}

1;
