rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_group;

create table bug_group (
	bugid 		INTEGER		CONSTRAINT BUGGROUP_NN_BUGID	NOT NULL,
	groupid 	INTEGER		CONSTRAINT BUGGROUP_NN_GROUPID 	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

create index buggroup_index on bug_group (bugid, groupid)
        Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
        tablespace eng_index02;

exit;
