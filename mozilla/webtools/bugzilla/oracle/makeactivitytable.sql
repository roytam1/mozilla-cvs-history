rem Creates the bug activity table
rem Contributed by David Lawrence <dkl@redhat.com>

drop table bugs_activity;

create table bugs_activity (
    bug_id		INTEGER 		CONSTRAINT ACT_NN_BUGID  NOT NULL,
    who 		INTEGER 		CONSTRAINT ACT_NN_WHO    NOT NULL,
    bug_when 	DATE 			CONSTRAINT ACT_NN_WHEN   NOT NULL,
    field 		VARCHAR2(64) 	CONSTRAINT ACT_NN_FIELD  NOT NULL,
    oldvalue 	VARCHAR2(400) 	,
    newvalue  	VARCHAR2(400) 	
);

create index bugact_index on bugs_activity (bug_id, bug_when);

exit;
