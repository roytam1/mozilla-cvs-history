rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table class cascade constraints;

create table class (
	value	VARCHAR(255)	CONSTRAINT CLASS_PK_VALUE PRIMARY KEY	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

alter index CLASS_PK_VALUE rebuild tablespace eng_index01
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);


rem insert into class (value) values ('install/upgrade');
rem insert into class (value) values ('packaging');
rem insert into class (value) values ('functionality');
rem insert into class (value) values ('security');
rem insert into class (value) values ('documentation');

exit;
