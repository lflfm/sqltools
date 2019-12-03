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

#pragma once
#ifndef _SQLFunctionCodes_h_
#define _SQLFunctionCodes_h_

#include <oci.h>

namespace OCI8
{

    enum ESQLFunctionCode
    {
        OFN_CREATE_TABLE             = 1,
        OFN_SET_ROLE                 = 2,
        OFN_INSERT                   = 3,
        OFN_SELECT                   = 4,
        OFN_UPDATE                   = 5,
        OFN_DROP_ROLE                = 6,
        OFN_DROP_VIEW                = 7,
        OFN_DROP_TABLE               = 8,
        OFN_DELETE                   = 9,
        OFN_CREATE_VIEW              = 10,
        OFN_DROP_USER                = 11,
        OFN_CREATE_ROLE              = 12,
        OFN_CREATE_SEQUENCE          = 13,
        OFN_ALTER_SEQUENCE           = 14,
        //OFN_(not used)               = 15,
        OFN_DROP_SEQUENCE            = 16,
        OFN_CREATE_SCHEMA            = 17,
        OFN_CREATE_CLUSTER           = 18,
        OFN_CREATE_USER              = 19,
        OFN_CREATE_INDEX             = 20,
        OFN_DROP_INDEX               = 21,
        OFN_DROP_CLUSTER             = 22,
        OFN_VALIDATE_INDEX           = 23,
        OFN_CREATE_PROCEDURE         = 24,
        OFN_ALTER_PROCEDURE          = 25,
        OFN_ALTER_TABLE              = 26,
        OFN_EXPLAIN                  = 27,
        OFN_GRANT                    = 28,
        OFN_REVOKE                   = 29,
        OFN_CREATE_SYNONYM           = 30,
        OFN_DROP_SYNONYM             = 31,
        OFN_ALTER_SYSTEM             = 32,
        OFN_SET_TRANSACTION          = 33,
        OFN_PL_SQL_EXECUTE           = 34,
        OFN_LOCK_TABLE               = 35,
        OFN_RENAME                   = 37,
        OFN_COMMENT                  = 38,
        OFN_AUDIT                    = 39,
        OFN_NOAUDIT                  = 40,
        OFN_ALTER_INDEX              = 41,
        OFN_CREATE_EXTERNAL_DATABASE = 42,
        OFN_DROP_EXTERNAL_DATABASE   = 43,
        OFN_CREATE_DATABASE          = 44,
        OFN_ALTER_DATABASE           = 45,
        OFN_CREATE_ROLLBACK_SEGMENT  = 46,
        OFN_ALTER_ROLLBACK_SEGMENT   = 47,
        OFN_DROP_ROLLBACK_SEGMENT    = 48,
        OFN_CREATE_TABLESPACE        = 49,
        OFN_ALTER_TABLESPACE         = 50,
        OFN_DROP_TABLESPACE          = 51,
        OFN_ALTER_SESSION            = 52,
        OFN_ALTER_USER               = 53,
        OFN_COMMIT                   = 54,
        OFN_ROLLBACK                 = 55,
        OFN_SAVEPOINT                = 56,
        OFN_CREATE_CONTROL_FILE      = 57,
        OFN_ALTER_TRACING            = 58,
        OFN_CREATE_TRIGGER           = 59,
        OFN_ALTER_TRIGGER            = 60,
        OFN_DROP_TRIGGER             = 61,
        OFN_ANALYZE_TABLE            = 62,
        OFN_ANALYZE_INDEX            = 63,
        OFN_ANALYZE_CLUSTER          = 64,
        OFN_CREATE_PROFILE           = 65,
        OFN_DROP_PROFILE             = 66,
        OFN_ALTER_PROFILE            = 67,
        OFN_DROP_PROCEDURE           = 68,
        //OFN_(not used)               = 69,
        OFN_ALTER_RESOURCE_COST      = 70,
        OFN_CREATE_SNAPSHOT_LOG      = 71,
        OFN_ALTER_SNAPSHOT_LOG       = 72,
        OFN_DROP_SNAPSHOT_LOG        = 73,
        OFN_CREATE_SNAPSHOT          = 74,
        OFN_ALTER_SNAPSHOT           = 75,
        OFN_DROP_SNAPSHOT            = 76,

        OFN_CREATE_TYPE              = 77,
        OFN_DROP_TYPE                = 78,
        OFN_ALTER_ROLE               = 79,
        OFN_ALTER_TYPE               = 80,

        OFN_CREATE_TYPE_BODY         = 81,
        OFN_ALTER_TYPE_BODY          = 82,
        OFN_DROP_TYPE_BODY           = 83,

        OFN_TRUNCATE_TABLE           = 85,
        OFN_TRUNCATE_CLUSTER         = 86,
        OFN_ALTER_VIEW               = 88,

        OFN_CREATE_FUNCTION          = 91,
        OFN_ALTER_FUNCTION           = 92,
        OFN_DROP_FUNCTION            = 93,
        OFN_CREATE_PACKAGE           = 94,
        OFN_ALTER_PACKAGE            = 95,
        OFN_DROP_PACKAGE             = 96,
        OFN_CREATE_PACKAGE_BODY      = 97,
        OFN_ALTER_PACKAGE_BODY       = 98,
        OFN_DROP_PACKAGE_BODY        = 99,

        OFN_NOOP                     = 36,
        OFN_DROP_LIBRARY             = 84,
        OFN_CREATE_BITMAPFILE        = 87,
        OFN_DROP_BITMAPFILE          = 89,
        OFN_SET_CONSTRAINTS          = 90,

        OFN_CREATE_DIRECTORY        = 157,
        OFN_DROP_DIRECTORY          = 158,
        OFN_CREATE_LIBRARY          = 159,
        OFN_CREATE_JAVA             = 160,
        OFN_ALTER_JAVA              = 161,
        OFN_DROP_JAVA               = 162,
        OFN_CREATE_OPERATOR         = 163,
        OFN_CREATE_INDEXTYPE        = 164,
        OFN_DROP_INDEXTYPE          = 165,
        OFN_ALTER_INDEXTYPE         = 166,
        OFN_DROP_OPERATOR           = 167,
        OFN_ASSOCIATE_STATISTICS    = 168,
        OFN_DISASSOCIATE_STATISTICS = 169,
        OFN_CALL_METHOD             = 170,
        OFN_CREATE_SUMMARY          = 171,
        OFN_ALTER_SUMMARY           = 172,
        OFN_DROP_SUMMARY            = 173,
        OFN_CREATE_DIMENSION        = 174,
        OFN_ALTER_DIMENSION         = 175,
        OFN_DROP_DIMENSION          = 176,
        OFN_CREATE_CONTEXT          = 177,
        OFN_DROP_CONTEXT            = 178,
        OFN_ALTER_OUTLINE           = 179,
        OFN_CREATE_OUTLINE          = 180,
        OFN_DROP_OUTLINE            = 181,
        OFN_UPDATE_INDEXES          = 182,
        OFN_ALTER_OPERATOR          = 183,
        OFN_MERGE                   = 189,

    };

	bool PossibleCompilationErrors (int code);
    const char* GetSQLFunctionDescription (int code);

}; // namespace OCI8

#endif//_SQLFunctionCodes_h_
