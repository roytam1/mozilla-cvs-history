rem * Table to hold valid member queries in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table namedqueries ;

create table namedqueries (
     userid         INTEGER         CONSTRAINT NAMED_NN_USRID not null,
     name           VARCHAR2(255)   CONSTRAINT NAMED_NN_NAME  not null,
     watchfordiffs  INTEGER         DEFAULT(0) not null,
     linkinfooter   INTEGER         DEFAULT(0) not null,
     query          LONG            CONSTRAINT NAMED_NN_QUERY not null
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

create index queries_index on namedqueries (name)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index02;

exit;
