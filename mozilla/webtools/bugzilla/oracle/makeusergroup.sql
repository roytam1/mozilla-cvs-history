rem * Table to hold valid user group values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table user_group;
drop index usergroup_index;

create table user_group (
	userid 		INTEGER			CONSTRAINT USERGROUP_NN_BUGID		NOT NULL,
	groupid 	INTEGER			CONSTRAINT USERGROUP_NN_GROUPID 	NOT NULL
);

create index usergroup_index on user_group (userid, groupid);

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
