rem * Table to hold valid bug status values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_status;

create table bug_status (
	id 		INTEGER			CONSTRAINT STATUS_PK_ID		PRIMARY KEY NOT NULL,
	value	VARCHAR(255)	CONSTRAINT STATUS_NN_VALUE	NOT NULL
);

rem insert into bug_status (id, value) values ('1', 'NEW');
rem insert into bug_status (id, value) values ('2', 'VERIFIED');
rem insert into bug_status (id, value) values ('3', 'ASSIGNED');
rem insert into bug_status (id, value) values ('4', 'REOPENED');
rem insert into bug_status (id, value) values ('5', 'RESOLVED');
rem insert into bug_status (id, value) values ('6', 'CLOSED');

exit;
