SELECT * FROM all_tab_columns WHERE data_type = 'XMLTYPE';

SELECT To_Clob(res) FROM XDB.PATH_VIEW;
SELECT res FROM XDB.PATH_VIEW;
SELECT * FROM XDB.PATH_VIEW;
SELECT * FROM XDB.PATH_VIEW WHERE ROWNUM < 2;
SELECT Max(Length(res)) FROM XDB.PATH_VIEW WHERE ROWNUM < 2;
SELECT Max(Length(xmltype.getclobval(res))) FROM XDB.PATH_VIEW;

SELECT path, res FROM XDB.PATH_VIEW;

BEGIN
  FOR buff IN (
    SELECT path, xmltype.getclobval(res) cl FROM XDB.PATH_VIEW
  ) LOOP
    Dbms_Output.Put_Line(SubStr(buff.cl, 1, 1000));
  END LOOP;
END;
/