SELECT * FROM user_tab_columns;
SELECT * FROM all_views WHERE text_length > 20000;

create table t(x varchar2(24000));

INSERT INTO t
SELECT LISTAGG(name, Chr(10)) WITHIN GROUP (ORDER BY line)
FROM all_source
  WHERE name = 'UTL_RAW'
    AND type = 'PACKAGE'
  GROUP BY name
/

COMMIT
/

SELECT Length(x), x FROM t;
