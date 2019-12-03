--
-- tabsize = 4
-- indent  = 2
--
		PROCEDURE test IS
		BEGIN
				FOR buff IN (SELECT * FROM dual) LOOP
						Dbms_Output.Put_Line('test');
				END LOOP;
		END;