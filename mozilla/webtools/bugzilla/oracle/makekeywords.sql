rem table to hold keywords
rem Contributed by David Lawrence <dkl@redhat.com>

drop table keywords;
drop index keywords_index;

create table keywords (
	bug_id		INTEGER		CONSTRAINT KEYWORDS_NN_BUGID	NOT NULL,
	keywordid	INTEGER		CONSTRAINT KEYWORDS_NN_KEYID	NOT NULL
);

create index keywords_index on keywords (bug_id, keywordid);

exit;
