rem * Table to hold valid rep platform values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table rep_platform cascade constraints;

create table rep_platform (
	value	VARCHAR(255)	CONSTRAINT PLATFM_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into rep_platform (value) values ('All');
rem insert into rep_platform (value) values ('i386');
rem insert into rep_platform (value) values ('alpha');
rem insert into rep_platform (value) values ('sparc');
rem insert into rep_platform (value) values ('noarch');

exit;
