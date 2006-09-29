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
 # The Original Code is Litmus.
 #
 # The Initial Developer of the Original Code is
 # the Mozilla Corporation.
 # Portions created by the Initial Developer are Copyright (C) 2006
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s):
 #   Chris Cooper <ccooper@deadsquid.com>
 #   Zach Lipton <zach@zachlipton.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

package Litmus::DB::Locale;

use strict;
use base 'Litmus::DBI';

Litmus::DB::Locale->table('locale_lookup');

Litmus::DB::Locale->columns(All => qw/abbrev name/);
Litmus::DB::Locale->columns(TEMP => qw //);

Litmus::DB::Locale->column_alias("abbrev", "locale");

Litmus::DB::Locale->has_many(test_results => "Litmus::DB::Testresult");

__PACKAGE__->set_sql(RetrieveAll => qq{
                                       SELECT __ESSENTIAL__
                                       FROM   __TABLE__
                                       ORDER BY abbrev ASC
});

1;
