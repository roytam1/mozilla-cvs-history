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
	emailnotification 	VARCHAR2(30)    CONSTRAINT PROFILE_NN_EMAIL NOT NULL,
	disabledtext		VARCHAR2(255)   DEFAULT(''),
	newemailtech 		INTEGER		DEFAULT('0'),
	mybugslink		INTEGER		DEFAULT('1')
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

alter index PROFILE_PK_USRID rebuild tablespace eng_index01
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

create index profiles_index on profiles (login_name)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01;


drop sequence profiles_seq;

create sequence profiles_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
