rem Table to hold bugzilla group information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table groups;

create table groups (
    bit		INTEGER		DEFAULT('0'),
    groupid 	INTEGER 	CONSTRAINT GROUPS_PK_GROUPID 	PRIMARY KEY NOT NULL,
    name 	VARCHAR2(255) 	CONSTRAINT GROUPS_NN_NAME 	NOT NULL,
    description VARCHAR2(2000) 	CONSTRAINT GROUPS_NN_DESC 	NOT NULL,
    isbuggroup 	INTEGER		CONSTRAINT GROUPS_NN_BUGGRP 	NOT NULL,
    userregexp 	VARCHAR2(255)   DEFAULT(''),
    contract 	INTEGER         DEFAULT(0) 
);

drop sequence groups_seq;
create sequence groups_seq start with 1 increment by 1;

rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (1, groups_seq.nextval, 'tweakparams', 'Can tweak operating parameters', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (2, groups_seq.nextval, 'editgroupmembers', 'Can put people in and out of groups that they are members of.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (4, groups_seq.nextval, 'creategroups', 'Can create and destroy groups.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (8, groups_seq.nextval, 'editcomponents', 'Can create, destroy, and edit components.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (16, groups_seq.nextval, 'support', 'Red Hat Technical Supprt', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (32, groups_seq.nextval, 'qa', 'Red Hat Quality Assurance', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (64, groups_seq.nextval, 'devel', 'Red Hat Development', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (128, groups_seq.nextval, 'marketing', 'Red Hat Marketing', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (256, groups_seq.nextval, 'web', 'Red Hat Web Group', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (512, groups_seq.nextval, 'beta', 'Red Hat Beta Program', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (1024, groups_seq.nextval, 'intel', 'Intel Confidential Group', 1, '', 1);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (2048, groups_seq.nextval, 'errata', 'Red Hat Errata Group', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (4096, groups_seq.nextval, 'setcontract', 'Can set a bug report to contract priority', 1, '', 0);

exit;
