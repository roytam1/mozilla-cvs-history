rem create script for fielddefs table
rem Cotributed by David Lawrence <dkl@redhat.com>

drop table fielddefs;


create table fielddefs (
	fieldid		INTEGER		CONSTRAINT FIELDDEF_PK_ID	PRIMARY KEY NOT NULL,
	name		VARCHAR2(64)	CONSTRAINT FIELDDEF_NN_NAME	NOT NULL,
	description	VARCHAR2(255)	CONSTRAINT FIELDDEF_NN_DESC	NOT NULL,
	mailhead	INTEGER		DEFAULT ('0'), 
	sortkey		INTEGER		CONSTRAINT FIELDDEF_NN_SORT	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index FIELDDEF_PK_ID rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);


drop index fielddefs_index;

create index fielddefs_index on fielddefs (sortkey)
        Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
        tablespace eng_index02;

drop sequence fielddefs_seq;

create sequence fielddefs_seq START WITH 1 INCREMENT BY 1;

exit;
	
