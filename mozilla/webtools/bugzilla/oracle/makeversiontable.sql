rem * Table to hold valid product versions in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table versions;

create table versions (
	value 	VARCHAR2(255)	CONSTRAINT VERS_NN_VALUE 	NOT NULL,
	program VARCHAR2(255)	CONSTRAINT VERS_NN_PROGM	NOT NULL
);

rem insert into versions (value, program) values ('other', 'TestProduct');

exit;
