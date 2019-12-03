var d DATE

EXEC :d := SYSDATE

print d

var c refcursor

BEGIN
  OPEN :c FOR select * from user_objects;
END;
/

print c