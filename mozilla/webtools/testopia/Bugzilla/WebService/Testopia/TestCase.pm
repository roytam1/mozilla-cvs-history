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
# Contributor(s): Marc Schumann <wurblzap@gmail.com>
#                 Dallas Harken <dharken@novell.com>

package Bugzilla::WebService::Testopia::TestCase;

use strict;
use base qw(Bugzilla::WebService);
use Bugzilla::User;
use Bugzilla::Testopia::TestCase;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Table;

# Utility method called by the list method
sub _list
{
    my ($query) = @_;
    
    my $cgi = Bugzilla->cgi;

    $cgi->param("current_tab", "case");
    
    foreach (keys(%$query))
    {
        $cgi->param($_, $$query{$_});
    }
        
    my $search = Bugzilla::Testopia::Search->new($cgi);

    # Result is an array of test plan hash maps 
    return Bugzilla::Testopia::Table->new('case', 
                                          'tr_xmlrpc.cgi', 
                                          $cgi, 
                                          undef,
                                          $search->query()
                                          )->list();
}

sub get
{
    my $self = shift;
    my ($test_case_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }
    
    $self->logout;

    #Result is a test case hash map
    return $test_case;
}

sub list
{
    my $self = shift;
    my ($query) = @_;

    $self->login;
    
    my $list = _list($query);
    
    $self->logout;
    
    return $list;    
}

sub create
{
    my $self =shift;
    my ($new_values) = @_;
    
    $self->login;
        
    my $plan = Bugzilla::Testopia::TestPlan->new($new_values->{plan_id});
    unless ($plan)
    {
        $self->logout;
        die "Plan ID Number (plan_id) Required When Creating A TestCase";
    }

    # Remove plan id from new_values hash    
    delete $$new_values{plan_id};
    
    $new_values->{'plans'} = [$plan];
    
    my $test_case = Bugzilla::Testopia::TestCase->create($new_values);
    
    $test_case->link_plan($plan_id, $test_case->id);
    
    $self->logout;

    return $test_case->id
}

sub createBatch
{
    my $self =shift;
    my (@values) = @_;
    my @ids;
    
    for my $new_values (@values)
    {
    if (not defined $$new_values{plan_id})
    {
        die "Plan ID Number (plan_id) Required When Creating A TestCase"
    }

    # Plan id linked to new test case after store method is called
    my $plan_id = $$new_values{plan_id};

    # Remove plan id from new_values hash    
    delete $$new_values{plan_id};

    $self->login;

    my $test_case = Bugzilla::Testopia::TestCase->create($new_values);
    
    $test_case->link_plan($plan_id, $test_case->id);
    
    $self->logout;

    push @ids, $test_case->id;
    }
    
    return @ids;
}

sub update
{
    my $self =shift;
    my ($test_case_id, $new_values) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }

    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    if (defined $$new_values{author_id})
    {
        die "Update of TestCase's author_id is not allowed";
    }

    $test_case->set_case_status($new_values->{'case_status_id'});
    $test_case->set_category($new_values->{'category_id'});
    $test_case->set_priority($new_values->{'priority_id'});
    $test_case->set_default_tester($new_values->{'default_tester_id'});
    $test_case->set_sortkey($new_values->{'sortkey'});
    $test_case->set_requirement($new_values->{'requirement'});
    $test_case->set_isautomated($new_values->{'isautomated'});
    $test_case->set_script($new_values->{'script'});
    $test_case->set_arguments($new_values->{'arguments'});
    $test_case->set_summary($new_values->{'summary'});
    $test_case->set_alias($new_values->{'alias'});
    $test_case->set_estimated_time($new_values->{'estimated_time'});
    $test_case->set_dependson($new_values->{'dependson'});
    $test_case->set_blocks($new_values->{'blocks'});

    $test_case->update();

    $self->logout;
    
    # Result is modified test case on success, otherwise an exception will be thrown
    return $test_case;
}

sub get_text
{
    my $self =shift;
    my ($test_case_id) = @_;

    $self->login;
    
    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $doc = $test_case->text();

    $self->logout;
    
    #Result is the latest test case doc hash map
    return $doc;
}

sub store_text
{
    my $self =shift;
    my ($test_case_id, $author_id, $action, $effect, $setup, $breakdown) = @_;

    $self->login;
    
    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $version = $test_case->store_text($test_case_id, $author_id, $action, $effect, $setup, $breakdown);
    
    $self->logout;
    
    # Result is new test case doc version on success, otherwise an exception will be thrown
    return $version;
}

sub get_plans
{
    my $self = shift;
    my ($test_case_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }
    
    my $result = $test_case->plans();

    $self->logout;
    
    # Result is list of test plans for the given test case
    return $result;
}

sub get_bugs
{
    my $self = shift;
    my ($test_case_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }
    
    my $result = $test_case->bugs;

    $self->logout;
    
    # Result is list of bugs for the given test case
    return $result;
}

sub add_component
{
    my $self =shift;
    my ($test_case_id, $component_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $result = $test_case->add_component($component_id);

    $self->logout;
    
    # Result 0 on success, otherwise an exception will be thrown
    return 0;
}

sub remove_component
{
    my $self =shift;
    my ($test_case_id, $component_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $result = $test_case->remove_component($component_id);

    $self->logout;
    
    # Result 0 on success, otherwise an exception will be thrown
    return 0;
}

sub get_components
{
    my $self =shift;
    my ($test_case_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $result = $test_case->components();

    $self->logout;
    
    # Result list of components otherwise an exception will be thrown
    return $result;
}

sub add_tag
{
    my $self =shift;
    my ($test_case_id, $tag_name) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }
    
    $test_case->add_tag($tag_name);

    $self->logout;
    
    # Result 0 on success, otherwise an exception will be thrown
    return 0;
}

sub remove_tag
{
    my $self =shift;
    my ($test_case_id, $tag_name) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $test_tag = Bugzilla::Testopia::TestTag->check_tag($tag_name);
    if (not defined $test_tag)
    {
        $self->logout;
        die "Tag, " . $tag_name . ", does not exist";
    }
    
    my $result = $test_case->remove_tag($test_tag->name);

    $self->logout;
    
    # Result 0 on success, otherwise an exception will be thrown
    return 0;
}

sub get_tags
{
    my $self =shift;
    my ($test_case_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canview)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $result = $test_case->tags;

    $self->logout;
    
    # Result list of tags otherwise an exception will be thrown
    return $result;
}

sub lookup_status_id_by_name
{
    my $self =shift;
    my ($name) = @_;
    
    $self->login;

      my $result = lookup_status_by_name($name);

    $self->logout;

    # Result is test case status id for the given test case status name
    return $result;
}

sub lookup_status_name_by_id
{
    my $self =shift;
    my ($id) = @_;
    
    $self->login;

     
    my $result = lookup_status($id);

    $self->logout;

    if (!defined $result) 
    {
      $result = 0;
    };
    
    # Result is test case status name for the given test case status id
    return $result;
}

sub lookup_category_id_by_name
{
    my $self =shift;
    my ($name) = @_;
    
    $self->login;

      my $result = lookup_category_by_name($name);

    $self->logout;

    # Result is test case category id for the given test case category name
    return $result;
}

sub lookup_category_name_by_id
{
    my $self =shift;
    my ($id) = @_;
    
    $self->login;

    my $result = lookup_category($id);

    $self->logout;

    if (!defined $result) 
    {
      $result = 0;
    };
    
    # Result is test case category name for the given test case category id
    return $result;
}

sub lookup_priority_id_by_name
{
    my $self =shift;
    my ($name) = @_;
    
    $self->login;

      my $result = lookup_priority_by_value($name);

    $self->logout;

    # Result is test case priority id for the given test case priority name
    return $result;
}

sub lookup_priority_name_by_id
{
    my $self =shift;
    my ($id) = @_;
    
    $self->login;

    my $result = lookup_priority($id);

    $self->logout;

    if (!defined $result) 
    {
      $result = 0;
    };
    
    # Result is test case priority name for the given test case priority id
    return $result;
}

sub link_plan
{
    my $self =shift;
    my ($test_case_id, $test_plan_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    $test_case->link_plan($test_plan_id);
    
    my $result = $test_case->plans;

    $self->logout;
    
    # Result is list of plans for test case on success, otherwise an exception will be thrown
    return $result;
}

sub unlink_plan
{
    my $self =shift;
    my ($test_case_id, $test_plan_id) = @_;

    $self->login;

    my $test_case = new Bugzilla::Testopia::TestCase($test_case_id);

    if (not defined $test_case)
    {
        $self->logout;
        die "Testcase, " . $test_case_id . ", not found"; 
    }
    
    if (not $test_case->canedit)
    {
        $self->logout;
        die "User Not Authorized";
    }

    my $rtn_code = $test_case->unlink_plan($test_plan_id);
    
    if ($rtn_code == 0)
    {
        $self->logout;
        die "User Can Not Unlink Plan, " . $test_plan_id;
    }
    
    my $result = $test_case->plans;

    $self->logout;
    
    # Result is list of plans for test case on success, otherwise an exception will be thrown
    return $result;
}


1;