rem Table to hold list of cc persons for particular bug
rem Contributed by David Lawrence <dkl@redhat.com>

drop table cc;

create table cc (
    bug_id 	INTEGER CONSTRAINT CC_NN_BUGID NOT NULL,
    who 	INTEGER CONSTRAINT CC_NN_WHO   NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

create index cc_index on cc (bug_id, who)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01;

exit;

