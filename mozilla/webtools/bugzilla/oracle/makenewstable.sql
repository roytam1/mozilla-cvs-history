rem Creates news table for articles
rem Contributed by David Lawrence

drop table news;

create table news (
	id		INTEGER			CONSTRAINT NEWS_PK_ID	PRIMARY KEY NOT NULL, 	
	add_date	DATE			CONSTRAINT NEWS_NN_DATE	NOT NULL,
	headline	VARCHAR2(2000)		CONSTRAINT NEWS_NN_HEAD NOT NULL,
	story		LONG			CONSTRAINT NEWS_NN_STORY NOT NULL
	)	
	Storage(initial 4096k next 2048k pctincrease 0
		minextents 1
		maxextents 256)
	tablespace eng_data01;

alter index NEWS_PK_ID rebuild tablespace eng_index02
	Storage(initial 2048k next 1024k pctincrease 0
                minextents 1
                maxextents 256);

drop sequence news_seq;

create sequence news_seq START WITH 1 INCREMENT BY 1;

exit; 	
