rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table longdescs;

create table longdescs (
	bug_id 		INTEGER 	CONSTRAINT LONG_NN_BUGID	NOT NULL,
	who			INTEGER		CONSTRAINT LONG_NN_WHO		NOT NULL,
	bug_when	DATE		CONSTRAINT LONG_NN_WHEN		NOT NULL,
	thetext		LONG
);

create index longdescs_index on longdescs (bug_id, bug_when);

exit;	
