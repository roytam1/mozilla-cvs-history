rem * Table to hold valid email choices values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table emailnotification cascade constraints;

create table emailnotification (
	value	VARCHAR(255)	CONSTRAINT EMAIL_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into emailnotification (value) values ('ExcludeSelfChanges');
rem insert into emailnotification (value) values ('CConly');
rem insert into emailnotification (value) values ('All');

exit;
