rem * Table to hold valid bug class values in bugzilla
rem * Contributed by David Lawrence <dkl@redhat.com>

drop table longdescs;

create table longdescs (
	bug_id 		INTEGER 	CONSTRAINT LONG_NN_BUGID	NOT NULL,
	who		INTEGER		CONSTRAINT LONG_NN_WHO		NOT NULL,
	bug_when	DATE		CONSTRAINT LONG_NN_WHEN		NOT NULL,
	thetext		LONG
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

create index longdescs_index on longdescs (bug_id, bug_when)
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
                                        storage(initial 4096K next 2048K pctincrease 0))
;

exit;	
