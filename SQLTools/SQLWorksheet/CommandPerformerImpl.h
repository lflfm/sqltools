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

// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#pragma once
#include "CommandParser.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include "DocumentProxy.h"

    struct CommandPerformerSettings
    {
        bool   autocommit              ;
        int    gridBlobHexRowLength    ;
        string gridNlsDateFormat       ;
        string gridNlsNumberFormat     ;
        bool   gridSkipLobs            ;
        int    gridMaxLobLength        ;
        string gridNullRepresentation  ;
        string gridNlsTimeFormat       ;
        string gridNlsTimestampFormat  ;
        string gridNlsTimestampTzFormat;
        bool   historyValidOnly        ;
        int    commitOnDisconnect      ;
        bool   autoscrollToExecutedStatement;
        bool   highlightExecutedStatement;
        bool   ignoreCommmentsAfterSemicolon;

        CommandPerformerSettings (const SQLToolsSettings& settings)
        {
            autocommit               = settings.GetAutocommit              ();
            gridBlobHexRowLength     = settings.GetGridBlobHexRowLength    ();
            gridNlsDateFormat        = settings.GetGridNlsDateFormat       ();
            gridNlsNumberFormat      = settings.GetGridNlsNumberFormat     ();
            gridSkipLobs             = settings.GetGridSkipLobs            ();
            gridMaxLobLength         = settings.GetGridMaxLobLength        ();
            gridNullRepresentation   = settings.GetGridNullRepresentation  ();
            gridNlsTimeFormat        = settings.GetGridNlsTimeFormat       ();
            gridNlsTimestampFormat   = settings.GetGridNlsTimestampFormat  ();
            gridNlsTimestampTzFormat = settings.GetGridNlsTimestampTzFormat();
            historyValidOnly         = settings.GetHistoryValidOnly        ();
            commitOnDisconnect       = settings.GetCommitOnDisconnect      ();
            autoscrollToExecutedStatement = settings.GetAutoscrollToExecutedStatement();
            highlightExecutedStatement    = settings.GetHighlightExecutedStatement   ();
            ignoreCommmentsAfterSemicolon = settings.GetIgnoreCommmentsAfterSemicolon();
        }
    };

/** @brief Executes script commands
 */
class CommandPerformerImpl : public CommandPerformer
{
public:
    CommandPerformerImpl (DocumentProxy& proxy, OciConnect& connect, arg::counted_ptr<BindVarHolder>& bindVarHolder, const CommandPerformerSettings&);

    virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>& bindVariables);
    virtual void DoMessage    (CommandParser&, const string& msg);
    virtual void DoNothing    (CommandParser&);
    virtual bool InputValue   (CommandParser&, const string& prompt, string& value);
    virtual void GoTo         (CommandParser&, int line);
	virtual void DoConnect    (CommandParser&, const string& connectStr);
	virtual void DoDisconnect (CommandParser&);
	virtual void CheckConnect ();
	virtual void AddBind	  (CommandParser&, const string& var, EBindVariableTypes type, ub2 size);
    virtual void PrintBind	  (CommandParser&, const string& var);

    int  GetStatementCount () const     { return m_statementCount; }
    int  GetExceptionCount () const     { return m_exceptionCount; }
    bool HasErrors () const             { return m_errors; }
    bool IsLastStatementFailed () const { return m_lastStatementFailed; }
    bool IsCancelled () const           { return m_cancelled; }
    bool IsLastStatementSelect () const { return m_lastStatementIsSelect; }
	bool IsLastStatementBind () const	{ return m_lastStatementIsBind; }
    double GetLastExecTime () const     { return m_lastExecTime; }

    std::auto_ptr<OciAutoCursor> GetCursorOwnership ();
private:
    DocumentProxy m_proxy;
    CommandPerformerSettings m_settings;
    OciConnect& m_connect;
    std::auto_ptr<OciAutoCursor> m_cursor;
    arg::counted_ptr<BindVarHolder> m_bindVarHolder;

    int m_statementCount, m_exceptionCount;
    bool m_errors, m_lastStatementFailed, m_cancelled, m_lastStatementIsSelect, m_lastStatementIsBind;
	double m_lastExecTime;

    void refreshBinds ();
    void fetchDbmsOutputLines ();

    // prohibited
    CommandPerformerImpl (const CommandPerformerImpl&);
    void operator = (const CommandPerformerImpl&);
};

/** @brief Executes explain plan.
 */
class SqlPlanPerformerImpl : public CommandPerformer
{
public:
    SqlPlanPerformerImpl (DocumentProxy& proxy, bool highlightBeforeExec);

    bool IsCompleted () const           { return m_completed; }

    virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>& bindVariables);
    virtual void DoMessage    (CommandParser&, const string& msg);
    virtual void DoNothing    (CommandParser&);
    virtual bool InputValue   (CommandParser&, const string& prompt, string& value);
    virtual void GoTo         (CommandParser&, int line);
	virtual void DoConnect    (CommandParser&, const string& connectStr);
	virtual void DoDisconnect (CommandParser&);
	virtual void AddBind	  (CommandParser&, const string& var, EBindVariableTypes type, ub2 size);
    virtual void PrintBind	  (CommandParser&, const string&) {}


private:
    DocumentProxy m_proxy;
    bool m_completed;
    bool m_highlightBeforeExec;
    // prohibited
    SqlPlanPerformerImpl (const SqlPlanPerformerImpl&);
    void operator = (const SqlPlanPerformerImpl&);
};

class DummyPerformerImpl : public CommandPerformer
{
public:
    DummyPerformerImpl (DocumentProxy& proxy) : m_proxy(proxy) {};

    virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>& /*bindVariables*/) { m_sqlText = sql; }
    virtual void DoMessage    (CommandParser&, const string& /*msg*/) {}
    virtual void DoNothing    (CommandParser&) {}
    virtual bool InputValue   (CommandParser&, const string& /*prompt*/, string& /*value*/);
    virtual void GoTo         (CommandParser&, int /*line*/) {}
	virtual void DoConnect    (CommandParser&, const string& /*connectStr*/) {}
	virtual void DoDisconnect (CommandParser&) {}
	virtual void AddBind	  (CommandParser&, const string& /*var*/, EBindVariableTypes /*type*/, ub2 /*size*/) {}
    virtual void PrintBind	  (CommandParser&, const string&) {}

    const string& GetSqlText () const { return m_sqlText; }

private:
    DocumentProxy m_proxy;
    string m_sqlText;
    // prohibited
    DummyPerformerImpl (const DummyPerformerImpl&);
    void operator = (const DummyPerformerImpl&);
};
