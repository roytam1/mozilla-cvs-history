rem * Table to hold valid bug resolution values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table resolution cascade constraints;

create table resolution (
	value	VARCHAR2(255) CONSTRAINT RESOL_PK_VALUE PRIMARY KEY NOT NULL
);

rem insert into resolution (value) values ('NOTABUG');
rem insert into resolution (value) values ('WONTFIX');
rem insert into resolution (value) values ('DEFERRED');
rem insert into resolution (value) values ('WORKSFORME');
rem insert into resolution (value) values ('CURRENTRELEASE');
rem insert into resolution (value) values ('RAWHIDE');
rem insert into resolution (value) values ('ERRATA');
rem insert into resolution (value) values ('DUPLICATE');

exit;
