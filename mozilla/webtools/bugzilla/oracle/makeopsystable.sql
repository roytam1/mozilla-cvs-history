rem * Table to hold valid op sys values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table op_sys cascade constraints;

create table op_sys (
	value	VARCHAR2(255)	CONSTRAINT OPSYS_PK_VALUE PRIMARY KEY NOT NULL
	)	
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index OPSYS_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into op_sys (value) values ('Linux');

exit;
