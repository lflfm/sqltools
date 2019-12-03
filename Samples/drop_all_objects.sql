BEGIN
  FOR buff IN (
    SELECT * FROM user_queues ORDER BY qid DESC
  ) LOOP
    BEGIN
      dbms_aqadm.stop_queue(queue_name => buff.name);
    EXCEPTION WHEN Others THEN NULL; END;

    BEGIN
      dbms_aqadm.drop_queue(queue_name => buff.name);
    EXCEPTION WHEN Others THEN NULL; END;

    BEGIN
      dbms_aqadm.drop_queue_table(Queue_table => buff.queue_table);
    EXCEPTION WHEN Others THEN NULL; END;
  END LOOP;
END;
/
SELECT * FROM user_queues;

DECLARE
  lv_cursor INTEGER;
  CURSOR crs_objects IS
    SELECT 'drop '||Lower(object_type)||' "'||rtrim(object_name)||'" '||Decode(object_type,'TABLE',' CASCADE constraints') str
      FROM user_objects
      WHERE object_type NOT IN ('INDEX','TRIGGER', 'PACKAGE BODY', 'LOB', 'QUEUE')
      ORDER BY object_type,object_name;
BEGIN
  lv_cursor := Dbms_Sql.Open_Cursor;
  FOR buf_object IN crs_objects LOOP
    BEGIN
      Dbms_Sql.Parse(lv_cursor, buf_object.str, Dbms_Sql.v7);
    EXCEPTION
      WHEN Others THEN
        Dbms_Output.Put_Line(buf_object.str);
        Dbms_Output.Put_Line(SQLERRM);
    END;
  END LOOP;
  Dbms_Sql.Close_Cursor(lv_cursor);
EXCEPTION
  WHEN OTHERS THEN
    Dbms_Sql.Close_Cursor(lv_cursor);
    RAISE;
END;
/

SELECT * FROM user_objects;
rem SELECT * FROM user_jobs;
