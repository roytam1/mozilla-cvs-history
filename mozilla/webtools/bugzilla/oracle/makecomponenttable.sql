rem Table to hold component information divided by product
rem Contributed by David Lawrence <dkl@redhat.com>

drop table components;

create table components (
	value 				VARCHAR2(255)	CONSTRAINT COMP_NN_VALUE  NOT NULL,
	program 			VARCHAR2(255) 	CONSTRAINT COMP_NN_PROGRM NOT NULL,
	initialowner 		VARCHAR2(64) 	CONSTRAINT COMP_NN_INTOWN NOT NULL,	
	devowner	        VARCHAR2(64),
	initialqacontact 	VARCHAR2(64), 
	description 		VARCHAR2(2000)
);

rem insert into components (value, program, initialowner, description) 
rem 	values ('TestComponent', 'TestProduct', 'dkl@redhat.com', 
rem 		'This is a test component in the test product database.  This ought to be blown away and replaced with real stuffrem  in a finished installation of bugzilla.');

exit;
