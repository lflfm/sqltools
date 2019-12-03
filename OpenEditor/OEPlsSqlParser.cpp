/* 
    Copyright (C) 2002-2015 Aleksey Kochetov

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

/*
    14.06.2005 the implementation has been moved into Common namespace
*/

#include "stdafx.h"
#include <map>
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEContext.h"
#include "OpenEditor/OEPlsSqlParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
    using namespace std;

	struct TokenDesc
    {
		EToken token;
		const char* keyword;
        unsigned int minLength;
	};

    // tokens for script mode
    static TokenDesc s_scriptTokens[] =
    {
        etAT,                   "@",                1,
		etSEMICOLON,	        ";",                1,
		etQUOTE,		        "'",	            1,
		etDOUBLE_QUOTE,	        "\"",	            1,
		etMINUS,		        "-",	            1,
		etSLASH,		        "/",	            1,
		etSTAR,			        "*",                1,
		etLEFT_ROUND_BRACKET,	"(",			    1,
		etRIGHT_ROUND_BRACKET,	")",			    1,
        etACCEPT,               "ACCEPT",           3,
        etARCHIVE,              "ARCHIVE",          7,
        etAPPEND,               "APPEND",           1,
        etATTRIBUTE,            "ATTRIBUTE",        9,
        etBREAK,                "BREAK",            3,
        etBTITLE,               "BTITLE",           3,
        etCLEAR,                "CLEAR",            2,
        etCOLUMN,               "COLUMN",           3,
        etCOMPUTE,              "COMPUTE",          4,
        etCONNECT,              "CONNECT",          4,
        etCOPY,                 "COPY",             4,
        etDEFINE,               "DEFINE",           3,
        etDESCRIBE,             "DESCRIBE",         4,
        etDISCONNECT,           "DISCONNECT",       4,
        etEXECUTE,              "EXECUTE",          4,
        etEXIT,                 "EXIT",             4,
        etEXIT,                 "QUIT",             4,
        etHELP,                 "HELP",             4,
        etHOST,                 "HOST",             4,
        etPAUSE,                "PAUSE",            3,
        etPRINT,                "PRINT",            3,
        etPROMPT,               "PROMPT",           3,
        etRECOVER,              "RECOVER",          7,
        etREMARK,               "REMARK",           3,
        etREPFOOTER,            "REPFOOTER",        9,
        etREPHEADER,            "REPHEADER",        9,
        etRUN,                  "RUN",              1,
        etSET,                  "SET",              3,
        etSHOW,                 "SHOW",             3,
        etSHUTDOWN,             "SHUTDOWN",         8,
        etSPOOL,                "SPOOL",            3,
        etSTART,                "START",            3,
        etSTARTUP,              "STARTUP",          7,
        etTIMING,               "TIMING",           6,
        etTTITLE,               "TTITLE",           3,
        etUNDEFINE,             "UNDEFINE",         5,
        etVARIABLE,             "VARIABLE",         3,
        etWHENEVER,             "WHENEVER",         8,
        // SET parameters
        etAPPINFO,              "APPINFO",          2,
        etARRAYSIZE,            "ARRAYSIZE",        5,
        etAUTOCOMMIT,           "AUTOCOMMIT",       4,
        etAUTOPRINT,            "AUTOPRINT",        5,
        etAUTORECOVERY,         "AUTORECOVERY",     12,
        etAUTOTRACE,            "AUTOTRACE",        5,
        etBLOCKTERMINATOR,      "BLOCKTERMINATOR",  3,
        etCMDSEP,               "CMDSEP",           4,
        etCOLSEP,               "COLSEP",           6,
        etCOMPATIBILITY,        "COMPATIBILITY",    3,
        etCONCAT,               "CONCAT",           3,
        etCOPYCOMMIT,           "COPYCOMMIT",       5,
        etCOPYTYPECHECK,        "COPYTYPECHECK",    13,
        etDEFINE,               "DEFINE",           3,
        etDESCRIBE,             "DESCRIBE",         8,
        etECHO,                 "ECHO",             4,
        etEDITFILE,             "EDITFILE",         5,
        etEMBEDDED,             "EMBEDDED",         3,
        etESCAPE,               "ESCAPE",           3,
        etFEEDBACK,             "FEEDBACK",         4,
        etFLAGGER,              "FLAGGER",          7,
        etFLUSH,                "FLUSH",            3,
        etHEADING,              "HEADING",          3,
        etHEADSEP,              "HEADSEP",          5,
        etINSTANCE,             "INSTANCE",         8,
        etLINESIZE,             "LINESIZE",         3,
        etLOBOFFSET,            "LOBOFFSET",        5,
        etLOGSOURCE,            "LOGSOURCE",        9,
        etLONG,                 "LONG",             4,
        etLONGCHUNKSIZE,        "LONGCHUNKSIZE",    5,
        etMARKUP,               "MARKUP",           4,
        etNEWPAGE,              "NEWPAGE",          4,
        etNULL,                 "NULL",             4,
        etNUMFORMAT,            "NUMFORMAT",        4,
        etNUMWIDTH,             "NUMWIDTH",         3,
        etPAGESIZE,             "PAGESIZE",         5,
        etPAUSE,                "PAUSE",            3,
        etRECSEP,               "RECSEP",           6,
        etRECSEPCHAR,           "RECSEPCHAR",       10,
        etSCAN,                 "SCAN",             4,
        etSERVEROUTPUT,         "SERVEROUTPUT",     9,
        etSHIFTINOUT,           "SHIFTINOUT",       5,
        etSHOWMODE,             "SHOWMODE",         4,
        etSQLBLANKLINES,        "SQLBLANKLINES",    5,
        etSQLCASE,              "SQLCASE",          4,
        etSQLCONTINUE,          "SQLCONTINUE",      5,
        etSQLNUMBER,            "SQLNUMBER",        4,
        etSQLPLUSCOMPATIBILITY, "SQLPLUSCOMPATIBILITY", 13,
        etSQLPREFIX,            "SQLPREFIX",        6,
        etSQLPROMPT,            "SQLPROMPT",        4,
        etSQLTERMINATOR,        "SQLTERMINATOR",    4,
        etSUFFIX,               "SUFFIX",           3,
        etTAB,                  "TAB",              3,
        etTERMOUT,              "TERMOUT",          4,
        etTIME,                 "TIME",             2,
        etTIMING,               "TIMING",           4,
        etTRIMOUT,              "TRIMOUT",          4,
        etTRIMSPOOL,            "TRIMSPOOL",        5,
        etUNDERLINE,            "UNDERLINE",        3,
        etVERIFY,               "VERIFY",           3,
        etWRAP,                 "WRAP",             3,
    };
        
    // tokens for pl/sql mode
    static TokenDesc s_plsqlTokens[] =
    {
		etDECLARE,				"DECLARE",          0,
		etFUNCTION,				"FUNCTION",         0,
        etRETURN,               "RETURN",           0,
		etPROCEDURE,			"PROCEDURE",        0,
		etPACKAGE,				"PACKAGE",          0,
		etBODY,					"BODY",             0,
        etWRAPPED,              "WRAPPED",          0,
        etAUTHID,               "AUTHID",           0,
        etCURRENT_USER,         "CURRENT_USER",     0,
        etDEFINER,              "DEFINER",          0,
		etBEGIN,				"BEGIN",            0,
		etEXCEPTION,			"EXCEPTION",        0,
		etEND,					"END",              0,
		etIF,					"IF",               0,
		etTHEN,					"THEN",             0,
		etELSE,					"ELSE",             0,
		etELSIF,				"ELSIF",            0,
		etFOR,					"FOR",              0,
		etWHILE,				"WHILE",            0,
		etLOOP,					"LOOP",             0,
		etEXIT,					"EXIT",             0,
		etIS,					"IS",               0,
		etAS,					"AS",               0,
		etSEMICOLON,			";",                0,
		etQUOTE,				"'",                0,
		etDOUBLE_QUOTE,			"\"",               0,
		etLEFT_ROUND_BRACKET,	"(",                0,
		etRIGHT_ROUND_BRACKET,	")",                0,
		etMINUS,				"-",                0,
		etSLASH,				"/",                0,
		etSTAR,					"*",                0,
		etSELECT,				"SELECT",           0,
		etINSERT,				"INSERT",           0,
		etUPDATE,				"UPDATE",           0,
		etDELETE,				"DELETE",           0,
		etALTER,				"ALTER",            0,
		etANALYZE,				"ANALYZE",          0,
		etCREATE,				"CREATE",           0,
		etDROP,					"DROP",             0,
		etFROM,					"FROM",             0,
		etWHERE,				"WHERE",            0,
		etSET,					"SET",              0,
		etOPEN,                 "OPEN",             0,
        etLANGUAGE,             "LANGUAGE",         0,
        etLESS,                 "<",                0,
        etGREATER,              ">",                0,
        etTRIGGER,              "TRIGGER",          0,
        etTYPE,                 "TYPE",             0,
        etOR,                   "OR",               0,
        etREPLACE,              "REPLACE",          0,
        etDOT,                  ".",                0,
        etCOLON,			    ":",                0,
        etEQUAL,                "=",                0,
        etWITH,                 "WITH",             0,
        etUNION,                "UNION",            0,
        etINTERSECT,            "INTERSECT",        0,
        etMINUS_SQL,            "MINUS",            0,
        etALL,                  "ALL",              0,
        etCASE,                 "CASE",             0,
        etWHEN,                 "WHEN",             0,
        etGRANT,                "GRANT",            0,
        etREVOKE,               "REVOKE",           0,
        etAND,                  "AND",              0,
        etCOMPILE,              "COMPILE",          0,
        etJAVA,                 "JAVA",             0,
    };


    static
    void buildTokenMap (const TokenDesc* descs, size_t size, TokenMap& map)
    {
	    for (size_t i = 0; i < size; i++)
        {
            if (descs[i].minLength)
            {
                string keyword = descs[i].keyword;
                while (keyword.length() > 0
                && keyword.length() >=  descs[i].minLength)
                {
		            map.insert(make_pair(keyword, descs[i].token));
                    keyword.resize(keyword.length()-1);
                }
            }
            else
            {
                std::pair<TokenMap::iterator, bool> pair =
                    map.insert(make_pair(descs[i].keyword, descs[i].token));

#ifdef _DEBUG
                if (!pair.second)
                {
                    TRACE1("buildTokenMap: found duplicate %s", descs[i].keyword);
                    //ASSERT(pair.second);
                }
#endif
            }
        }
}

        
	static class PrivateTokenMapPtr : public TokenMapPtr 
	{ 
	public:
		PrivateTokenMapPtr ()
            : TokenMapPtr(new TokenMap)
		{
            buildTokenMap(s_scriptTokens, sizeof(s_scriptTokens)/sizeof(s_scriptTokens[0]), *get());
            buildTokenMap(s_plsqlTokens, sizeof(s_plsqlTokens)/sizeof(s_plsqlTokens[0]), *get());
		}
	} 
	g_privateTokenMapPtr;

/*
BEGIN
  FOR buff IN (
    SELECT * FROM v$reserved_words
      WHERE Length > 1
        --AND NOT(reserved = 'Y' OR res_semi = 'Y')
        ORDER BY 1
  ) LOOP

    BEGIN
      EXECUTE IMMEDIATE 'create procedure '||buff.keyword||' is begin null; end;';

      Dbms_Output.Put_Line('good:'||buff.keyword);
    EXCEPTION
      WHEN Others THEN
        Dbms_Output.Put_Line('bad:'||buff.keyword);
    END;

    BEGIN
      EXECUTE IMMEDIATE 'drop procedure '||buff.keyword;
    EXCEPTION
      WHEN Others THEN NULL;
    END;

  END LOOP;
END;
*/

    static struct
    {
        const char* word;
        int sql_reserved;
        int plsql_reserved;
    }
    g_11g_reserved[] = 
    {
        "ACCESS",     1, 0,
        "ADD",        1, 0,
        "ALL",        1, 1,
        "ALTER",      1, 1,
        "AND",        1, 1,
        "ANY",        1, 1,
        "AS",         1, 1,
        "ASC",        1, 1,
        "AT",         1, 1,
        "AUDIT",      1, 0,
        "BEGIN",      1, 1,
        "BETWEEN",    1, 1,
        "BY",         1, 1,
        "CASE",       1, 1,
        "CHAR",       1, 0,
        "CHECK",      1, 1,
        "CLUSTER",    1, 1,
        "COLUMN",     1, 0,
        "COLUMNS",    1, 1,
        "COMMENT",    1, 0,
        "COMPRESS",   1, 1,
        "CONNECT",    1, 1,
        "CREATE",     1, 1,
        "CURRENT",    1, 1,
        "DATE",       1, 0,
        "DECIMAL",    1, 0,
        "DECLARE",    1, 1,
        "DEFAULT",    1, 1,
        "DELETE",     1, 0,
        "DESC",       1, 1,
        "DISTINCT",   1, 1,
        "DROP",       1, 1,
        "ELSE",       1, 1,
        "END",        1, 1,
        "EXCLUSIVE",  1, 1,
        "EXISTS",     1, 0,
        "FILE",       1, 0,
        "FLOAT",      1, 0,
        "FOR",        1, 1,
        "FROM",       1, 1,
        "GRANT",      1, 1,
        "GROUP",      1, 1,
        "HAVING",     1, 1,
        "IDENTIFIED", 1, 1,
        "IF",         1, 1,
        "IMMEDIATE",  1, 0,
        "IN",         1, 1,
        "INCREMENT",  1, 0,
        "INDEX",      1, 1,
        "INDEXES",    1, 1,
        "INITIAL",    1, 0,
        "INSERT",     1, 1,
        "INTEGER",    1, 0,
        "INTERSECT",  1, 1,
        "INTO",       1, 1,
        "IS",         1, 1,
        "LEVEL",      1, 0,
        "LIKE",       1, 1,
        "LOCK",       1, 1,
        "LONG",       1, 0,
        "MAXEXTENTS", 1, 0,
        "MINUS",      1, 1,
        "MLSLABEL",   1, 0,
        "MODE",       1, 1,
        "MODIFY",     1, 0,
        "NOAUDIT",    1, 0,
        "NOCOMPRESS", 1, 1,
        "NOT",        1, 1,
        "NOWAIT",     1, 1,
        "NULL",       1, 1,
        "NUMBER",     1, 0,
        "OF",         1, 1,
        "OFFLINE",    1, 0,
        "ON",         1, 1,
        "ONLINE",     1, 0,
        "OPTION",     1, 1,
        "OR",         1, 1,
        "ORDER",      1, 1,
        "OVERLAPS",   1, 1,
        "PCTFREE",    1, 0,
        "PRIOR",      1, 0,
        "PRIVILEGES", 1, 0,
        "PROCEDURE",  1, 1,
        "PUBLIC",     1, 1,
        "RAW",        1, 0,
        "RENAME",     1, 0,
        "RESOURCE",   1, 1,
        "REVOKE",     1, 1,
        "ROW",        1, 0,
        "ROWID",      1, 0,
        "ROWNUM",     1, 0,
        "ROWS",       1, 0,
        "SELECT",     1, 1,
        "SESSION",    1, 0,
        "SET",        1, 0,
        "SHARE",      1, 1,
        "SIZE",       1, 1,
        "SMALLINT",   1, 0,
        "SQL",        1, 1,
        "START",      1, 1,
        "SUCCESSFUL", 1, 0,
        "SYNONYM",    1, 0,
        "SYSDATE",    1, 0,
        "TABLE",      1, 1,
        "THEN",       1, 1,
        "TO",         1, 1,
        "TRIGGER",    1, 0,
        "UID",        1, 0,
        "UNION",      1, 1,
        "UNIQUE",     1, 1,
        "UPDATE",     1, 1,
        "USER",       1, 0,
        "VALIDATE",   1, 0,
        "VALUES",     1, 1,
        "VARCHAR",    1, 0,
        "VARCHAR2",   1, 0,
        "VIEW",       1, 1,
        "WHEN",       1, 1,
        "WHENEVER",   1, 0,
        "WHERE",      1, 1,
        "WITH",       1, 1,
    };

    static class PrivateReservedtPtr : public ReservedPtr
    {
    public:
        PrivateReservedtPtr ()
            : ReservedPtr(new Reserved)
        {
            for (int i = 0; i < sizeof g_11g_reserved/sizeof g_11g_reserved[0]; ++i)
            {
                (*this)->insert(make_pair(g_11g_reserved[i].word, (g_11g_reserved[i].sql_reserved | g_11g_reserved[i].plsql_reserved << 1)));
            }
        }
    }
    g_reservedPtr;
	
PlSqlParser::PlSqlParser (SyntaxAnalyser& analyzer)
: Common::PlsSql::PlSqlParser(&analyzer, g_privateTokenMapPtr, g_reservedPtr)
{
}

const char* PlSqlParser::GetStringToken (EToken token)
{
    for (int i(0); i < sizeof(s_scriptTokens)/sizeof(s_scriptTokens[0]); i++)
		if (s_scriptTokens[i].token == token)
			return s_scriptTokens[i].keyword;

    for (int i(0); i < sizeof(s_plsqlTokens)/sizeof(s_plsqlTokens[0]); i++)
		if (s_plsqlTokens[i].token == token)
			return s_plsqlTokens[i].keyword;

	return "Unknown";
}

bool PlSqlParser::IsPlSqlToken (EToken token)
{
    for (int i(0); i < sizeof(s_plsqlTokens)/sizeof(s_plsqlTokens[0]); i++)
		if (s_plsqlTokens[i].token == token)
			return true;

    return false;
}

bool PlSqlParser::IsScriptToken (EToken token)
{
    for (int i(0); i < sizeof(s_scriptTokens)/sizeof(s_scriptTokens[0]); i++)
		if (s_scriptTokens[i].token == token)
			return true;

    return false;
}

};