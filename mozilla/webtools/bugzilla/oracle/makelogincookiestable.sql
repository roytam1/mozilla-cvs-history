rem * Table to hold login cookie information for user authentification
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table logincookies;
drop index login_index;
drop sequence login_seq;

create table logincookies (
    cookie 		  INTEGER 		CONSTRAINT LOGIN_PK_COOKIE PRIMARY KEY,
    userid 		  INTEGER 		CONSTRAINT LOGIN_NN_USERID NOT NULL,
    cryptpassword VARCHAR2(64),
    hostname 	  VARCHAR2(128),
    lastused 	  DATE
);

create index login_index on logincookies (lastused);

create sequence login_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
