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
		const wchar_t* keyword;
        unsigned int minLength;
	};

    // tokens for script mode
    static TokenDesc s_scriptTokens[] =
    {
        etAT,                   L"@",                1,
		etSEMICOLON,	        L";",                1,
		etQUOTE,		        L"'",	            1,
		etDOUBLE_QUOTE,	        L"\"",	            1,
		etMINUS,		        L"-",	            1,
		etSLASH,		        L"/",	            1,
		etSTAR,			        L"*",                1,
		etLEFT_ROUND_BRACKET,	L"(",			    1,
		etRIGHT_ROUND_BRACKET,	L")",			    1,
        etACCEPT,               L"ACCEPT",           3,
        etARCHIVE,              L"ARCHIVE",          7,
        etAPPEND,               L"APPEND",           1,
        etATTRIBUTE,            L"ATTRIBUTE",        9,
        etBREAK,                L"BREAK",            3,
        etBTITLE,               L"BTITLE",           3,
        etCLEAR,                L"CLEAR",            2,
        etCOLUMN,               L"COLUMN",           3,
        etCOMPUTE,              L"COMPUTE",          4,
        etCONNECT,              L"CONNECT",          4,
        etCOPY,                 L"COPY",             4,
        etDEFINE,               L"DEFINE",           3,
        etDESCRIBE,             L"DESCRIBE",         4,
        etDISCONNECT,           L"DISCONNECT",       4,
        etEXECUTE,              L"EXECUTE",          4,
        etEXIT,                 L"EXIT",             4,
        etEXIT,                 L"QUIT",             4,
        etHELP,                 L"HELP",             4,
        etHOST,                 L"HOST",             4,
        etPAUSE,                L"PAUSE",            3,
        etPRINT,                L"PRINT",            3,
        etPROMPT,               L"PROMPT",           3,
        etRECOVER,              L"RECOVER",          7,
        etREMARK,               L"REMARK",           3,
        etREPFOOTER,            L"REPFOOTER",        9,
        etREPHEADER,            L"REPHEADER",        9,
        etRUN,                  L"RUN",              1,
        etSET,                  L"SET",              3,
        etSHOW,                 L"SHOW",             3,
        etSHUTDOWN,             L"SHUTDOWN",         8,
        etSPOOL,                L"SPOOL",            3,
        etSTART,                L"START",            3,
        etSTARTUP,              L"STARTUP",          7,
        etTIMING,               L"TIMING",           6,
        etTTITLE,               L"TTITLE",           3,
        etUNDEFINE,             L"UNDEFINE",         5,
        etVARIABLE,             L"VARIABLE",         3,
        etWHENEVER,             L"WHENEVER",         8,
        // SET parameters
        etAPPINFO,              L"APPINFO",          2,
        etARRAYSIZE,            L"ARRAYSIZE",        5,
        etAUTOCOMMIT,           L"AUTOCOMMIT",       4,
        etAUTOPRINT,            L"AUTOPRINT",        5,
        etAUTORECOVERY,         L"AUTORECOVERY",     12,
        etAUTOTRACE,            L"AUTOTRACE",        5,
        etBLOCKTERMINATOR,      L"BLOCKTERMINATOR",  3,
        etCMDSEP,               L"CMDSEP",           4,
        etCOLSEP,               L"COLSEP",           6,
        etCOMPATIBILITY,        L"COMPATIBILITY",    3,
        etCONCAT,               L"CONCAT",           3,
        etCOPYCOMMIT,           L"COPYCOMMIT",       5,
        etCOPYTYPECHECK,        L"COPYTYPECHECK",    13,
        etDEFINE,               L"DEFINE",           3,
        etDESCRIBE,             L"DESCRIBE",         8,
        etECHO,                 L"ECHO",             4,
        etEDITFILE,             L"EDITFILE",         5,
        etEMBEDDED,             L"EMBEDDED",         3,
        etESCAPE,               L"ESCAPE",           3,
        etFEEDBACK,             L"FEEDBACK",         4,
        etFLAGGER,              L"FLAGGER",          7,
        etFLUSH,                L"FLUSH",            3,
        etHEADING,              L"HEADING",          3,
        etHEADSEP,              L"HEADSEP",          5,
        etINSTANCE,             L"INSTANCE",         8,
        etLINESIZE,             L"LINESIZE",         3,
        etLOBOFFSET,            L"LOBOFFSET",        5,
        etLOGSOURCE,            L"LOGSOURCE",        9,
        etLONG,                 L"LONG",             4,
        etLONGCHUNKSIZE,        L"LONGCHUNKSIZE",    5,
        etMARKUP,               L"MARKUP",           4,
        etNEWPAGE,              L"NEWPAGE",          4,
        etNULL,                 L"NULL",             4,
        etNUMFORMAT,            L"NUMFORMAT",        4,
        etNUMWIDTH,             L"NUMWIDTH",         3,
        etPAGESIZE,             L"PAGESIZE",         5,
        etPAUSE,                L"PAUSE",            3,
        etRECSEP,               L"RECSEP",           6,
        etRECSEPCHAR,           L"RECSEPCHAR",       10,
        etSCAN,                 L"SCAN",             4,
        etSERVEROUTPUT,         L"SERVEROUTPUT",     9,
        etSHIFTINOUT,           L"SHIFTINOUT",       5,
        etSHOWMODE,             L"SHOWMODE",         4,
        etSQLBLANKLINES,        L"SQLBLANKLINES",    5,
        etSQLCASE,              L"SQLCASE",          4,
        etSQLCONTINUE,          L"SQLCONTINUE",      5,
        etSQLNUMBER,            L"SQLNUMBER",        4,
        etSQLPLUSCOMPATIBILITY, L"SQLPLUSCOMPATIBILITY", 13,
        etSQLPREFIX,            L"SQLPREFIX",        6,
        etSQLPROMPT,            L"SQLPROMPT",        4,
        etSQLTERMINATOR,        L"SQLTERMINATOR",    4,
        etSUFFIX,               L"SUFFIX",           3,
        etTAB,                  L"TAB",              3,
        etTERMOUT,              L"TERMOUT",          4,
        etTIME,                 L"TIME",             2,
        etTIMING,               L"TIMING",           4,
        etTRIMOUT,              L"TRIMOUT",          4,
        etTRIMSPOOL,            L"TRIMSPOOL",        5,
        etUNDERLINE,            L"UNDERLINE",        3,
        etVERIFY,               L"VERIFY",           3,
        etWRAP,                 L"WRAP",             3,
    };
        
    // tokens for pl/sql mode
    static TokenDesc s_plsqlTokens[] =
    {
		etDECLARE,				L"DECLARE",          0,
		etFUNCTION,				L"FUNCTION",         0,
        etRETURN,               L"RETURN",           0,
		etPROCEDURE,			L"PROCEDURE",        0,
		etPACKAGE,				L"PACKAGE",          0,
		etBODY,					L"BODY",             0,
        etWRAPPED,              L"WRAPPED",          0,
        etAUTHID,               L"AUTHID",           0,
        etCURRENT_USER,         L"CURRENT_USER",     0,
        etDEFINER,              L"DEFINER",          0,
		etBEGIN,				L"BEGIN",            0,
		etEXCEPTION,			L"EXCEPTION",        0,
		etEND,					L"END",              0,
		etIF,					L"IF",               0,
		etTHEN,					L"THEN",             0,
		etELSE,					L"ELSE",             0,
		etELSIF,				L"ELSIF",            0,
		etFOR,					L"FOR",              0,
		etWHILE,				L"WHILE",            0,
		etLOOP,					L"LOOP",             0,
		etEXIT,					L"EXIT",             0,
		etIS,					L"IS",               0,
		etAS,					L"AS",               0,
		etSEMICOLON,			L";",                0,
        etCOMMA,	            L",",                0,
		etQUOTE,				L"'",                0,
		etDOUBLE_QUOTE,			L"\"",               0,
		etLEFT_ROUND_BRACKET,	L"(",                0,
		etRIGHT_ROUND_BRACKET,	L")",                0,
		etMINUS,				L"-",                0,
		etSLASH,				L"/",                0,
		etSTAR,					L"*",                0,
		etSELECT,				L"SELECT",           0,
		etINSERT,				L"INSERT",           0,
		etUPDATE,				L"UPDATE",           0,
		etDELETE,				L"DELETE",           0,
		etALTER,				L"ALTER",            0,
		etANALYZE,				L"ANALYZE",          0,
		etCREATE,				L"CREATE",           0,
		etDROP,					L"DROP",             0,
		etFROM,					L"FROM",             0,
		etWHERE,				L"WHERE",            0,
        etJOIN,				    L"JOIN",             0,
        etINNER,                L"INNER",            0,
        etLEFT,                 L"LEFT",             0,
        etRIGHT,                L"RIGHT",            0,
        etFULL,                 L"FULL",             0,
        etNATURAL,              L"NATURAL",          0,
        etUSING,                L"USING",            0,
        etON,				    L"ON",               0,
        etORDER,				L"ORDER",            0,
        etGROUP,				L"GROUP",            0,
        etBY,                   L"BY",               0,
		etSET,					L"SET",              0,
		etOPEN,                 L"OPEN",             0,
        etLANGUAGE,             L"LANGUAGE",         0,
        etLESS,                 L"<",                0,
        etGREATER,              L">",                0,
        etTRIGGER,              L"TRIGGER",          0,
        etTYPE,                 L"TYPE",             0,
        etOR,                   L"OR",               0,
        etREPLACE,              L"REPLACE",          0,
        etDOT,                  L".",                0,
        etCOLON,			    L":",                0,
        etEQUAL,                L"=",                0,
        etWITH,                 L"WITH",             0,
        etUNION,                L"UNION",            0,
        etINTERSECT,            L"INTERSECT",        0,
        etMINUS_SQL,            L"MINUS",            0,
        etALL,                  L"ALL",              0,
        etCASE,                 L"CASE",             0,
        etWHEN,                 L"WHEN",             0,
        etGRANT,                L"GRANT",            0,
        etREVOKE,               L"REVOKE",           0,
        etAND,                  L"AND",              0,
        etCOMPILE,              L"COMPILE",          0,
        etJAVA,                 L"JAVA",             0,
    };


    static
    void buildTokenMap (const TokenDesc* descs, size_t size, TokenMap& map)
    {
	    for (size_t i = 0; i < size; i++)
        {
            if (descs[i].minLength)
            {
                wstring keyword = descs[i].keyword;
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
        const wchar_t* word;
        int sql_reserved;
        int plsql_reserved;
    }
    g_11g_reserved[] = 
    {
        L"ACCESS",     1, 0,
        L"ADD",        1, 0,
        L"ALL",        1, 1,
        L"ALTER",      1, 1,
        L"AND",        1, 1,
        L"ANY",        1, 1,
        L"AS",         1, 1,
        L"ASC",        1, 1,
        L"AT",         1, 1,
        L"AUDIT",      1, 0,
        L"BEGIN",      1, 1,
        L"BETWEEN",    1, 1,
        L"BY",         1, 1,
        L"CASE",       1, 1,
        L"CHAR",       1, 0,
        L"CHECK",      1, 1,
        L"CLUSTER",    1, 1,
        L"COLUMN",     1, 0,
        L"COLUMNS",    1, 1,
        L"COMMENT",    1, 0,
        L"COMPRESS",   1, 1,
        L"CONNECT",    1, 1,
        L"CREATE",     1, 1,
        L"CURRENT",    1, 1,
        L"DATE",       1, 0,
        L"DECIMAL",    1, 0,
        L"DECLARE",    1, 1,
        L"DEFAULT",    1, 1,
        L"DELETE",     1, 0,
        L"DESC",       1, 1,
        L"DISTINCT",   1, 1,
        L"DROP",       1, 1,
        L"ELSE",       1, 1,
        L"END",        1, 1,
        L"EXCLUSIVE",  1, 1,
        L"EXISTS",     1, 0,
        L"FILE",       1, 0,
        L"FLOAT",      1, 0,
        L"FOR",        1, 1,
        L"FROM",       1, 1,
        L"GRANT",      1, 1,
        L"GROUP",      1, 1,
        L"HAVING",     1, 1,
        L"IDENTIFIED", 1, 1,
        L"IF",         1, 1,
        L"IMMEDIATE",  1, 0,
        L"IN",         1, 1,
        L"INCREMENT",  1, 0,
        L"INDEX",      1, 1,
        L"INDEXES",    1, 1,
        L"INITIAL",    1, 0,
        L"INSERT",     1, 1,
        L"INTEGER",    1, 0,
        L"INTERSECT",  1, 1,
        L"INTO",       1, 1,
        L"IS",         1, 1,
        L"LEVEL",      1, 0,
        L"LIKE",       1, 1,
        L"LOCK",       1, 1,
        L"LONG",       1, 0,
        L"MAXEXTENTS", 1, 0,
        L"MINUS",      1, 1,
        L"MLSLABEL",   1, 0,
        L"MODE",       1, 1,
        L"MODIFY",     1, 0,
        L"NOAUDIT",    1, 0,
        L"NOCOMPRESS", 1, 1,
        L"NOT",        1, 1,
        L"NOWAIT",     1, 1,
        L"NULL",       1, 1,
        L"NUMBER",     1, 0,
        L"OF",         1, 1,
        L"OFFLINE",    1, 0,
        L"ON",         1, 1,
        L"ONLINE",     1, 0,
        L"OPTION",     1, 1,
        L"OR",         1, 1,
        L"ORDER",      1, 1,
        L"OVERLAPS",   1, 1,
        L"PCTFREE",    1, 0,
        L"PRIOR",      1, 0,
        L"PRIVILEGES", 1, 0,
        L"PROCEDURE",  1, 1,
        L"PUBLIC",     1, 1,
        L"RAW",        1, 0,
        L"RENAME",     1, 0,
        L"RESOURCE",   1, 1,
        L"REVOKE",     1, 1,
        L"ROW",        1, 0,
        L"ROWID",      1, 0,
        L"ROWNUM",     1, 0,
        L"ROWS",       1, 0,
        L"SELECT",     1, 1,
        L"SESSION",    1, 0,
        L"SET",        1, 0,
        L"SHARE",      1, 1,
        L"SIZE",       1, 1,
        L"SMALLINT",   1, 0,
        L"SQL",        1, 1,
        L"START",      1, 1,
        L"SUCCESSFUL", 1, 0,
        L"SYNONYM",    1, 0,
        L"SYSDATE",    1, 0,
        L"TABLE",      1, 1,
        L"THEN",       1, 1,
        L"TO",         1, 1,
        L"TRIGGER",    1, 0,
        L"UID",        1, 0,
        L"UNION",      1, 1,
        L"UNIQUE",     1, 1,
        L"UPDATE",     1, 1,
        L"USER",       1, 0,
        L"VALIDATE",   1, 0,
        L"VALUES",     1, 1,
        L"VARCHAR",    1, 0,
        L"VARCHAR2",   1, 0,
        L"VIEW",       1, 1,
        L"WHEN",       1, 1,
        L"WHENEVER",   1, 0,
        L"WHERE",      1, 1,
        L"WITH",       1, 1,
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

const wchar_t* PlSqlParser::GetStringToken (EToken token)
{
    for (int i(0); i < sizeof(s_scriptTokens)/sizeof(s_scriptTokens[0]); i++)
		if (s_scriptTokens[i].token == token)
			return s_scriptTokens[i].keyword;

    for (int i(0); i < sizeof(s_plsqlTokens)/sizeof(s_plsqlTokens[0]); i++)
		if (s_plsqlTokens[i].token == token)
			return s_plsqlTokens[i].keyword;

	return L"Unknown";
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