var x VARCHAR2(20)
EXEC :x := 'a'

SELECT * FROM dual
  WHERE EXISTS (SELECT * FROM dual WHERE 'a' = :x)
;
