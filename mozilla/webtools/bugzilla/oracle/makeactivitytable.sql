rem Creates the bug activity table
rem Contributed by David Lawrence <dkl@redhat.com>

drop table bugs_activity;

create table bugs_activity (
    bug_id		INTEGER 	CONSTRAINT ACT_NN_BUGID  NOT NULL,
    who 		INTEGER 	CONSTRAINT ACT_NN_WHO    NOT NULL,
    field		VARCHAR2(64),
    fieldid 		INTEGER,   
    bug_when 		DATE 		CONSTRAINT ACT_NN_WHEN   NOT NULL,
    oldvalue 	VARCHAR2(400),
    newvalue  	VARCHAR2(400) 	
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

create index bugact_index on bugs_activity (bug_id, bug_when) 
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01; 

exit;
