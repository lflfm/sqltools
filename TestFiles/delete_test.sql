CREATE TABLE del_test AS SELECT * FROM user_tables;

DELETE FROM del_test WHERE table_name = 'DEPT';

DELETE FROM del_test WHERE table_name = 'EMP';

