alter session set nls_length_semantics=char;

DROP TABLE tab_col_length_test;

CREATE TABLE tab_col_length_test
(
  col1 VARCHAR2(100 BYTE),
  col2 VARCHAR2(100 CHAR),
  col3 NVARCHAR2(100)
);

CREATE INDEX tab_col_length_test_i1 ON tab_col_length_test(col1);


alter session set nls_length_semantics=byte;


SELECT * FROM user_tab_columns WHERE table_name = 'TAB_COL_LENGTH_TEST';


SELECT * FROM nls_session_parameters WHERE parameter = 'NLS_LENGTH_SEMANTICS';