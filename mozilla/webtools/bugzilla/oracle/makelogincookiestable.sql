rem * Table to hold login cookie information for user authentification
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table logincookies;

create table logincookies (
    cookie 		INTEGER 	CONSTRAINT LOGIN_PK_COOKIE PRIMARY KEY,
    userid 		INTEGER 	CONSTRAINT LOGIN_NN_USERID NOT NULL,
    cryptpassword 	VARCHAR2(64),
    hostname 	  	VARCHAR2(128),
    lastused 	  	DATE
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

alter index LOGIN_PK_COOKIE rebuild tablespace eng_index01
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

create index logincookies_index on logincookies (lastused)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01;


drop sequence logincookies_seq;

create sequence logincookies_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
