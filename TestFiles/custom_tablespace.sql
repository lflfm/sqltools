SELECT * FROM dba_tablespaces WHERE tablespace_name = 'SYSTEM';
SELECT * FROM dba_data_files WHERE tablespace_name = 'SYSTEM';

CREATE TABLESPACE custom DATAFILE 'C:\ORACLE\ORADATA\TEST8I\CUSTOM.DBF' SIZE 100m REUSE
  --DEFAULT COMPRESS
  DEFAULT STORAGE (
    INITIAL      128 K
    NEXT         128 K
    MINEXTENTS    1
    MAXEXTENTS    100
    PCTINCREASE   0
  )
  NOLOGGING
  EXTENT MANAGEMENT DICTIONARY
  --SEGMENT SPACE MANAGEMENT MANUAL
/

define location = "C:\ORACLE\ORA92\TEST9"
define location = "C:\APP\ADMINISTRATOR\ORADATA\TEST11"

CREATE TABLESPACE custom DATAFILE '&location\CUSTOM.DBF' SIZE 100m REUSE
  DEFAULT STORAGE (
    INITIAL      128 K
    NEXT         128 K
    MINEXTENTS    1
    MAXEXTENTS    100
    PCTINCREASE   0
  )
  --COMPRESS
  NOLOGGING
  --EXTENT MANAGEMENT DICTIONARY
  SEGMENT SPACE MANAGEMENT MANUAL
/


ALTER USER scott QUOTA UNLIMITED ON custom;
