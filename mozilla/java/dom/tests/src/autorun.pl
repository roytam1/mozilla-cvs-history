# !/bin/perl
#
# Created By:     Raju Pallath
# Creation Date:  Aug 2nd 1999
#
# Ported by:	  Konstantin S. Ermakov
# Ported date:	  Aug 24th 1999
#
# This script is used to  invoke all test case for DOM API
# through apprunner and to recored their results
#
#

# Attach Perl Libraries
###################################################################
use Cwd;
use File::Copy;
use Win32::Process;

###################################################################


# time in seconds after which the apprunner has to be killed.
# by default the apprunner will be up for so much time regardless of
# whether over or not. User can either decrease it or increase it.
#
$DELAY_FACTOR = 25;
# time period in seconds of periodically checking: is the apprunner still alive
$DELAY_OF_CYCLE = 5;

$DOCROOT = $ENV{"TEST_URL"};

# delimiter for logfile
$delimiter="###################################################\n";

# win32 specific values 
# STILL_ALIVE is a constant defined in winbase.h and indicates what
# process still alive
$STILL_ALIVE = 0x103;

$path_separator = ";";



##################################################################
# Usage
##################################################################
sub usage() {

    print "\n";
    print "##################################################################\n";
    print "   perl autorun.pl [ -t <test case> ]\n";
    print "\n";
    print " where <test case> is one of the test cases prefixed with package\n";
    print " name ex: org.mozilla.dom.AttrImpl_getSpecified\n";
    print "\n";
    print "##################################################################\n";
    print "\n";
   
}

##################################################################
# Title display
##################################################################
sub title() {
	
    print "\n";
    print "################################################\n";
    print "   Automated Execution of DOM API TestSuite\n";
    print "################################################\n";
    print "\n";
    print "NOTE: You need to copy files test.html and test.xml into \n";
    print "      some document directory of HTTP server. TEST_URL environment \n";
    print "      variable should contain the URL of this directory.\n";
    print "\n";
    print "\n";

}

##################################################################
#
# check which tests to run. XML/HTML or both
#
##################################################################
sub checkRun() {
    $runtype = "0";
    while( true ) {
	print "Run 1) HTML Test suite.\n";
	print "    2) XML Test suite.\n";
	print "    3) BOTH HTML and XML.\n";
	print "Enter choice (1-3) :\n";
	$runtype = getc;
	  
	if(( $runtype ne "1" ) &&
	   ( $runtype ne "2" ) &&
	   ( $runtype ne "3" ) ) 
	{
	    print "Invaid choice. Range is from 1-3...\n";
	    print "\n";
	    next;
	} else {
	    last;
	}
    }
}


#########################################################################
#
# Append table entries to Output HTML File 
#
#########################################################################
sub appendEntries() {
   print LOGHTML "<tr><td></td><td></td></tr>\n";
   print LOGHTML "<tr><td></td><td></td></tr>\n";
   print LOGHTML "<tr bgcolor=\"#FF6666\"><td>Test Status (XML)</td><td>Result</td></tr>\n";

}

#########################################################################
#
# Construct Output HTML file Header
#
#########################################################################
sub constructHTMLHeader() {
   print LOGHTML "<html><head><title>\n";
   print LOGHTML "DOMAPI Core Level 1 Test Status\n";
   print LOGHTML "</title></head><body bgcolor=\"white\">\n";
   print LOGHTML "<center><h1>\n";
   print LOGHTML "DOM API Automated TestRun Results\n";
   $date = localtime;
   print LOGHTML "</h1><h2>", $date;
   print LOGHTML "</h2></center>\n";
   print LOGHTML "<hr noshade>";
   print LOGHTML "<table bgcolor=\"#99FFCC\">\n";
   print LOGHTML "<tr bgcolor=\"#FF6666\">\n";
   print LOGHTML "<td>Test Case</td>\n";
   print LOGHTML "<td>Result</td>\n";
   print LOGHTML "</tr>\n";
}

#########################################################################
#
# Construct Output HTML file indicating status of each run
#
#########################################################################
sub constructHTMLBody() {
	
    open( MYLOG, $LOGTXT );
    @all_lines = <MYLOG>;
    close( MYLOG );
    
    my $line;
    foreach $line ( sort @all_lines ) {
        # avoid linebreaks
	chop $line;  
	# assuming that all lines are kind'of 'aaa=bbb'
	($class, $status) = split /=/, $line;	
	
	print LOGHTML "<tr><td>",$class,"</td><td>",$status,"</td></tr>\n";
    } 
}
		
#########################################################################
#
# Construct Output HTML file Footer
#
#########################################################################
sub constructHTMLFooter() {
    print LOGHTML "</table></body></html>\n";
}

#########################################################################
#
# Construct Output HTML file indicating status of each run
#
#########################################################################
sub constructHTML() {
	constructHTMLHeader;
	constructHTMLBody;
}

#########################################################################
#
# Construct LogFile Header. The Log file is always appended with entries
#
#########################################################################
sub constructLogHeader() {
    print LOGFILE "\n";
    print LOGFILE "\n";    
    print LOGFILE $delimiter;
    $date = localtime;
    print LOGFILE "Logging Test Run on $date ...\n";
    print LOGFILE $delimiter;
    print LOGFILE "\n";    	

    print "All Log Entries are maintained in LogFile $LOGFILE\n";
    print "\n";
}

#########################################################################
#
# Construct LogFile Footer. 
#
#########################################################################
sub constructLogFooter() {
    print "\n";
    print LOGFILE $delimiter;
    $date = localtime;
    print LOGFILE "End of Logging Test $date ...\n";
    print LOGFILE $delimiter;
    print "\n"; 
}

########################################################################
#
# Construct Log String
#
########################################################################
sub constructLogString {
    my $logstring = shift(@_);
    print LOGFILE "$logstring\n";

}

########################################################################
#
# Safely append to file : open, append, close.
#
########################################################################
sub safeAppend {
    my $file = shift(@_);
    my $line = shift(@_);
    open (FILE, ">>$file") or die ("Cann't open $file");
    print FILE $line;
    close FILE;
}



##################################################################
# main
##################################################################
title;

$curdir = cwd();           

# Prepare file names
$LOGFILE = "$curdir/log/BWTestRun.log";
$LOGTXT = "$curdir/log/BWTest.txt";
$LOGHTML = "$curdir/log/BWTest.html";

# process command-line parameters
# and check for valid usage

$testparam = "";

if ( $#ARGV > 1 ) {
    usage;
    die;
}

if ( $#ARGV > 0 ) {
    if ( $ARGV[0] != "-t" ) {
	usage;
	die;
    } else {
	$testparam = $ARGV[1];
    }
}

if ( $testparam eq "" ) {
    print "WARNING: Your are going to execute the whole test suite....\n";
    checkRun;
}

$mozhome = @ENV{"MOZILLA_FIVE_HOME"};
if ( $mozhome eq "" ) {
    print "MOZILLA_FIVE_HOME is not set. Please set it and rerun this script....\n";
    die;
}

if ( ! -f "$mozhome/apprunner.exe" ) {
    print "Could not find 'apprunner.exe' in MOZILLA_FIVE_HOME.\n";
    print "Please check your setting...\n";
    die;
}

# Here must come a piece of code, that determinates 
# apprunner instance, removes core, but there's no
# core under win32

# Backup existing .lst file
if ( -f "$curdir/BWTestClass.lst" ) {
    rename "$curdir/BWTestClass.lst", "$curdir/BWTestClass.lst.bak";
}

# Check if ORIG list file is present
# this file contains all test cases
if ( ! -f "$curdir/BWTestClass.lst.ORIG" ) {
    print "\n";
    print "File BWTestClass.lst.ORIG not found ...\n";
    print "Check Mozilla Source base and bringover this file....\n";
    print "\n";
}

$id=$$;
# Check if BWProperties file exists
#
if ( -f "$mozhome/BWProperties" ) {
    $newfile="$mozhome/BWProperties.$id";
    rename "$mozhome/BWProperties", $newfile;
}
copy "$curdir/BWProperties", "$mozhome/BWProperties";

# check if output text file of previous run exists.
# if so, then save it as .bak
#
if ( -f "$LOGTXT" ) {
    $newfile="$LOGTXT.bak";
    rename $LOGTXT, $newfile;
}

# check if output html file of previous run exists.
# if so, then save it as .bak
#
if ( -f "$LOGHTML" ) {
    $newfile="$LOGHTML.bak";
    rename $LOGHTML, $newfile;
}

# construct DOCFILE
$DOCFILE = "$DOCROOT/test.html";
$runcnt = 1;
$filename = "$curdir/BWTestClass.lst.ORIG";

if ($runtype == 1) {
    $DOCFILE = "$DOCROOT/test.html";
    $filename = "$curdir/BWTestClass.lst.html.ORIG";
    $runcnt = 1;
}

if ($runtype == 2) {
    $DOCFILE = "$DOCROOT/test.xml";
    $filename = "$curdir/BWTestClass.lst.xml.ORIG";
    $runcnt = 1;
}

if ($runtype == 3) {
    $DOCFILE = "$DOCROOT/test.html";
    $filename = "$curdir/BWTestClass.lst.html.ORIG";
    $runcnt = 2;
}

# Prepare log streams
open( LOGFILE, ">>$LOGFILE" ) or die("Can't open LOG file...\n");
select LOGFILE; $| = 1; select STDOUT;
open( LOGHTML, ">$LOGHTML" ) or die("Can't open HTML file...\n");

print "Runtype is $runtype \n";
constructLogHeader();

$ENV{"CLASSPATH"} = "$curdir/../classes".$path_separator.$ENV{"CLASSPATH"};

$LST_OUT = "$curdir/BWTestClass.lst";


$currcnt = 0;
while (true) {
    open( file, $filename ) or die("Can't open $filename...\n");
    while( $line = <file> ) {
	chop $line;
	if ( $testparam ne "" ) {
	    $testcase = $testparam;
	} else {
	    $testcase = $line;
	}

	if ( $testcase =~ /^\s*#/ ) {
	    next;
	} 

	open(LST_OUT, ">$LST_OUT") or die ("Can't open LST_OUT file...\n");
	print LST_OUT $testcase;
	close(LST_OUT);

	chdir( $mozhome );
	print "========================================\n";
	$logstr="Running TestCase $testcase....";
	print "$logstr\n";
	constructLogString "$logstr";

	($nom) = ($testcase =~ /([^\.]*)$/);
	$testlog = "$curdir/log/$nom.$id.log";
	
	open( SAVEOUT, ">&STDOUT" );
	open( SAVEERR, ">&STDERR" );
	open( STDOUT, ">$testlog" ) or die "Can't redirect stdout";
	open( STDERR, ">&STDOUT" );
        Win32::Process::Create($ProcessObj,
			       "$mozhome/apprunner.exe",
			       "$mozhome/apprunner.exe $DOCFILE ",
			       1,
			       NORMAL_PRIORITY_CLASS,
			       "$mozhome" ) || die "cann't start apprunner";
	close( STDOUT );
	close( STDERR );
	open( STDOUT, ">&SAVEOUT" );
	open( STDERR, ">&SAVEERR" );
	   	
	$flag = 0;
	$cnt = 0;
	while (true) {
	    sleep($DELAY_OF_CYCLE);
	    
	    $ProcessObj->GetExitCode($exit_code);
	    if ( $exit_code != $STILL_ALIVE ) {
		
		$logstr = "Test terminated with exit code $exit_code.";
		constructLogString "$logstr";
		if ( $exit_code != 0 ) {
		    safeAppend $LOGTXT, "$testcase=FAILED\n";
		    $logstr = "Test FAILED...";
		    constructLogString "$logstr";
		}
		$logstr = "Check ErrorLog File $testlog ";
		constructLogString "$logstr";
		constructLogString "";
		constructLogString "";
		
		$flag = 1;
		last;
	    }
	    
	    $cnt += $DELAY_OF_CYCLE;
	    if ( $cnt >= $DELAY_FACTOR ) {
		$flag = 0;
		$ProcessObj->Kill(0);
		last;
	    }
	} # while with sleep
	
	( $testparam eq "" ) || last;
    
    } # while ( $line
    
    ( ++$currcnt < $runcnt ) || last;
    
    if ( $runtype == 3 ) {
	$DOCFILE="$DOCROOT/test.xml";
	$filename="$curdir/BWTestClass.lst.xml.ORIG";
	constructHTML;
	appendEntries;
	if ( -f "$LOGTXT" ) {
	    $newfile = "$LOGTXT.bak";
	    rename ($LOGTXT, $newfile) or die( "Cann't clear LOGTXT..." ) ;
	}
    }
    
} # while(true)

constructLogFooter;
if ( $runtype == 3 ) {
    constructHTMLBody;
} else {
    constructHTML;
}
constructHTMLFooter;

$newfile="$mozhome/BWProperties.$id";
rename "$newfile", "$mozhome/BWProperties";
chdir($curdir);
	
			









