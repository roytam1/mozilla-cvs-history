#!/perl

# make-jars [-f] [-v] [-l] [-d <chromeDir>] [-s <srcdir>] < <jar.mn>

use strict;

use Getopt::Std;
use Cwd;
use File::stat;
use Time::localtime;
use Cwd;
use File::Copy;
use File::Path;
use IO::File;
use mozLock;

my $objdir = getcwd;

getopts("d:s:f:vl");

my $baseFilesDir = ".";
if (defined($::opt_s)) {
    $baseFilesDir = $::opt_s;
}

my $chromeDir = ".";
if (defined($::opt_d)) {
    $chromeDir = $::opt_d;
}

my $verbose = 0;
if (defined($::opt_v)) {
    $verbose = 1;
}

my $fileformat = "jar";
if (defined($::opt_f)) {
    ($fileformat = $::opt_f) =~ tr/A-Z/a-z/;
}

if ("$fileformat" ne "jar" &&
    "$fileformat" ne "flat" &&
    "$fileformat" ne "both") {
    print "File format specified by -f option must be one of: jar, flat, or both.\n";
    exit(1);
}

my $zipmoveopt = "";
if ("$fileformat" eq "jar") {
    $zipmoveopt = "-m";
}

my $nofilelocks = 0;
if (defined($::opt_l)) {
    $nofilelocks = 1;
}

if ($verbose) {
    print "make-jars "
        . "-v -d $chromeDir "
        . ($fileformat ? "-f $fileformat " : "")
        . ($nofilelocks ? "-l " : "")
        . ($baseFilesDir ? "-s $baseFilesDir " : "")
        . "\n";
}

sub zipErrorCheck($$)
{
    my ($err,$lockfile) = @_;
    return if ($err == 0 || $err == 12);
    mozUnlock($lockfile) if (!$nofilelocks);
    die ("Error invoking zip: $err");
}

sub JarIt
{
    my ($destPath, $jarfile, $args, $overrides) = @_;
    my $oldDir = cwd();
    chdir("$destPath/$jarfile");

    if ("$fileformat" eq "flat") {
	unlink("../$jarfile.jar") if ( -e "../$jarfile.jar");
	chdir($oldDir);
        return 0;
    }

    #print "cd $destPath/$jarfile\n";

    my $lockfile = "../$jarfile.lck";

    mozLock($lockfile) if (!$nofilelocks);

    if (!($args eq "")) {
	my $cwd = getcwd;
	my $err = 0; 

        #print "zip $zipmoveopt -u ../$jarfile.jar $args\n";

	# Handle posix cmdline limits (4096)
	while (length($args) > 4000) {
	    #print "Exceeding POSIX cmdline limit: " . length($args) . "\n";
	    my $subargs = substr($args, 0, 3999);
	    my $pos = rindex($subargs, " ");
	    $subargs = substr($args, 0, $pos);
	    $args = substr($args, $pos);
	    
	    #print "zip $zipmoveopt -u ../$jarfile.jar $subargs\n";	    
	    #print "Length of subargs: " . length($subargs) . "\n";
	    system("zip $zipmoveopt -u ../$jarfile.jar $subargs") == 0 or
		$err = $? >> 8;
	    zipErrorCheck($err,$lockfile);
	}
	#print "Length of args: " . length($args) . "\n";
        #print "zip $zipmoveopt -u ../$jarfile.jar $args\n";
        system("zip $zipmoveopt -u ../$jarfile.jar $args") == 0 or
	    $err = $? >> 8;
	zipErrorCheck($err,$lockfile);
    }

    if (!($overrides eq "")) {
	my $err = 0; 
        print "+++ overriding $overrides\n";

	while (length($args) > 4000) {
	    #print "Exceeding POSIX cmdline limit: " . length($args) . "\n";
	    my $subargs = substr($args, 0, 3999);
	    my $pos = rindex($subargs, " ");
	    $subargs = substr($args, 0, $pos);
	    $args = substr($args, $pos);
	    
	    #print "zip $zipmoveopt ../$jarfile.jar $subargs\n";	    
	    #print "Length of subargs: " . length($subargs) . "\n";
	    system("zip $zipmoveopt ../$jarfile.jar $subargs") == 0 or
		$err = $? >> 8;
	    zipErrorCheck($err,$lockfile);
	}
        #print "zip $zipmoveopt ../$jarfile.jar $overrides\n";
        system("zip $zipmoveopt ../$jarfile.jar $overrides\n") == 0 or 
	    $err = $? >> 8;
	zipErrorCheck($err,$lockfile);
    }
    mozUnlock($lockfile) if (!$nofilelocks);
    chdir($oldDir);
    #print "cd $oldDir\n";
}

sub EnsureFileInDir
{
    my ($destPath, $srcPath, $destFile, $srcFile, $override) = @_;

    #print "EnsureFileInDir($destPath, $srcPath, $destFile, $srcFile, $override)\n";

    my $src = $srcFile;
    if (defined($src)) {
	if (! -e $src ) {
        	$src = "$srcPath/$srcFile";
	}
    }
    else {
        $src = "$srcPath/$destFile";
        # check for the complete jar path in the dest dir
        if (!-e $src) {
            #else check for just the file name in the dest dir
            my $dir = "";
            my $file;
            if ($destFile =~ /([\w\d.\-\_\\\/]+)[\\\/]([\w\d.\-\_]+)/) {
                $dir = $1;
                $file = $2;
            }
            else {
                die "file not found: $srcPath/$destFile";
            }
            $src = "$srcPath/$file";
            if (!-e $src) {
                die "file not found: $srcPath/$destFile";
            }
        }
    }

    $srcPath = $src;
    $destPath = "$destPath/$destFile";

    my $srcStat = stat($srcPath);
    my $srcMtime = $srcStat ? $srcStat->mtime : 0;

    my $destStat = stat($destPath);
    my $destMtime = $destStat ? $destStat->mtime : 0;
    #print "destMtime = $destMtime, srcMtime = $srcMtime\n";

    if (!-e $destPath || $destMtime < $srcMtime || $override) {
        #print "copying $destPath, from $srcPath\n";
        my $dir = "";
        my $file;
        if ($destPath =~ /([\w\d.\-\_\\\/]+)[\\\/]([\w\d.\-\_]+)/) {
            $dir = $1;
            $file = $2;
        }
        else {
            $file = $destPath;
        }

        if ($srcPath) {
            $file = $srcPath;
        }

        if (!-e $file) {
            die "error: file '$file' doesn't exist";
        }
        if (!-e $dir) {
            mkpath($dir, 0, 0775) || die "can't mkpath $dir: $!";
        }
        unlink $destPath;       # in case we had a symlink on unix
        copy($file, $destPath) || die "copy($file, $destPath) failed: $!";

        # fix the mod date so we don't jar everything (is this faster than just jarring everything?)
        my $atime = stat($file)->atime || die $!;
        my $mtime = stat($file)->mtime || die $!;
        utime($atime, $mtime, $destPath);

        return 1;
    }
    return 0;
}

while (<STDIN>) {
    chomp;
  start: 
    if (/^([\w\d.\-\_\\\/]+).jar\:\s*$/) {
        my $jarfile = $1;
        my $args = "";
        my $overrides = "";
	my $cwd = cwd();
	print "+++ making chrome $cwd  => $chromeDir/$jarfile.jar\n";
        while (<STDIN>) {
            if (/^\s+([\w\d.\-\_\\\/]+)\s*(\([\w\d.\-\_\\\/]+\))?$\s*/) {
                my $dest = $1;
                my $srcPath = defined($2) ? substr($2, 1, -1) : $2;
		EnsureFileInDir("$chromeDir/$jarfile", $baseFilesDir, $dest, $srcPath, 0);
                $args = "$args$dest ";
            } elsif (/^\+\s+([\w\d.\-\_\\\/]+)\s*(\([\w\d.\-\_\\\/]+\))?$\s*/) {
                my $dest = $1;
                my $srcPath = defined($2) ? substr($2, 1, -1) : $2;
                EnsureFileInDir("$chromeDir/$jarfile", $baseFilesDir, $dest, $srcPath, 1);
                $overrides = "$overrides$dest ";
            } elsif (/^\s*$/) {
                # end with blank line
                last;
            } else {
                JarIt($chromeDir, $jarfile, $args, $overrides);
                goto start;
            }
        }
        JarIt($chromeDir, $jarfile, $args, $overrides);

    } elsif (/^\s*\#.*$/) {
        # skip comments
    } elsif (/^\s*$/) {
        # skip blank lines
    } else {
        close;
        die "bad jar rule head at: $_";
    }
}
