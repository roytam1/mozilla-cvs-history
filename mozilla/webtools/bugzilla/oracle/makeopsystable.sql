rem * Table to hold valid op sys values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table op_sys cascade constraints;

create table op_sys (
	value	VARCHAR2(255)	CONSTRAINT OPSYS_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into op_sys (value) values ('Linux');

exit;
