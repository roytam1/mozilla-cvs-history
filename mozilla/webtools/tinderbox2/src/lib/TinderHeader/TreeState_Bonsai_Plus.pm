# -*- Mode: perl; indent-tabs-mode: nil -*-
#

# TinderHeader::TreeState_Bonsai_Plus - an interface to the bonasi VC
# managment system this module will get the current tree state.  I
# also knows about states which Bonsai does not know about.  It can
# display these states and can also set the states which are not the
# Bonsai States.

# $Revision$ 
# $Date$ 
# $Author$ 
# $Source$ 
# $Name$ 

# The contents of this file are subject to the Mozilla Public
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

# complete rewrite by Ken Estes:
#	 kestes@staff.mail.com Old work.
#	 kestes@reefedge.com New work.
#	 kestes@walrus.com Home.
# Contributor(s): 


package TinderHeader::TreeState;

# Load standard perl libraries


# Load Tinderbox libraries

use lib '#tinder_libdir#';

use BonsaiData;


@ISA = qw(TinderHeader::BasicTxtHeader);

$VERSION = ( qw $Revision$ )[1];

# load the simple name of this module into TinderHeader so we can
# track the implementations provided.

$TinderHeader::NAMES2OBJS{ 'TreeState' } = 
  TinderHeader::TreeState->new();



sub new {
  my $type = shift;
  my %params = @_;
  my $self = {};

  bless $self, $type;

  return  $self;
}


sub gettree_header {
  my ($self, $tree,) = @_;

  my ($bonsai_treestate) = BonsaiData::get_tree_state($tree);
  my ($text_treestate) = $self->SUPER::gettree_header($tree);
  my $treestate;

  if (!($text_treestate)) {

      # If there is no state, then force the default. This is needed
      # for the first time a new tree is run.

      $self->SUPER::savetree_header($tree, 'Current_Bonsai_State');
      $text_treestate = 'Current_Bonsai_State';
  }

  if (
      ($text_treestate) &&
      ($text_treestate != 'Current_Bonsai_State') 
      ) {
      $treestate = $text_treestate;
  } else {
      $treestate = $bonsai_treestate;
  }

  return $treestate;
}

# We save the states which are not the bonsai states. If people wish
# to change bonsai they must use the bonsai pages.

sub savetree_header {
  my ($self, $tree, $value) = @_;

  $self->SUPER::savetree_header($tree, $value);

  return ;
}

sub get_all_sorted_setable_tree_states {

    my @valid_states =( 'Current_Bonsai_State', (
                             grep { !/^((Open)|(Closed))$/ } 
                           TreeData::get_all_sorted_tree_states()
                             )
                        );

  return @valid_states;
}


1;

