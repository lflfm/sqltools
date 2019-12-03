DROP TABLE sales_interval PURGE;
CREATE TABLE sales_interval
(
  time_id                    DATE,
  product_id                 NUMBER(6)
)
PCTFREE 50
STORAGE (
  INITIAL      64 K
  NEXT       1024 K
  MINEXTENTS    1
  MAXEXTENTS    UNLIMITED
  BUFFER_POOL   DEFAULT
)
PARTITION BY RANGE (time_id)
INTERVAL(NUMTOYMINTERVAL(1, 'MONTH'))
(
  PARTITION t0 VALUES LESS THAN (TO_DATE('1-1-2005','DD-MM-YYYY')) TABLESPACE example,
  PARTITION t1 VALUES LESS THAN (TO_DATE('1-1-2006','DD-MM-YYYY')) PCTFREE 70,
  PARTITION t2 VALUES LESS THAN (TO_DATE('1-7-2006','DD-MM-YYYY')) TABLESPACE example,
  PARTITION t3 VALUES LESS THAN (TO_DATE('1-1-2007','DD-MM-YYYY')) PCTFREE 70
);

INSERT INTO sales_interval VALUES (DATE'2004-01-01', -1);
INSERT INTO sales_interval VALUES (DATE'2005-01-01', -1);
INSERT INTO sales_interval VALUES (DATE'2006-01-01', -1);
INSERT INTO sales_interval VALUES (DATE'2007-01-01', -1);
INSERT INTO sales_interval VALUES (DATE'2007-02-01', -1);
INSERT INTO sales_interval VALUES (DATE'2007-03-01', -1);

COMMIT;

SELECT * FROM sales_interval;

CREATE INDEX sales_interval_i1 ON sales_interval(time_id, product_id) LOCAL;

SELECT * FROM user_tables WHERE table_name = 'SALES_INTERVAL';
SELECT INTERVAL, t.* FROM user_part_tables t WHERE table_name = 'SALES_INTERVAL';
SELECT INTERVAL, t.* FROM user_tab_partitions t WHERE table_name = 'SALES_INTERVAL';
SELECT INTERVAL, t.* FROM user_ind_partitions t WHERE index_name = 'SALES_INTERVAL_I1';
SELECT * FROM user_tablespaces;