rem Table to hold attachements to bugs
rem Contributed by David Lawrence <dkl@redhat.com>

drop table attachments;

create table attachments (
	attach_id 	INTEGER 	CONSTRAINT ATTACH_PK_ATTACHID PRIMARY KEY NOT NULL,
	bug_id 		INTEGER 	CONSTRAINT ATTACH_NN_BUGID 		NOT NULL,
	creation_ts 	DATE 		CONSTRAINT ATTACH_NN_CREATION   	NOT NULL,
	description 	VARCHAR2(2000) 	CONSTRAINT ATTACH_NN_DESC		NOT NULL,
	mimetype 	VARCHAR2(255) 	CONSTRAINT ATTACH_NN_MIME		NOT NULL,
	ispatch 	INTEGER 		,
	filename 	VARCHAR2(255) 	CONSTRAINT ATTACH_NN_FILE		NOT NULL,
	thedata 	BLOB		CONSTRAINT ATTACH_NN_DATA		NOT NULL,
	submitter_id 	INTEGER 	CONSTRAINT ATTACH_NN_SUBMIT		NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

alter index ATTACH_PK_ATTACHID rebuild tablespace eng_index01
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);


create index attach_index on attachments (bug_id, creation_ts)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01;

drop sequence attachments_seq;

create sequence attachments_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
