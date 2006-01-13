#! /usr/bin/perl
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bonsai CVS tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s):


# You need to put this in your CVSROOT directory, and check it in.  (Change the
# first line above to point to a real live perl5.)  Add "dolog.pl" to
# CVSROOT/checkoutlist, and check it in. Then, add a line to your
# CVSROOT/loginfo file that says something like:
#
#      ALL      $CVSROOT/CVSROOT/dolog.pl [-u ${USER}] -r /cvsroot bonsai-checkin-daemon@my.bonsai.machine
#
# or if you do not want to use SMTP at all, add:
#
#      ALL      ( $CVSROOT/CVSROOT/dolog.pl -r /cvsroot -n | /bonsai/handleCheckinMail.pl )
#
# Replace "/cvsroot" with the name of the CVS root directory, and
# "my.bonsai.machine" with the name of the machine Bonsai runs on.
# Now, on my.bonsai.machine, add a mail alias so that mail sent to
# "bonsai-checkin-daemon" will get piped to handleCheckinMail.pl.

use bytes;
use Mail::Mailer;

$username = $ENV{"CVS_USER"} || getlogin || (getpwuid($<))[0] || "nobody";
$envcvsroot = $ENV{'CVSROOT'};
$cvsroot = $envcvsroot;
$flag_debug = 0;
$flag_tagcmd = 0;
$repository = '';
$repository_tag = '';
$mailhost = 'localhost';
$rlogcommand = '/usr/bin/rlog';
$output2mail = 1;

@mailto = ();
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

if ($flag_debug) {
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

if ($flag_debug) {
    print STDERR "----------------------------------------------\n";
    print STDERR @outlist;
    print STDERR "----------------------------------------------\n";
}

if ($output2mail) {
    &mail_notification;
} else {
    &stdout_notification;
}

0;

sub process_args {
    while (@ARGV) {
        $arg = shift @ARGV;

        if ($arg eq '-d') {
            $flag_debug = 1;
            print STDERR "Debug turned on...\n";
        } elsif ($arg eq '-r') {
            $cvsroot = shift @ARGV;
        } elsif ($arg eq '-u') {
            $username = shift @ARGV;
        } elsif ($arg eq '-t') {
            $flag_tagcmd = 1;
            last;              # Keep the rest in ARGV; they're handled later.
        } elsif ($arg eq '-h') {
            $mailhost = shift @ARGV;
        } elsif ($arg eq '-n') {
            $output2mail = 0;
        } else {
            push(@mailto, $arg);
        }
    }
    if ($repository eq '') {
        open(REP, "<CVS/Repository");
        $repository = <REP>;
        chop($repository);
        close(REP);
    }
    $repository =~ s:^$cvsroot/::;
    $repository =~ s:^$envcvsroot/::;

    if (!$flag_tagcmd) {
        if (open(REP, "<CVS/Tag")) {
            $repository_tag = <REP>;
            chop($repository_tag);
            close(REP);
        }
    }
}

sub get_loginfo {

    if ($flag_debug) {
        print STDERR "----------------------------------------------\n";
    }

    # Iterate over the body of the message collecting information.
    #
    while (<STDIN>) {
        chop;                  # Drop the newline

        if ($flag_debug) {
            print STDERR "$_\n";
        }

        if (/^In directory/) {
            next;
        }

        if (/^Modified Files/) { $state = $STATE_CHANGED; next; }
        if (/^Added Files/)    { $state = $STATE_ADDED;   next; }
        if (/^Removed Files/)  { $state = $STATE_REMOVED; next; }
        if (/^Log Message/)    { $state = $STATE_LOG;     next; }

        s/^[ \t\n]+//;         # delete leading whitespace
        s/[ \t\n]+$//;         # delete trailing whitespace

        if ($state == $STATE_CHANGED && !(/^Tag:/)) { push(@changed_files, split); }
        if ($state == $STATE_ADDED && !(/^Tag:/))   { push(@added_files,   split); }
        if ($state == $STATE_REMOVED && !(/^Tag:/)) { push(@removed_files, split); }
        if ($state == $STATE_LOG)     { push(@log_lines,     $_); }
    }

    # If any of the filenames in the arrays below contain spaces,
    # things get broken later on in the code.
    # fix the filename array by using the get_filename sub.
    @fixed_changed_files = @{&get_filename("C", @changed_files)};
    @fixed_added_files   = @{&get_filename("A", @added_files)};
    @fixed_removed_files = @{&get_filename("R", @removed_files)};

    # now replace the old broken arrays with the new fixed arrays and
    # carry on.

    @changed_files = @fixed_changed_files;
    @added_files   = @fixed_added_files;
    @removed_files = @fixed_removed_files;
    
    if ($flag_debug) {
        print STDERR "----------------------------------------------\n"
                     . "changed files: @changed_files\n"
                     . "added files: @added_files\n"
                     . "removed files: @removed_files\n";
        print STDERR "----------------------------------------------\n";
    }

}

sub get_filename {

    my ($state, @files) = @_;
    my @fixed_files;
    my $FILE_EXIST = 0;
    my $FILE_CHECKED = 0;
    my $file;
    my $partial_file;
    my $path;
    if ($flag_debug) {
        print STDERR "\n-- get_filename ------------------------\n";
    }
    foreach my $scalar (@files) {
        if ($FILE_CHECKED && ! $FILE_EXISTS) {
            $file = "$partial_file $scalar";
        } else{
            $file = $scalar;
        }
        if ($state eq "R") {
            $path = "$envcvsroot/$repository/Attic/$file";
        } else {
            $path = "$envcvsroot/$repository/$file";
        }
        if ($flag_debug) {
            print STDERR "changed file: $file\n";
            print STDERR "path: $path\n";
        }
        if (-r "$path,v") {
            push(@fixed_files, $file);
            $FILE_EXISTS = 1;
            $FILE_CHECKED = 1;
            if ($flag_debug){
                print STDERR "file exists\n";
            }
        } else {
            $partial_file = $file;
            $FILE_EXISTS = 0;
            $FILE_CHECKED = 1;
            if ($flag_debug) {
                print STDERR "file does not exist\n";
            }
        }
    }
    if ($flag_debug) {
        print STDERR "\@fixed_files: @fixed_files\n";
        print STDERR "-------------------------------------------\n\n";
    }
    return \@fixed_files;
}

sub process_cvs_info {
    local($d,$fn,$rev,$mod_time,$sticky,$tag,$stat,@d,$l,$rcsfile);
    if (!open(ENT, "<CVS/Entries.Log")) {
        open(ENT, "<CVS/Entries");
    }
    $time = time;
    while (<ENT>) {
        chop;
        ($d,$fn,$rev,$mod_time,$sticky,$tag) = split(/\//);
        $stat = 'C';
        for $i (@changed_files, "BEATME.NOW", @added_files) {
            if ($i eq "BEATME.NOW") { $stat = 'A'; }
            if ($i eq $fn) {
                $rcsfile = "$envcvsroot/$repository/$fn,v";
                if (! -r $rcsfile) {
                    $rcsfile = "$envcvsroot/$repository/Attic/$fn,v";
                }
                $rlogcmd = "$rlogcommand -N -r$rev " . shell_escape($rcsfile);
                open(LOG, "$rlogcmd |")
                        || print STDERR "dolog.pl: Couldn't run rlog\n";
                while (<LOG>) {
                    if (/^date:.* author: ([^;]*);.*/) {
                        $username = $1;
                        if (/lines: \+([0-9]*) -([0-9]*)/) {
                            $lines_added = $1;
                            $lines_removed = $2;
                        }
                    }
                }
                close(LOG);
                push(@outlist,
                     ("$stat|$time|$username|$cvsroot|$repository|$fn|$rev|$sticky|$tag|$lines_added|$lines_removed\n"));
            }
        }
    }
    close(ENT);

    for $i (@removed_files) {
        push(@outlist,
             ("R|$time|$username|$cvsroot|$repository|$i|||$repository_tag\n"));
    }

    # make sure dolog has something to parse when it sends its load off
    if (!scalar(@log_lines)) {
        push @log_lines, "EMPTY LOG MESSAGE"; 
    }

    push(@outlist, "LOGCOMMENT\n");
    push(@outlist, join("\n",@log_lines));
    push(@outlist, "\n:ENDLOGCOMMENT\n");
}


sub process_tag_command {
    local($str,$part,$time);
    $time = time;
    $str = "Tag|$cvsroot|$time";
    while (@ARGV) {
        $part = shift @ARGV;
        $str .= "|" . $part;
    }
    push(@outlist, ("$str\n"));
}



sub do_commitinfo {
}

sub mail_notification {
    chop(my $hostname = `hostname`);
    my $mailer = Mail::Mailer->new("smtp", Server => $mailhost) ||
        die("Failed to send mail notification\n");
    my %headers;

    $headers{'From'} = "bonsai-daemon\@$hostname";
    $headers{'To'} = \@mailto;
    if ($flag_tagcmd) {
        $headers{'Subject'} = "cvs tag in $repository";
    } else {
        $headers{'Subject'} = "cvs commit to $repository";
    }
    $mailer->open(\%headers);
    print $mailer @outlist;
    $mailer->close;
}

sub stdout_notification { 
    chop(my $hostname = `hostname`);

    print  "MAIL FROM: bonsai-daemon\@$hostname\n";
    print  "RCPT TO: root\@localhost\n";
    print  "DATA\n";
    if ($flag_tagcmd) {
        print  "Subject:  cvs tag in $repository\n";
    } else {
        print  "Subject:  cvs commit to $repository\n";
    }
    print  "\n";
    print  @outlist, "\n";
    print  ".\n";
}

# Quotify a string, suitable for invoking a shell process
sub shell_escape {
    my ($file) = @_;
    $file =~ s/([ \"\'\?\$\&\|\!<>\(\)\[\]\;\:])/\\$1/g;
    return $file;
}

