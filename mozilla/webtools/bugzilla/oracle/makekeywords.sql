rem table to hold keywords
rem Contributed by David Lawrence <dkl@redhat.com>

drop table keywords;

create table keywords (
	bug_id		INTEGER		CONSTRAINT KEYWORDS_NN_BUGID	NOT NULL,
	keywordid	INTEGER		CONSTRAINT KEYWORDS_NN_KEYID	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;


create index keywords_index on keywords (bug_id, keywordid)
        Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
        tablespace eng_index02;

exit;
