rem Create table for storing errata information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table errata;

create table errata (
	revision		INTEGER		DEFAULT('0'),
	type			VARCHAR2(10)	CONSTRAINT ERRATA_NN_TYPE	NOT NULL,
	issue_date		DATE		CONSTRAINT ERRATA_NN_ISSUE	NOT NULL,
	updated_on		DATE		,
	id			INTEGER		CONSTRAINT ERRATA_PK_ID		PRIMARY KEY NOT NULL,
	synopsis		VARCHAR2(2000)	CONSTRAINT ERRATA_NN_SYNOP	NOT NULL,
	mail			INTEGER		DEFAULT('0'),
	files			INTEGER		DEFAULT('0'),
	valid			INTEGER
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index ERRATA_PK_ID rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

drop sequence errata_seq;

create sequence errata_seq START WITH 1 INCREMENT BY 1;

exit;

