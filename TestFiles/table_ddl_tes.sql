/*
  regular table
*/
--simple table test
SELECT * FROM bonus;
SELECT * FROM user_tablespaces;
SELECT * FROM user_all_tables WHERE table_name = 'BONUS';
SELECT * FROM all_tab_columns WHERE column_name = 'FREELISTS';

DROP TABLE bonus PURGE;
--all defualt parms
CREATE TABLE bonus (
  ename VARCHAR2(10) NOT NULL,
  job   VARCHAR2(9)  DEFAULT 'NA' NULL,
  sal   NUMBER       DEFAULT 0 NULL,
  comm  NUMBER       NULL
)
TABLESPACE custom
/

/*
  show COMMIT PRESERVE/DELETE ROWS in Object Viewer
  add TABLESPACE to temp tables!
*/
DROP TABLE temp_table_1;
CREATE GLOBAL TEMPORARY TABLE temp_table_1
(
  zip_code     VARCHAR2(5),
  by_user      VARCHAR2(30),
  entry_date   DATE
)
  ON COMMIT PRESERVE ROWS
  --TABLESPACE temp
/

CREATE INDEX temp_table_1_i1
  ON temp_table_1 (zip_code)
/

DROP TABLE temp_table_2 PURGE;
CREATE GLOBAL TEMPORARY TABLE temp_table_2
(
  zip_code     VARCHAR2(5),
  by_user      VARCHAR2(30),
  entry_date   DATE
)
  ON COMMIT DELETE ROWS
  --TABLESPACE temp
/

/*
  iot table
  8i failed to get DDL
  why do we call load partition here?
*/

DROP TABLE iot_table PURGE;
CREATE TABLE iot_table
(
  Token           CHAR(20),
  Doc_id          NUMBER,
  Token_frequency NUMBER,
  Token_offsets   VARCHAR(512),
  CONSTRAINT pk_iot_table PRIMARY KEY (Token, Doc_id)
)
ORGANIZATION INDEX TABLESPACE custom INCLUDING Token_frequency
OVERFLOW TABLESPACE custom;

CREATE INDEX iot_table_i1 ON iot_table(doc_id);

/*
  partitioned table
*/

DROP TABLE sales_interval PURGE;
CREATE TABLE sales_interval
(
  time_id    DATE,
  product_id NUMBER(6)
)
  STORAGE (
    INITIAL      64 K
    NEXT       1024 K
  )
PARTITION BY RANGE (time_id)
--INTERVAL(NUMTOYMINTERVAL(1, 'MONTH')) -- introduced in 11g
(
  PARTITION t0 VALUES LESS THAN (TO_DATE('1-1-2005','DD-MM-YYYY')) TABLESPACE custom,
  PARTITION t1 VALUES LESS THAN (TO_DATE('1-1-2006','DD-MM-YYYY')) PCTFREE 30,
  PARTITION t2 VALUES LESS THAN (TO_DATE('1-7-2006','DD-MM-YYYY')) TABLESPACE custom,
  PARTITION t3 VALUES LESS THAN (TO_DATE('1-1-2007','DD-MM-YYYY')) PCTFREE 30
);

CREATE INDEX sales_interval_i1 ON sales_interval(time_id) LOCAL;


DROP TABLE pt_iot_list PURGE;
CREATE TABLE pt_iot_list (
  acct_no        NUMBER(5,0)    NOT NULL,
  acct_name      VARCHAR2(30)   NOT NULL,
  amount_of_sale NUMBER(6,0)    NULL,
  state_cd       VARCHAR2(2)    NOT NULL,
  sale_details   VARCHAR2(1000) NULL,
  PRIMARY KEY (
    acct_no,
    acct_name,
    state_cd
  )
)
  ORGANIZATION INDEX INCLUDING state_cd
  OVERFLOW
  PARTITION BY LIST (state_cd)
  (
    PARTITION reg_east    VALUES ('MA', 'NY', 'CT', 'NH'),
    PARTITION reg_west    VALUES ('CA', 'AZ', 'NM', 'OR'),
    PARTITION reg_south   VALUES ('TX', 'KY', 'TN', 'GA'),
    PARTITION reg_central VALUES ('OH', 'ND', 'SD', 'IA'),
    PARTITION reg_null    VALUES (NULL),
    PARTITION reg_unknown VALUES (DEFAULT)
  )
/

/*
  subpartitioned table
*/

DROP TABLE sub_example_81;
CREATE TABLE sub_example_81
      (deptno number, item_no varchar2(20),
       txn_date date, txn_amount number, state varchar2(2))
  PARTITION BY RANGE (txn_date)
    SUBPARTITION BY HASH (state)
      (
        PARTITION q1_1999 VALUES LESS THAN (TO_DATE('1-APR-1999','DD-MON-YYYY'))
          SUBPARTITIONS 16
      );

CREATE INDEX sub_example_81_i1 ON sub_example_81(deptno) LOCAL;

CREATE TABLE quarterly_regional_sales
(deptno NUMBER,  item_no VARCHAR2(20),
 txn_date DATE,  txn_amount NUMBER,  state VARCHAR2(2))
PARTITION BY RANGE (txn_date)
SUBPARTITION BY LIST (state)
SUBPARTITION TEMPLATE(
SUBPARTITION northwest VALUES ('OR', 'WA') TABLESPACE users,
SUBPARTITION southwest VALUES ('AZ', 'UT', 'NV') TABLESPACE users,
SUBPARTITION northeast VALUES ('NY', 'VM', 'NJ') TABLESPACE users,
SUBPARTITION southeast VALUES ('FL', 'GA') TABLESPACE users,
SUBPARTITION northcentral VALUES ('SD', 'WI') TABLESPACE users,
SUBPARTITION southcentral VALUES ('NM', 'TX') TABLESPACE users)
(
PARTITION q1_1999 VALUES LESS THAN(TO_DATE('1-APR-1999','DD-MON-YYYY')),
PARTITION q2_1999 VALUES LESS THAN(TO_DATE('1-JUL-1999','DD-MON-YYYY')),
PARTITION q3_1999 VALUES LESS THAN(TO_DATE('1-OCT-1999','DD-MON-YYYY')),
PARTITION q4_1999 VALUES LESS THAN(TO_DATE('1-JAN-2000','DD-MON-YYYY')));

CREATE INDEX quarterly_regional_sales_i1
  ON quarterly_regional_sales(deptno);

CREATE INDEX quarterly_regional_sales_i2
  ON quarterly_regional_sales(item_no)
    LOCAL;


--DROP TABLE spt_iot_list PURGE;
--CREATE TABLE spt_iot_list (
--  acct_no        NUMBER(5,0)    NOT NULL,
--  acct_name      VARCHAR2(30)   NOT NULL,
--  amount_of_sale NUMBER(6,0)    NULL,
--  state_cd       VARCHAR2(2)    NOT NULL,
--  sale_details   VARCHAR2(1000) NULL,
--  PRIMARY KEY (
--    acct_no,
--    acct_name,
--    state_cd
--  )
--)
--  ORGANIZATION INDEX INCLUDING state_cd
--  OVERFLOW
--  PARTITION BY LIST (state_cd)
--  SUBPARTITION BY RANGE (amount_of_sale)
--  SUBPARTITION TEMPLATE
--  (
--    SUBPARTITION low LESS THAN(1000),
--    SUBPARTITION hight LESS THAN(1000000)
--  )
--  (
--    PARTITION reg_east    VALUES ('MA', 'NY', 'CT', 'NH'),
--    PARTITION reg_west    VALUES ('CA', 'AZ', 'NM', 'OR'),
--    PARTITION reg_south   VALUES ('TX', 'KY', 'TN', 'GA'),
--    PARTITION reg_central VALUES ('OH', 'ND', 'SD', 'IA'),
--    PARTITION reg_null    VALUES (NULL),
--    PARTITION reg_unknown VALUES (DEFAULT)
--  )
--/

/*
  tables with lob segments - not implemented yet
*/
