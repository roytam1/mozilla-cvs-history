rem Table to hold list of cc persons for particular bug
rem Contributed by David Lawrence <dkl@redhat.com>

drop table cc;

create table cc (
    bug_id 	INTEGER CONSTRAINT CC_NN_BUGID NOT NULL,
    who 	INTEGER CONSTRAINT CC_NN_WHO   NOT NULL
);

create index cc_index on cc (bug_id, who);

exit;

