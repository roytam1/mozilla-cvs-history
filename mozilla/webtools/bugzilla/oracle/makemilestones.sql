rem Create table for storing milestone information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table milestones;

create table milestones (
	value 		VARCHAR2(20) 	CONSTRAINT MILE_NN_VALUE 	NOT NULL,
	product		VARCHAR(64)	CONSTRAINT MILE_NN_PRODUCT	NOT NULL,
	sortkey		INTEGER		CONSTRAINT MILE_NN_SORTKEY  	NOT NULL
);

exit;

