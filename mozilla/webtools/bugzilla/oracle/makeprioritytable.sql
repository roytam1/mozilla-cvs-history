rem * Table to hold valid priority values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table priority cascade constraints;

create table priority (
	value	VARCHAR(255)	CONSTRAINT PRIORITY_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into priority (value) values ('high');
rem insert into priority (value) values ('normal');
rem insert into priority (value) values ('low');
rem insert into priority (value) values ('contract');

exit;
