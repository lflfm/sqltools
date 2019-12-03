--http://www.dbazine.com/oracle/or-articles/fernandez2
SELECT dbms_metadata.get_ddl('TABLE','SAMPLE') FROM dual;

purge recyclebin;

SELECT * FROM all_views WHERE owner = 'SYS' AND view_name LIKE '%PART%';
SELECT * FROM user_tab_partitions;
SELECT * FROM user_part_key_columns;


SELECT * FROM user_tables;
SELECT * FROM user_indexes;
SELECT * FROM user_tab_partitions          ;
SELECT * FROM user_ind_partitions          ;
SELECT * FROM user_lob_partitions          ;

SELECT * FROM user_part_tables;
SELECT * FROM user_part_indexes            ;
SELECT * FROM user_part_key_columns        ;
SELECT * FROM user_part_lobs               ;
SELECT * FROM user_part_tables             ;

SELECT * FROM user_tab_subpartitions       ;
SELECT * FROM user_ind_subpartitions       ;
SELECT * FROM user_lob_subpartitions       ;

SELECT * FROM user_subpart_key_columns     ;
SELECT * FROM user_subpartition_templates  ;

DROP TABLE sample;

CREATE TABLE sample
(
cob_date DATE NOT NULL,
run_type NUMBER NOT NULL,
region   CHAR(3) NOT NULL
)
PARTITION BY RANGE (cob_date, run_type)
--SUBPARTITION BY LIST(region)
--SUBPARTITION TEMPLATE
--(
--  SUBPARTITION region_a VALUES('A'),
--  SUBPARTITION region_b VALUES('B'),
--  SUBPARTITION region_c VALUES('C')
--)
(
PARTITION smpl_01APR1999_1    VALUES LESS THAN (TO_DATE('01-APR-1999', 'DD-MON-YYYY'), 2),
PARTITION smpl_01APR1999_2    VALUES LESS THAN (TO_DATE('01-APR-1999', 'DD-MON-YYYY'), 3),
PARTITION smpl_01APR1999_rest VALUES LESS THAN (TO_DATE('01-APR-1999', 'DD-MON-YYYY'), MAXVALUE),

PARTITION smpl_02APR1999_1    VALUES LESS THAN (TO_DATE('02-APR-1999', 'DD-MON-YYYY'), 2),
PARTITION smpl_02APR1999_2    VALUES LESS THAN (TO_DATE('02-APR-1999', 'DD-MON-YYYY'), 3),
PARTITION smpl_02APR1999_rest VALUES LESS THAN (TO_DATE('02-APR-1999', 'DD-MON-YYYY'), MAXVALUE)
)
;

ALTER TABLE sample ADD PARTITION smpl_03APR1999_1 VALUES LESS THAN (TO_DATE('03-APR-1999', 'DD-MON-YYYY'), 2);

ALTER TABLE sample truncate PARTITION smpl_03APR1999_1;
ALTER TABLE sample truncate SUBPARTITION smpl_03APR1999_1_region_a;

CREATE INDEX sample_i ON sample(cob_date, run_type, region)
  LOCAL (PARTITION smpl_03APR1999_1 SUBPARTITION region_a);

SELECT partitioned, buffer_pool FROM user_tables;
SELECT partitioned, buffer_pool FROM user_indexes;
