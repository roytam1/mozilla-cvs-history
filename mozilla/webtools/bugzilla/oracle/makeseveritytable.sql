rem * Table to hold valid bug severity values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_severity cascade constraints;

create table bug_severity (
	value	VARCHAR(255)	CONSTRAINT SEVERE_PK_VALUE  PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index SEVERE_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into bug_severity (value) values ('security');
rem insert into bug_severity (value) values ('high');
rem insert into bug_severity (value) values ('normal');
rem insert into bug_severity (value) values ('low');
rem insert into bug_severity (value) values ('enhancement');

exit;
