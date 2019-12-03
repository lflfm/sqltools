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

// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#pragma once
#include "SQLWorksheet/PlsSqlParser.h"

class PlSqlAnalyser : public SyntaxAnalyser
{
public:
    PlSqlAnalyser ();

    virtual void Clear ();
    virtual void PutToken (const Token&);

    bool IsEmpty () const           { return m_tokens.empty(); }
    bool IsCompleted () const       { return m_state == COMPLETED; }

    const char* GetSqlType () const;
    bool mustHaveClosingSemicolon () const;
    bool mayHaveCompilationErrors () const;
    bool GetNameTokens (Token& owner, Token& name) const;
    Token GetCompilationErrorBaseToken () const;

    static TokenMapPtr getTokenMap ();

    const std::vector<Token> GetBindVarTokens () const { return m_bindVarTokens; }
    Token GetEndOfStatementTocken () const             { return m_endOfStatementTocken; }


private:
    enum EMatch {
        INCOMPLETE_MATCH, EXACT_MATCH, NO_MATCH
    };
    enum EState {
        TYPE_RECOGNITION, NAME_RECOGNITION, PENDING, COMPLETED
    };
    enum ESQLType {
        SQL, WITH_QUERY_12C, BLOCK, FUNCTION, PROCEDURE, PACKAGE, PACKAGE_BODY, TRIGGER, TYPE, TYPE_BODY, JAVA, JAVA_SOURCE
    };

    EState m_state;
    ESQLType m_sqlType;
    std::vector<Token> m_tokens, m_bindVarTokens;
    Token m_nameTokens[2];
    Token m_lastToken, m_endOfStatementTocken;

    struct RecognitionSeq;
    static RecognitionSeq g_recognitionSeq[];
    EMatch findMatch (ESQLType&) const;

    // prohibited
    PlSqlAnalyser (const PlSqlAnalyser&);
    void operator = (const PlSqlAnalyser&);
};
