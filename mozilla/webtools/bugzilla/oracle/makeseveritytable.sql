rem * Table to hold valid bug severity values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_severity cascade constraints;

create table bug_severity (
	value	VARCHAR(255)	CONSTRAINT SEVERE_PK_VALUE  PRIMARY KEY NOT NULL
);

rem insert into bug_severity (value) values ('security');
rem insert into bug_severity (value) values ('high');
rem insert into bug_severity (value) values ('normal');
rem insert into bug_severity (value) values ('low');
rem insert into bug_severity (value) values ('enhancement');

exit;
