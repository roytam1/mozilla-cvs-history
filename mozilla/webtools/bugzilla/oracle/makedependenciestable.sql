rem Table to hold bug report dependency information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table dependencies;

create table dependencies (
    blocked 	INTEGER	CONSTRAINT DEPEND_NN_BLOCKED	NOT NULL,
    dependson 	INTEGER	CONSTRAINT DEPEND_NN_DPNDSON    NOT NULL
)	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data02;

create index depend_index on dependencies (blocked, dependson)
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
	tablespace eng_index01;



exit;
