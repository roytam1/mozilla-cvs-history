rem * Table to hold valid priority values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table priority cascade constraints;

create table priority (
	value	VARCHAR(255)	CONSTRAINT PRIORITY_PK_VALUE PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index PRIORITY_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into priority (value) values ('high');
rem insert into priority (value) values ('normal');
rem insert into priority (value) values ('low');
rem insert into priority (value) values ('contract');

exit;
