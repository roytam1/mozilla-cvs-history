rem * Table to hold bug vote information in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table votes;

create table votes (
	who	INTEGER CONSTRAINT VOTE_NN_WHO NOT NULL,
	bug_id  INTEGER	CONSTRAINT VOTE_NN_BUGID NOT NULL,
	count	INTEGER CONSTRAINT VOTE_NN_COUNT NOT NULL
);

exit;
