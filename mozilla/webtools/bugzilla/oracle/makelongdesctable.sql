rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table longdescs;

create table longdescs (
	bug_id 		INTEGER 	CONSTRAINT LONG_NN_BUGID	NOT NULL,
	who		INTEGER		CONSTRAINT LONG_NN_WHO		NOT NULL,
	bug_when	DATE		CONSTRAINT LONG_NN_WHEN		NOT NULL,
	thetext		LONG
)	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data06;

create index longdescs_index on longdescs (bug_id, bug_when)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index03;


exit;	
