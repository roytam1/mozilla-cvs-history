rem * Table to hold valid bug status values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_status cascade constraints;

create table bug_status (
	value	VARCHAR(255)	CONSTRAINT STATUS_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into bug_status (value) values ('NEW');
rem insert into bug_status (value) values ('VERIFIED');
rem insert into bug_status (value) values ('ASSIGNED');
rem insert into bug_status (value) values ('REOPENED');
rem insert into bug_status (value) values ('RESOLVED');
rem insert into bug_status (value) values ('CLOSED');

exit;
