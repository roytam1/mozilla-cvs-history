rem table to hold keyword definitions
rem Contributed by David Lawrence <dkl@redhat.com>

drop table keyworddefs;

create table keyworddefs (
	id		INTEGER		CONSTRAINT KEYDEF_PK_ID		PRIMARY KEY NOT NULL,
	name		VARCHAR2(64)	CONSTRAINT KEYDEF_NN_NAME	NOT NULL,
	description VARCHAR2(2000) 
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index KEYDEF_PK_ID rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

exit;
