rem * Table to hold login cookie information for user authentification
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table logincookies;

create table logincookies (
    cookie 		  INTEGER 		CONSTRAINT LOGIN_PK_COOKIE PRIMARY KEY,
    userid 		  INTEGER 		CONSTRAINT LOGIN_NN_USERID NOT NULL,
    cryptpassword VARCHAR2(64),
    hostname 	  VARCHAR2(128),
    lastused 	  DATE
);

create index logincookies_index on logincookies (lastused);

drop sequence logincookies_seq;
create sequence logincookies_seq NOCACHE START WITH 1 INCREMENT BY 1;

exit;
