SELECT
  CASE
    WHEN dummy = 'X' THEN 'x'
    WHEN dummy = 'Y' THEN 'y'
    ELSE
      CASE
        WHEN dummy = 'X' THEN 'x'
        ELSE 'y'
      END
  END
  FROM dual;

WITH
  FUNCTION with_function(p_id IN NUMBER) RETURN NUMBER IS
  BEGIN
    RETURN p_id;
  END;
SELECT with_function(rownum)
FROM   dual;
/

WITH
  PROCEDURE Log (p_message VARCHAR2) IS
  BEGIN
    Dbms_Output.Put_Line(p_message);
  END;
  FUNCTION with_function_1 (p_id IN NUMBER) RETURN NUMBER IS
  BEGIN
    Log('with_function_1 is called');
    RETURN p_id;
  END;
  FUNCTION with_function_2 (p_id IN NUMBER) RETURN NUMBER IS
  BEGIN
    Log('with_function_2 is called');
    RETURN p_id;
  END;
  a AS
  (
    SELECT * FROM dual
  )
SELECT with_function_1(rownum), with_function_2(rownum)
FROM a;
/
