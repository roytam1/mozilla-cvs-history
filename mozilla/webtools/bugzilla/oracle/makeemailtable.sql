rem * Table to hold valid email choices values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table emailnotification cascade constraints;

create table emailnotification (
	value	VARCHAR(255)	CONSTRAINT EMAIL_PK_VALUE PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

alter index EMAIL_PK_VALUE rebuild tablespace eng_index01
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into emailnotification (value) values ('ExcludeSelfChanges');
rem insert into emailnotification (value) values ('CConly');
rem insert into emailnotification (value) values ('All');

exit;
