rem * Table to hold all the valid members of bugzilla and their information
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table profiles; 
drop index profiles_index;
drop sequence profiles_seq;

create table profiles (
	userid 			INTEGER 	CONSTRAINT PROFILE_PK_USRID 	PRIMARY KEY,
	login_name 		VARCHAR2(255) 	CONSTRAINT PROFILE_NN_LOGIN 	NOT NULL,
	password 		VARCHAR2(16)	CONSTRAINT PROFILE_NN_PASSWD	NOT NULL,
	cryptpassword 		VARCHAR2(64)	CONSTRAINT PROFILE_NN_CRYPT	NOT NULL,	
	realname 		VARCHAR2(255),
	groupid 		INTEGER 	DEFAULT(0),
	groupset 		INTEGER		DEFAULT(0),
	emailnotification 	VARCHAR2(30)	DEFAULT('ExcludeSelfChanges'),
	disabledtext		VARCHAR2(255),
	newemailtech 		INTEGER,
	mybugslink		INTEGER		DEFAULT('1')
);

create sequence profiles_seq NOCACHE START WITH 1 INCREMENT BY 1;

create index profiles_index on profiles (login_name);

exit;
