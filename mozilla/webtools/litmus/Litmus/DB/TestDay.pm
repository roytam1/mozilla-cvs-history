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
 #   Zach Lipton <zach@zachlipton.com>
 #   Chris Cooper <ccooper@deadsquid.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

package Litmus::DB::TestDay; 
$VERSION = 1.00;
use strict;
use base 'Litmus::DBI';

Litmus::DB::TestDay->table('testdays');

Litmus::DB::TestDay->columns(All => qw/testday_id last_updated start_timestamp finish_timestamp description product_id testgroup_id build_id branch_id locale_abbrev creator_id creation_date/);
Litmus::DB::TestDay->columns(Essential => qw/testday_id last_updated start_timestamp finish_timestamp description product_id testgroup_id build_id branch_id locale_abbrev creator_id creation_date/);
Litmus::DB::TestDay->utf8_columns(qw/description locale_abbrev/);
Litmus::DB::TestDay->columns(TEMP => qw /subgroups creator/);

Litmus::DB::TestDay->column_alias("product_id", "product");
Litmus::DB::TestDay->column_alias("testgroup_id", "testgroup");
Litmus::DB::TestDay->column_alias("branch_id", "branch");
Litmus::DB::TestDay->column_alias("locale_abbrev", "locale");
Litmus::DB::TestDay->column_alias("creator_id", "creator");

Litmus::DB::TestDay->has_a(product => "Litmus::DB::Product");
Litmus::DB::TestDay->has_a(branch => "Litmus::DB::Branch");
Litmus::DB::TestDay->has_a(testgroup => "Litmus::DB::Testgroup");
Litmus::DB::TestDay->has_a(locale => "Litmus::DB::Locale");
Litmus::DB::TestDay->has_a(creator => "Litmus::DB::User");

Litmus::DB::TestDay->set_sql('daterange' => qq {
	SELECT __ESSENTIAL__ 
	FROM __TABLE__
	WHERE
	  start_timestamp<=? AND finish_timestamp>=?
	ORDER BY finish_timestamp ASC, product_id ASC, branch_id ASC, testday_id ASC
});

#########################################################################
sub delete_from_subgroups() {
  my $self = shift;
  
  my $dbh = __PACKAGE__->db_Main();  
  my $sql = "DELETE from testday_subgroups WHERE testday_id=?";
  return $dbh->do($sql,
                  undef,
                  $self->testday_id
                 );
}

#########################################################################
sub update_subgroups() {
  my $self = shift;
  my $new_subgroup_ids = shift;
  
  # We always want to delete the existing testgroups. 
  # Failing to delete testgroups is _not_ fatal when adding a new subgroup.
  my $rv = $self->delete_from_subgroups();
  if (scalar @$new_subgroup_ids) {
    my $dbh = __PACKAGE__->db_Main();  
    my $sql = "INSERT INTO testday_subgroups (testday_id, subgroup_id) VALUES (?,?)";
    foreach my $new_subgroup_id (@$new_subgroup_ids) {
      my $rows = $dbh->do($sql, 
			  undef,
			  $self->testday_id,
			  $new_subgroup_id
			 );
    }
  }
}

#########################################################################
sub delete_with_refs() {
  my $self = shift;
  $self->delete_from_subgroups();
  return $self->delete;
}

1;
