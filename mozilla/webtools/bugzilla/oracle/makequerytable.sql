rem * Table to hold valid member queries in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table queries ;
drop index queries_index;

create table queries (
	userid 		INTEGER		CONSTRAINT QUERY_NN_USRID 	NOT NULL,
	query_name	VARCHAR2(255)	CONSTRAINT QUERY_NN_NAME 	NOT NULL,
	query		LONG		CONSTRAINT QUERY_NN_QUERY	NOT NULL
);

create index queries_index on queries (query_name);

exit;
