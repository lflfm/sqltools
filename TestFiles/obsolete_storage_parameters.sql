CONNECT scott/tiger@w2k-10g:1521:test10

CREATE TABLE bonus (
  ename VARCHAR2(10) NULL,
  job   VARCHAR2(9)  NULL,
  sal   NUMBER       NULL,
  comm  NUMBER       NULL
)
/


SELECT * FROM user_tables;

SELECT * FROM user_tablespaces;

SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TABLES'
INTERSECT
SELECT column_name FROM all_tab_columns WHERE table_name = 'USER_TABLESPACES'


SELECT
  table_name, initial_extent, logging, max_extents, min_extents,
  next_extent, pct_increase, status, tablespace_name
FROM user_tables
UNION ALL SELECT
  NULL table_name, initial_extent, logging, max_extents, min_extents,
  next_extent, pct_increase, status, tablespace_name
FROM user_tablespaces
  WHERE tablespace_name = 'USERS'
/

SELECT table_name FROM all_tab_columns
  WHERE column_name IN ('FREELISTS', 'FREELIST_GROUPS')
/