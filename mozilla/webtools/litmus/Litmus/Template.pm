# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

=head1 COPYRIGHT

 # ***** BEGIN LICENSE BLOCK *****
 # Version: MPL 1.1
 #
 # The contents of this file are subject to the Mozilla Public License
 # Version 1.1 (the "License"); you may not use this file except in
 # compliance with the License. You may obtain a copy of the License
 # at http://www.mozilla.org/MPL/
 #
 # Software distributed under the License is distributed on an "AS IS"
 # basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 # the License for the specific language governing rights and
 # limitations under the License.
 #
 # The Original Code is the Bugzilla Bug Tracking System.
 #
 # The Initial Developer of the Original Code is Netscape Communications
 # Corporation. Portions created by Netscape are
 # Copyright (C) 1998 Netscape Communications Corporation. All
 # Rights Reserved.
 #
 # Contributor(s): Terry Weissman <terry@mozilla.org>
 #                 Dan Mosedale <dmose@mozilla.org>
 #                 Jacob Steenhagen <jake@bugzilla.org>
 #                 Bradley Baetz <bbaetz@student.usyd.edu.au>
 #                 Christopher Aillon <christopher@aillon.com>
 #                 Tobias Burnus <burnus@net-b.de>
 #                 Myk Melez <myk@mozilla.org>
 #                 Max Kanat-Alexander <mkanat@bugzilla.org>
 #                 Zach Lipton <zach@zachlipton.com>
 #                 Chris Cooper <ccooper@deadsquid.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

# This is mostly a placeholder. At some point in the future, we might 
# want to be more like Bugzilla and support multiple languages and 
# other fine features, so we keep this around now so that adding new 
# things later won't require changing every source file. 

package Litmus::Template;

use strict;

use Litmus::Config;
use Litmus::StripScripts;
use Text::Markdown;

use base qw(Template);

my $template_include_path;

$Template::Directive::WHILE_MAX = 30000;

# Returns the path to the templates based on the Accept-Language
# settings of the user and of the available languages
# If no Accept-Language is present it uses the defined default
sub getTemplateIncludePath () {
    return Litmus::Config::litmus_locations()->{'templates'}."/en/default";
}

# Constants:
my %constants = ();
$constants{litmus_version} = $Litmus::Config::version;

# html tag stripper:
my $strip = Litmus::StripScripts->new(
                                      {
                                       AllowHref => 1,
                                       AllowSrc => 1,
                                       Context => 'Document'
                                      },
                                      strict_names => 1,
                                     );

###############################################################################
# Templatization Code

# Use the Toolkit Template's Stash module to add utility pseudo-methods
# to template variables.
use Template::Stash;

# Add "contains***" methods to list variables that search for one or more 
# items in a list and return boolean values representing whether or not 
# one/all/any item(s) were found.
$Template::Stash::LIST_OPS->{ contains } =
  sub {
      my ($list, $item) = @_;
      return grep($_ eq $item, @$list);
  };

$Template::Stash::LIST_OPS->{ containsany } =
  sub {
      my ($list, $items) = @_;
      foreach my $item (@$items) { 
          return 1 if grep($_ eq $item, @$list);
      }
      return 0;
  };

# Allow us to still get the scalar if we use the list operation ".0" on it
$Template::Stash::SCALAR_OPS->{ 0 } = 
  sub {
      return $_[0];
  };

# Add a "substr" method to the Template Toolkit's "scalar" object
# that returns a substring of a string.
$Template::Stash::SCALAR_OPS->{ substr } = 
  sub {
      my ($scalar, $offset, $length) = @_;
      return substr($scalar, $offset, $length);
  };

# Add a "truncate" method to the Template Toolkit's "scalar" object
# that truncates a string to a certain length.
$Template::Stash::SCALAR_OPS->{ truncate } = 
  sub {
      my ($string, $length, $ellipsis) = @_;
      $ellipsis ||= "";
      
      return $string if !$length || length($string) <= $length;
      
      my $strlen = $length - length($ellipsis);
      my $newstr = substr($string, 0, $strlen) . $ellipsis;
      return $newstr;
  };

# Create the template object that processes templates and specify
# configuration parameters that apply to all templates.
sub create {
    my $class = shift;
    return $class->new({
        INCLUDE_PATH => &getTemplateIncludePath,
        CONSTANTS => \%constants,
        POST_CHOMP => 1,
        EVAL_PERL => 1,
        
        COMPILE_DIR => Litmus::Config::litmus_locations()->{'datadir'},
        
        FILTERS => {
            # disallow all html in testcase data except for non-evil tags
            # also sneak target="blank" into hrefs so that links open in new 
            # tabs or windows (based on the user's browser prefs)
            testdata => sub {
                my ($data) = @_;                
                $data =~  s/^\s+//g;
                $data =~  s/\s+$//g;
                $data =~ s/<a /<a target="external_link" /g;
                return $data;
            }, 

            html => sub {
                my ($data) = @_;
                my $filtered = &Template::Filters::html_filter($data);
                $filtered =~ s/(bug)\s+(\d+)/<a target=\'external_link\' href=\'${Litmus::Config::local_bug_url}\'$2\'>$1 $2<\/a>/ig; 
                $filtered =~ s/(testcase|test)\s+(\d+)/<a target=\'external_link\' href=\'show_test.cgi?id=$2\'>$1 $2<\/a>/ig;

                return $filtered;
            },
            
            # process the text with the markdown text processor
            markdown => sub {
            	my ($data) = @_;
                eval {
            	  $data = Text::Markdown::markdown($data);
                };
                return $data;
            },
            
            # Returns the text with backslashes, single/double quotes,
            # and newlines/carriage returns escaped for use in JS strings.
            # thanks to bugzilla!
            js => sub {
                my ($var) = @_;
                $var =~ s/([\\\'\"\/])/\\$1/g;
                $var =~ s/\n/\\n/g;
                $var =~ s/\r/\\r/g;
                $var =~ s/\@/\\x40/g; # anti-spam for email addresses
                return $var;
            },
            
            # anti-spam filtering of email addresses
            email => sub {
                my ($var) = @_;
                $var =~ s/\@/\&#64;/g;
                return $var;    
            },
            
            userFromEmail => sub {
                my ($var) = @_;
                $var =~ s/\@.*$//g;
                return $var;
            },

            # dummy filter when we don't actually need to filter anything
            none => sub {
            	my ($var) = @_;
                return $var;
            },
        },
    });
}

# override the process() method to sneak defaultemail into all template 
# variable spaces
sub process {
    my ($self, $template, $vars, $outstream, @opts) = @_;
    my %vars = %$vars;
	
    if (!$vars{defaultemail}) {
        $vars{defaultemail} = $vars{defaultemail} ? $vars{defaultemail} :
          Litmus->getCurrentUser();
    }
    
    if (!$vars{show_admin}) {
	$vars{show_admin} = Litmus->getCurrentUser() ? 
	  Litmus->getCurrentUser()->is_admin() : 0;
    }
 
    binmode STDOUT, ":utf8";
    $self->SUPER::process($template, \%vars, $outstream, @opts);
}
