#!/usr/bonsaitools/bin/perl5 -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Harrison Page <harrison@netscape.com>
#                 Terry Weissman <terry@mozilla.org>
#                 Risto Kotalampi <risto@kotalampi.com>
#
# mozbot.pl harrison@netscape.com 10/14/98
# "irc bot for the gang on #mozilla"
#
# features: reports tinderbox status upon request.
# remembers urls. tells you the phase of the moon.
# grabs mozillaZine headlines. fetches slashdot.org
# news. bot will auto-op based on nick and remote host.
#
# hack on me! required reading:
#
# Net::IRC web page: 
#   http://netirc.betterbox.net/
#   (free software)
#   or get it from CPAN @ http://www.perl.com/CPAN
#
# RFC 1459 (Internet Relay Chat Protocol):
#   http://sunsite.cnlab-switch.ch/ftp/doc/standard/rfc/14xx/1459


$SIG{'INT'}  = 'killed';
$SIG{'KILL'} = 'killed';
$SIG{'TERM'} = 'killed';

use strict;
use diagnostics;
use lib ".";
use Net::IRC;
use LWP::Simple;
use Tinderbox;
use Carp;
use Chatbot::Eliza;
use babel;
use IPC::Open2;
use FileHandle;
use Net::FTP;

$|++;

my $VERSION = "1.63"; # keep me in sync with the mozilla.org cvs repository
my $debug = 1; # debug output also includes warnings, errors

my %msgcmds = (
               "url" => \&bot_urls,
               "(stocks|stock)" => \&bot_stocks,
               );

my %pubcmds = (
               "(help|about)" => \&bot_about,
               "(hi|hello|lo|sup)" => \&bot_hi,
               "moon" => \&bot_moon,
               "uuid" => \&bot_uuid,
               "up" => \&bot_up,
               "(trees|tree)" => \&bot_tinderbox,
               "debug" => \&bot_debug,
               "(stocks|stock)" => \&bot_pub_stocks,
               "(translate|xlate|x)" => \&bot_translate,
               "review" => \&bot_review,
               "ftp" => \&ftp_stamp
               );

my %admincmds = (
                 "bless" => \&bot_bless,
                 "unbless" => \&bot_unbless,
                 "shutdown" => \&bot_shutdown,
                 "say" => \&bot_say,
                 "list" => \&bot_list,
                 "ftpx" => \&ftp_check,
                 "ftpc" => \&ftp_clear
                 );

my %rdfcmds = (
               "(slashdot|sd|\/\.)" => "http://www.slashdot.org/slashdot.rdf",
               "(mozillaorg|mozilla|mo)" => "http://www.mozilla.org/news.rdf",
               "(newsbot|nb)" => "http://www.mozilla.org/newsbot/newsbot.rdf",
               "(xptoolkit|xpfe)" => "http://www.mozilla.org/xpfe/toolkit.rdf",
               "(freshmeat|fm)" => "http://freshmeat.net/backend/fm.rdf",
               "(mozillazine|zine|mz)" => "http://www.mozillazine.org/contents.rdf",
               );

my %rdf_title;
my %rdf_link;
my %rdf_last;
my %rdf_items;
my %latestbuilds;
my $ftpscanned = 0;

@::origargv = @ARGV;

my $server = shift;
my $port = shift;
my $nick = shift;
my $channel = shift;

my $stockf = "stocklist";
my %stocklist = ();
my %stockvals = ();
my %stockhist;

$server = $server               || "irc.mozilla.org";
$port = $port                   || "6667";
$nick = $nick                   || "mozbot";
$channel = $channel             || "#mozilla";

&debug ("mozbot $VERSION starting up");

LoadStockList();

&create_pid_file;

# read admin list 
my %admins = ( "rko" => "netscape.com", "cyeh" => "netscape.com", "terry" => "netscape.com" );
my $adminf = ".$nick-admins";
&fetch_admin_conf (\%admins);

my $uptime = 0;

$::moon = "./moon";
$::moon = (-f $::moon) ? $::moon : ""; 
delete $pubcmds{'moon'} if (! $::moon);

$::uuid = "./uuidgen/uuidgen";
$::uuid = (-f $::uuid) ? $::uuid : "";
delete $pubcmds{'uuid'} if (! $::uuid);

my $phase;
my $last_moon = 0;
my $last_uuid = 0;

# leave @trees empty if you don't want tinderbox details

my @all_trees = qw (SeaMonkey SeaMonkey-Ports);
my @trees = qw (SeaMonkey);
if ($nick =~ /grend/) {
    @trees = qw (Grendel);
}
my $all_trees;
my $trees;
my $status;
my $last_tree;
my %broken;
my @urls;
my $ftpsite="ftp.mozilla.org";

my $greet = 0;
my @greetings = 
	( 
	"g'day", "bonjour", "guten tag", "konnichiwa",
	"hello", "hola", "hi", "buono giorno", "aloha",
	"hey", "'sup", "lo", "howdy", "saluton", "hei",
	"hallo", "word", "yo yo yo", "rheeet", "bom dia",
	"ciao"
	);


my $irc = new Net::IRC or confess "$0: duh?";

my $bot = $irc->newconn
  (
  Server => $server,
  Port => $port,
  Nick => $nick,
  Ircname => "mozilla.org bot/thing $VERSION",
  Username => $nick,
  )
or die "$0: can't connect to $server, port $port";

&debug ("adding global handlers");
$bot->add_global_handler ([ 251,252,253,254,302,255 ], \&on_startup);
$bot->add_global_handler (376, \&on_connect);
$bot->add_global_handler (433, \&on_nick_taken);
$bot->add_global_handler ([ 'disconnect', 'kill', 474, 465 ], \&on_boot);

&debug ("adding more handlers");
$bot->add_handler ('msg', \&on_msg);
$bot->add_handler ('public', \&on_public);
$bot->add_handler ('join',   \&on_join);

&debug ("scheduling stuff");
$bot->schedule (0, \&tinderbox);
$bot->schedule (0, \&checksourcechange);
$bot->schedule (0, \&stocks);
$bot->schedule (0, \&ftp_scan);

foreach my $i (keys %rdfcmds) {
    $bot->schedule(0, \&rdfchannel, $rdfcmds{$i});
    $pubcmds{$i} = $rdfcmds{$i};
}

&debug ("connecting to $server $port as $nick on $channel");

$::megahal = "./megahal/megahal";
$::megahal = (-f $::megahal) ? $::megahal : "";
#david: why does the following line need to be commented out?
#$::megahal_pid;

if ($::megahal) {
   $::WTR = FileHandle->new;
   $::RDR = FileHandle->new;
   $::megahal_pid = &init_megahal;
   &debug ("Initializing MEGAHAL conversation AI\n");
}

# and done.


# Use this routine, always, instead of the standard "privmsg" routine.  This
# one makes sure we don't send more than one message every two seconds or so,
# which will make servers not whine about us flooding the channel.
#
# Actually, it seems that we can send two in a row just fine, but at that
# point we should start throttling.

my $lastsenttime = 0;
my @msgqueue = ();
sub sendmsg {
    my ($who, $msg) = (@_);
    my $now = time();
    if ($now > $lastsenttime && 0 == @msgqueue) {
        $bot->privmsg($who, $msg);
        $lastsenttime = $now;
    } else {
        push(@msgqueue, [$who, $msg]);
        if (1 == @msgqueue) {
            $bot->schedule(0, \&drainmsgqueue);
        }
    }
}

sub drainmsgqueue {
    if (0 < @msgqueue) {
        my ($who, $msg) = (@{shift(@msgqueue)});
        $bot->privmsg($who, $msg);
        $lastsenttime = time();
        if (0 < @msgqueue) {
            $bot->schedule(2, \&drainmsgqueue);
        }
    }
}
        




################################
# Net::IRC handler subroutines #
################################

sub on_startup
  {
  my ($self, $event) = @_;
  my (@args) = ($event->args);
  shift (@args);

  &debug ("@args\n");
  }

sub on_connect
  {
  my $self = shift;
    &debug ("startup took " . (time - $^T) . " seconds");

    $self->join ($channel);
    
    $uptime = time;
    }

# on_nick_taken: or do something smarter

sub on_nick_taken
    {
    die "hey! somebody took my nick!";
    }


sub do_command {
    my ($hashref, $nick, $cmd, $rest) = (@_);
    foreach my $m (keys %$hashref) {
        if ($cmd =~ m/^$m$/) {
            my $ref = $hashref->{$m};
            if (ref($ref)) {
                &{$ref} ($nick, $cmd, $rest);
            } else {
                bot_rdfchannel($nick, $cmd, $rest, $ref);
            }
            return 1;
        }
    }
    return 0;
}



# on_msg: private message received via /msg

sub on_msg {
    my ($self, $event) = @_;
    my ($nick) = $event->nick;
    my ($arg) = $event->args;
    my @arglist = split(' ', $arg);
    my $cmd = shift @arglist;
    my $rest = join(' ', @arglist);
    $::speaker = $nick;         # Hack!!!


    if (exists $admins{$nick}) {
        if (do_command(\%admincmds, $nick, $cmd, $rest)) {
            return;
        }
    }
    if (do_command(\%msgcmds, $nick, $cmd, $rest)) {
        return;
    }
    if (do_command(\%pubcmds, $nick, $cmd, $rest)) {
        return;
    }
    do_unknown($nick, $cmd, $rest);
}



sub bot_bless {
    my ($nick, $cmd, $rest) = (@_);
    my ($who, $where) = split(' ', $rest);
    if (! $who or ! $where) {
        sendmsg($nick, "usage: bless [ user ] [ host ] " . 
                "(example: bless marca netscape.com)");
        return;
    }
    $admins{$who} = $where;
    &debug ("$nick blessed $who ($where)");
    &store_admin_conf (\%admins);
    sendmsg($nick, "mozbot admins: " . join ' ', (sort keys %admins));
}

sub ftp_clear {
 %latestbuilds=();
}

sub ftp_stamp {
    my $nick=$_[0];
    my $msg = "Available from $ftpsite: " .
        join(' ', sort(keys(%latestbuilds)));
    saylongline($nick, $msg, " ... ");
}

sub ftp_check {
    my $nick=($_[0])?$_[0]:$channel;
    my $buf="";
    my $mdtms;
    my $ftpserver=($_[2])?$_[2]:$ftpsite;
    my $ftp = new Net::FTP($ftpserver, Debug => 0);
    &debug ("fetching FTP $ftpserver nightly/latest");
    if ($ftp->login("anonymous","mozbot\@localhost")){
        $ftp->cwd("/pub/mozilla/nightly/latest");
        for my $f ($ftp->ls){
            $mdtms=$ftp->mdtm($f);
            if (!$latestbuilds{$f} || $mdtms>$latestbuilds{$f}) {
                $buf.=$f."; "; 
                $latestbuilds{$f}=$mdtms;
            } 
        }
        $ftp->quit;
    };
    $buf="New files @ ftp://$ftpserver/pub/mozilla/nightly/latest ".$buf if ($buf);
    &debug("$nick $buf");
    if ($ftpscanned) {
        sendmsg($nick, $buf);
    }
    $ftpscanned = 1;
}
#DBM or DB_File %hash that's tied to a file
#DBM and DB_File are documented on the
# http://www.perl.com/CPAN-local/doc/manual/html/lib/ page.
# (note there are several DBM links there so be sure to check each
# (= exhaustive look not brief look))
# timeless believes ftp info doesn't need to persist across multiple sessions
# of mozbot 
# Lots of help from

sub ftp_scan{
 ftp_check(0);
 $bot->schedule(300,\&ftp_scan);
}

sub bot_unbless {
    my ($nick, $cmd, $rest) = (@_);
    my ($who) = ($rest);
    if (exists ($admins{$who})) {
        delete $admins{$who};
        &debug ("$nick unblessed $who");
        &store_admin_conf (\%admins);
        sendmsg($nick, "mozbot admins: " . join ' ', (sort keys %admins));
        return;
    }
    sendmsg($nick, "Can only unbless one of: " .
            join(' ', (sort keys %admins)));
}

sub bot_shutdown {
    my ($nick, $cmd, $rest) = (@_);
    if ($rest ne "yes") {
        sendmsg($nick, "usage: shutdown yes");
        return;
    }
    &debug ("forced shutdown from $nick");
    $::dontQuitOnSignal++;
    $bot->quit ("$nick told me to shutdown");
    exit (0);
}


sub bot_say {
    my ($nick, $cmd, $rest) = (@_);
    my $text = $rest;
    if ($text =~ m@^/me (.*)@) {
        $bot->me($channel, $1);
    } else {
        sendmsg($channel, $text);
    }
}


sub bot_list {
    my ($nick, $cmd, $rest) = (@_);
    my @list;
    foreach (sort keys %admins) {
        push(@list, "$_ $admins{$_}");
    }
    my $spacer = " ... ";
    saylongline($nick, join($spacer, @list), $spacer);
}




sub on_public {
    my ($self, $event) = @_;
    my ($to) = $event->to;
    my ($arg) = $event->args;
    my ($nick, $me) = ($event->nick, $self->nick);
    $::speaker = $nick;         # Hack!!!
    
# catch urls, stick them in a list for mozbot's url command

    if ($arg =~ /(http|ftp|gopher):/i && $nick ne $me) {
        push @urls, "$arg (" . &logdate() . ")";
        while ($#urls > 10) {
            shift @urls;
        }
    }
    if (my ($cmd, $rest) = $arg =~ /^$me[:,]?\s+(\S+)(?:\s+(.*))?$/i) {
        if (do_command(\%pubcmds, $channel, $cmd, $rest)) {
            return;
        } else {
            do_unknown($channel, $cmd, $rest);
        }
    }
}


sub do_unknown {
    my ($nick, $cmd, $rest) = (@_);
    if (defined $::megahal_pid) {
       my $sentence = $cmd . " " . $rest . "\n";
       &debug($sentence);
       print $::WTR $sentence . "\n";
       my $result = ($::RDR)->getline;
       &debug($result . "\n");
       sendmsg($nick, $result);
    }
    else {
        if (!defined $::eliza) {
            $::eliza = new Chatbot::Eliza;
        }
        my $result = $::eliza->transform("$cmd $rest");
        sendmsg($nick, $result);
    }
}


sub saylongline {
    my ($nick, $str, $spacer) = (@_);
    my $MAXPROTOCOLLENGTH = 255;

    while (length ($str) > $MAXPROTOCOLLENGTH) {
        my $pos;
        $pos = rindex($str, $spacer, $MAXPROTOCOLLENGTH - length($spacer));
        if ($pos < 0) {
            $pos = rindex($str, " ", $MAXPROTOCOLLENGTH - 1);
            if ($pos < 0) {
                $pos = $MAXPROTOCOLLENGTH - 1;
            }
        }
        sendmsg($nick, substr($str, 0, $pos));
        $str = substr($str, $pos);
        if (index($str, $spacer) == 0) {
            $str = substr($str, length($spacer));
        }
    }
    if ($str ne "") {
        sendmsg($nick, $str);
    }
}




sub do_headlines {
    my ($nick, $header, $ref) = (@_);
    my $spacer = " ... ";
    my $str = $header . ": " . join($spacer, @$ref);
    saylongline($nick, $str, $spacer);
}

sub reportDiffs {
    my ($name, $url, $ref) = (@_);

    my $firsttime = 0;
    if (!exists $::headCache{$url}) {
        $firsttime = 1;
        $::headCache{$url} = {};
    }
    my $spacer = " ... ";
    my $outstr = "";
    foreach my $i (@$ref) {
        if ($i =~ /^last update/) {
            next;
        }
        if (!exists $::headCache{$url}->{$i}) {
            $::headCache{$url}->{$i} = 1;
            if ($outstr eq "") {
                $outstr = "Just appeared in $name - $url : ";
            } else {
                $outstr .= $spacer;
            }
            $outstr .= $i;
        }
    }
    if (!$firsttime) {
        saylongline($channel, $outstr, $spacer);
    }
}
    
        

        
    
sub bot_debug
	{
	my ($nick, $cmd, $rest) = (@_);
	
	my @list;
	my %last = 
		(
		"tinderbox" => $last_tree,
		"moon" => $last_moon,
                "uuid" => $last_uuid,
		);
	
	foreach (keys %last)
		{
		if ($last{$_} != 0)
			{
			push @list, "$_ updated: " . &logdate ($last{$_}) . ", " . 
				&days ($last{$_});
			}
		else
			{
			push @list, "$_ never updated!";
			}
		}

    foreach (sort(keys %rdf_last)) {
        push @list, "$_ updated: " . logdate($rdf_last{$_}) . ", " .
            days($rdf_last{$_});
    }

	do_headlines ($nick, "Boring Debug Information", \@list);
	}

#timeless didn't like http://server): it upset his irc client/browser link
sub bot_rdfchannel {
    my ($nick, $cmd, $rest, $url) = (@_);
    if (defined $rdf_title{$url}) {
        do_headlines($nick, "Items in $rdf_title{$url} - $rdf_link{$url} ",
                     $rdf_items{$url});
    } else {
        sendmsg($nick, "Nothing has been found yet at $url");
    }
}



sub bot_hi {
    my ($nick, $cmd, $rest) = (@_);
    sendmsg($nick, $greetings[$greet++] . " $::speaker");
    $greet = 0 if ($greet > $#greetings);
}

 

sub on_join
	{
  my ($self, $event) = @_;
  my ($channel) = ($event->to)[0];
  my $nick = $event->nick;
	my $userhost = $event->userhost;
	
	# auto-op if user is a mozbot admin and coming in from
	# the right host 

	if (exists $admins{$nick} && $userhost =~ /$admins{$nick}$/i)
		{
		$self->mode ($channel, "+o", $nick);
		&debug ("auto-op for $nick on $channel");
		}
	}

$::dontQuitOnSignal = 0;
sub on_boot
    {
        if (!$::dontQuitOnSignal) {
            die "$0: disconnected from network";
        }
    }


sub listcmds {
    my ($hashref) = (@_);
    my @list;
    foreach my $k (keys %$hashref) {
        if ($k =~ m/^\(([a-z]+)\|/) {
            push @list, $1;
        } else {
            push @list, $k;
        }
    }
    return join(' ', sort(@list));
}





################
# bot commands #
################

# bot_about: it's either an about box or the 
# address of the guy to blame when the bot 
# breaks

sub bot_about {
    my ($nick, $cmd, $rest) = @_;
    sendmsg($::speaker,  "i am mozbot version $VERSION. hack on me! " .
        "harrison\@netscape.com 10/16/98. " .
        "connected to $server since " .
                  &logdate ($uptime) . " (" . &days ($uptime) . "). " .
				"see http://bonsai.mozilla.org/cvsquery.cgi?branch=HEAD&file=mozilla/webtools/mozbot/&date=week " .
                  "for a changelog.");
    sendmsg($::speaker, "Known commands are: " .
            listcmds(\%pubcmds));
    sendmsg($::speaker, "If you /msg me, I'll also respond to: " .
            listcmds(\%msgcmds));
    if (exists $admins{$::speaker}) {
        sendmsg($::speaker, "And you're an admin, so you can also do: " .
                listcmds(\%admincmds));
    }
    if ($nick eq $channel) {
        sendmsg($nick,
                "[ Directions on talking to me have been sent to $::speaker ]");
    }

}

# bot_moon: goodnight moon

sub get_moon_str
    {
    return "- no moon -" if (! defined $::moon); 
    return $phase if ($phase && (time - $last_moon > (60 * 60 * 24)));
    
    # we only want to run this once/day
    $phase = `$::moon`;
    $last_moon = time;
    return $phase;
    }

sub bot_moon {
    my ($nick, $cmd, $rest) = @_;
    sendmsg($nick, get_moon_str());
}

sub get_uuid_str  {
    my $this_uuid;
    return "- no uuid -" if (! defined $::uuid);
    $this_uuid = `$::uuid`;
    return $this_uuid;
  }

sub bot_uuid {
    my ($nick, $cmd, $rest) = @_;
    sendmsg($nick, get_uuid_str());
  }


# bot_up: report uptime

sub bot_up {
    my ($nick, $cmd, $rest) = @_;
    sendmsg($nick,  &logdate ($uptime) . " (" . &days ($uptime) . ")");
}

# bot_urls: show last ten urls caught by mozbot

sub bot_urls {
    my ($nick, $cmd, $rest) = @_;
    if ($#urls == -1) {
        sendmsg($nick, "- mozbot has seen no URLs yet -");
    } else {
        foreach my $m (@urls) {
            sendmsg($nick, $m);
        }
    }
}

# show tinderbox status
#
# this is a messy little function but it works. 

sub bot_tinderbox {
    my ($nick, $cmd, $rest) = @_;
    my $bustage;
    my $buf;
    my @buf;
    my @tree;
#    my @rstats;
#    my $ttree;
    my $rg=0;
    my $ry=0;
    my $rr=0;
    my $terse = (defined $rest && $rest eq "all");
    if ($nick eq $channel) {
        $terse = 1;
    }
    
    # user can supply a list of trees separated
    # by whitespace, default is all trees

    push @tree, $rest ? (split /\s+/, $rest) : @trees;
    
    # loop through requested trees

    push @buf, "Tinderbox status from http://tinderbox.mozilla.org/showbuilds.cgi";

    foreach my $t (@tree)
        {
        $bustage = 0;
        $buf = "$t " . ($$status{$t} ? "<$$status{$t}> " : "") . ": ";
        # politely report failures
        if (! exists $$all_trees{$t})
            {
            $buf .= "unknown tree \"$t\", trees include @all_trees. ";
            }
        else
            {
    $rg=0;
    $ry=0;
    $rr=0;
            foreach my $e (sort keys %{$$trees{$t}})
                {
$rg++           if ($$trees{$t}{$e}=~/Success/); 
$ry++           if ($$trees{$t}{$e}=~/Test Failed/);
$rr++           if ($$trees{$t}{$e}=~/Horked/);
&debug("$rg $ry $rr $e => $$trees{$t}{$e}");
                next if ($terse && $$trees{$t}{$e} eq "Success");
                $buf .= "[$e: $$trees{$t}{$e}] ";
                $bustage++;
                }
            }
        
#        $buf .= "- no known bustage -" if (! $bustage);
        if ( $bustage) {$buf .= 
"$rg success, $ry test failures, and $rr horked.";}
        else { $buf .= "- no known bustage -";};
        push @buf, $buf;
        }

    $buf = $buf || 
			"something broke. report a bug here: " .
			"http://bugzilla.mozilla.org/enter_bug.cgi " .
			"with product of Webtools and component set to Mozbot";

    push @buf, "last update: " .
        &logdate ($last_tree) . " (" . &days ($last_tree) . " ago)";


    foreach my $m (@buf) {
         sendmsg($nick, $m);
    }

}

#############
# utilities #
#############

sub debug
    {
    return if (! $debug);

    foreach (@_)
        {
#timeless: Broken
#        chomp;
        print &logdate() . " $_ [$$]\n";
        }
    }

# logdate: return nice looking date (10/16/98 18:29)

sub logdate
  {
  my $t = shift;
  $t = time unless ($t);
  my ($sec,$min,$hour,$mday,$mon,$year) = localtime ($t);

  return sprintf ("%02d/%02d/%02d %02d:%02d",
    $mon + 1, $mday, $year + 1900, $hour, $min);
  }

# days: how long ago was that? 

sub days
  {
  my ($then) = shift;
    
  my $seconds = time - $then;
  my $minutes = int ($seconds / 60);
  my $hours = int ($minutes / 60);
  my $days = int ($hours / 24);

  if ($seconds < 60)
    { return (sprintf "%d second%s", $seconds, $seconds == 1 ? "" : "s"); }
  elsif ($minutes < 60)
    { return (sprintf "%d minute%s", $minutes, $minutes == 1 ? "" : "s"); }
  elsif ($hours < 24)
    { return (sprintf "%d hour%s", $hours, $hours == 1 ? "" : "s"); }
  else
    { return (sprintf "%d day%s", $days, $days == 1 ? "" : "s"); }
  }

# signal handler

sub killed
    {
    if ($::megahal_pid) {
       &debug("Killing megahal.\n");
       kill (2, $::megahal_pid);
    }
    confess "i have received a signal of some manner. good night.\n\n";
    }

# write admin list 

sub store_admin_conf
	{
	my $admins = shift;
	my $when = localtime (time) . " by $$";

	if (open ADMINS, ">$adminf")
		{
		print ADMINS <<FIN;
# mozbot admin list file
#
# this file is generated. do not edit.
# generated $when 
#
# version: 1.0

FIN

		foreach (sort keys %admins)
			{
			print ADMINS "$_ $admins{$_}\n";
			}
		close ADMINS;
		}
	else
		{
		&debug ("&store_admin_conf $adminf: $!");
		}
	}

# fetch list of admins

sub fetch_admin_conf
	{
	my $admins = shift;

	if (open ADMINS, $adminf)
		{
		while (<ADMINS>)
			{
			chomp;
			next if ($_ =~ /^#/ or ! $_);
			my ($user, $host) = split /\s+/, $_;
			$$admins{$user} = $host;
			}
		&debug ("admins: " . keys %$admins);
		}
	else
		{
		&debug ("&fetch_admin_conf $adminf: $!");
		}

	close ADMINS;
	}

# create a pid file if we can

sub create_pid_file
	{
	my $pid = ".$nick-pid";

	if (open PID, ">$pid")
    {
    print PID "$$\n";
    close PID;
    }
	else
    {
    &debug ("warning: problem creating pid file: $pid, $!");
    }
	}

sub init_megahal {
    my $pid;
    chdir("./megahal");
    $pid = open2($::RDR, $::WTR, "./megahal");
    chdir("..");
    return($pid);

}

sub rdfchannel {
    my ($foo, $url) = (@_);
    debug("fetching rdfchannel $url");
    $bot->schedule(60*60, \&rdfchannel, $url);

    my $output = get $url;
    $rdf_last{$url} = time();
    return if (!$output);

    my $channelpart = "";
    if ($output =~ m@<channel>(.*)</channel>@si) {
        $channelpart = $1;
    }
    $output =~ s@<image>.*</image>@@si;

    $rdf_title{$url} = $url;
    if ($channelpart =~ m@<title>(.+?)</title>@si) {
        $rdf_title{$url} = trim($1);
    }
    $rdf_link{$url} = $url;
    if ($channelpart =~ m@<link>(.+?)</link>@si) {
        $rdf_link{$url} = trim($1);
    }

    my @list;
    while ($output =~ m@<item>(.*?)<title>(.+?)</title>(.*?)</item>@sig) {
        push(@list, $2);
    }
    $rdf_items{$url} = \@list;


    reportDiffs($rdf_title{$url}, $rdf_link{$url}, \@list);
}

    


# fetch tinderbox details

sub tinderbox
    {
    &debug ("fetching tinderbox status");
    my ($newtrees, $newstatus) = Tinderbox::status (\@trees);

		if (! $newtrees)
			{
    	$bot->schedule (90, \&tinderbox);
			&debug ("hmm, couldn't get tinderbox status");
			return;
			}

    $last_tree = time;

		if (defined $status)
			{
			foreach my $s (keys %$newstatus)
				{
				if (defined $$newstatus{$s} && $$status{$s} ne $$newstatus{$s})
					{
					sendmsg($channel,
					  "$s changed state from $$status{$s} to $$newstatus{$s}");
					}
				}
			}

    if (defined $trees) {
        foreach my $t (@trees) {
            foreach my $e (sort keys %{$$newtrees{$t}}) {
                if (!defined $$trees{$t}{$e}) {
                    sendmsg($channel, "$t: A new column '$e' has appeared ($$newtrees{$t}{$e})");
                } else {
                    if ($$trees{$t}{$e} ne $$newtrees{$t}{$e}) {
                        sendmsg($channel, "$t: '$e' has changed state from $$trees{$t}{$e} to $$newtrees{$t}{$e}");
                    }
                }
            }
        }
    }
    $trees = $newtrees;
		$status = $newstatus;
    
    $bot->schedule (360, \&tinderbox);
    }


# See if someone has changed our source.

sub checksourcechange {
    my ($self) = @_;
    my $lastourdate = $::ourdate;
    my $lasttinderboxdate = $::tinderboxdate;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat("./mozbot.pl");
    $::ourdate = $mtime;
    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat("./Tinderbox.pm");
    $::tinderboxdate = $mtime;

    if (defined $lastourdate && 
			($::ourdate > $lastourdate ||
                             $::tinderboxdate > $lasttinderboxdate)) {
        $::dontQuitOnSignal = 1;
        $self->quit("someone seems to have changed my source code.  Be right back");
        &debug ("restarting self");
        exec "$0 @::origargv";
    }
    $bot->schedule (60, \&checksourcechange);
}


sub stocks {
    $bot->schedule(15 * 60, \&stocks);

    my $url = "http://quote.yahoo.com/d/quotes.csv?f=sl1d1t1c1ohgv&e=.csv&s=" .
        join("+", sort(keys %stocklist));
    &debug ("fetching stock quotes  $url");
    my $output = get $url;
    return if (!$output);
    %stockvals = ();
    foreach my $line (split(/\n/, $output)) {
        my @list = split(/,/, $line);
        my $name = shift(@list);
        $name =~ s/"(.*)"/$1/;
        &debug ("parsing stock quote $name ($list[0]) $line");
        $stockvals{$name} = \@list;
        foreach my $ref (@{$stockhist{$name}}) {
            my $oldval = $ref->[0];
            my $newval = $list[0];
            my $ratio = $newval / $oldval;
            if ($ratio > 1.05 || $ratio < 0.95) {
                foreach my $who ($stocklist{$name}) {
                    ReportStock($who, $name, "Large Stock Change");
                }
                $stockhist{$name} = [];
                last;
            }
        }
        if (!exists $stockhist{$name}) {
            $stockhist{$name} = [];
        }
        push (@{$stockhist{$name}}, \@list);
        while (30 < @{$stockhist{$name}}) {
            shift @{$stockhist{$name}};
        }
    }
}


sub LoadStockList {
    %stocklist = ("AOL"  => [$channel],
                  "RHAT" => [$channel],
                  "^DJI" => [$channel],
                  "^IXIC" => [$channel]);

    if (open(LIST, $stockf)) {
        %stocklist = ();
        while (<LIST>) {
            my @list = split(/\|/, $_);
            my $name = shift(@list);
            $stocklist{$name} = \@list;
        }
    }
}


sub FracStr {
    my ($num, $needplus) = (@_);
    my $sign;
    if ($num < 0) {
        $sign = "-";
        $num = - $num;
    } else {
        $sign = $needplus ? "+" : "";
        $num =~ s/^\+//;
    }
    my $orignum = $num;
    
    my $bdot = int($num);
    my $adot = $num - $bdot;

    if ($adot == 0) {
        return "$sign$bdot";
    }

    my $base = 64;
    $num = int($adot * $base);

    while ($num % 2 == 0 && $base > 1) {
        $base /= 2;
        $num /= 2;
    }

    if ($adot == $num / $base) {
        if ($bdot == 0) {
            $bdot = "";
        } else {
            $bdot .= " ";
        }
        return "$sign$bdot$num/$base";
    }
    return "$sign$orignum";
}



sub ReportStock {
    my ($nick, $name, $title) = (@_);
    if ($title ne "") {
        $title .= ": ";
    }
    my $ref = $stockvals{$name};
    my $a = FracStr($ref->[0], 0);
    my $b = FracStr($ref->[3], 1);
    sendmsg($nick, "$title$name at $a ($b)");
}

sub bot_stocks {
    my ($nick, $cmd, $rest) = (@_);
    foreach my $name (sort(keys %stocklist)) {
        foreach my $who (@{$stocklist{$name}}) {
            if ($who eq $nick || $who eq $channel) {
                ReportStock($nick, $name, "");
            }
        }
    }
}

sub bot_pub_stocks {
    my ($nick, $cmd, $rest) = (@_);
    bot_stocks($::speaker, $cmd, $rest);
    sendmsg($nick, "[ Stocks sent to $::speaker. In the future, use \"/msg mozbot stocks\" ]");
}

sub translate_usage {
    my ($nick) = (@_);
    sendmsg($nick, "Usage: translate (to|from) (en|fr|pt|it|de) sentence");
}

sub bot_translate {
    my ($nick, $cmd, $rest) = (@_);
    my ($dir, $lang, @words) = split(/ +/, $rest);
    if ($dir ne "to" && $dir ne "from") {
        return translate_usage($nick);
    }
    my $result = babel::babelfish($dir, $lang, join(' ', @words));
    if ($result ne '') {
        sendmsg($nick, $result);
    } else {
        translate_usage($nick);
    }
}

sub bot_review {
    my ($nick, $cmd, $rest) = (@_);
    sendmsg($nick, "$::speaker, I've reviewed your code, and it looks great.  r=mozbot.");
}

sub trim {
    ($_) = (@_);
    s/^\s+//g;
    s/\s+$//g;
    return $_;
}



# Do this at the very end, so we can intersperse "my" initializations outside
# of routines above and be assured that they will run.

$irc->start;
