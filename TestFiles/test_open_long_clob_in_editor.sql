CREATE TABLE clob_test (
  clob1 CLOB,
  clob2 CLOB
);

INSERT INTO clob_test values(Empty_Clob(), Empty_Clob());
COMMIT;

DECLARE
  l_clob1 CLOB;
  l_clob2 CLOB;
BEGIN
  SELECT clob1, clob2 INTO l_clob1, l_clob2 FROM clob_test FOR update;

  FOR i IN 1..1000 LOOP
    Dbms_Lob.writeappend(l_clob1, 80, LPad(Chr(10), 80, 'X'));
    Dbms_Lob.writeappend(l_clob2, 80, LPad(Chr(10), 80, 'Y'));
  END LOOP;

  COMMIT;
END;
/

SELECT clob1 FROM clob_test;
SELECT * FROM clob_test;
