define x = 'y'

define y = 'x'

DROP TABLE &xxx;
/

SELECT '&yyy' FROM dual;

SELECT *
  FROM
    all_users
  WHERE username LIKE 'SYS%'
/

SELECT * FROM dual;

SELECT
  *
  FROM user_tables;

BEGIN
  dbms_lock.sleep(3);
  Raise_Application_Error(-20999,'Test 1');
END;
/

SELECT
 *
 FROM bonus;


BEGIN
  dbms_lock.sleep(3);
  Raise_Application_Error(-20999,'Test 2');
END;
/

SELECT * FROM user_views;
SELECT * FROM user_triggers;
DROP TABLE yyy;
SELECT * FROM user_sequences;