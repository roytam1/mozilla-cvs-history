rem * Table to hold valid bug status values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_status cascade constraints;

create table bug_status (
	value	VARCHAR(255)	CONSTRAINT STATUS_PK_VALUE PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index STATUS_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);
;

rem insert into bug_status (value) values ('NEW');
rem insert into bug_status (value) values ('VERIFIED');
rem insert into bug_status (value) values ('ASSIGNED');
rem insert into bug_status (value) values ('REOPENED');
rem insert into bug_status (value) values ('RESOLVED');
rem insert into bug_status (value) values ('CLOSED');

exit;
