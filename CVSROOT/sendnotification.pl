#! /tools/ns/bin/perl5

use Socket;

sub get_response_code {
    my ($expecting) = @_;
#     if ($flag_debug) {
# 	print STDERR "SMTP: Waiting for code $expecting\n";
#     }
    while (1) {
	my $line = <S>;
# 	if ($flag_debug) {
# 	    print STDERR "SMTP: $line";
# 	}
	if ($line =~ /^[0-9]*-/) {
	    next;
	}
	if ($line =~ /(^[0-9]*) /) {
	    my $code = $1;
	    if ($code == $expecting) {
# 		if ($flag_debug) {
# 		    print STDERR "SMTP: got it.\n";
# 		}
		return;
	    }
	    die "Bad response from SMTP -- $line";
	}
    }
}

my @mailto;
my $i;
foreach $i (@ARGV) {
    # Deal with our "%" encoding of email addresses.
    if ($i !~ /\@/) {
	$i =~ s/%/\@/;
    }
    push(@mailto, $i);
}
    
chop(my $hostname = `hostname`);

my ($remote,$port, $iaddr, $paddr, $proto, $line);

$remote  = $mailhost;
$port    = 25;
if ($port =~ /\D/) { $port = getservbyname($port, 'tcp') }
die "No port" unless $port;
$iaddr   = inet_aton($remote)               || die "no host: $remote";
$paddr   = sockaddr_in($port, $iaddr);

$proto   = getprotobyname('tcp');
socket(S, PF_INET, SOCK_STREAM, $proto)  || die "socket: $!";
connect(S, $paddr)    || die "connect: $!";
select(S); $| = 1; select(STDOUT);

get_response_code(220);
print S "EHLO $hostname\n";
get_response_code(250);
print S "MAIL FROM: cvs-notify-daemon@$hostname\n";
get_response_code(250);
foreach $i (@mailto) {
    print S "RCPT TO: $i\n";
    get_response_code(250);
}
print S "DATA\n";
get_response_code(354);
# Get one line starting with "354 ".
print S "Subject: CVS notification\n";
print S "To: " . join(',', @mailto) . "\n";
print S "\n";
while (<STDIN>) {
    print S $_;
}
print S ".\n";
get_response_code(250);
print S "QUIT\n";
close(S);
