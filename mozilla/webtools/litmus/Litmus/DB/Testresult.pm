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

package Litmus::DB::Testresult;

use strict;
use base 'Litmus::DBI';

use Class::DBI::Pager;
use Date::Manip;
use Litmus::DB::TestRun;

our $_num_results_default = 15;

Litmus::DB::Testresult->table('test_results');

Litmus::DB::Testresult->columns(All => qw/testresult_id testcase_id last_updated submission_time user_id opsys_id branch_id build_id user_agent result_status_id build_type_id machine_name exit_status_id duration_ms talkback_id valid vetted validated_by_user_id vetted_by_user_id validated_timestamp vetted_timestamp locale_abbrev is_automated_result/);
Litmus::DB::Testresult->columns(Essential => qw/testresult_id testcase_id last_updated submission_time user_id opsys_id branch_id build_id user_agent result_status_id build_type_id machine_name exit_status_id duration_ms talkback_id valid vetted validated_by_user_id vetted_by_user_id validated_timestamp vetted_timestamp locale_abbrev is_automated_result/);
Litmus::DB::Testresult->utf8_columns(qw/user_agent machine_name locale_abbrev summary created platform_name product_name result_class class_name result_status_class branch_name email iconpath/);
Litmus::DB::Testresult->columns(TEMP => qw /summary created platform_name product_name result_status class_name result_status_class branch_name email num_results most_recent max_id iconpath/);

Litmus::DB::Testresult->column_alias("testcase_id", "testcase");
Litmus::DB::Testresult->column_alias("submission_time", "timestamp");
Litmus::DB::Testresult->column_alias("submission_time", "created");
Litmus::DB::Testresult->column_alias("user_id", "user");
Litmus::DB::Testresult->column_alias("opsys_id", "opsys");
Litmus::DB::Testresult->column_alias("branch_id", "branch");
Litmus::DB::Testresult->column_alias("user_agent", "useragent");
Litmus::DB::Testresult->column_alias("result_status_id", "result_status");
Litmus::DB::Testresult->column_alias("build_type_id", "build_type");
Litmus::DB::Testresult->column_alias("exit_status_id", "exit_status");
Litmus::DB::Testresult->column_alias("validity_id", "validity");
Litmus::DB::Testresult->column_alias("vetting_status_id", "vetting_status");
Litmus::DB::Testresult->column_alias("validated_by_user_id", "validated_by_user");
Litmus::DB::Testresult->column_alias("vetted_by_user_id", "vetted_by_user");
Litmus::DB::Testresult->column_alias("locale_abbrev", "locale");
Litmus::DB::Testresult->column_alias("is_automated_result", "isAutomated");

Litmus::DB::Testresult->has_a(opsys => "Litmus::DB::Opsys");
Litmus::DB::Testresult->has_a(branch => "Litmus::DB::Branch");
Litmus::DB::Testresult->has_a(testcase => "Litmus::DB::Testcase");
Litmus::DB::Testresult->has_a(result_status => "Litmus::DB::ResultStatus");
Litmus::DB::Testresult->has_a(user => "Litmus::DB::User");
Litmus::DB::Testresult->has_a(useragent => "Litmus::UserAgentDetect");
Litmus::DB::Testresult->has_a(build_type => "Litmus::DB::BuildType");
Litmus::DB::Testresult->has_a(exit_status => "Litmus::DB::ExitStatus");
Litmus::DB::Testresult->has_a(locale => "Litmus::DB::Locale");
Litmus::DB::Testresult->has_a(platform => 
                              [ "Litmus::DB::Opsys" => "platform" ]);
Litmus::DB::Testresult->has_a(vetted_by_user => "Litmus::DB::User");
Litmus::DB::Testresult->has_a(validated_by_user => "Litmus::DB::User");

Litmus::DB::Testresult->has_many(logs => 
						  ["Litmus::DB::LogTestresult" => 'log_id']);
Litmus::DB::Testresult->has_many(comments => "Litmus::DB::Comment", {order_by => 'comment_id ASC, submission_time ASC'});
Litmus::DB::Testresult->has_many(bugs => "Litmus::DB::Resultbug", {order_by => 'bug_id ASC, submission_time DESC'});

#Litmus::DB::Testresult->autoinflate(dates => 'Time::Piece');

Litmus::DB::Testresult->set_sql(Completed => qq{
    SELECT tr.* 
    FROM test_results tr
    WHERE tr.testcase_id=? AND 
        tr.build_id=? AND 
        tr.locale_abbrev=? AND
        tr.opsys_id=?
    ORDER BY tr.submission_time DESC
});

Litmus::DB::Testresult->set_sql(CompletedByUser => qq{
    SELECT tr.* 
    FROM test_results tr
    WHERE tr.testcase_id=? AND 
        tr.build_id=? AND 
        tr.locale_abbrev=? AND
        tr.opsys_id=? AND
        tr.user_id=?
    ORDER BY tr.submission_time DESC
});

Litmus::DB::Testresult->set_sql(CompletedByTrusted => qq{
    SELECT tr.*
    FROM test_results tr, users u,
      user_group_map ugm, security_groups sg
    WHERE tr.testcase_id=? AND 
        tr.build_id=? AND 
        tr.locale_abbrev=? AND
        tr.opsys_id=? AND
        tr.user_id=u.user_id AND 
        u.user_id=ugm.user_id AND
        ugm.group_id=sg.group_id AND
        (sg.grouptype=1 OR sg.grouptype=3)
    ORDER BY tr.submission_time DESC
});

Litmus::DB::Testresult->set_sql(NumResultsByUserDays => qq {
	SELECT COUNT(*) AS num_results
		FROM test_results
	WHERE
		user_id = ? AND
		submission_time > DATE_SUB(NOW(), interval ? day)
});

#########################################################################
# for historical reasons, note() is a shorthand way of saying "the text of 
# the first comment on this result if that comment was submitted by the 
# result submitter"
#########################################################################
sub note {
    my $self = shift;
    
    my @comments = $self->comments();
    
    if (@comments && $comments[0] &&
        $comments[0]->user() == $self->user()) {
        return $comments[0]->comment();
    } else {
        return undef;
    }
}

#########################################################################
# is this test result from a trusted user?
#########################################################################
sub istrusted {
  my $self = shift; 
  
   if ($self->user()->istrusted()) {
    return 1;
  } else {
    return 0;
  }
}

#########################################################################
# &getDefaultTestResults($)
#
#########################################################################
sub getDefaultTestResults($$) {
    my $self = shift;
    my $page = shift || 1;

    my $sql = qq{
SELECT DISTINCT(tr.testresult_id),tr.testcase_id,t.summary,
       tr.submission_time AS created,pl.name AS platform_name,
       pr.name as product_name,trsl.name AS result_status,
       trsl.class_name AS result_status_class,b.name AS branch_name,
       tr.locale_abbrev, u.email, pl.iconpath
FROM test_results tr, testcases t, platforms pl, opsyses o, branches b,
     products pr, test_result_status_lookup trsl, users u
WHERE tr.valid=1 AND tr.testcase_id=t.testcase_id AND tr.opsys_id=o.opsys_id AND
    o.platform_id=pl.platform_id AND tr.branch_id=b.branch_id AND
    b.product_id=pr.product_id AND tr.result_status_id=trsl.result_status_id AND
    tr.user_id=u.user_id
ORDER BY tr.submission_time DESC, t.testcase_id DESC
    };

    my $dbh = Litmus::DBI->db_ReadOnly();
    my $sth = $dbh->prepare($sql);
    $sth->execute();
    my $pager = __PACKAGE__->pager($_num_results_default,$page);
    my @rows = $pager->sth_to_objects($sth);
    my $criteria = "Default<br/>Ordered by Created<br/>Limit to $_num_results_default results per page";

    return $criteria, \@rows, $pager;
}

#########################################################################
# &getTestResults($\@\@$)
# 
######################################################################### 
sub getTestResults($\@\@$) {
    my ($self,$where_criteria,$order_by_criteria,$limit_value,$page) = @_;

    if (!$limit_value or $limit_value eq '') {
        $limit_value = "$_num_results_default";
    }

    if (!$page) {
        $page = 1;
    }
    
    my $select = 'SELECT DISTINCT(tr.testresult_id),tr.testcase_id,t.summary,tr.submission_time AS created,pl.name AS platform_name,pr.name as product_name,trsl.name AS result_status,trsl.class_name AS result_status_class,b.name AS branch_name,tg.name AS test_group_name, tr.locale_abbrev, u.email, pl.iconpath';
    
    my $from = 'FROM test_results tr, testcases t, platforms pl, opsyses o, branches b, products pr, test_result_status_lookup trsl, testgroups tg, subgroups sg, users u, testcase_subgroups tcsg, subgroup_testgroups sgtg';
    
    my $where = 'WHERE tr.testcase_id=t.testcase_id AND tr.opsys_id=o.opsys_id AND o.platform_id=pl.platform_id AND tr.branch_id=b.branch_id AND b.product_id=pr.product_id AND tr.result_status_id=trsl.result_status_id AND tcsg.testcase_id=tr.testcase_id AND tcsg.subgroup_id=sg.subgroup_id AND sg.subgroup_id=sgtg.subgroup_id AND sgtg.testgroup_id=tg.testgroup_id AND tr.user_id=u.user_id';
    
    foreach my $criterion (@$where_criteria) {
        $criterion->{'value'} =~ s/'/\\\'/g;
        if ($criterion->{'field'} eq 'product') {
            $where .= " AND pr.product_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'product_name') {
            $where .= " AND pr.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'branch') {
            $where .= " AND b.branch_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'branch_name') {
            $where .= " AND b.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'testgroup') {
            $where .= " AND tg.testgroup_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'testgroup_name') {
            $where .= " AND tg.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'subgroup') {
            $where .= " AND sg.subgroup_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'subgroup_name') {
            $where .= " AND sg.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'testcase' or
                 $criterion->{'field'} eq 'testcase_id') {
            $where .= " AND tr.testcase_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'platform') {
            $where .= " AND pl.platform_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'platform_name') {
            $where .= " AND pl.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'opsys') {
            $where .= " AND o.opsys_id=" . $criterion->{'value'};
        } elsif ($criterion->{'field'} eq 'opsys_name') {
            $where .= " AND o.name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'locale') {
            $where .= " AND tr.locale_abbrev='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'result_status') {
            $where .= " AND trsl.class_name='" . $criterion->{'value'} . "'";
        } elsif ($criterion->{'field'} eq 'summary') {
            $where .= ' AND t.summary LIKE \'%%' . $criterion->{'value'} . '%%\'';
        } elsif ($criterion->{'field'} eq 'email') {
            $where .= ' AND u.email LIKE \'%%' . $criterion->{'value'} . '%%\'';
        } elsif ($criterion->{'field'} eq 'trusted_only') {            
            if ($criterion->{'value'} == 1 or $criterion->{'value'} eq 'on') {
                $from .= ", user_group_map ugm, ";
                $from .= "security_groups secgps, group_product_map gpm";
                
                $where .= qq{ AND u.user_id=ugm.user_id AND ((
                ugm.group_id=secgps.group_id AND
                 secgps.grouptype=1)  OR 
                 (gpm.product_id=pr.product_id AND 
                 gpm.group_id=secgps.group_id AND
                 secgps.grouptype=3))};
            } if ($criterion->{'value'} == 0 or $criterion->{'value'} eq 'off') {
            	$from .= ", user_group_map ugm, security_groups secgps";
                $where .= ' AND (ugm.group_id != secgps.group_id)';
            }
        } elsif ($criterion->{'field'} eq 'vetted_only') {
            if ($criterion->{'value'} ne 'all') {
                $where .= " AND tr.vetted=";
                $where .= $criterion->{'value'} == 1 ? '1' : '0';
            }
        } elsif ($criterion->{'field'} eq 'valid_only') {        
            if ($criterion->{'value'} ne 'all') {
                $where .= " AND tr.valid=";
                $where .= $criterion->{'value'} == 1 ? '1' : '0';
            }   
        } elsif ($criterion->{'field'} eq 'automated') {        
            if ($criterion->{'value'} ne 'all') {
                $where .= " AND tr.is_automated_result=";
                $where .= $criterion->{'value'} == 1 ? '1' : '0';
            }
        } elsif ($criterion->{'field'} eq 'withbugs') {        
            if ($criterion->{'value'} ne 'all') {
              if ($criterion->{'value'} eq '1') {
                $from .= ", test_result_bugs trb";
                $where .= " AND tr.testresult_id=trb.test_result_id";
              } else {
                $from =~ s/test_results tr([ ,])/test_results tr LEFT JOIN test_result_bugs trb ON (tr.testresult_id=trb.test_result_id)$1/;
                $where .= " AND trb.bug_id IS NULL";                
              }
            }
        } elsif ($criterion->{'field'} eq 'user_id') {        
            if ($from !~ /users u/) {
                $from .= ", users u";
            }
            $where .= " AND u.user_id=" . $criterion->{'value'};            
        } elsif ($criterion->{'field'} eq 'start_date') {
            my $start_timestamp = &Date::Manip::UnixDate(&Date::Manip::ParseDateString($criterion->{'value'}),"%q");
            if ($start_timestamp !~ /^\d\d\d\d\d\d\d\d\d\d\d\d\d\d$/) {
                Litmus::Error::logError("Unable to parse a valid start date from '$criterion->{'value'},' ignoring.",
                                        caller(0));
            } else {
                $where .= " AND tr.submission_time>=$start_timestamp";
            }
        } elsif ($criterion->{'field'} eq 'end_date') {
            my $end_timestamp = &Date::Manip::UnixDate(&Date::Manip::ParseDateString($criterion->{'value'}),"%q");
            if ($end_timestamp !~ /^\d\d\d\d\d\d\d\d\d\d\d\d\d\d$/) {
                Litmus::Error::logError("Unable to parse a valid end date from '$criterion->{'value'},' ignoring.",
                                        caller(0));;
            } else {
                $where .= " AND tr.submission_time<=$end_timestamp";
            }
        } elsif ($criterion->{'field'} eq 'vetted_start_date') {
            my $start_timestamp = &Date::Manip::UnixDate(&Date::Manip::ParseDateString($criterion->{'value'}),"%q");
            if ($start_timestamp !~ /^\d\d\d\d\d\d\d\d\d\d\d\d\d\d$/) {
                Litmus::Error::logError("Unable to parse a valid start date from '$criterion->{'value'},' ignoring.",
                                        caller(0));
            } else {
                $where .= " AND tr.vetted_timestamp>=$start_timestamp";
            }
        } elsif ($criterion->{'field'} eq 'vetted_end_date') {
            my $end_timestamp = &Date::Manip::UnixDate(&Date::Manip::ParseDateString($criterion->{'value'}),"%q");
            if ($end_timestamp !~ /^\d\d\d\d\d\d\d\d\d\d\d\d\d\d$/) {
                Litmus::Error::logError("Unable to parse a valid end date from '$criterion->{'value'},' ignoring.",
                                        caller(0));;
            } else {
                $where .= " AND tr.vetted_timestamp<=$end_timestamp";
            }    
        } elsif ($criterion->{'field'} eq 'timespan') {
            next if ($criterion->{'value'} eq 'all');
            my $day_delta = $criterion->{'value'};
            my $err;
            my $timestamp = 
                &Date::Manip::UnixDate(&Date::Manip::DateCalc("now",
                                                              "$day_delta days",
                                                              \$err),
                                       "%q");
            $where .= " AND tr.submission_time>=$timestamp";

        } elsif ($criterion->{'field'} eq 'search_field') {
            ($from,$where) = &_processSearchField($criterion,$from,$where);
        } elsif ($criterion->{'field'} eq 'has_comments') {
          $from =~ s/test_results tr([ ,])/test_results tr INNER JOIN test_result_comments trc ON tr.testresult_id=trc.test_result_id$1/;
       } elsif ($criterion->{'field'} eq 'test_run') {
            # First check to make sure test run exists.
            my $test_run = Litmus::DB::TestRun->retrieve($criterion->{'value'});
            if ($test_run) {
                # We have to match against *ALL* the test run parameters. 
                $from .= ", test_run_testgroups truntg, test_runs trun";
                $where .= " AND trun.test_run_id=" . $criterion->{'value'};
                $where .= " AND trun.test_run_id=truntg.test_run_id";
                $where .= " AND truntg.testgroup_id=sgtg.testgroup_id";
                $where .= " AND tr.submission_time>=trun.start_timestamp";
                $where .= " AND tr.submission_time<trun.finish_timestamp";
                $where .= " AND trun.product_id=pr.product_id";
                $where .= " AND trun.branch_id=b.branch_id";
                $where .= " AND trun.branch_id=tr.branch_id";
            
                $where .= $test_run->getCriteriaSql();
            }
        } else {
            # Skip unknown field
        }
    }

    my $group_by = 'GROUP BY tr.testresult_id';

    my $order_by = 'ORDER BY ';
    foreach my $criterion (@$order_by_criteria) {
        # Skip empty fields.
        next if (!$criterion or !$criterion->{'field'});

        if ($criterion->{'field'} eq 'created') {
            $order_by .= "tr.submission_time $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'vetted_date') {
            $order_by .= "tr.vetted_timestamp $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'product') {
            $order_by .= "pr.name $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'platform') {
            $order_by .= "pl.name $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'test_group') {
            $order_by .= "tg.name $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'testcase_id') {
            $order_by .= "tr.testcase_id $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'summary') {
            $order_by .= "t.summary $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'result_status') {
            $order_by .= "trsl.class_name $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'branch') {
            $order_by .= "b.name $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'locale') {
            $order_by .= "tr.locale_abbrev $criterion->{'direction'},";
        } elsif ($criterion->{'field'} eq 'email') {
            $order_by .= "u.email $criterion->{'direction'},";
        } else {
            # Skip unknown field
        }
    }
    if ($order_by eq 'ORDER BY ') {
        $order_by .= 'tr.submission_time DESC';
    } else {
        chop($order_by);
    }
  
    my $sql = "$select $from $where $group_by $order_by";
    Litmus::Error::logError($sql, caller(0)) if $Litmus::Config::DEBUG;
    my $dbh = Litmus::DBI->db_ReadOnly();
    my $sth = $dbh->prepare($sql);
    $sth->execute();
    my $pager = __PACKAGE__->pager($limit_value,$page);
    my @rows = $pager->sth_to_objects($sth);
    
    return \@rows, $pager;
}

#########################################################################
# &_processSearchField(\%\$\$)
# 
######################################################################### 
sub _processSearchField(\%\$\$) {
    my ($search_field,$from,$where) = @_;
 
    my $table_field = "";
    if ($search_field->{'search_field'} eq 'build_id') {
        $table_field='tr.build_id';
    } elsif ($search_field->{'search_field'} eq 'comment') {
        $table_field='trc.comment';
        $from =~ s/test_results tr([ ,])/test_results tr INNER JOIN test_result_comments trc ON tr.testresult_id=trc.test_result_id$1/;
    } elsif ($search_field->{'search_field'} eq 'locale') {
        $table_field='tr.locale_abbrev';        
    } elsif ($search_field->{'search_field'} eq 'opsys') {
        $table_field='o.name';
    } elsif ($search_field->{'search_field'} eq 'platform') {
        $table_field='pl.name';
    } elsif ($search_field->{'search_field'} eq 'product') {
        $table_field='pr.name';
    } elsif ($search_field->{'search_field'} eq 'result_status') {
        $table_field='trsl.name';        
    } elsif ($search_field->{'search_field'} eq 'subgroup') {
        $table_field='sg.name';        
    } elsif ($search_field->{'search_field'} eq 'email') {
        if ($from !~ /users u/) {
            $from .= ", users u";
            $where .= " AND tr.user_id=u.user_id";
        }
        $table_field='u.email';        
    } elsif ($search_field->{'search_field'} eq 'summary') {
        $table_field='t.summary';
    } elsif ($search_field->{'search_field'} eq 'test_group') {
        $table_field='tg.name';        
    } elsif ($search_field->{'search_field'} eq 'user_agent') {
        $table_field='tr.user_agent';        
    } else {
        return ($from,$where);
    }

    if ($search_field->{'match_criteria'} eq 'contains_all' or
        $search_field->{'match_criteria'} eq 'contains_any' or
        $search_field->{'match_criteria'} eq 'not_contain_any') {
        
        my $join = "";
        if ($search_field->{'match_criteria'} eq 'contains_all') {
            $join = 'AND';
        } else {
            $join = 'OR';
        }

        my @words = split(/ /,$search_field->{'value'});
        if ($search_field->{'match_criteria'} eq 'not_contain_any') {
            $where .= " AND NOT (";
        } else {
            $where .= " AND (";
        }
        my $first_pass = 1;
        foreach my $word (@words) {
            if ( $first_pass ) {
                $where .= "UPPER($table_field) LIKE UPPER('%%" . $word . "%%')";
                $first_pass = 0;
            } else {
                $where .= " $join UPPER($table_field) LIKE UPPER('%%" . $word . "%%')";
            }
        }
        $where .= ")";        
    } elsif ($search_field->{'match_criteria'} eq 'contains') {
        $where .= " AND UPPER($table_field) LIKE UPPER('%%" . $search_field->{'value'} . "%%')";
    } elsif ($search_field->{'match_criteria'} eq 'contains_case') {
        $where .= " AND $table_field LIKE '%%" . $search_field->{'value'} . "%%'";
    } elsif ($search_field->{'match_criteria'} eq 'not_contain') {
        $where .= " AND UPPER($table_field) NOT LIKE UPPER('%%" . $search_field->{'value'} . "%%')";
    } elsif ($search_field->{'match_criteria'} eq 'regexp') {
        $where .= " AND $table_field REGEXP '" . $search_field->{'value'} . "'";        
    } elsif ($search_field->{'match_criteria'} eq 'not_regexp') {        
        $where .= " AND $table_field NOT REGEXP '" . $search_field->{'value'} . "'";        
    } else {
      # Ignore unknown match criteria.
      return ($from,$where);
    }

    return ($from,$where);
}

#########################################################################
# &getCommonResults($$$$)
#########################################################################
sub getCommonResults($$$$) {
    my ($self,$status,$limit_value,$page) = @_;
    
    if (!$status) {
      return undef;
    }
    
    if (!$limit_value) {
        $limit_value = $_num_results_default;
    }

    if (!$page) {
        $page = 1;
    }

    my $sql =  qq{
SELECT COUNT(tr.testcase_id) AS num_results, tr.testresult_id, tr.testcase_id,
       t.summary, MAX(tr.submission_time) AS most_recent,
       MAX(tr.testresult_id) AS max_id 
FROM test_results tr, testcases t, test_result_status_lookup trsl 
WHERE trsl.class_name=? AND
      tr.result_status_id=trsl.result_status_id AND
      tr.testcase_id=t.testcase_id
GROUP BY tr.testcase_id 
ORDER BY num_results DESC, tr.testresult_id DESC
};

    my $dbh = Litmus::DBI->db_ReadOnly();
    my $sth = $dbh->prepare($sql);
    my $pager = __PACKAGE__->pager($limit_value,$page);
    $sth->execute($status);
    my @rows = $pager->sth_to_objects($sth);
    
    return \@rows, $pager;
}

#########################################################################
sub getL10nAggregateResults($\%) {
    my ($self,$args) = @_;
 
    my $l10n;
    
    # If the user has specified a testday rather than a start/finish time,
    # lookup the testday info so we can use the start/finish time from it.
    if ($args->{'testday_id'}) {
        my $testday = Litmus::DB::TestDay->retrieve($args->{'testday_id'});
        if (!$testday) {
            $l10n->{'error'} = 'Unable to retrieve testday info.';
            return $l10n;
        }
        $l10n->{'testday'} = $testday;
        $args->{'start_timestamp'} = $testday->{'start_timestamp'};
        $args->{'finish_timestamp'} = $testday->{'finish_timestamp'};
    }
    
    my $locale_where="";
    if ($args->{'locale'}) {
        $locale_where = " AND tr.locale_abbrev='" . quotemeta($args->{'locale'}) . "'";
    }
    
    # 1. Get # test results per locale.
    my $select = "SELECT tr.locale_abbrev,count(tr.testresult_id)";
    my $from = "FROM test_results tr, testcases tc";
    my $where = "WHERE tc.product_id=? AND tc.branch_id=? AND tr.testcase_id=tc.testcase_id AND tr.submission_time>=? AND tr.submission_time<=? $locale_where";
    my $group_by = "GROUP BY tr.locale_abbrev";

    my $sql = "$select $from $where $group_by";

    my $dbh = Litmus::DBI->db_ReadOnly();
    my $sth = $dbh->prepare($sql);
    $sth->execute($args->{"product_id"},
                  $args->{"branch_id"},
                  $args->{"start_timestamp"},
                  $args->{"finish_timestamp"},
                  );  
    my $num_locales = 0;
    while (my ($locale_abbrev,$num_results) = $sth->fetchrow_array) {
        $l10n->{$locale_abbrev}->{'num_results'} = $num_results;
        # Setup placeholders
        $l10n->{$locale_abbrev}->{'num_pass'} = 0;
        $l10n->{$locale_abbrev}->{'num_fail'} = 0;
        $l10n->{$locale_abbrev}->{'num_unclear'} = 0;        
        $l10n->{$locale_abbrev}->{'num_testers'} = 0;
        $num_locales++;
    }
    $sth->finish();
    
    # No results? Don't bother looking up the rest.
    if ($num_locales == 0) {
      $l10n->{'error'} = 'No results';
      return $l10n;
    }

    # 2-4. Get # passes/fails/unclear per locale
    my $status_select = "SELECT tr.locale_abbrev,count(distinct(tr.testcase_id))";
    my $status_where = $where;
    $status_where .= " AND tr.result_status_id=?";
    $sql = "$status_select $from $status_where $group_by";
    $sth = $dbh->prepare($sql);
    my %statuses = (
                    'pass' => 1,
                    'fail' => 2,
                    'unclear' => 3
                   );
    foreach my $status (keys %statuses) {
        $sth->execute($args->{"product_id"},
                      $args->{"branch_id"},
                      $args->{"start_timestamp"},
                      $args->{"finish_timestamp"},
                      $statuses{$status}
                      );  
        while (my ($locale_abbrev,$num_results) = $sth->fetchrow_array) {
            $l10n->{$locale_abbrev}->{'num_' . $status} = $num_results;
        }
        $sth->finish();        
    }
    
    # 5. Get # testers per locale
    my $user_select = "SELECT tr.locale_abbrev,count(distinct(tr.user_id))";
    $sql = "$user_select $from $where $group_by";
    $sth = $dbh->prepare($sql);
    $sth->execute($args->{"product_id"},
                  $args->{"branch_id"},
                  $args->{"start_timestamp"},
                  $args->{"finish_timestamp"},
                  );  
    while (my ($locale_abbrev,$num_testers) = $sth->fetchrow_array) {
        $l10n->{$locale_abbrev}->{'num_testers'} = $num_testers;
    }
    $sth->finish();
    return $l10n;
}

1;
