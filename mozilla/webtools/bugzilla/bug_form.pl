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
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>

use diagnostics;
use strict;

use RelationSet;

# Use the Attachment module to display attachments for the bug.
use Attachment;

sub show_bug {    
    # Shut up misguided -w warnings about "used only once".  For some reason,
    # "use vars" chokes on me when I try it here.
    sub bug_form_pl_sillyness {
        my $zz;
        $zz = %::FORM;
        $zz = %::proddesc;
        $zz = %::prodmaxvotes;
        $zz = @::enterable_products;                                            
        $zz = @::settable_resolution;
        $zz = $::unconfirmedstate;
        $zz = $::milestoneurl;
        $zz = $::template;
        $zz = $::vars;
        $zz = @::legal_priority;
        $zz = @::legal_platform;
        $zz = @::legal_severity;
        $zz = @::legal_bug_status;
        $zz = @::target_milestone;
        $zz = @::components;
        $zz = @::legal_keywords;
        $zz = @::versions;
        $zz = @::legal_opsys;
    }

    # Use templates
    my $template = $::template;
    my $vars = $::vars;
    
    $vars->{'GetBugLink'} = \&GetBugLink;
    $vars->{'quoteUrls'} = \&quoteUrls,
    $vars->{'lsearch'} = \&lsearch,
    $vars->{'header_done'} = (@_),

    quietly_check_login();

    my $id = $::FORM{'id'};
    
    if (!defined($id)) {
      $template->process("bug/choose.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
      exit;
    }
    
    my %user = %{$vars->{'user'}};
    my %bug;

    # Populate the bug hash with the info we get directly from the DB.
    my $query = "
    SELECT bugs.bug_id, alias, product, version, rep_platform, 
        op_sys, bug_status, resolution, priority, 
        bug_severity, component, assigned_to, reporter, 
        bug_file_loc, short_desc, target_milestone, 
        qa_contact, status_whiteboard, 
        date_format(creation_ts,'%Y-%m-%d %H:%i'),
        groupset, delta_ts, sum(votes.count)
    FROM bugs LEFT JOIN votes USING(bug_id)
    WHERE bugs.bug_id = $id
    GROUP BY bugs.bug_id";

    SendSQL($query);

    my $value;
    my @row = FetchSQLData();
    foreach my $field ("bug_id", "alias", "product", "version", "rep_platform",
                       "op_sys", "bug_status", "resolution", "priority",
                       "bug_severity", "component", "assigned_to", "reporter",
                       "bug_file_loc", "short_desc", "target_milestone",
                       "qa_contact", "status_whiteboard", "creation_ts",
                       "groupset", "delta_ts", "votes") 
    {
        $value = shift(@row);
        $bug{$field} = defined($value) ? $value : "";
    }

    # General arrays of info about the database state
    GetVersionTable();

    # Fiddle the product list.
    my $seen_curr_prod;
    my @prodlist;
    
    foreach my $product (@::enterable_products) {
        if ($product eq $bug{'product'}) {
            # if it's the product the bug is already in, it's ALWAYS in
            # the popup, period, whether the user can see it or not, and
            # regardless of the disallownew setting.
            $seen_curr_prod = 1;
            push(@prodlist, $product);
            next;
        }

        if (Param("usebuggroupsentry")
          && GroupExists($product)
          && !UserInGroup($product))
        {
            # If we're using bug groups to restrict entry on products, and
            # this product has a bug group, and the user is not in that
            # group, we don't want to include that product in this list.
            next;
        }

        push(@prodlist, $product);
    }

    # The current product is part of the popup, even if new bugs are no longer
    # allowed for that product
    if (!$seen_curr_prod) {
        push (@prodlist, $bug{'product'});
        @prodlist = sort @prodlist;
    }

    $vars->{'product'} = \@prodlist;
    $vars->{'rep_platform'} = \@::legal_platform;
    $vars->{'priority'} = \@::legal_priority;
    $vars->{'bug_severity'} = \@::legal_severity;
    $vars->{'op_sys'} = \@::legal_opsys;
    $vars->{'bug_status'} = \@::legal_bug_status;

    # Hack - this array contains "" for some reason. See bug 106589.
    shift @::settable_resolution; 
    $vars->{'resolution'} = \@::settable_resolution;

    $vars->{'component_'} = $::components{$bug{'product'}};
    $vars->{'version'} = $::versions{$bug{'product'}};
    $vars->{'target_milestone'} = $::target_milestone{$bug{'product'}};
    $bug{'milestoneurl'} = $::milestoneurl{$bug{'product'}} || 
                           "notargetmilestone.html";

    $vars->{'use_votes'} = Param('usevotes')
                           && $::prodmaxvotes{$bug{'product'}} > 0;

    # Add additional, calculated fields to the bug hash
    if (@::legal_keywords) {
        $vars->{'use_keywords'} = 1;

        SendSQL("SELECT keyworddefs.name 
                 FROM keyworddefs, keywords
                 WHERE keywords.bug_id = $id 
                 AND keyworddefs.id = keywords.keywordid
                 ORDER BY keyworddefs.name");
        my @keywords;
        while (MoreSQLData()) {
            push(@keywords, FetchOneColumn());
        }

        $bug{'keywords'} = \@keywords;
    }    

    # Attachments
    $bug{'attachments'} = Attachment::query($id);

    # Dependencies
    my @list;
    SendSQL("SELECT dependson FROM dependencies WHERE  
             blocked = $id ORDER BY dependson");
    while (MoreSQLData()) {
        my ($i) = FetchSQLData();
        push(@list, $i);
    }

    $bug{'dependson'} = \@list;

    my @list2;
    SendSQL("SELECT blocked FROM dependencies WHERE  
             dependson = $id ORDER BY blocked");
    while (MoreSQLData()) {
        my ($i) = FetchSQLData();
        push(@list2, $i);
    }

    $bug{'blocked'} = \@list2;

    # Groups
    my @groups;
    if ($::usergroupset ne '0' || $bug{'groupset'} ne '0') {      
        my $bug_groupset = $bug{'groupset'};

        SendSQL("SELECT bit, name, description, (bit & $bug_groupset != 0),
                 (bit & $::usergroupset != 0) FROM groups 
                 WHERE isbuggroup != 0 " .
                 # Include active groups as well as inactive groups to which
                 # the bug already belongs.  This way the bug can be removed
                 # from an inactive group but can only be added to active ones.
                "AND ((isactive = 1 AND (bit & $::usergroupset != 0)) OR
                 (bit & $bug_groupset != 0))");

        $user{'inallgroups'} = 1;

        while (MoreSQLData()) {
            my ($bit, $name, $description, $ison, $ingroup) = FetchSQLData();
            # For product groups, we only want to display the checkbox if either
            # (1) The bit is already set, or
            # (2) The user is in the group, but either:
            #     (a) The group is a product group for the current product, or
            #     (b) The group name isn't a product name
            # This means that all product groups will be skipped, but 
            # non-product bug groups will still be displayed.
            if($ison || 
               ($ingroup && (($name eq $bug{'product'}) ||
                             (!defined $::proddesc{$name}))))
            {
                $user{'inallgroups'} &= $ingroup;

                push (@groups, { "bit" => $bit,
                                 "ison" => $ison,
                                 "ingroup" => $ingroup,
                                 "description" => $description });            
            }
        }

        # If the bug is restricted to a group, display checkboxes that allow
        # the user to set whether or not the reporter 
        # and cc list can see the bug even if they are not members of all 
        # groups to which the bug is restricted.
        if ($bug{'groupset'} != 0) {
            $bug{'inagroup'} = 1;

            # Determine whether or not the bug is always accessible by the
            # reporter, QA contact, and/or users on the cc: list.
            SendSQL("SELECT reporter_accessible, cclist_accessible
                     FROM   bugs
                     WHERE  bug_id = $id
                    ");
            ($bug{'reporter_accessible'}, 
             $bug{'cclist_accessible'}) = FetchSQLData();        
        }
    }
    $vars->{'groups'} = \@groups;

    my $movers = Param("movers");
    $user{'canmove'} = Param("move-enabled") 
                       && (defined $::COOKIE{"Bugzilla_login"}) 
                       && ($::COOKIE{"Bugzilla_login"} =~ /$movers/);

    # User permissions

    # In the below, if the person hasn't logged in ($::userid == 0), then
    # we treat them as if they can do anything.  That's because we don't
    # know why they haven't logged in; it may just be because they don't
    # use cookies.  Display everything as if they have all the permissions
    # in the world; their permissions will get checked when they log in
    # and actually try to make the change.
    $user{'canedit'} = $::userid == 0
                       || $::userid == $bug{'reporter'}
                       || $::userid == $bug{'qa_contact'}
                       || $::userid == $bug{'assigned_to'}
                       || UserInGroup("editbugs");
    $user{'canconfirm'} = ($::userid == 0)
                          || UserInGroup("canconfirm")
                          || UserInGroup("editbugs");

    # Bug states
    $bug{'isunconfirmed'} = ($bug{'bug_status'} eq $::unconfirmedstate);
    $bug{'isopened'} = IsOpenedState($bug{'bug_status'});

    # People involved with the bug
    $bug{'assigned_to_email'} = DBID_to_name($bug{'assigned_to'});
    $bug{'assigned_to'} = DBID_to_real_or_loginname($bug{'assigned_to'});
    $bug{'reporter'} = DBID_to_real_or_loginname($bug{'reporter'});
    $bug{'qa_contact'} = $bug{'qa_contact'} > 0 ? 
                                          DBID_to_name($bug{'qa_contact'}) : "";

    my $ccset = new RelationSet;
    $ccset->mergeFromDB("SELECT who FROM cc WHERE bug_id=$id");
    
    my @cc = $ccset->toArrayOfStrings();
    $bug{'cc'} = \@cc if $cc[0];

    # Next bug in list (if there is one)
    my @bug_list;
    if ($::COOKIE{"BUGLIST"} && $id) 
    {
        @bug_list = split(/:/, $::COOKIE{"BUGLIST"});
    }
    $vars->{'bug_list'} = \@bug_list;

    $bug{'comments'} = GetComments($bug{'bug_id'});

    # This is length in number of comments
    $bug{'longdesclength'} = scalar(@{$bug{'comments'}});

    # Add the bug and user hashes to the variables
    $vars->{'bug'} = \%bug;
    $vars->{'user'} = \%user;

    # Create the <link> elements for browsing bug lists
    $vars->{'navigation_links'} = navigation_links(join(':',@bug_list));

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process("bug/edit.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}
 
1;

