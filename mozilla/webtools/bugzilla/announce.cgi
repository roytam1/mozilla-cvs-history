#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# $Id$
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Andrew Anderson <andrew@redhat.com>
#		  		  Dave Lawrence <dkl@redhat.com>

# include necessary packages
use strict;
use CGI;

require 'CGI.pl';
require 'globals.pl';

# set up necessary variables
$::cgi = new CGI;

my $submit = $::cgi->param('submit');
my $product = $::cgi->param('product');
my $version = $::cgi->param('version');
my $platform = $::cgi->param('arch');
my $severity = $::cgi->param('severity');
my $priority = $::cgi->param('priority');
my $class = $::cgi->param('class');
my $component = $::cgi->param('component');
my $synopsis = $::cgi->param('synopsis');
my $advisory = $::cgi->param('advisory');
my $keywords = $::cgi->param('keywords');
my $cross = $::cgi->param('cross');
my $topic = $::cgi->param('topic');
my $fixed = $::cgi->param('fixed');

# my $summary = "";
# my $desc = "";

print "Content-type: text/html\n\n";

if (!$submit) {
	# construct a form for the rest of the information
	PutHeader("Errata Announcement");
	
	print 	$::cgi->startform(-ACTION=>'announce.cgi'),
			$::cgi->table({-BGCOLOR=>'#ECECEC', -ALIGN=>'left', -WIDTH=>'75%', -CELLSPACING=>'0', -CELLPADDING=>'4'},
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Package ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"component",
						                  -VALUE=>"$component",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Product ')
					),
					$::cgi->td({-ALIGN=>'left'},
					    $::cgi->textfield(-NAME=>"product",
										  -VALUE=>"$product",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Version ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"version",
									      -VALUE=>"$version",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Advisory ID ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"advisory",
						                  -VALUE=>"$advisory",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Synopsis ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"synopsis",
										  -VALUE=>"$synopsis",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Key Words ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"keywords",
						                  -VALUE=>"$keywords",
										  -SIZE=>60)
				    )
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Cross Reference ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textfield(-NAME=>"cross",
										  -VALUE=>"$cross",
										  -SIZE=>60)
					)
				),
				$::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Topic ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textarea(-NAME=>"topic",
					    				 -WRAP=>"hard",
										 -ROWS=>"2",
										 -COLUMNS=>"60",
										 -DEFAULT=>"$topic")
				    )
				),
                $::cgi->TR(
					$::cgi->td({-BGCOLOR=>'#BFBFBF', -ALIGN=>'right'},
						$::cgi->b('Bug IDs Fixed ')
					),
					$::cgi->td({-ALIGN=>'left'},
						$::cgi->textarea(-NAME=>"fixed",
										 -WRAP=>"hard",
										 -ROWS=>"2",
										 -COLUMNS=>"60",
										 -DEFAULT=>"$fixed")
					)
				)
			),
			$::cgi->center($::cgi->submit(-name=>"submit", -value=>"View Final")),
			$::cgi->endform;
	PutFooter();
	exit;
}

# After submit, print out form on clean page
print <<End_of_html;

<PRE>
---------------------------------------------------------------------
                   Red Hat, Inc. Errata Advisory

Synopsis:               $synopsis 
Advisory ID:            $advisory 
Issue date:             
Updated on:             
Keywords:               $keywords 
Cross references:       $cross
---------------------------------------------------------------------
      
1. Topic:
     
$topic      
	   
2. Bug IDs fixed:

$fixed

3. Relevant releases/architectures:

$platform

4. Obsoleted by:



5. Conflicts with:



6. RPMs required:

Intel:

Alpha:
      
Sparc:

Source packages:


7. Problem description:


8. Solution:

For each RPM for your particular architecture, run:
 
rpm -Uvh <filename>
 
where filename is the name of the RPM.
      
9. Verification:

MD5 sum                           Package Name
--------------------------------------------------------------------------

These packages are also PGP signed by Red Hat Inc. for security. Our
key is available at:

http://www.redhat.com/corp/contact.html
You can verify each package with the following command:

rpm --checksig  <filename>

If you only wish to verify that each package has not been corrupted or
tampered with, examine only the md5sum with the following command:

rpm --checksig --nopgp <filename>
 
10. References:


</PRE>

End_of_html

exit;
