DROP TABLE employees PURGE;

CREATE TABLE employees (
  id          NUMBER,
  first_name  VARCHAR2(10),
  last_name   VARCHAR2(10),
  salary      NUMBER(9,2),
  comm1       NUMBER(3),
  comm2       NUMBER(3),
  salary1     VARCHAR(1000) AS (ROUND(salary*(1+comm1/100),2)) NOT NULL,
  salary2     NUMBER GENERATED ALWAYS AS (ROUND(salary*(1+comm2/100),2)) VIRTUAL,
  CONSTRAINT employees_pk PRIMARY KEY (id)
)
;

INSERT INTO employees (id, first_name, last_name, salary, comm1, comm2)
VALUES (1, 'JOHN', 'DOE', 100, 5, 10);

INSERT INTO employees (id, first_name, last_name, salary, comm1, comm2)
VALUES (2, 'JAYNE', 'DOE', 200, 10, 20);
COMMIT;

SELECT * FROM employees;

SELECT * FROM user_tab_cols WHERE table_name = 'EMPLOYEES' AND hidden_column = 'NO';
