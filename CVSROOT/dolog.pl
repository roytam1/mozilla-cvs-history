#! /tools/ns/bin/perl5

$username = getlogin || (getpwuid($<))[0] || "nobody";
$MAILER = "/usr/lib/sendmail -t";
$envcvsroot = $ENV{'CVSROOT'};
$cvsroot = $envcvsroot;
$flag_debug = 0;
$flag_tagcmd = 0;
$repository = '';
$repository_tag = '';

@mailto=();
@changed_files = ();
@added_files = ();
@removed_files = ();
@log_lines = ();
@outlist = ();

$STATE_NONE    = 0;
$STATE_CHANGED = 1;
$STATE_ADDED   = 2;
$STATE_REMOVED = 3;
$STATE_LOG     = 4;

&process_args;

if ($flag_debug ){
    print STDERR "----------------------------------------------\n";
    print STDERR "LOGINFO:\n";
    print STDERR " pwd:" . `pwd` . "\n";
    print STDERR " Args @ARGV\n";
    print STDERR " CVSROOT: $cvsroot\n";                      
    print STDERR " who: $username\n";                      
    print STDERR " Repository: $repository\n";                      
    print STDERR " mailto: @mailto\n";
    print STDERR "----------------------------------------------\n";
}

if ($flag_tagcmd) {
    &process_tag_command;
} else {
    &get_loginfo;
    &process_cvs_info;
}

if( $flag_debug){
    print STDERR "----------------------------------------------\n";
    print STDERR @outlist;
    print STDERR "----------------------------------------------\n";
}

&mail_notification;

0;

sub process_args {
    while (@ARGV) {
        $arg = shift @ARGV;

        if ($arg eq '-d') {
            $flag_debug = 1;
            print STDERR "Debug turned on...\n";
	} elsif ($arg eq '-r') {
	    $cvsroot = shift @ARGV;
        } elsif ($arg eq '-t') {
	    $flag_tagcmd = 1;
	    last;		# Keep the rest in ARGV; they're handled later.
	} else {
            push(@mailto, $arg);
        }
    }
    if( $repository eq '' ){
	open( REP, "<CVS/Repository");
	$repository = <REP>;
	chop($repository);
	close(REP);
    }
    $repository =~ s:^$cvsroot/::;
    $repository =~ s:^$envcvsroot/::;
    
    if (!$flag_tagcmd) {
	if( open( REP, "<CVS/Tag") ) {
	    $repository_tag = <REP>;
	    chop($repository_tag);
	    close(REP);
	}
    }
}

sub get_loginfo {

    if( $flag_debug){
        print STDERR "----------------------------------------------\n";
    }

    # Iterate over the body of the message collecting information.
    #
    while (<STDIN>) {
        chop;			# Drop the newline

        if( $flag_debug){
            print STDERR "$_\n";
        }

        if (/^In directory/) {
            next;
        }

        if (/^Modified Files/) { $state = $STATE_CHANGED; next; }
        if (/^Added Files/)    { $state = $STATE_ADDED;   next; }
        if (/^Removed Files/)  { $state = $STATE_REMOVED; next; }
        if (/^Log Message/)    { $state = $STATE_LOG;     next; }

        s/^[ \t\n]+//;		# delete leading whitespace
        s/[ \t\n]+$//;		# delete trailing whitespace
        
        if ($state == $STATE_CHANGED) { push(@changed_files, split); }
        if ($state == $STATE_ADDED)   { push(@added_files,   split); }
        if ($state == $STATE_REMOVED) { push(@removed_files, split); }
        if ($state == $STATE_LOG)     { push(@log_lines,     $_); }
    }
    
    if( $flag_debug){
        print STDERR "----------------------------------------------\n"
                     . "changed files: @changed_files\n"
                     . "added files: @added_files\n"
                     . "removed files: @removed_files\n";
        print STDERR "----------------------------------------------\n";
    }

}

sub process_cvs_info {
    local($d,$fn,$rev,$mod_time,$sticky,$tag,$stat,@d,$l,$rcsfile);
    open(ENT, "<CVS/Entries" );
    $time = time;
    while( <ENT> ){
        chop;
        ($d,$fn,$rev,$mod_time,$sticky,$tag) = split(/\//);
        $stat = 'C';
        for $i (@changed_files, "BEATME.NOW", @added_files ) {
            if( $i eq "BEATME.NOW" ){ $stat = 'A'; }
            if($i eq $fn ){
                $rcsfile = "$envcvsroot/$repository/$fn,v";
                if( ! -r $rcsfile ){
                    $rcsfile = "$envcvsroot/$repository/Attic/$fn,v";
                }
                open(LOG, "/tools/ns/bin/rlog -N -r$rev $rcsfile |") 
                        || print STDERR "dolog.pl: Couldn't run rlog\n";
                while(<LOG>){
                    if( /^revision /){
                        $l = <LOG>;
                        chop($l);
                        if( $flag_debug ){ print STDERR "$l\n";}
                        @d = split(/[ ]+/,$l);
                        $lines_added = $d[8];
                        $lines_removed = $d[9];
                    }
                }
                close( LOG );
                push(@outlist, ("$stat|$time|$username|$cvsroot|$repository|$fn|$rev|$sticky|$tag|$lines_added|$lines_removed\n"));
            }
        }
    }
    close(ENT);

    for $i (@removed_files) {
        push( @outlist, ("R|$time|$username|$cvsroot|$repository|$i|||$repository_tag\n"));
    }

    push (@outlist, "LOGCOMMENT\n");
    push (@outlist, join("\n",@log_lines));
    push (@outlist, "\n:ENDLOGCOMMENT\n");
}


sub process_tag_command {
    local($str,$part,$time);
    $time = time;
    $str = "Tag|$cvsroot|$time";
    while (@ARGV) {
	$part = shift @ARGV;
	$str .= "|" . $part;
    }
    push (@outlist, ("$str\n"));
}
	


sub do_commitinfo {
}



sub mail_notification {

    open(MAIL, "| /bin/mail @mailto");
    if ($flag_tagcmd) {
	print MAIL "Subject:  cvs tag in $repository\n";
    } else {
	print MAIL "Subject:  cvs commit to $repository\n";
    }
    print MAIL "\n";
    print MAIL @outlist, "\n";
    close(MAIL);
}
