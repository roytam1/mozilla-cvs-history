rem Table to hold bugzilla group information
rem Contributed by David Lawrence <dkl@redhat.com>

drop table groups;

create table groups (
    bit		INTEGER		DEFAULT('0'),
    groupid 	INTEGER 	CONSTRAINT GROUPS_PK_GROUPID 	PRIMARY KEY NOT NULL,
    name 	VARCHAR2(255) 	CONSTRAINT GROUPS_NN_NAME 	NOT NULL,
    description VARCHAR2(2000) 	CONSTRAINT GROUPS_NN_DESC 	NOT NULL,
    isbuggroup 	INTEGER		CONSTRAINT GROUPS_NN_BUGGRP 	NOT NULL,
    userregexp 	VARCHAR2(255),
    contract 	INTEGER         CONSTRAINT GROUPS_NN_CONTACT    NOT NULL
);

rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (1, 1, 'tweakparams', 'Can tweak operating parameters', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (2, 2, 'editgroupmembers', 'Can put people in and out of groups that they are members of.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (4, 3, 'creategroups', 'Can create and destroy groups.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (8, 4, 'editcomponents', 'Can create, destroy, and edit components.', 0, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (16, 5, 'support', 'Red Hat Technical Supprt', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (32, 6, 'qa', 'Red Hat Quality Assurance', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (64, 7, 'devel', 'Red Hat Development', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (128, 8, 'marketing', 'Red Hat Marketing', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (256, 9, 'web', 'Red Hat Web Group', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (512, 10, 'beta', 'Red Hat Beta Program', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (1024, 11, 'intel', 'Intel Confidential Group', 1, '', 1);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (2048, 12, 'errata', 'Red Hat Errata Group', 1, '', 0);
rem insert into groups (bit, groupid, name, description, isbuggroup, userregexp, contract) values (4096, 13, 'setcontract', 'Can set a bug report to contract priority', 1, '', 0);

exit;
