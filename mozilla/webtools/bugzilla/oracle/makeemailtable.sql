rem * Table to hold valid email choices values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table emailnotification;

create table emailnotification (
	id 		INTEGER			CONSTRAINT EMAIL_PK_ID		PRIMARY KEY NOT NULL,
	value	VARCHAR(255)	CONSTRAINT EMAIL_NN_VALUE NOT NULL
);

rem insert into emailnotification (id, value) values ('1', 'ExcludeSelfChanges');
rem insert into emailnotification (id, value) values ('2', 'CConly');
rem insert into emailnotification (id, value) values ('3', 'All');

exit;
