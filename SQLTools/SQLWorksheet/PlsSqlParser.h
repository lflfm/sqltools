/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

// 25.12.2004 (ak) added SimpleSyntaxAnalyser - a simple helper class
// 13.03.2005 (ak) R1105003: Bind variables

#pragma once
#ifndef __PlsSqlParser_h__
#define __PlsSqlParser_h__

#include <map>
#include <string>
#include "arg_shared.h"
#include "Common/Fastmap.h"


    using std::string;
    using std::vector;
    using Common::Fastmap;

    typedef std::map<std::string, int> TokenMap;
    typedef arg::counted_ptr<TokenMap> TokenMapPtr;

    enum ParserMode
    {
        emSCRIPT,
        emPLSQL
    };

    enum EToken
    {
        etNONE = -1,
        etUNKNOWN = 0,

        // tokens for pl/sql
        etPROCEDURE,
        etFUNCTION,
        etPACKAGE,
        etBODY,
        etDECLARE,
        etBEGIN,
        etEXCEPTION,
        etEND,
        etIF,
        etTHEN,
        etELSE,
        etELSIF,
        etFOR,
        etWHILE,
        etLOOP,
        etEXIT,
        etDOT,
        etCOLON,
        etSEMICOLON,
        etQUOTE,
        etDOUBLE_QUOTE,
        etLEFT_ROUND_BRACKET,
        etRIGHT_ROUND_BRACKET,
        etMINUS,
        etSLASH,
        etSTAR,
        etIS,
        etAS,
        etSELECT,
        etINSERT,
        etUPDATE,
        etDELETE,
        etALTER,
        etANALYZE,
        etCREATE,
        etDROP,
        etFROM,
        etWHERE,
        etSET,
        etOPEN,
        etLANGUAGE,
        etLESS,
        etGREATER, 
        etTRIGGER,
        etTYPE,
        etOR,
        etREPLACE,
        etAT_SIGN,
        etPERCENT_SIGN,
        etEQUAL,
        etJAVA,
        etSOURCE,
        etAND,
        etCOMPILE,
        etNAMED,
        etWITH,

        // second level tokens
        etEOL,
        etEOF,
        etEOS, // end of statement
        etNUMBER,
        etDELIMITER,
        etIDENTIFIER,
        etQUOTED_STRING,
        etDOUBLE_QUOTED_STRING,
        etCOMMENT,

        // tokens for script mode
        etCOMMERCIAL_AT,
        etACCEPT,
        etARCHIVE,
        etAPPEND,
        etATTRIBUTE,
        etBREAK,
        etBTITLE,
        etCLEAR,
        etCOLUMN,
        etCOMPUTE,
        etCONNECT,
        etCOPY,
        etDEFINE,
        etDESCRIBE,
        etDISCONNECT,
        etEXECUTE,
        //etEXIT, defined for pl/sql
        etHELP,
        etHOST,
        etPAUSE,
        etPRINT,
        etPROMPT,
        etRECOVER,
        etREMARK,
        etREPFOOTER,
        etREPHEADER,
        etRUN,
        //etSET, defined for pl/sql
        etSHOW,
        etSHUTDOWN,
        etSPOOL,
        etSTART,
        etSTARTUP,
        etTIMING,
        etTTITLE,
        etUNDEFINE,
        etVARIABLE,
        etWHENEVER,
        // SET parameters
        etAPPINFO,
        etARRAYSIZE,
        etAUTOCOMMIT,
        etAUTOPRINT,
        etAUTORECOVERY,
        etAUTOTRACE,   
        etBLOCKTERMINATOR,
        etCMDSEP,      
        etCOLSEP,      
        etCOMPATIBILITY,
        etCONCAT,       
        etCOPYCOMMIT,   
        etCOPYTYPECHECK,
        //etDEFINE, defined as command      
        //etDESCRIBE, defined as command      
        etECHO,         
        etEDITFILE,     
        etEMBEDDED,     
        etESCAPE,       
        etFEEDBACK,     
        etFLAGGER,      
        etFLUSH,        
        etHEADING,      
        etHEADSEP,      
        etINSTANCE,     
        etLINESIZE,     
        etLOBOFFSET,    
        etLOGSOURCE,    
        etLONG,         
        etLONGCHUNKSIZE,
        etMARKUP,       
        etNEWPAGE,      
        etNULL,         
        etNUMFORMAT,    
        etNUMWIDTH,     
        etPAGESIZE,     
        //etPAUSE, defined as command
        etRECSEP,       
        etRECSEPCHAR, 
        etSCAN,
        etSERVEROUTPUT, 
        etSHIFTINOUT,   
        etSHOWMODE,     
        etSQLBLANKLINES,
        etSQLCASE,      
        etSQLCONTINUE,  
        etSQLNUMBER,    
        etSQLPLUSCOMPATIBILITY,
        etSQLPREFIX,    
        etSQLPROMPT,    
        etSQLTERMINATOR,
        etSUFFIX,       
        etTAB,          
        etTERMOUT,      
        etTIME,         
        //etTIMING, defined as command
        etTRIMOUT,      
        etTRIMSPOOL,    
        etUNDERLINE,    
        etVERIFY,       
        etWRAP,         

        // for TRIGGER header parsing
        etON,
        etBEFORE,
        etAFTER,
        etINSTEAD,
        etCOMPOUND,
    };

// TODO: This is a single line token but strings or comments might be multiline
    struct Token
    {
        EToken token;
        int line, offset, length;
        string value;

        int begCol () const { return offset; }
        int endCol () const { return offset + length; }

        Token () : token(etNONE), line(0), offset(0), length(0) {}
        operator EToken () const { return token; }
        Token& operator = (EToken tk) { token = tk; return *this; }
    };

    class SyntaxAnalyser
    {
    public:
        virtual ~SyntaxAnalyser () {};
        virtual void Clear () = 0;
        virtual void PutToken (const Token&) = 0;
    };

    class SimpleSyntaxAnalyser : public SyntaxAnalyser
    {
    public:
        const std::vector<Token> GetTokens () const { return m_tokens; }

        virtual void Clear ()                   { m_tokens.clear(); }
        virtual void PutToken (const Token& tk) { m_tokens.push_back(tk); }
    private:
        std::vector<Token> m_tokens;
    };

// TODO: this parser does not handle a sequence of 2 single quotes
// so we can get 2 literal strings insted of one but this is not an issue for now
class PlsSqlParser
{
public:
    PlsSqlParser (SyntaxAnalyser*, TokenMapPtr);
    void Clear ();
    void SetTokenMap (TokenMapPtr);
    TokenMapPtr GetTokenMapPtr () const { return m_tokenMap; }

    virtual bool PutLine (int line, const char*, int length);
    void PutEOF  (int line);

    void SetAnalyzer (SyntaxAnalyser* analyzer) { m_analyzer = analyzer; m_analyzer->Clear(); }
    SyntaxAnalyser* GetAnalyzer () const        { return m_analyzer; }

    bool IsEmpty () const                       { return m_sequenceOf == eNone; }

protected:
    SyntaxAnalyser* m_analyzer;
    TokenMapPtr m_tokenMap;

    enum ESequenceOf { eNone, eQuotedString, eDblQuotedString, eEndLineComment, eComment };
    ESequenceOf m_sequenceOf;
    int m_sequenceOfLength;
    Token m_sequenceToken;

private:
    // prohibited
    PlsSqlParser (const PlsSqlParser&) {};
    void operator = (const PlsSqlParser&) {};
};

#endif//__PlsSqlParser_h__
