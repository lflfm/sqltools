VARIABLE dept_sel REFCURSOR

BEGIN
  OPEN :dept_sel FOR SELECT * FROM DEPT;
END;
/

PRINT dept_sel

var str VARCHAR2(10)

BEGIN :str:= 'hello'; END;
/

print str


var vlob CLOB

BEGIN
  --
  :vlob := 'hello 1';
END;
/

BEGIN
  SELECT 'hello 2' INTO :vlob FROM dual;
END;
/

print vlob

var n NUMBER

BEGIN :n := 9999; END;
/

print n


var x VARCHAR2(1000)

EXEC :x := 'hi'

begin :x := 'hi'; end;
/