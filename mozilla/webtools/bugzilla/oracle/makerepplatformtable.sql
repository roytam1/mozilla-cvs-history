rem * Table to hold valid rep platform values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table rep_platform;

create table rep_platform (
	id 		INTEGER			CONSTRAINT PLATFM_PK_ID    PRIMARY KEY,
	value	VARCHAR(255)	CONSTRAINT PLATFM_NN_VALUE NOT NULL
);

rem insert into rep_platform (id, value) values ('1', 'All');
rem insert into rep_platform (id, value) values ('2', 'i386');
rem insert into rep_platform (id, value) values ('3', 'alpha');
rem insert into rep_platform (id, value) values ('4', 'sparc');
rem insert into rep_platform (id, value) values ('5', 'noarch');

exit;
