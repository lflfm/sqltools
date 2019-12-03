spool test.log

SELECT * FROM emp;

CREATE OR REPLACE VIEW v_emp AS SELECT * FROM emp WITH READ ONLY CONSTRAINT v_emp_ro;

CREATE OR REPLACE VIEW v_emp_2 AS SELECT * FROM emp WITH CHECK OPTION CONSTRAINT v_emp_2_cho;

ALTER VIEW v_emp ADD CONSTRAINT v_emp_pk PRIMARY KEY (EMPNO) RELY DISABLE NOVALIDATE;

ALTER VIEW v_emp
  ADD CONSTRAINT v_emp_fk1 FOREIGN KEY (
    deptno
  ) REFERENCES dept (
    deptno
  ) RELY DISABLE NOVALIDATE
/

SELECT * FROM user_constraints WHERE table_name IN ('V_EMP','V_EMP_2');

spool off