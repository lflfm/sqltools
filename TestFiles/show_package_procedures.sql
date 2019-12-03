var p_schema VARCHAR2(100)
EXEC :p_schema := 'SYS'
var p_parent VARCHAR2(100)
EXEC :p_parent := 'SPACE_ERROR_INFO'

SYS.SPACE_ERROR_INFO;

EXEC Dbms_Utility.analyze_schema(schema varchar2, method varchar2, estimate_rows number,...)

    SELECT
      Decode(position, 0, 'RETURN', argument_name)
        ||' '||Lower(
          Decode(in_out,'IN',NULL,in_out||' ')
          ||Nvl((RTrim(LTrim(type_name||'.'||type_subname,'.'),'.')),
            Decode(data_type,'PL/SQL BOOLEAN', 'BOOLEAN', data_type)
          )
        )
    FROM all_arguments
      WHERE
        owner = :p_schema
        AND object_name = :p_parent
        AND package_name IS NULL
        AND data_level = 0
    ORDER BY Decode(position, 0, 999999999, position)
/

WITH a AS (
  SELECT
--    *
    Lower(object_name) object_name,
    object_name||'.'||overload subprogram_id,
    Nvl((RTrim(LTrim(TYPE_NAME||'.'||TYPE_SUBNAME,'.'),'.')), DATA_TYPE) return_type,
    Lower(argument_name)||' '||IN_OUT||' '||Nvl((RTrim(LTrim(TYPE_NAME||'.'||TYPE_SUBNAME,'.'),'.')), DATA_TYPE) parameter,
    position,
    sequence,
    TYPE_NAME,
    TYPE_SUBNAME,
    data_type,
    overload,
    data_level
  FROM all_arguments
    WHERE
      owner = :p_schema
      AND object_name = :p_parent
      AND package_name IS NULL
      AND data_level = 0
)
SELECT
  --p.subprogram_id,
  --p.overload,
  Decode(a0.return_type, NULL, 'PROCEDURE', 'FUNCTION') type,
  p.object_name
    ||' (' ||a1.parameter||a2.parameter||a3.parameter||a4.parameter||')'
    ||Decode(a0.return_type, NULL, NULL, ' RETURN ')|| a0.return_type
  FROM (SELECT DISTINCT object_name, overload, subprogram_id FROM a) p,
    (SELECT subprogram_id, return_type FROM a WHERE position = 0) a0,
    (SELECT subprogram_id, parameter FROM a WHERE position = 1 AND parameter IS
NOT NULL) a1,
    (SELECT subprogram_id, ', '||parameter parameter FROM a WHERE position = 2
AND parameter IS NOT NULL) a2,
    (SELECT subprogram_id, ', '||parameter parameter FROM a WHERE position = 3
AND parameter IS NOT NULL) a3,
    (SELECT subprogram_id, ',...' parameter FROM a WHERE position = 4 AND
parameter IS NOT NULL) a4
    WHERE p.subprogram_id = a0.subprogram_id(+)
      AND p.subprogram_id = a1.subprogram_id(+)
      AND p.subprogram_id = a2.subprogram_id(+)
      AND p.subprogram_id = a3.subprogram_id(+)
      AND p.subprogram_id = a4.subprogram_id(+)
ORDER BY p.subprogram_id
/


WITH a AS (
  SELECT
--    *
    Lower(object_name) object_name,
    object_name||'.'||overload subprogram_id,
    Nvl((RTrim(LTrim(TYPE_NAME||'.'||TYPE_SUBNAME,'.'),'.')), DATA_TYPE) return_type,
    Lower(argument_name)||' '||IN_OUT||' '||Nvl((RTrim(LTrim(TYPE_NAME||'.'||TYPE_SUBNAME,'.'),'.')), DATA_TYPE) parameter,
    position,
    sequence,
    TYPE_NAME,
    TYPE_SUBNAME,
    data_type,
    overload,
    data_level
  FROM all_arguments
    WHERE
      owner = :p_schema
      AND package_name = :p_parent
      AND data_level = 0
)
SELECT
  --p.subprogram_id,
  --p.overload,
  Decode(a0.return_type, NULL, 'PROCEDURE ', 'FUNCTION ') type,
  p.object_name
    ||' (' ||a1.parameter||a2.parameter||a3.parameter||a4.parameter||')'
    ||Decode(a0.return_type, NULL, NULL, ' RETURN ')|| a0.return_type
  FROM (SELECT DISTINCT object_name, overload, subprogram_id FROM a) p,
    (SELECT subprogram_id, return_type FROM a WHERE position = 0) a0,
    (SELECT subprogram_id, parameter FROM a WHERE position = 1 AND parameter IS
NOT NULL) a1,
    (SELECT subprogram_id, ', '||parameter parameter FROM a WHERE position = 2
AND parameter IS NOT NULL) a2,
    (SELECT subprogram_id, ', '||parameter parameter FROM a WHERE position = 3
AND parameter IS NOT NULL) a3,
    (SELECT subprogram_id, ',...' parameter FROM a WHERE position = 4 AND
parameter IS NOT NULL) a4
    WHERE p.subprogram_id = a0.subprogram_id(+)
      AND p.subprogram_id = a1.subprogram_id(+)
      AND p.subprogram_id = a2.subprogram_id(+)
      AND p.subprogram_id = a3.subprogram_id(+)
      AND p.subprogram_id = a4.subprogram_id(+)
ORDER BY p.subprogram_id
/


