/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/

#include "stdafx.h"
#include "OCI8/SQLFunctionCodes.h"

namespace OCI8
{

bool PossibleCompilationErrors (int code)
{
  switch (code) {
  case OFN_CREATE_PROCEDURE           :
  case OFN_CREATE_TRIGGER             :
  case OFN_CREATE_FUNCTION            :
  case OFN_CREATE_PACKAGE             :
  case OFN_CREATE_PACKAGE_BODY        :
  case OFN_CREATE_TYPE                :
  case OFN_CREATE_TYPE_BODY           :
    return true;
  }
  return false;
}

const char* GetSQLFunctionDescription (int code)
{
    switch (code)
    {
    case OFN_CREATE_TABLE:            return "Create table";
    case OFN_SET_ROLE:                return "Set role";
    case OFN_INSERT:                  return "Insert";
    case OFN_SELECT:                  return "Select";
    case OFN_UPDATE:                  return "Update";
    case OFN_MERGE:                   return "Merge";
    case OFN_DROP_ROLE:               return "Drop role";
    case OFN_DROP_VIEW:               return "Drop view";
    case OFN_DROP_TABLE:              return "Drop table";
    case OFN_DELETE:                  return "Delete";
    case OFN_CREATE_VIEW:             return "Create view";
    case OFN_DROP_USER:               return "Drop user";
    case OFN_CREATE_ROLE:             return "Create role";
    case OFN_CREATE_SEQUENCE:         return "Create sequence";
    case OFN_ALTER_SEQUENCE:          return "Alter sequence";

    case OFN_DROP_SEQUENCE:           return "Drop sequence";
    case OFN_CREATE_SCHEMA:           return "Create schema";
    case OFN_CREATE_CLUSTER:          return "Create cluster";
    case OFN_CREATE_USER:             return "Create user";
    case OFN_CREATE_INDEX:            return "Create index";
    case OFN_DROP_INDEX:              return "Drop index";
    case OFN_DROP_CLUSTER:            return "Drop cluster";
    case OFN_VALIDATE_INDEX:          return "Validate index";
    case OFN_CREATE_PROCEDURE:        return "Create procedure";
    case OFN_ALTER_PROCEDURE:         return "Alter procedure";
    case OFN_ALTER_TABLE:             return "Alter table";
    case OFN_EXPLAIN:                 return "Explain";
    case OFN_GRANT:                   return "Grant";
    case OFN_REVOKE:                  return "Revoke";
    case OFN_CREATE_SYNONYM:          return "Create synonym";
    case OFN_DROP_SYNONYM:            return "Drop synonym";
    case OFN_ALTER_SYSTEM:            return "Alter system";
    case OFN_SET_TRANSACTION:         return "Set transaction";
    case OFN_PL_SQL_EXECUTE:          return "PL/SQL block";
    case OFN_LOCK_TABLE:              return "Lock table";

    case OFN_RENAME:                  return "Rename";
    case OFN_COMMENT:                 return "Comment";
    case OFN_AUDIT:                   return "Audit";
    case OFN_NOAUDIT:                 return "Noaudit";
    case OFN_ALTER_INDEX:             return "Alter index";
    case OFN_CREATE_EXTERNAL_DATABASE:return "Create external database";
    case OFN_DROP_EXTERNAL_DATABASE:  return "Drop external database";
    case OFN_CREATE_DATABASE:         return "Create database";
    case OFN_ALTER_DATABASE:          return "Alter database";
    case OFN_CREATE_ROLLBACK_SEGMENT: return "Create rollback segment";
    case OFN_ALTER_ROLLBACK_SEGMENT:  return "Alter rollback segment";
    case OFN_DROP_ROLLBACK_SEGMENT:   return "Drop rollback segment";
    case OFN_CREATE_TABLESPACE:       return "Create tablespace";
    case OFN_ALTER_TABLESPACE:        return "Alter tablespace";
    case OFN_DROP_TABLESPACE:         return "Drop tablespace";
    case OFN_ALTER_SESSION:           return "Alter session";
    case OFN_ALTER_USER:              return "Alter user";
    case OFN_COMMIT:                  return "Commit";
    case OFN_ROLLBACK:                return "Rollback";
    case OFN_SAVEPOINT:               return "Savepoint";
    case OFN_CREATE_CONTROL_FILE:     return "Create control file";
    case OFN_ALTER_TRACING:           return "Alter tracing";
    case OFN_CREATE_TRIGGER:          return "Create trigger";
    case OFN_ALTER_TRIGGER:           return "Alter trigger";
    case OFN_DROP_TRIGGER:            return "Drop trigger";
    case OFN_ANALYZE_TABLE:           return "Analyze table";
    case OFN_ANALYZE_INDEX:           return "Analyze index";
    case OFN_ANALYZE_CLUSTER:         return "Analyze cluster";
    case OFN_CREATE_PROFILE:          return "Create profile";
    case OFN_DROP_PROFILE:            return "Drop profile";
    case OFN_ALTER_PROFILE:           return "Alter profile";
    case OFN_DROP_PROCEDURE:          return "Drop procedure";

    case OFN_ALTER_RESOURCE_COST:     return "Alter resource cost";
    case OFN_CREATE_SNAPSHOT_LOG:     return "Create snapshot log";
    case OFN_ALTER_SNAPSHOT_LOG:      return "Alter snapshot log";
    case OFN_DROP_SNAPSHOT_LOG:       return "Drop snapshot log";
    case OFN_CREATE_SNAPSHOT:         return "Create snapshot";
    case OFN_ALTER_SNAPSHOT:          return "Alter snap shot";
    case OFN_DROP_SNAPSHOT:           return "Drop snapshot";

    case OFN_ALTER_ROLE:              return "Alter Role";
    case OFN_TRUNCATE_TABLE:          return "Truncate table";
    case OFN_TRUNCATE_CLUSTER:        return "Truncate cluster";
    case OFN_ALTER_VIEW:              return "Alter view";

    case OFN_CREATE_FUNCTION:         return "Create function";
    case OFN_ALTER_FUNCTION:          return "Alter function";
    case OFN_DROP_FUNCTION:           return "Drop function";
    case OFN_CREATE_PACKAGE:          return "Create package";
    case OFN_ALTER_PACKAGE:           return "Alter package";
    case OFN_DROP_PACKAGE:            return "Drop package";
    case OFN_CREATE_PACKAGE_BODY:     return "Create package body";
    case OFN_ALTER_PACKAGE_BODY:      return "Alter package body";
    case OFN_DROP_PACKAGE_BODY:       return "Drop package body";

    case OFN_CREATE_TYPE:          return "Create type";
    case OFN_ALTER_TYPE:           return "Alter type";
    case OFN_DROP_TYPE:            return "Drop type";
    case OFN_CREATE_TYPE_BODY:     return "Create type body";
    case OFN_ALTER_TYPE_BODY:      return "Alter type body";
    case OFN_DROP_TYPE_BODY:       return "Drop type body";

    case OFN_NOOP:                   return "Noop";
    case OFN_DROP_LIBRARY:           return "Drop library";
    case OFN_CREATE_BITMAPFILE:      return "Create bitmapfile";
    case OFN_DROP_BITMAPFILE:        return "Drop bitmapfile";
    case OFN_SET_CONSTRAINTS:        return "Set constraints";

    case OFN_CREATE_DIRECTORY:       return "Create directory";
    case OFN_DROP_DIRECTORY:         return "Drop directory";
    case OFN_CREATE_LIBRARY:         return "Create library";
    case OFN_CREATE_JAVA:            return "Create java";
    case OFN_ALTER_JAVA:             return "Alter java";
    case OFN_DROP_JAVA:              return "Drop java";
    case OFN_CREATE_OPERATOR:        return "Create operator";
    case OFN_CREATE_INDEXTYPE:       return "Create indextype";
    case OFN_DROP_INDEXTYPE:         return "Drop indextype";
    case OFN_ALTER_INDEXTYPE:        return "Alter indextype";
    case OFN_DROP_OPERATOR:          return "Drop operator";
    case OFN_ASSOCIATE_STATISTICS:   return "Associate statistics";
    case OFN_DISASSOCIATE_STATISTICS:return "Disassociate statistics";
    case OFN_CALL_METHOD:            return "Call method";
    case OFN_CREATE_SUMMARY:         return "Create summary";
    case OFN_ALTER_SUMMARY:          return "Alter summary";
    case OFN_DROP_SUMMARY:           return "Drop summary";
    case OFN_CREATE_DIMENSION:       return "Create dimension";
    case OFN_ALTER_DIMENSION:        return "Alter dimension";
    case OFN_DROP_DIMENSION:         return "Drop dimension";
    case OFN_CREATE_CONTEXT:         return "Create context";
    case OFN_DROP_CONTEXT:           return "Drop context";
    case OFN_ALTER_OUTLINE:          return "Alter outline";
    case OFN_CREATE_OUTLINE:         return "Create outline";
    case OFN_DROP_OUTLINE:           return "Drop outline";
    case OFN_UPDATE_INDEXES:         return "Update indexes";
    case OFN_ALTER_OPERATOR:         return "Alter operator";
    }
    return "Statement processed";
}

};//namespace OCI8