rem * Table to hold valid product versions in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table versions;

create table versions (
	value 	VARCHAR2(255)	CONSTRAINT VERS_NN_VALUE 	NOT NULL,
	program VARCHAR2(255)	CONSTRAINT VERS_NN_PROGM	NOT NULL
)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

rem insert into versions (value, program) values ('other', 'TestProduct');

exit;
