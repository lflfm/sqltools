BEGIN
EXECUTE IMMEDIATE '
CREATE OR REPLACE AND COMPILE JAVA SOURCE NAMED "Test" AS
    import java.lang.*;
    import java.util.*;
    import java.io.*;
    import java.sql.Timestamp;

    public class Test
    {
        public static String hello () {
            return "Hello World!!!";
        }
    };
';
END;
/

CREATE OR REPLACE FUNCTION test_hello RETURN VARCHAR2 AS LANGUAGE JAVA name 'Test.hello() return java.lang.String';
/

DROP JAVA SOURCE "Test";

SELECT * FROM user_objects ORDER BY created DESC;

SELECT test_hello FROM dual;


SELECT * FROM USER_OBJECTS WHERE OBJECT_TYPE LIKE 'JAVA%' AND object_name = 'Test';
SELECT * FROM USER_source WHERE name = 'Test';