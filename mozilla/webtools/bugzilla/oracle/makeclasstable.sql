rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table class cascade constraints;

create table class (
	value	VARCHAR(255)	CONSTRAINT CLASS_PK_VALUE PRIMARY KEY	NOT NULL
);

rem insert into class (value) values ('install/upgrade');
rem insert into class (value) values ('packaging');
rem insert into class (value) values ('functionality');
rem insert into class (value) values ('security');
rem insert into class (value) values ('documentation');

exit;
