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

package Litmus::DB::Branch;

use strict;
use base 'Litmus::DBI';

Litmus::DB::Branch->table('branches');

Litmus::DB::Branch->columns(All => qw/branch_id product_id name detect_regexp enabled creation_date last_updated creator_id/);
Litmus::DB::Branch->columns(Essential => qw/branch_id product_id name detect_regexp enabled creation_date last_updated creator_id/);
Litmus::DB::Branch->utf8_columns(qw/name detect_regexp/);
Litmus::DB::Branch->columns(TEMP => qw//);

Litmus::DB::Branch->column_alias("creator_id", "creator");
Litmus::DB::Branch->column_alias("product_id", "product");

Litmus::DB::Branch->has_many(test_results => 'Litmus::DB::Testresult');
Litmus::DB::Branch->has_a(product_id=>'Litmus::DB::Product');
Litmus::DB::Branch->has_a(creator_id=>'Litmus::DB::User');

__PACKAGE__->set_sql(ByTestgroup => qq{
                                       SELECT b.* 
                                       FROM branches b, testgroups tg 
                                       WHERE tg.testgroup_id=? AND tg.branch_id=b.branch_id
                                       ORDER BY b.name ASC
});


1;
