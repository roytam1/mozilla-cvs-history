rem * Table to hold valid user group values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table user_group;

create table user_group (
	userid 		INTEGER			CONSTRAINT USERGROUP_NN_BUGID		NOT NULL,
	groupid 	INTEGER			CONSTRAINT USERGROUP_NN_GROUPID 	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

drop index usergroup_index;

create index usergroup_index on user_group (userid, groupid)
        Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
        tablespace eng_index02;

rem insert into user_group values (1, 1);
rem insert into user_group values (1, 2);
rem insert into user_group values (1, 3);
rem insert into user_group values (1, 4);
rem insert into user_group values (1, 5);
rem insert into user_group values (1, 6);
rem insert into user_group values (1, 7);
rem insert into user_group values (1, 8);
rem insert into user_group values (1, 9);
rem insert into user_group values (1, 10);
rem insert into user_group values (1, 11);
rem insert into user_group values (1, 12);
rem insert into user_group values (1, 13);

exit;
