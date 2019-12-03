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
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#include "stdafx.h"
#include "ScriptAnalyser.h"

    // tokens for script mode
	static struct 
    {
		EToken token;
		const char* keyword;
        unsigned int minLength;
	} 
    g_scriptTokens[] = 
    {
		etSEMICOLON,	    ";",                1,        
		etQUOTE,		    "'",	            1,	
		etDOUBLE_QUOTE,	    "\"",	            1,	
		etMINUS,		    "-",	            1,	
		etSLASH,		    "/",	            1,	
		etSTAR,			    "*",                1,
		etCOMMERCIAL_AT,    "@",                1,
        etLEFT_ROUND_BRACKET,	"(",			1,		
		etRIGHT_ROUND_BRACKET,	")",			1,		
        etACCEPT,           "ACCEPT",           3,
        etARCHIVE,          "ARCHIVE",          7,
        etAPPEND,           "APPEND",           1,
        etATTRIBUTE,        "ATTRIBUTE",        9,
        etBREAK,            "BREAK",            3,
        etBTITLE,           "BTITLE",           3,
        etCLEAR,            "CLEAR",            2,
        etCOLUMN,           "COLUMN",           3,
        etCOMPUTE,          "COMPUTE",          4,
        etCONNECT,          "CONNECT",          4,
        etCOPY,             "COPY",             4,
        etDEFINE,           "DEFINE",           3,
        etDESCRIBE,         "DESCRIBE",         4,
        etDISCONNECT,       "DISCONNECT",       4,
        etEXECUTE,          "EXECUTE",          4,
        etEXIT,             "EXIT",             4,
        etEXIT,             "QUIT",             4,
        etHELP,             "HELP",             4,
        etHOST,             "HOST",             4,
        etPAUSE,            "PAUSE",            3,
        etPRINT,            "PRINT",            3,
        etPROMPT,           "PROMPT",           3,
        etRECOVER,          "RECOVER",          7,
        etREMARK,           "REMARK",           3,
        etREPFOOTER,        "REPFOOTER",        9,
        etREPHEADER,        "REPHEADER",        9,
        etRUN,              "RUN",              1,
        etSET,              "SET",              3,
        etSHOW,             "SHOW",             3,
        etSHUTDOWN,         "SHUTDOWN",         8,
        etSPOOL,            "SPOOL",            3,
        etSTART,            "START",            3,
        etSTARTUP,          "STARTUP",          7,
        etTIMING,           "TIMING",           6,
        etTTITLE,           "TTITLE",           3,
        etUNDEFINE,         "UNDEFINE",         5,
        etVARIABLE,         "VARIABLE",         3,
        etWHENEVER,         "WHENEVER",         8,
        // SET parameters
        etAPPINFO,          "APPINFO",          2,
        etARRAYSIZE,        "ARRAYSIZE",        5,
        etAUTOCOMMIT,       "AUTOCOMMIT",       4,
        etAUTOPRINT,        "AUTOPRINT",        5,
        etAUTORECOVERY,     "AUTORECOVERY",     12,
        etAUTOTRACE,        "AUTOTRACE",        5,
        etBLOCKTERMINATOR,  "BLOCKTERMINATOR",  3,
        etCMDSEP,           "CMDSEP",           4,
        etCOLSEP,           "COLSEP",           6,
        etCOMPATIBILITY,    "COMPATIBILITY",    3,
        etCONCAT,           "CONCAT",           3,
        etCOPYCOMMIT,       "COPYCOMMIT",       5,
        etCOPYTYPECHECK,    "COPYTYPECHECK",    13,
        etDEFINE,           "DEFINE",           3,
        etDESCRIBE,         "DESCRIBE",         8,
        etECHO,             "ECHO",             4,
        etEDITFILE,         "EDITFILE",         5,
        etEMBEDDED,         "EMBEDDED",         3,
        etESCAPE,           "ESCAPE",           3,
        etFEEDBACK,         "FEEDBACK",         4,
        etFLAGGER,          "FLAGGER",          7,
        etFLUSH,            "FLUSH",            3,
        etHEADING,          "HEADING",          3,
        etHEADSEP,          "HEADSEP",          5,
        etINSTANCE,         "INSTANCE",         8,
        etLINESIZE,         "LINESIZE",         3,
        etLOBOFFSET,        "LOBOFFSET",        5,
        etLOGSOURCE,        "LOGSOURCE",        9,
        etLONG,             "LONG",             4,
        etLONGCHUNKSIZE,    "LONGCHUNKSIZE",    5,
        etMARKUP,           "MARKUP",           4,
        etNEWPAGE,          "NEWPAGE",          4,
        etNULL,             "NULL",             4,
        etNUMFORMAT,        "NUMFORMAT",        4,
        etNUMWIDTH,         "NUMWIDTH",         3,
        etPAGESIZE,         "PAGESIZE",         5,
        etPAUSE,            "PAUSE",            3,
        etRECSEP,           "RECSEP",           6,
        etRECSEPCHAR,       "RECSEPCHAR",       10,
        etSCAN,             "SCAN",             4,
        etSERVEROUTPUT,     "SERVEROUTPUT",     9,
        etSHIFTINOUT,       "SHIFTINOUT",       5,
        etSHOWMODE,         "SHOWMODE",         4,
        etSQLBLANKLINES,    "SQLBLANKLINES",    5,
        etSQLCASE,          "SQLCASE",          4,
        etSQLCONTINUE,      "SQLCONTINUE",      5,
        etSQLNUMBER,        "SQLNUMBER",        4,
        etSQLPLUSCOMPATIBILITY, "SQLPLUSCOMPATIBILITY", 13,
        etSQLPREFIX,        "SQLPREFIX",        6,
        etSQLPROMPT,        "SQLPROMPT",        4,
        etSQLTERMINATOR,    "SQLTERMINATOR",    4,
        etSUFFIX,           "SUFFIX",           3,
        etTAB,              "TAB",              3,
        etTERMOUT,          "TERMOUT",          4,
        etTIME,             "TIME",             2,
        etTIMING,           "TIMING",           4,
        etTRIMOUT,          "TRIMOUT",          4,
        etTRIMSPOOL,        "TRIMSPOOL",        5,
        etUNDERLINE,        "UNDERLINE",        3,
        etVERIFY,           "VERIFY",           3,
        etWRAP,             "WRAP",             3,
        etEQUAL,            "=",                1,       
        etCOLON,            ":",                1,       
    };

TokenMapPtr ScriptAnalyser::getTokenMap ()
{
    TokenMapPtr map(new std::map<std::string, int>);
	for (int i(0); i < sizeof(g_scriptTokens)/sizeof(g_scriptTokens[0]); i++)
    {
        string keyword = g_scriptTokens[i].keyword;
        while (keyword.length() > 0
        && keyword.length() >=  g_scriptTokens[i].minLength)
        {
		    map->insert(std::map<string, int>::value_type(keyword, g_scriptTokens[i].token));
            keyword.resize(keyword.length()-1);
        } 
    }
    return map;
}

void ScriptAnalyser::Clear ()
{
    m_state = PENDING; 
    m_tokens.clear();
    m_bindVarTokens.clear();
    m_lastToken = Token(); 
}

void ScriptAnalyser::PutToken (const Token& token)
{
    if (token == etCOMMENT) return;

    switch (m_state)
    {
    case PENDING:
        switch (token)
        {
        case etACCEPT:
        case etARCHIVE:
        case etAPPEND:
        case etATTRIBUTE:
        case etBREAK:
        case etBTITLE:
        case etCLEAR:
        case etCOLUMN:
        case etCOMPUTE:
        case etCONNECT:
        case etCOPY:
        case etDEFINE:
        case etDESCRIBE:
        case etDISCONNECT:
        case etEXECUTE:
        case etEXIT:
        case etHELP:
        case etHOST:
        case etPAUSE:
        case etPRINT:
        case etPROMPT:
        case etRECOVER:
        case etREMARK:
        case etREPFOOTER:
        case etREPHEADER:
        case etRUN:
        case etSET:
        case etSHOW:
        case etSHUTDOWN:
        case etSPOOL:
        case etSTART:
        case etSTARTUP:
        case etTIMING:
        case etTTITLE:
        case etUNDEFINE:
        case etVARIABLE:
        case etWHENEVER:
        case etCOMMERCIAL_AT:
            m_state = RECOGNIZED;
            m_tokens.push_back(token);
            break;
        case etCOMMENT:
        case etEOL:
        case etEOF:
            break;
        default:
            m_state = UNRECOGNIZED;
            ;
        }
        break;
    case RECOGNIZED:
        switch (token)
        {
        case etEOL:
        case etEOF:
            m_state = COMPLETED;
            break;
        default:
            if (m_tokens.size() == 1 
            && m_tokens.at(0) == etSET)
            {
                switch (token)
                {
                default:
                    m_tokens.clear();
                    m_state = UNRECOGNIZED;
                    break;
                case etAPPINFO:
                case etARRAYSIZE:
                case etAUTOCOMMIT:
                case etAUTOPRINT:
                case etAUTORECOVERY:
                case etAUTOTRACE:
                case etBLOCKTERMINATOR:
                case etCMDSEP:
                case etCOLSEP:      
                case etCOMPATIBILITY:
                case etCONCAT:
                case etCOPYCOMMIT:
                case etCOPYTYPECHECK:
                case etDEFINE:
                case etDESCRIBE:
                case etECHO:         
                case etEDITFILE:     
                case etEMBEDDED:     
                case etESCAPE:       
                case etFEEDBACK:     
                case etFLAGGER:      
                case etFLUSH:        
                case etHEADING:      
                case etHEADSEP:      
                case etINSTANCE:     
                case etLINESIZE:     
                case etLOBOFFSET:    
                case etLOGSOURCE:    
                case etLONG:         
                case etLONGCHUNKSIZE:
                case etMARKUP:       
                case etNEWPAGE:      
                case etNULL:         
                case etNUMFORMAT:    
                case etNUMWIDTH:     
                case etPAGESIZE:     
                case etPAUSE:
                case etRECSEP:       
                case etRECSEPCHAR:   
                case etSCAN: 
                case etSERVEROUTPUT: 
                case etSHIFTINOUT:   
                case etSHOWMODE:     
                case etSQLBLANKLINES:
                case etSQLCASE:      
                case etSQLCONTINUE:  
                case etSQLNUMBER:    
                case etSQLPLUSCOMPATIBILITY:
                case etSQLPREFIX:    
                case etSQLPROMPT:    
                case etSQLTERMINATOR:
                case etSUFFIX:       
                case etTAB:          
                case etTERMOUT:      
                case etTIME:         
                case etTIMING:
                case etTRIMOUT:      
                case etTRIMSPOOL:    
                case etUNDERLINE:    
                case etVERIFY:       
                case etWRAP: 
                    ; // this is a known SET parameter
                }
            }

            if (!m_tokens.empty() 
            && m_tokens.at(0) == etEXECUTE)
            {
                if (m_lastToken == etCOLON && token != etEQUAL)
                    m_bindVarTokens.push_back(token);

                m_lastToken = token;
            }

            if (m_state != UNRECOGNIZED)
                m_tokens.push_back(token);
            break;
        }
        break;
    case UNRECOGNIZED:
        ;// just skip
    }
}
