rem * Table to hold valid rep platform values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table rep_platform cascade constraints;

create table rep_platform (
	value	VARCHAR(255)	CONSTRAINT PLATFM_PK_VALUE PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index PLATFM_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into rep_platform (value) values ('All');
rem insert into rep_platform (value) values ('i386');
rem insert into rep_platform (value) values ('alpha');
rem insert into rep_platform (value) values ('sparc');
rem insert into rep_platform (value) values ('noarch');

exit;
