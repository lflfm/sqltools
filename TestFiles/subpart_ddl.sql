SELECT * FROM user_tab_partitions;
SELECT * FROM user_tab_subpartitions;

SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TAB_PARTITIONS'
MINUS
SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TAB_SUBPARTITIONS'
/

SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TAB_SUBPARTITIONS'
MINUS
SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TAB_PARTITIONS'
/

SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_IND_PARTITIONS'
MINUS
SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_IND_SUBPARTITIONS'
/

SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_IND_SUBPARTITIONS'
MINUS
SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_IND_PARTITIONS'
/

SELECT * FROM user_part_tables WHERE table_name IN ('QUARTERLY_REGIONAL_SALES','PT_LIST');
SELECT * FROM user_tab_partitions WHERE table_name in ('QUARTERLY_REGIONAL_SALES','PT_LIST');
SELECT * FROM user_tab_subpartitions WHERE table_name = 'QUARTERLY_REGIONAL_SALES';

SELECT * FROM user_ind_partitions WHERE index_name LIKE 'QUARTERLY_REGIONAL_SALES%';
SELECT * FROM user_ind_subpartitions WHERE index_name LIKE 'QUARTERLY_REGIONAL_SALES%';

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

ALTER TABLE quarterly_regional_sales MODIFY PARTITION q1_1999 ADD  SUBPARTITION central VALUES ('KZ');
ALTER TABLE quarterly_regional_sales MODIFY PARTITION q1_1999 DROP  SUBPARTITION southcentral VALUES ('KZ');
ALTER TABLE quarterly_regional_sales DROP  SUBPARTITION q1_1999_southcentral;
ALTER TABLE quarterly_regional_sales DROP  SUBPARTITION central;
ALTER TABLE quarterly_regional_sales MODIFY PARTITION q1_1999 ADD  SUBPARTITION q1_1999_southcentral VALUES ('KZ');

CREATE INDEX quarterly_regional_sales_i1
  ON quarterly_regional_sales(deptno);

CREATE INDEX quarterly_regional_sales_i2
  ON quarterly_regional_sales(item_no)
    LOCAL;

ALTER TABLE quarterly_regional_sales PARALLEL (DEGREE 10 INSTANCES 2);
ALTER INDEX quarterly_regional_sales_i1 PARALLEL (DEGREE 10 INSTANCES 2);
ALTER INDEX quarterly_regional_sales_i2 PARALLEL (DEGREE 10 INSTANCES 2);

--------CREATE INDEX quarterly_regional_sales_i3 ON quarterly_regional_sales(state, deptno)
--------PARTITION BY LIST (state)
--------(
--------PARTITION northwest VALUES ('OR', 'WA') ,
--------PARTITION southwest VALUES ('AZ', 'UT', 'NV') ,
--------PARTITION northeast VALUES ('NY', 'VM', 'NJ') ,
--------PARTITION southeast VALUES ('FL', 'GA') ,
--------PARTITION northcentral VALUES ('SD', 'WI') ,
--------PARTITION southcentral VALUES ('NM', 'TX')
--------);


CREATE TABLE quarterly_regional_sales
(deptno NUMBER, item_no VARCHAR2(20),
 txn_date DATE, txn_amount NUMBER, state VARCHAR2(2))
PARTITION BY RANGE (txn_date)
SUBPARTITION BY LIST (state)
(
PARTITION q1_1999 VALUES LESS THAN(TO_DATE('1-APR-1999','DD-MON-YYYY'))
(SUBPARTITION q1_1999_northwest VALUES ('OR', 'WA'),
SUBPARTITION q1_1999_southwest VALUES ('AZ', 'UT', 'NM'),
SUBPARTITION q1_1999_northeast VALUES ('NY', 'VM', 'NJ'),
SUBPARTITION q1_1999_southeast VALUES  ('FL', 'GA'),
SUBPARTITION q1_1999_northcentral VALUES ('SD', 'WI'),
SUBPARTITION q1_1999_southcentral VALUES ('NM', 'TX')),
PARTITION q2_1999 VALUES LESS THAN(TO_DATE('1-JUL-1999','DD-MON-YYYY'))
(SUBPARTITION q2_1999_northwest VALUES ('OR', 'WA'),
SUBPARTITION q2_1999_southwest VALUES ('AZ', 'UT', 'NM'),
SUBPARTITION q2_1999_northeast VALUES ('NY', 'VM', 'NJ'),
SUBPARTITION q2_1999_southeast VALUES ('FL', 'GA'),
SUBPARTITION q2_1999_northcentral VALUES ('SD', 'WI'),
SUBPARTITION q2_1999_southcentral VALUES ('NM', 'TX')),
PARTITION q3_1999 VALUES LESS THAN (TO_DATE('1-OCT-1999','DD-MON-YYYY'))
(SUBPARTITION q3_1999_northwest VALUES ('OR', 'WA'),
SUBPARTITION q3_1999_southwest VALUES  ('AZ', 'UT', 'NM'),
SUBPARTITION q3_1999_northeast VALUES ('NY', 'VM', 'NJ'),
SUBPARTITION q3_1999_southeast VALUES ('FL', 'GA'),
SUBPARTITION q3_1999_northcentral VALUES ('SD', 'WI'),
SUBPARTITION q3_1999_southcentral VALUES ('NM', 'TX')),
PARTITION q4_1999 VALUES LESS THAN (TO_DATE('1-JAN-2000','DD-MON-YYYY'))
(SUBPARTITION q4_1999_northwest VALUES('OR', 'WA'),
SUBPARTITION q4_1999_southwest VALUES('AZ', 'UT', 'NM'),
SUBPARTITION q4_1999_northeast VALUES('NY', 'VM', 'NJ'),
SUBPARTITION q4_1999_southeast VALUES('FL', 'GA'),
SUBPARTITION q4_1999_northcentral VALUES ('SD', 'WI'),
SUBPARTITION q4_1999_southcentral VALUES ('NM', 'TX')));


CREATE TABLE sub_example_81
      (deptno number, item_no varchar2(20),
       txn_date date, txn_amount number, state varchar2(2))
  PARTITION BY RANGE (txn_date)
    SUBPARTITION BY HASH (state)
      (PARTITION q1_1999 VALUES LESS THAN (TO_DATE('1-APR-1999','DD-MON-YYYY'))
      );