rem Drops all tables in bugzilla
rem Contributed by David Lawrence

drop table bugs_activity;
drop table attachments;
drop sequence attachments_seq;
drop table bug_group;
drop table bug_status cascade constraints;
drop table bugs;
drop sequence bugs_seq;
drop table cc;
drop table components;
drop table dependencies;
drop table emailnotification cascade constraints;
drop table errata;
drop sequence errata_seq;
drop table fielddefs;
drop sequence fielddefs_seq;
drop table groups;
drop sequence groups_seq;
drop table keyworddefs;
drop table keywords;
drop table logincookies;
drop sequence logincookies_seq;
drop table longdescs;
drop table milestones;
drop table namedqueries ;
drop table news;
drop sequence news_seq;
drop table op_sys cascade constraints;
drop table priority cascade constraints;
drop table product_group;
drop table products;
drop sequence products_seq;
drop table profiles; 
drop sequence profiles_seq;
drop table rep_platform cascade constraints;
drop table resolution cascade constraints;
drop table bug_severity cascade constraints;
drop table user_group;
drop table versions;
drop table votes;

exit;
