PROMPT 17 Managing Partitioned Tables and Indexes

CREATE TABLE state_sales(
  acct_no NUMBER(5),
  acct_name         VARCHAR2(30),
  amount_of_sale    NUMBER(6),
  state_cd          varchar2(2),
  sale_details      VARCHAR2(1000),
  PRIMARY KEY (acct_no, acct_name, state_cd)
)
  ORGANIZATION INDEX
  INCLUDING state_cd
  OVERFLOW TABLESPACE users
  PARTITION BY LIST (state_cd)
  (
    PARTITION reg_east VALUES ('MA','NY','CT','NH'),
    PARTITION reg_west VALUES ('CA', 'AZ', 'NM','OR'),
    PARTITION reg_south VALUES ('TX','KY','TN','GA'),
    PARTITION reg_central VALUES ('OH','ND','SD','IA'),
    PARTITION reg_null VALUES (NULL),
    PARTITION reg_unknown VALUES (DEFAULT)
  )
/

PROMPT CREATE TABLE x1
CREATE TABLE x1 (
  cob_date DATE    NOT NULL,
  run_type NUMBER  NOT NULL,
  region   CHAR(3) NOT NULL
)
  PARTITION BY RANGE (
    cob_date,
    run_type
  )
  (
    PARTITION smpl_01apr1999_1    VALUES LESS THAN (TO_DATE(' 1999-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), 2),
    PARTITION smpl_01apr1999_2    VALUES LESS THAN (TO_DATE(' 1999-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), 3),
    PARTITION smpl_01apr1999_rest VALUES LESS THAN (TO_DATE(' 1999-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), MAXVALUE),
    PARTITION smpl_02apr1999_1    VALUES LESS THAN (TO_DATE(' 1999-04-02 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), 2),
    PARTITION smpl_02apr1999_2    VALUES LESS THAN (TO_DATE(' 1999-04-02 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), 3),
    PARTITION smpl_02apr1999_rest VALUES LESS THAN (TO_DATE(' 1999-04-02 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'), MAXVALUE)
  )
  ENABLE ROW MOVEMENT
/

CREATE INDEX x1_local ON x1(cob_date) LOCAL;
CREATE INDEX x1_global ON x1(run_type);
CREATE INDEX x1_global_part ON x1(run_type);

DROP INDEX x1_global_part;
CREATE INDEX x1_global_part ON x1(region)
   GLOBAL PARTITION BY RANGE(region)
      (PARTITION VALUES LESS THAN ('A'),
       PARTITION VALUES LESS THAN ('B'),
       PARTITION VALUES LESS THAN ('C'),
       PARTITION VALUES LESS THAN (MAXVALUE))
    STORAGE (INITIAL 1m NEXT 500k )
       ;

CREATE INDEX x2_global_h_part ON x2(cob_date,run_type,region)
   GLOBAL PARTITION BY HASH (cob_date,run_type,region)
    (PARTITION p1, PARTITION p2, PARTITION p3)
/


SELECT * FROM user_part_indexes;
SELECT * FROM user_indexes WHERE table_name = 'X1';
SELECT * FROM user_ind_partitions WHERE index_name LIKE 'X1%';



SELECT * FROM user_part_key_columns;
SELECT 1048576/128 FROM dual;
SELECT * FROM user_tablespaces;

