rem * Table to hold valid bug resolution values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table resolution cascade constraints;

create table resolution (
	value	VARCHAR2(255) CONSTRAINT RESOL_PK_VALUE PRIMARY KEY NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index RESOL_PK_VALUE rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

rem insert into resolution (value) values ('NOTABUG');
rem insert into resolution (value) values ('WONTFIX');
rem insert into resolution (value) values ('DEFERRED');
rem insert into resolution (value) values ('WORKSFORME');
rem insert into resolution (value) values ('CURRENTRELEASE');
rem insert into resolution (value) values ('RAWHIDE');
rem insert into resolution (value) values ('ERRATA');
rem insert into resolution (value) values ('DUPLICATE');

exit;
