rem * Table to hold bug vote information in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table votes;

create table votes (
	who	INTEGER CONSTRAINT VOTE_NN_WHO NOT NULL,
	bug_id  INTEGER	CONSTRAINT VOTE_NN_BUGID NOT NULL,
	count	INTEGER CONSTRAINT VOTE_NN_COUNT NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;


exit;
