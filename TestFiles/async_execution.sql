--SELECT * FROM all_objects t1 ORDER BY length(object_name);
--SELECT Count(*), Count(DISTINCT created) FROM all_objects t1;
SELECT * FROM all_objects t1, all_objects t2;

EXEC Dbms_Lock.sleep(15);

SELECT '&test' FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM xdual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;

EXEC Dbms_Lock.sleep(3);

SELECT * FROM dual;
