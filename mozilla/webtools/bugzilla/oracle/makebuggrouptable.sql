rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_group;

create table bug_group (
	bugid 		INTEGER			CONSTRAINT BUGGROUP_NN_BUGID	NOT NULL,
	groupid 	INTEGER			CONSTRAINT BUGGROUP_NN_GROUPID 	NOT NULL
);

create index buggroup_index on bug_group (bugid, groupid);

exit;
