rem * Table to hold valid member queries in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table namedqueries ;

create table namedqueries (
     userid         INTEGER         CONSTRAINT NAMED_NN_USRID not null,
     name           VARCHAR2(255)   CONSTRAINT NAMED_NN_NAME  not null,
     watchfordiffs  INTEGER         DEFAULT(0) not null,
     linkinfooter   INTEGER         DEFAULT(0) not null,
     query          LONG            CONSTRAINT NAMED_NN_QUERY not null
);

create index queries_index on namedqueries (name);

exit;
