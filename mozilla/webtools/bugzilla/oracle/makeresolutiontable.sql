rem * Table to hold valid bug resolution values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table resolution;

create table resolution (
	id 		INTEGER			CONSTRAINT RESO_PK_ID		PRIMARY KEY,
	value	VARCHAR2(255)
);

rem insert into resolution (id, value) values ('1', 'NOTABUG');
rem insert into resolution (id, value) values ('2', 'WONTFIX');
rem insert into resolution (id, value) values ('3', 'DEFERRED');
rem insert into resolution (id, value) values ('4', 'WORKSFORME');
rem insert into resolution (id, value) values ('5', 'CURRENTRELEASE');
rem insert into resolution (id, value) values ('6', 'RAWHIDE');
rem insert into resolution (id, value) values ('7', 'ERRATA');
rem insert into resolution (id, value) values ('8', 'DUPLICATE');
rem insert into resolution (id, value) values ('9', '');

exit;
