rem * Table to hold the current list of products in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table products;

create table products (
	product 		VARCHAR2(255) 	CONSTRAINT PRODUCT_NN_PRODUCT 	NOT NULL,
	description 	VARCHAR2(2000)	CONSTRAINT PRODUCT_NN_DESC 		NOT NULL,
	milestoneurl 	VARCHAR2(255),
	disallownew 	VARCHAR2(255),
	votesperuser	INTEGER,
	id				INTEGER			CONSTRAINT PRODUCT_PK_ID 		PRIMARY KEY NOT NULL 
);

drop sequence product_seq;
create sequence product_seq NOCACHE START WITH 1 INCREMENT BY 1;

rem insert into products(product, description, id) values ('TestProduct', 'This is a test product.  This ought to be blown away and replaced with real stuff in a finished installation of bugzilla.', product_seq.nextval);

exit;
