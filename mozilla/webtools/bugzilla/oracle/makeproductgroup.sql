rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table product_group;

create table product_group (
	productid 		INTEGER			CONSTRAINT PRODGROUP_NN_PRODID		NOT NULL,
	groupid 		INTEGER			CONSTRAINT PRODGROUP_NN_GROUPID 	NOT NULL
);

create index prodgroup_index on product_group (productid, groupid);

exit;

