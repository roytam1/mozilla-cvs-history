rem * Table to hold valid op sys values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table op_sys;

create table op_sys (
	id 		INTEGER			CONSTRAINT OPSYS_PK_ID    PRIMARY KEY,
	value	VARCHAR2(255)	CONSTRAINT OPSYS_NN_VALUE NOT NULL
);

rem insert into op_sys (id, value) values ('1', 'Linux');

exit;
