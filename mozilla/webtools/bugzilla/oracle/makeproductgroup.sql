rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table product_group;

create table product_group (
	productid 		INTEGER		CONSTRAINT PRODGROUP_NN_PRODID		NOT NULL,
	groupid 		INTEGER		CONSTRAINT PRODGROUP_NN_GROUPID 	NOT NULL
	)
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

create index prodgroup_index on product_group (productid, groupid)
        Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256)
        tablespace eng_index02;

exit;

