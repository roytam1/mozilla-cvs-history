rem * Table to hold valid bug severity values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table bug_severity;

create table bug_severity (
	id 		INTEGER			CONSTRAINT SEVERE_PK_ID     PRIMARY KEY,
	value	VARCHAR(255)	CONSTRAINT SEVERE_NN_VALUE  NOT NULL
);

rem insert into bug_severity (id, value) values ('1', 'security');
rem insert into bug_severity (id, value) values ('2', 'high');
rem insert into bug_severity (id, value) values ('3', 'normal');
rem insert into bug_severity (id, value) values ('4', 'low');
rem insert into bug_severity (id, value) values ('5', 'enhancement');

exit;
