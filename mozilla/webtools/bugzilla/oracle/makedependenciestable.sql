rem Table to hold bug report dependency information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table dependencies;

create table dependencies (
    blocked 	INTEGER	CONSTRAINT DEPEND_NN_BLOCKED	NOT NULL,
    dependson 	INTEGER	CONSTRAINT DEPEND_NN_DPNDSON    NOT NULL
);

create index depend_index on dependencies (blocked, dependson);

exit;
