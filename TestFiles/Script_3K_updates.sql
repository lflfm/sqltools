CREATE TABLE dept_3k (
  deptno NUMBER  NOT NULL,
  dname  VARCHAR2(14) NULL,
  loc    VARCHAR2(13) NULL
)
/

ALTER TABLE dept_3k
  ADD CONSTRAINT pk_dept_3k PRIMARY KEY (
    deptno
  )
/


BEGIN
  FOR i IN 1..3000 LOOP
    INSERT INTO dept_3k (deptno, dname, loc)
      VALUES (i, 'dname#'||i, 'loc#'||i);
  END LOOP;
  COMMIT;
END;
/

SELECT * FROM scott.dept_3k;

SELECT 'update dept_3k set loc = ''LOC#'||deptno||''' where deptno = '||deptno||';' FROM dept_3k;