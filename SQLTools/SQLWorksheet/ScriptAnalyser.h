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

// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#pragma once

#include <vector>
#include "SQLWorksheet/PlsSqlParser.h" 

class ScriptAnalyser : public SyntaxAnalyser
{
public:
    ScriptAnalyser () : m_state(PENDING) {}

    virtual void Clear ();
    virtual void PutToken (const Token&);

    bool IsEmpty () const           { return m_state == PENDING && m_tokens.empty(); }
    bool IsCompleted () const       { return m_state == COMPLETED; }
    bool IsUnrecognized () const    { return m_state == UNRECOGNIZED; }

    const std::vector<Token>& GetTokens () const { return m_tokens; }
    const std::vector<Token> GetBindVarTokens () const { return m_bindVarTokens; }

    static TokenMapPtr getTokenMap ();

private:
    enum EState { PENDING, RECOGNIZED, COMPLETED, UNRECOGNIZED } m_state; 
    std::vector<Token> m_tokens;
    std::vector<Token> m_bindVarTokens;
    Token m_lastToken; 

    // prohibited
    ScriptAnalyser (const ScriptAnalyser&);
    void operator = (const ScriptAnalyser&);
};
