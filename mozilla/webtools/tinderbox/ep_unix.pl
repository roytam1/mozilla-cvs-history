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
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

1;
# 
# Scan a line and see if it has an error
#
BEGIN {
  # Make $last_error_was_gmake persistent private variable for has_error().
  my $last_error_was_gmake = 0; 

  sub has_error {
    local $_ = $_[0];
  
    # Special case gmake to mark the "Leaving directory"
    # line as an error too.
    if (/^g?make(?:\[\d\d?\])?: \*\*\*/) {
      $last_error_was_gmake = 1;
      return 1;
    }

    if ($last_error_was_gmake 
        and /^g?make(?:\[\d\d?\])?: Leaving directory/) {
      return 1;
    }

    $last_error_was_gmake = 0;

    /fatal error/ # . . . . . . . . . . . . Link
      or /^C /  # . . . . . . . . . . . . . cvs merge conflict
      or /Error: /  # . . . . . . . . . . . C or smoketest
      or / error\([0-9]*\)\:/ # . . . . . . C
      or /\[checkout aborted\]/   # . . . . cvs
      or /\: cannot find module/  # . . . . cvs
      ;
  }
}


sub has_warning {                                    
  local $_ = $_[0];
  /^[A-Za-z0-9_]+\.[A-Za-z0-9]+\:[0-9]+\:/ 
    or /^\"[A-Za-z0-9_]+\.[A-Za-z0-9]+\"\, line [0-9]+\:/ 
    ;
}

sub has_errorline {
  local $_ = $_[0];
  my $out  = $_[1];

  if (/^(([A-Za-z0-9_]+\.[A-Za-z0-9]+):([0-9]+):)/) {
    $out->{error_file}     = $1;
    $out->{error_file_ref} = $2;
    $out->{error_line}     = $3;
    $out->{error_guess}    = 1;
    return 1;
  }
  if (/^("([A-Za-z0-9_]+\.[A-Za-z0-9]+)", line ([0-9]+):)/) {
    $out->{error_file}     = $1;
    $out->{error_file_ref} = $2;
    $out->{error_line}     = $3;
    $out->{error_guess}    = 1;
    return 1;
  }
  return 0;
}
