rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table class;

create table class (
	id 	INTEGER		CONSTRAINT CLASS_PK_ID 		PRIMARY KEY,
	value	VARCHAR(255)	CONSTRAINT CLASS_NN_VALUE	NOT NULL
);

rem insert into class (id, value) values ('1', 'install/upgrade');
rem insert into class (id, value) values ('2', 'packaging');
rem insert into class (id, value) values ('3', 'functionality');
rem insert into class (id, value) values ('4', 'security');
rem insert into class (id, value) values ('5', 'documentation');

exit;
