rem create script for fielddefs table
rem Cotributed by David Lawrence <dkl@redhat.com>

drop table fielddefs;

create table fielddefs (
	fieldid		INTEGER		CONSTRAINT FIELDDEF_PK_ID	PRIMARY KEY NOT NULL,
	name		VARCHAR2(64)	CONSTRAINT FIELDDEF_NN_NAME	NOT NULL,
	description	VARCHAR2(255)	CONSTRAINT FIELDDEF_NN_DESC	NOT NULL,
	mailhead	INTEGER		DEFAULT ('0'), 
	sortkey		INTEGER		CONSTRAINT FIELDDEF_NN_SORT	NOT NULL
);

create index fielddefs_index on fielddefs (sortkey);

drop sequence fielddefs.seq;

create sequence fielddefs_seq START WITH 1 INCREMENT BY 1;

exit;
	
