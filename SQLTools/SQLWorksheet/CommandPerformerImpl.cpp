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

// 22.11.2004 bug fix, all compilation errors are ignored for Oracle7
// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 09.01.2005 (ak) R1092514: SQL*Plus CONNECT/DISCONNECT commands - small improvements
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables
// 28.09.2006 (ak) bXXXXXXX: poor fetch performance because prefetch rows disabled
// 2017-12-15 added support of SYSDBA and SYSOPER to the script CONNECT command

#include "stdafx.h"
#include "SQLTools.h"
#include "CommandPerformerImpl.h"
#include <OCI8/BCursor.h>
#include <OCI8/ACursor.h>
#include <OCI8/Statement.h>
#include "OCI8/SQLFunctionCodes.h"
#include <COMMON/StrHelpers.h>
#include <COMMON/InputDlg.h>
#include "ErrorLoader.h"
#include "ConnectionTasks.h"
#include "BindSource.h" // for BindVarHolder
#include <regex>


CommandPerformerImpl::CommandPerformerImpl (DocumentProxy& proxy, OciConnect& connect, arg::counted_ptr<BindVarHolder>& bindVarHolder, const CommandPerformerSettings& settings)
    : m_proxy(proxy),
    m_connect(connect),
    m_settings(settings),
    m_cursor(new OciAutoCursor(connect, 1000)),
    m_bindVarHolder(bindVarHolder),
    m_statementCount(0),
    m_errors(false),
    m_lastStatementFailed(false),
    m_cancelled(false),
    m_lastStatementIsSelect(false),
	m_lastStatementIsBind(false),
    m_exceptionCount(0)
{
}

std::auto_ptr<OciAutoCursor> CommandPerformerImpl::GetCursorOwnership ()
{
    std::auto_ptr<OciAutoCursor> cursor(new OciAutoCursor(m_connect, 1000));
    std::swap(m_cursor, cursor);
    return cursor;
}

/** @brief Handles execution of an SQL command or PL/SQL block.
 *
 * @arg commandParser Parser from which this was called.
 * @arg sql SQL command.
 */
void CommandPerformerImpl::DoExecuteSql (CommandParser& commandParser, const string& sql, const vector<string>& bindVars)
{
    if (m_settings.autoscrollToExecutedStatement) //TODO#1: check if it is single statement
    {
        m_proxy.ScrollTo(commandParser.GetBaseLine());
    }
    if (m_settings.highlightExecutedStatement)
    {
        int first, last;
        if (commandParser.GetLastExecutedLines(first, last))
            m_proxy.SetHighlighting(first, last, true);
    }

    CheckConnect();
    m_proxy.CheckUserCancel();

    m_statementCount++;
	m_lastStatementIsBind = false;
    m_lastStatementFailed = false;

    bool isOutputEnabled = m_connect.IsOutputEnabled();
    time_t startTime = time((time_t*)0);

    if (commandParser.IsTextSubstituted())
        m_proxy.PutMessage("NEW:\n" + sql, commandParser.GetBaseLine());
    string duration;
    try
    {
        if(m_cursor->IsOpen()) m_cursor->Close();

        m_proxy.SetExecutionStarted(clock64(), commandParser.GetBaseLine());

        m_cursor->Prepare(sql.c_str());

        m_lastStatementIsSelect = (m_cursor->GetType() == OCI8::StmtSelect);

        if (m_lastStatementIsSelect)
        {
            m_cursor->SetNumberFormat      (m_settings.gridNlsNumberFormat);
            m_cursor->SetDateFormat        (m_settings.gridNlsDateFormat);
            m_cursor->SetTimeFormat        (m_settings.gridNlsTimeFormat);
            m_cursor->SetTimestampFormat   (m_settings.gridNlsTimestampFormat);
            m_cursor->SetTimestampTzFormat (m_settings.gridNlsTimestampTzFormat);
            m_cursor->SetStringNull        (m_settings.gridNullRepresentation);
            m_cursor->SetSkipLobs          (m_settings.gridSkipLobs);
            m_cursor->SetStringLimit       (m_settings.gridMaxLobLength);
            m_cursor->SetBlobHexRowLength  (m_settings.gridBlobHexRowLength);
        }

        m_cursor->EnableAutocommit(m_settings.autocommit);

        if (!bindVars.empty())
		    m_bindVarHolder->DoBinds(*m_cursor, bindVars);

        if (isOutputEnabled)
            m_connect.ResetOutput();

        clock64_t startClock = clock64();
        m_cursor->Execute();

        double execTime = double(clock64() - startClock)/ CLOCKS_PER_SEC;
        m_proxy.SetExecutionStarted(0, -1);

        ostringstream message;
        message << m_cursor->GetSQLFunctionDescription();

        ub2 n_SqlFunctionCode = m_cursor->GetSQLFunctionCode();

        switch (m_cursor->GetType())
        {
        case OCI8::StmtInsert:
        case OCI8::StmtUpdate:
        case OCI8::StmtDelete:
        case OCI8::StmtMerge:
            message << " - " << m_cursor->GetRowCount() << " row(s)";
        }
        message << ", executed in ";
        duration = Common::print_time(execTime);
        message << duration;

		m_lastExecTime = execTime;

        m_proxy.PutMessage(message.str(), commandParser.GetBaseLine());

        // TODO#3: recognize changed variables, highlight them and switch to the "bins" page
        if (!bindVars.empty())
        {
            vector<string> data;
            m_bindVarHolder->GetTextData(data);
            m_proxy.RefreshBindView(data);
        }

        // TODO: implement isDML
        if (isOutputEnabled /*&& !m_cursor->IsDML()*/) // even select can write dbms output
            fetchDbmsOutputLines();

        // 22.11.2004 bug fix, all compilation errors are ignored for Oracle7
        // OCI8 does not report properly about sql type in case Oracle7 server
        // so ask parser about sql is parsed
        //if (m_cursor->PossibleCompilationErrors())
        if (commandParser.MayHaveCompilationErrors())
        {
            string name, owner, type;
            commandParser.GetCodeName(owner, name, type);
            m_errors |= ErrorLoader::Load(m_proxy, m_connect, owner.c_str(), name.c_str(), type.c_str(), commandParser.GetErrorBaseLine()) > 0;
        }

        if (n_SqlFunctionCode == OCI8::OFN_ALTER_SESSION)
        {
            m_connect.LoadSessionNlsParameters();
        }

        m_proxy.AddStatementToHistory(startTime, duration, m_connect.GetDisplayString(), sql);
    }
    catch (const OciException& x)
    {
        m_proxy.SetExecutionStarted(0, -1);

        if (x == 0 || !m_connect.IsOpen())
            throw;

        m_exceptionCount++;
        m_errors = true;
        m_lastStatementFailed = true;
        m_cancelled = (x == 1013) ? true : false;
        m_lastStatementIsSelect = (x, m_cursor->GetType() == OCI8::StmtSelect) ? true : false;

        // TODO: implement isDML
        if (isOutputEnabled /*&& !m_cursor->IsDML()*/) // even select can write dbms output
            fetchDbmsOutputLines();

        int line, col;
        commandParser.GetSelectErrorPos(x, x.what(), m_cursor->GetParseErrorOffset(), line, col);

        // ORA-00921: unexpected end of SQL command
        // move error position on last existing line
        // because we need line id for stable error position
        if (x == 921)
            m_proxy.AdjustErrorPosToLastLine(line, col);

        m_proxy.PutError(x.what(), line, col);

        if (!m_settings.historyValidOnly)
            m_proxy.AddStatementToHistory(startTime, duration, m_connect.GetDisplayString(), sql);

        if (!m_connect.IsOpen()) throw;
        // TODO: error filter here!!!
    }
}

void CommandPerformerImpl::DoMessage (CommandParser& commandParser, const string& text)
{
    m_statementCount++;
    m_proxy.PutMessage(text.c_str(), commandParser.GetBaseLine());
}

void CommandPerformerImpl::DoNothing (CommandParser& /*commandParser*/)
{
    m_statementCount++;
}

bool CommandPerformerImpl::InputValue (CommandParser&, const string& prompt, string& value)
{
    return m_proxy.InputValue(prompt, value);
}

void CommandPerformerImpl::GoTo (CommandParser&, int line)
{
    m_proxy.GoTo(line);
}

/** @brief Handles CONNECT command.
 *
 * If a connect string is given, we use that.  Otherwise, display the connect dialog box.
 *
 * @arg commandParser Parser from which this was called.
 * @arg connectStr Connect string.
 */
void CommandPerformerImpl::DoConnect (CommandParser& commandParser, const string& _connectStr)
{
    // 2017-12-15 added support of SYSDBA and SYSOPER to the script CONNECT command
    std::string connectStr;
    std::regex  pattern("[[:space:]]+as[[:space:]]+(sysdba|sysoper)[[:space:]]*$", std::regex_constants::icase|std::regex_constants::extended);
    std::smatch match;

    if (std::regex_search(_connectStr, match, pattern))
    {
        if (match.length() > 1)
        {
            connectStr = match.prefix();
            connectStr += '@';
            connectStr += match.str(1);
        }
    }
    else
    {
        connectStr = _connectStr;
    }

	string user, password, alias, port, sid, mode, service;
    m_statementCount++;
	m_lastStatementIsSelect = false;
	m_lastStatementIsBind = false;

	if (CCommandLineParser::GetConnectionInfo(connectStr, user, password, alias, port, sid, mode, service))
	{
		if (port.empty())
            ConnectionTasks::DoConnect(
                m_connect, 
                user, password, alias, 
                ConnectionTasks::ToConnectionMode(mode), false, false, 
                m_settings.autocommit, m_settings.commitOnDisconnect, 3
            );
		else
			ConnectionTasks::DoConnect(
                m_connect, 
                user, password, alias, port, !service.empty() ? service : sid, !service.empty(), 
                ConnectionTasks::ToConnectionMode(mode), false, false,
                m_settings.autocommit, m_settings.commitOnDisconnect, 3
            );
	}
	else
    {
        GoTo(commandParser, commandParser.GetBaseLine());
		m_proxy.OnSqlConnect();
    }

    string message = (m_connect.IsOpen())
        ? string("Connected to \"") + m_proxy.GetDisplayConnectionString() + '\"'
        : string("Connection failed");

    m_proxy.PutMessage(message, commandParser.GetBaseLine());
}

/** @brief Handles DISCONNECT command.
 *
 * @arg commandParser Parser from which this was called.
 */
void CommandPerformerImpl::DoDisconnect (CommandParser& commandParser)
{
    m_statementCount++;
	m_lastStatementIsSelect = false;
	m_lastStatementIsBind = false;

    GoTo(commandParser, commandParser.GetBaseLine());
	m_proxy.OnSqlDisconnect();
    m_proxy.PutMessage("Disconnected", commandParser.GetBaseLine());
}

/** @brief Verifies connection to database.
 *
 * Throws an exception if not connected.
 */
void CommandPerformerImpl::CheckConnect ()
{
	if (!m_connect.IsOpen())
    {
        ASSERT_EXCEPTION_FRAME;
		throw Common::AppException("SQL command: not connected to database!");
    }
}

/** @brief Adds a bind variable.
 *  See BindGridSource::AddBind().
 */
void CommandPerformerImpl::AddBind (CommandParser& /*commandParser*/, const string& name, EBindVariableTypes type, ub2 size)
{
	m_statementCount++;
	m_lastStatementIsSelect = false;
	m_lastStatementIsBind = true;

    m_bindVarHolder->Append(m_connect, name, type, size);
    vector<string> data;
    m_bindVarHolder->GetTextData(data);
    m_proxy.RefreshBindView(data);
}

void CommandPerformerImpl::PrintBind (CommandParser& commandParser, const string& name)
{
    m_statementCount++;

    arg::counted_ptr<OCI8::Variable> bindVar = m_bindVarHolder->GetBindVariable(name);

    if (bindVar.get())
    {
        CheckConnect();

	    m_lastStatementIsBind = false;
        m_lastStatementIsSelect = false;

        m_cursor->SetNumberFormat      (m_settings.gridNlsNumberFormat);
        m_cursor->SetDateFormat        (m_settings.gridNlsDateFormat);
        m_cursor->SetTimeFormat        (m_settings.gridNlsTimeFormat);
        m_cursor->SetTimestampFormat   (m_settings.gridNlsTimestampFormat);
        m_cursor->SetTimestampTzFormat (m_settings.gridNlsTimestampTzFormat);
        m_cursor->SetStringNull        (m_settings.gridNullRepresentation);
        m_cursor->SetSkipLobs          (m_settings.gridSkipLobs);
        m_cursor->SetStringLimit       (m_settings.gridMaxLobLength);
        m_cursor->SetBlobHexRowLength  (m_settings.gridBlobHexRowLength);
        
        if(m_cursor->IsOpen()) m_cursor->Close();

        if (OCI8::RefCursorVariable* refCur = dynamic_cast<OCI8::RefCursorVariable*>(bindVar.get()))
        {
            if (refCur->IsStateExecuted())
            {
                m_cursor->Attach(*refCur);
                m_lastStatementIsSelect = true;
		        m_lastExecTime = 0;

                m_proxy.PutMessage("PRINT " + name + " (REFCURSOR)", commandParser.GetBaseLine());
            }
            else
            {
                m_proxy.PutError("REFCURSOR " + name + " is not executed!", commandParser.GetBaseLine());
            }
        }
        else
        {
            m_cursor->Prepare(string("select :" + name + " from dual").c_str());
            m_bindVarHolder->DoBinds(*m_cursor, vector<string>(1, name));

            clock64_t startClock = clock64();
            m_cursor->Execute();
            m_lastExecTime = double(clock64() - startClock)/ CLOCKS_PER_SEC;

            m_lastStatementIsSelect = true;

            m_proxy.PutMessage("PRINT " + name, commandParser.GetBaseLine());
        }
    }
    else
        m_proxy.PutError("Bind variable " + name + " not found!", commandParser.GetBaseLine());
}

void CommandPerformerImpl::fetchDbmsOutputLines ()
{
    int nLineSize = 255;
    try
    {
        CWaitCursor wait;

        const int cnArraySize = 100;

        OciNumberVar linesV(m_connect);
        linesV.Assign(cnArraySize);

        // Version 10 and above support line size of 32767
        if (m_connect.GetVersion() >= OCI8::esvServer10X)
        {
            if (m_connect.GetClientVersion() >= OCI8::ecvClient10X)
                nLineSize = 32767;
            else if (m_connect.GetClientVersion() >= OCI8::ecvClient81X)
                nLineSize = 4000;
        }
        OciStringArray output(nLineSize, cnArraySize);

        OciStatement cursor(m_connect);
        cursor.Prepare("BEGIN dbms_output.get_lines(:lines,:numlines); END;");
        cursor.Bind(":lines", output);
        cursor.Bind(":numlines", linesV);

        string buffer;
        cursor.Execute(1, true/*guaranteedSafe*/);
        int lines = linesV.ToInt();

        while (lines > 0)
        {
            for (int i(0); i < lines; i++)
            {
                output.GetString(i, buffer);
                m_proxy.PutDbmsMessage(buffer.c_str());
            }

            cursor.Execute(1);
            lines = linesV.ToInt();
        }
    }
    catch (const OciException& x)
    {
        if (x != 1480)
        {
            ostringstream out;
            if (x == 6502 // ORA-06502: PL/SQL: numeric or value error: host bind array too small
            && nLineSize == 4000)
            {
                out << "Error while retrieving DBMS_OUTPUT data.\n"
                    "The probable cause is a DBMS_OUTPUT line length > 4000 while the current Oracle client supports only 4000.\n"
                    "You might want to upgrade the client to 10g version.\n";
            }
            else
            {
                out << "Error while retrieving DBMS_OUTPUT data.\n";
            }
            out << (const char*)x;
            m_proxy.PutError(out.str());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CommandPerformerImpl
SqlPlanPerformerImpl::SqlPlanPerformerImpl (DocumentProxy& proxy, bool highlightBeforeExec)
    : m_completed(false),
    m_highlightBeforeExec(highlightBeforeExec),
    m_proxy(proxy)
{
}

void SqlPlanPerformerImpl::DoExecuteSql (CommandParser& commandParser, const string& sql, const vector<string>&)
{
    if (m_highlightBeforeExec)
    {
        int first, last;
        if (commandParser.GetLastExecutedLines(first, last))
            m_proxy.SetHighlighting(first, last, true);
    }

    m_completed = true;

    // TODO#2: simplify, ged rid task in DoSqlExplainPlan
    m_proxy.DoSqlExplainPlan(sql);
}

void SqlPlanPerformerImpl::DoMessage (CommandParser&, const string&)
{
    // do nothing
}

void SqlPlanPerformerImpl::DoNothing (CommandParser&)
{
    // do nothing
}

bool SqlPlanPerformerImpl::InputValue (CommandParser&, const string& prompt, string& value)
{
    return m_proxy.InputValue(prompt, value);
}

void SqlPlanPerformerImpl::GoTo (CommandParser&, int line)
{
    m_proxy.GoTo(line);
}

void SqlPlanPerformerImpl::DoConnect (CommandParser&, const string& /*connectStr*/)
{
	// do nothing
}

void SqlPlanPerformerImpl::DoDisconnect (CommandParser&)
{
	// do nothing
}

void SqlPlanPerformerImpl::AddBind (CommandParser&, const string&, EBindVariableTypes, ub2)
{
	// do nothing
}

bool DummyPerformerImpl::InputValue (CommandParser&, const string& prompt, string& value)
{
    return m_proxy.InputValue(prompt, value);
}

