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

// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#ifndef __CommandParser_H__
#define __CommandParser_H__
#pragma once

#include <map>
#include <vector>
#include <string>
#include "PlsSqlParser.h"
#include "PlSqlAnalyser.h"
#include "ScriptAnalyser.h"
#include "BindVariableTypes.h"
#include "OCI8\Datatypes.h"

    class CommandParser;

    class CommandPerformer {
    public:
        virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>&) = 0;
        virtual void DoMessage    (CommandParser&, const string& msg) = 0;
        virtual void DoNothing    (CommandParser&) = 0;
        virtual bool InputValue   (CommandParser&, const string& prompt, string& value) = 0;
        virtual void GoTo         (CommandParser&, int line) = 0;
		virtual void DoConnect    (CommandParser&, const string& connectStr) = 0;
		virtual void DoDisconnect (CommandParser&) = 0;
		virtual void AddBind	  (CommandParser&, const string& var, EBindVariableTypes type, ub2 size) = 0;
		virtual void PrintBind	  (CommandParser&, const string& var) = 0;
    };

/** @brief Handles the parsing and execution of a single script command.
 */
class CommandParser
{
public:
    CommandParser (CommandPerformer&, bool substitution, bool ignoreCommmentsAfterSemicolon);
    virtual ~CommandParser ();

    void SetSqlOnly (bool sqlOnly)          { m_sqlOnly = sqlOnly; }

    void Start (int line, int column, bool inPlSqlMode = false);
    void Restart ();

    void PutLine (const char* str, int len);
    void PutEOF  ();

    bool IsEmpty () const                   { return m_scriptAnalyser.IsEmpty() && m_plsqlAnalyser.IsEmpty(); }
    bool IsCompleted () const               { return m_scriptAnalyser.IsCompleted() || m_plsqlAnalyser.IsCompleted(); }
    bool IsExecutable () const              { return !m_plsqlAnalyser.IsEmpty() && m_plsqlAnalyser.IsCompleted(); }

    int  GetBaseLine () const               { return m_baseLine; }
    bool IsTextSubstituted () const         { return m_textSubstituted; }

    bool MayHaveCompilationErrors () const  { return (m_parser.GetAnalyzer() == &m_plsqlAnalyser && m_plsqlAnalyser.mayHaveCompilationErrors()); }

    void GetCodeName (std::string& owner, std::string& name, std::string& type) const;
    void GetSelectErrorPos (int error, const char* error_text, int offset, int& line, int& column) const;
    int  GetErrorBaseLine () const;
    bool GetLastExecutedLines (int& first, int& last) const;

    void SetSkipDefineUndefine ()           { m_skipDefineUndefine = true; }

private:
    std::vector<std::string> getBindVariables (const string& text, const std::vector<Token>& bindVarTokens) const;
    void finalizeAndExecuteSql ();
    void makeSubstitution (const char* str, int len, string& buffer);

    void doPrompt   (const std::vector<Token>&, const string& text);
    void doRemark   (const std::vector<Token>&, const string& text);
    void doDefine   (const std::vector<Token>&, const string& text);
    void doUndefine (const std::vector<Token>&, const string& text);
    void doExecute  (const std::vector<Token>&, const string& text);
    void doDescribe (const std::vector<Token>&, const string& text);
	void doConnect	(const std::vector<Token>&, const string& text);
	void doDisconnect (const std::vector<Token>&, const string& text);
	void doVariable (const std::vector<Token>&, const string& text);
	void doPrint    (const std::vector<Token>&, const string& text);

    void doUnsupported (const std::vector<Token>&, const string& text);
    void doIncompleted (const std::vector<Token>&, const string& text);

    CommandPerformer& m_performer;
    PlsSqlParser   m_parser;
    ScriptAnalyser m_scriptAnalyser;
    PlSqlAnalyser  m_plsqlAnalyser;
    bool m_sqlOnly; // only sql allowed, no sql*plus commands
    bool m_substitution;
    bool m_ignoreCommmentsAfterSemicolon;

    // this is execution context, has to be reset after each statement
    std::string m_text;
    int m_lines/*, m_baseErrorLine*/;
    int m_baseLine, m_baseColumn;
    int m_lastExecutedBaseLine, m_lastExecutedLines;
    bool m_textSubstituted, m_exeCommand;
    bool m_skipDefineUndefine;

    static std::map<std::string,std::string> m_substitutions;

    // prohibited
    CommandParser (const CommandParser&);
    void operator = (const CommandParser&);
};

    inline
    void CommandParser::Restart () { Start(m_baseLine + m_lines, 0); }

    inline
    bool CommandParser::GetLastExecutedLines (int& first, int& last) const
    {
        if (m_lastExecutedBaseLine != -1)
        {
            first = m_lastExecutedBaseLine;
            last = m_lastExecutedBaseLine + m_lastExecutedLines - 1;
            return true;
        }
        return false;
    }

class CommandParserException : public Common::AppException
{
public:
    CommandParserException (const std::string& text)
        : Common::AppException(text) {}
};

#endif//__CommandParser_H__
