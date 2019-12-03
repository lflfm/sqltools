SELECT * FROM dual
UNION ALL
(
  SELECT * FROM dual
)
/

BEGIN
  FOR buff IN
  (
    SELECT * FROM dual
    UNION ALL
    (
      SELECT * FROM dual
    )
  )
  LOOP
    NULL;
  END LOOP;
END;
/