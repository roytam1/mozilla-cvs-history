rem * Table to hold all the valid members of bugzilla and their information
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table profiles; 

create table profiles (
	userid 			INTEGER 	CONSTRAINT PROFILE_PK_USRID 	PRIMARY KEY,
	login_name 		VARCHAR2(255) 	CONSTRAINT PROFILE_NN_LOGIN 	NOT NULL,
	password 		VARCHAR2(16)	CONSTRAINT PROFILE_NN_PASSWD	NOT NULL,
	cryptpassword 		VARCHAR2(64)	CONSTRAINT PROFILE_NN_CRYPT	NOT NULL,	
	realname 		VARCHAR2(255)	DEFAULT(''),
	groupid 		INTEGER 	DEFAULT(0),
	groupset 		INTEGER		DEFAULT(0),
	emailnotification 	VARCHAR2(30)    CONSTRAINT PROFILE_FK_EMAIL REFERENCES emailnotification(value),	
	disabledtext		VARCHAR2(255)   DEFAULT(''),
	newemailtech 		INTEGER		DEFAULT('0'),
	mybugslink		INTEGER		DEFAULT('1')
);

create index profiles_index on profiles (login_name);

drop sequence profiles_seq;
create sequence profiles_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
