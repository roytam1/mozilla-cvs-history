rem * Table to hold valid priority values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table priority;

create table priority (
	id 		INTEGER			CONSTRAINT PRIORITY_PK_ID    PRIMARY KEY NOT NULL, 
	value	VARCHAR(255)	CONSTRAINT PRIORITY_NN_VALUE NOT NULL
);

rem insert into priority (id, value) values ('1', 'high');
rem insert into priority (id, value) values ('2', 'normal');
rem insert into priority (id, value) values ('3', 'low');
rem insert into priority (id, value) values ('4', 'contract');

exit;
