rem table to hold keyword definitions
rem Contributed by David Lawrence <dkl@redhat.com>

drop table keyworddefs;

create table keyworddefs (
	id			INTEGER			CONSTRAINT KEYDEF_PK_ID		PRIMARY KEY NOT NULL,
	name		VARCHAR2(64)	CONSTRAINT KEYDEF_NN_NAME	NOT NULL,
	description VARCHAR2(2000) 
);

exit;
