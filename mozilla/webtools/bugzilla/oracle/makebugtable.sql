rem The big bugs table
rem Contributed by David Lawrence <dkl@redhat.com>

drop table bugs;
drop index bugs_index;
drop sequence bugs_seq;

create table bugs (
	bug_id 			INTEGER		CONSTRAINT BUGS_NN_BUGID  NOT NULL,
	groupset		INTEGER		DEFAULT('0'),
	group_id		INTEGER		DEFAULT('0'),
	assigned_to 		INTEGER	 	CONSTRAINT BUGS_NN_ASSITO NOT NULL, 
	bug_file_loc 		VARCHAR2(255)	DEFAULT(''),
	patch_file_loc		VARCHAR2(255)	DEFAULT(''),
	bug_severity 		VARCHAR2(64)   	CONSTRAINT BUGS_NN_SEVRTY NOT NULL,
	bug_status 		VARCHAR2(64)   	CONSTRAINT BUGS_NN_STATUS NOT NULL,
	bug_view		INTEGER		DEFAULT('0'),
	creation_ts 		DATE 		CONSTRAINT BUGS_NN_CRTETS NOT NULL,
	delta_ts 		DATE,
	short_desc 		VARCHAR2(4000) 	CONSTRAINT BUGS_NN_SHORT  NOT NULL,
	op_sys 			VARCHAR2(64)   	DEFAULT('Linux'),
	priority                VARCHAR2(64)    CONSTRAINT BUGS_NN_PRIRTY NOT NULL,
	product 		VARCHAR2(256)   CONSTRAINT BUGS_NN_PRODCT NOT NULL,
	rep_platform            VARCHAR2(64)    CONSTRAINT BUGS_NN_PLATFM NOT NULL,
	reporter 		INTEGER		CONSTRAINT BUGS_NN_REPRTR NOT NULL,
	version 		VARCHAR2(64)   	CONSTRAINT BUGS_NN_VERSN  NOT NULL,
	release			VARCHAR2(64)	DEFAULT(''),
	component 		VARCHAR2(64)   	CONSTRAINT BUGS_NN_COMPNT NOT NULL,
	resolution              VARCHAR2(64)    DEFAULT(''),
	class			VARCHAR2(255)	DEFAULT(''),
	target_milestone 	VARCHAR2(64)	DEFAULT(''),
	qa_contact 		VARCHAR2(255)	DEFAULT(''),
	status_whiteboard 	VARCHAR2(4000)	DEFAULT(''),
	votes			INTEGER		DEFAULT(''),
	keywords		VARCHAR(255)	DEFAULT(''),
	lastdiffed		DATE		
	)
	Storage( initial 4096 next 2048 minextents 1 maxextents 256)
		PARTITION BY range (bug_id)
			(partition PD1 values less than (4000)
				tablespace eng_data03
					storage(initial 4096K next 2048K pctincrease 0),
			partition PD2 values less than (8000)
				tablespace eng_data04
					storage(initial 4096K next 2048K pctincrease 0),
			partition PD3 values less than (12000)
				tablespace eng_data05
					storage(initial 4096K next 2048K pctincrease 0),
			partition PD4 values less than (MAXVALUE)
				tablespace eng_data06
					storage(initial 4096K next 2048K pctincrease 0));

Create index bugs_pk_id
        on bugs(bug_id)
        local
                ( partition PI1
                        tablespace eng_index06
                                storage(initial 4096K next 2048K pctincrease 0),
                         partition PI2
                                tablespace eng_index05
                                        storage(initial 4096K next 2048K pctincrease 0),
                        partition PI3
                                tablespace eng_index04
                                        storage(initial 4096K next 2048K pctincrease 0),
                        partition PI4
                                tablespace eng_index03
                                        storage(initial 4096K next 2048K pctincrease 0));

alter table bugs add constraint BUGS_PK_BUGID primary key (bug_id);

create sequence bugs_seq NOCACHE START WITH 1 INCREMENT BY 1;

create index bugs_index on bugs (assigned_to, 
				 creation_ts,
				 delta_ts,
				 bug_severity,
				 bug_status,
				 op_sys,
				 priority,
				 product,
				 reporter,
				 version,
				 component,
				 resolution,
				 target_milestone,
				 qa_contact)
        local
                ( partition PI1
                        tablespace eng_index06
                                storage(initial 4096K next 2048K pctincrease 0),
                         partition PI2
	                        tablespace eng_index05
                                        storage(initial 4096K next 2048K pctincrease 0),
                        partition PI3
                                tablespace eng_index04
                                        storage(initial 4096K next 2048K pctincrease 0),
                        partition PI4
                                tablespace eng_index03
                                        storage(initial 4096K next 2048K pctincrease 0));

exit;
