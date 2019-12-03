DROP TYPE employee_t;
DROP TYPE departments_t;

CREATE OR REPLACE TYPE department_t AS OBJECT (
   deptno number(10),
   dname CHAR(30));
/

CREATE OR REPLACE TYPE departments_t AS TABLE OF department_t;
/

CREATE OR REPLACE TYPE department_t_v2 AS OBJECT (
   deptno number(10),
   dname CHAR(30));
/

CREATE OR REPLACE TYPE employee_t AS OBJECT(
  empid RAW(16),
  ename CHAR(31),
  dept REF department_t,

  STATIC function construct_emp (name VARCHAR2, dept REF department_t) RETURN employee_t
);
/

CREATE OR REPLACE TYPE BODY employee_t IS

  STATIC
  FUNCTION construct_emp (name varchar2, dept REF department_t) RETURN employee_t IS
  BEGIN
    return employee_t(SYS_GUID(),name,dept);
  END;

END;
/