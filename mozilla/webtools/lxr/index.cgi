#!/usr/bin/perl 
use lib 'lib';
use LXR::Common;
use LXR::Config;

($Conf, $HTTP, $Path, $head) = &init($0);
print "$head
";

# this can be calculated from lxr.conf's baseurl parameter
# unless of course there are two urls which could map here
# http://landfill.mozilla.org/mxr-test/
# http://mxr-test.landfill.bugzilla.org/

my $myserver = $ENV{'HTTP_HOST'} || $ENV{'SERVER_NAME'}; 
my $depth = ($myserver =~ /[lm]xr.*\./) ? 2 : 3;
if ($ENV{SCRIPT_NAME}=~m%(?:/[^/]+){$depth,}%) {
open INDEX, "<index.html";
} else {
open INDEX, "<root/index.html";
}

{
local $/ = undef;
my $template = <INDEX>;
print &expandtemplate($template,
                      ('rootname', sub { return $Conf->{'sourceprefix'}; }),
                      ('treename', sub { return $Conf->{'treename'}; }),
                     );
}
close INDEX;

