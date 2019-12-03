/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2016 Aleksey Kochetov

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

#include "stdafx.h"
#include "DocumentProxy.h"
#include "ErrorLoader.h"
#include <OCI8/Statement.h>
#include <COMMON/InputDlg.h>

    DocumentProxy::document_destroyed::document_destroyed ()
        : std::exception("The execution failed because the documnet was closed!")
    {
    }

    static void switch_to_running (CPLSWorksheetDoc& doc)
    {
        if (CWnd* pWnd = AfxGetMainWnd())
        {
            if (!pWnd->IsTopParentActive())
            {
                if (pWnd->IsIconic())
                    pWnd->ShowWindow(SW_RESTORE);
                else
                    // does not activate actually but it shows "waitng" animation on the taskbar
                    pWnd->SetForegroundWindow(); 
            }
        }

        COEditorView* pEditor = doc.GetEditorView();
        if (CFrameWnd* pFrame = pEditor->GetParentFrame()) 
        {
            if (pFrame->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
            {
                static_cast<CMDIChildWnd*>(pFrame)->MDIActivate();
                if (pFrame->IsIconic())
                    static_cast<CMDIChildWnd*>(pFrame)->MDIRestore();
            }
            pFrame->SetActiveView(pEditor);
        }
    }

DocumentProxy::DocumentProxy (CPLSWorksheetDoc& doc) 
    : m_doc(doc),
    m_hMainDocWnd(doc.GetEditorView()->m_hWnd),
    m_msgWnd(ThreadCommunication::MessageOnlyWindow::GetWindow()) 
{
}

bool DocumentProxy::IsOk ()
{
    return IsWindow(m_hMainDocWnd) ? true : false;
}

void DocumentProxy::TestDocumnet ()
{
    if (!IsWindow(m_hMainDocWnd))
    {
        ASSERT_EXCEPTION_FRAME; 
        throw document_destroyed();
    }
}

void DocumentProxy::send (ThreadCommunication::Note& note)
{
    TestDocumnet();
    m_msgWnd.Send(note);
}

///////////////////////////////////////////////////////////////////////////////
    struct ActivePrimeExecutionNote : public ThreadCommunication::Note
    {
        bool m_on;

        ActivePrimeExecutionNote (bool on) : m_on(on) {}

        virtual void Deliver () 
        {
            theApp.SetActivePrimeExecution(m_on);
        }
    };

void DocumentProxy::SetActivePrimeExecution (bool on)
{
    // do not care if document is already closed
    m_msgWnd.Send(ActivePrimeExecutionNote(on));
}

///////////////////////////////////////////////////////////////////////////////
    struct SetHighlightingNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int  m_fistHighlightedLine; int m_lastHighlightedLine; bool m_updateWindow;

        SetHighlightingNote (CPLSWorksheetDoc& doc, int fistHighlightedLine, int lastHighlightedLine, bool updateWindow)
            : m_doc(doc), 
            m_fistHighlightedLine(fistHighlightedLine), m_lastHighlightedLine(lastHighlightedLine), m_updateWindow(updateWindow)
        {
        }

        virtual void Deliver () 
        {
            m_doc.SetHighlighting(m_fistHighlightedLine, m_lastHighlightedLine, m_updateWindow);
        }
    };

void DocumentProxy::SetHighlighting  (int fistHighlightedLine, int lastHighlightedLine, bool updateWindow)
{
    send(SetHighlightingNote(m_doc, fistHighlightedLine, lastHighlightedLine, updateWindow));
}

///////////////////////////////////////////////////////////////////////////////
    struct GoToNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int  m_line;

        GoToNote (CPLSWorksheetDoc& doc, int line)
            : m_doc(doc), 
            m_line(line)
        {
        }

        virtual void Deliver () 
        {
            //m_doc.GoTo(m_line);
            m_doc.GetEditorView()->SetExecutionLine(m_line); // it scrolls vieport to the line w/o moving cursor position
        }
    };

void DocumentProxy::GoTo (int line)
{
    send(GoToNote(m_doc, line));
}

///////////////////////////////////////////////////////////////////////////////
    struct PutErrorNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        const string& m_str; int m_line; int m_pos; bool m_skip;

        PutErrorNote (CPLSWorksheetDoc& doc, const string& str, int line, int pos, bool skip)
            : m_doc(doc), 
            m_str(str), m_line(line), m_pos(pos), m_skip(skip)
        {
        }

        virtual void Deliver () 
        {
            m_doc.PutError(m_str, m_line, m_pos, m_skip);
            m_doc.MoveToEndOfOutput();
        }
    };

void DocumentProxy::PutError (const string& str, int line, int pos, bool skip)
{
    send(PutErrorNote(m_doc, str, line, pos, skip));
}

///////////////////////////////////////////////////////////////////////////////
    struct PutMessageNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        const string& m_str; int m_line; int m_pos;

        PutMessageNote (CPLSWorksheetDoc& doc, const string& str, int line, int pos)
            : m_doc(doc), 
            m_str(str), m_line(line), m_pos(pos)
        {
        }

        virtual void Deliver () 
        {
            m_doc.PutMessage(m_str, m_line, m_pos);
            m_doc.MoveToEndOfOutput();
        }
    };

void DocumentProxy::PutMessage (const string& str, int line, int pos)
{
    send(PutMessageNote(m_doc, str, line, pos));
}

///////////////////////////////////////////////////////////////////////////////
    struct PutDbmsMessageNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        const string& m_str; int m_line; int m_pos;

        PutDbmsMessageNote (CPLSWorksheetDoc& doc, const string& str, int line, int pos)
            : m_doc(doc), 
            m_str(str), m_line(line), m_pos(pos)
        {
        }

        virtual void Deliver () 
        {
            m_doc.PutDbmsMessage(m_str, m_line, m_pos);
            m_doc.MoveToEndOfOutput();
        }
    };

void DocumentProxy::PutDbmsMessage (const string& str, int line, int pos)
{
    send(PutDbmsMessageNote(m_doc, str, line, pos));
}

///////////////////////////////////////////////////////////////////////////////
    struct AddStatementToHistoryNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        time_t m_startTime; const std::string& m_duration; const string& m_connectDesc; const string& m_sqlSttm;

        AddStatementToHistoryNote (CPLSWorksheetDoc& doc, time_t startTime, const std::string& duration, const string& connectDesc, const string& sqlSttm)
            : m_doc(doc), 
            m_startTime(startTime), m_duration(duration), m_connectDesc(connectDesc), m_sqlSttm(sqlSttm)
        {
        }

        virtual void Deliver () 
        {
            m_doc.AddStatementToHistory(m_startTime, m_duration, m_connectDesc, m_sqlSttm);
        }
    };

void DocumentProxy::AddStatementToHistory (time_t startTime, const std::string& duration, const string& connectDesc, const string& sqlSttm)
{
    send(AddStatementToHistoryNote(m_doc, startTime, duration, connectDesc, sqlSttm));
}

///////////////////////////////////////////////////////////////////////////////
    struct RefreshBindsNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        vector<string> m_data;

        RefreshBindsNote (CPLSWorksheetDoc& doc, const vector<string>& data)
            : m_doc(doc), m_data(data) {}

        virtual void Deliver () 
        {
            m_doc.RefreshBindView(m_data);
        }
    };

void DocumentProxy::RefreshBindView (const vector<string>& data)
{
	send(RefreshBindsNote(m_doc, data));
}

///////////////////////////////////////////////////////////////////////////////
    struct LoadStatisticsNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        vector<int> m_data;

        LoadStatisticsNote (CPLSWorksheetDoc& doc, const vector<int>& data)
            : m_doc(doc), m_data(data) {}

        virtual void Deliver () 
        {
            m_doc.LoadStatistics(m_data);
        }
    };

void DocumentProxy::LoadStatistics (const vector<int>& data)
{
	send(LoadStatisticsNote(m_doc, data));
}

///////////////////////////////////////////////////////////////////////////////
    struct AdjustErrorPosToLastLineNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int& m_line; int& m_col;

        AdjustErrorPosToLastLineNote (CPLSWorksheetDoc& doc, int& line, int& col)
            : m_doc(doc),
            m_line(line), m_col(col)
        {
        }

        virtual void Deliver () 
        {
            m_doc.AdjustErrorPosToLastLine(m_line, m_col);
        }
    };

void DocumentProxy::AdjustErrorPosToLastLine (int& line, int& col) 
{
    send(AdjustErrorPosToLastLineNote(m_doc, line, col));
}

///////////////////////////////////////////////////////////////////////////////
    struct OnSqlConnectNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;

        OnSqlConnectNote (CPLSWorksheetDoc& doc)
            : m_doc(doc)
        {
        }

        virtual void Deliver () 
        {
            theApp.OnSqlConnect();
        }
    };

void DocumentProxy::OnSqlConnect ()
{
    send(OnSqlConnectNote(m_doc));
}

///////////////////////////////////////////////////////////////////////////////
    struct OnSqlDisconnectNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;

        OnSqlDisconnectNote (CPLSWorksheetDoc& doc)
            : m_doc(doc)
        {
        }

        virtual void Deliver () 
        {
            theApp.OnSqlDisconnect();
        }
    };

void DocumentProxy::OnSqlDisconnect ()
{
    send(OnSqlDisconnectNote(m_doc));
}

///////////////////////////////////////////////////////////////////////////////
    struct GetDisplayConnectionStringNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        string m_connectionString;

        GetDisplayConnectionStringNote (CPLSWorksheetDoc& doc)
            : m_doc(doc)
        {
        }

        virtual void Deliver () 
        {
            m_connectionString = theApp.GetDisplayConnectionString();
        }
    };

//TODO#2: probably re-implement
string DocumentProxy::GetDisplayConnectionString ()
{
    GetDisplayConnectionStringNote note(m_doc);
    send(note);
    return note.m_connectionString;
}


///////////////////////////////////////////////////////////////////////////////
    using ErrorLoader::comp_error;

    struct PutErrorsNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        vector<comp_error>& m_errors;

        PutErrorsNote (CPLSWorksheetDoc& doc, vector<comp_error>& errors)
            : m_doc(doc),
            m_errors(errors)
        {
        }

        virtual void Deliver () 
        {
            m_doc.SetFreshErrors();
            vector<comp_error>::const_iterator it = m_errors.begin();
            for (; it != m_errors.end(); ++it)
                m_doc.PutError(it->text.c_str(), it->line, it->position, it->skip);
        }
    };

void DocumentProxy::PutErrors (vector<comp_error>& errors)
{
    send(PutErrorsNote(m_doc, errors));
}

///////////////////////////////////////////////////////////////////////////////
    struct GetLineCountNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int  m_lines;

        GetLineCountNote (CPLSWorksheetDoc& doc)
            : m_doc(doc), 
            m_lines(0)
        {
        }

        virtual void Deliver () 
        {
            m_lines = m_doc.GetEditorView()->GetLineCount();
        }
    };

int DocumentProxy::GetLineCount ()
{
    GetLineCountNote note(m_doc);
    send(note);
    return note.m_lines;
}

///////////////////////////////////////////////////////////////////////////////
    struct GetLineNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int  m_line;
        string m_buffer;
        string m_error;

        GetLineNote (CPLSWorksheetDoc& doc, int line)
            : m_doc(doc), 
            m_line(line)
        {
        }

        virtual void Deliver () 
        {
            try
            {
                const char* ptr;
                int len;
                m_doc.GetEditorView()->GetLine(m_line, ptr, len);
                m_buffer.assign(ptr, len);
            }
            catch (std::exception& ex)
            {
                m_error = ex.what();
            }
        }
    };

void DocumentProxy::GetLine (int line, string& buffer)
{
    GetLineNote note(m_doc, line);
    send(note);
    
    if (!note.m_error.empty())
        throw std::exception(("Re-thrown from DocumentProxy::GetLine\n\n" + note.m_error).c_str());

    buffer = note.m_buffer;
}
    
///////////////////////////////////////////////////////////////////////////////

    struct ActivateRunningNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;

        ActivateRunningNote (CPLSWorksheetDoc& doc)
            : m_doc(doc) {}

        virtual void Deliver () 
        {
            switch_to_running(m_doc);
        }
    };

    struct SetTaskbarProgressStateNote : public ThreadCommunication::Note
    {
        CSQLToolsApp::TaskbarProgressState m_state;

        SetTaskbarProgressStateNote (CSQLToolsApp::TaskbarProgressState state)
            : m_state(state) {}

        virtual void Deliver () { theApp.SetTaskbarProgressState(m_state); }
    };

    struct ShowLastErrorNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;

        ShowLastErrorNote (CPLSWorksheetDoc& doc)
            : m_doc(doc) {}

        virtual void Deliver () { m_doc.ShowLastError(); }
    };

bool DocumentProxy::AskIfUserWantToStopOnError ()
{
    send(SetTaskbarProgressStateNote(CSQLToolsApp::ERROR_STATE));
    send(ActivateRunningNote(m_doc));
    send(ShowLastErrorNote(m_doc));

    bool retVal = (AfxMessageBox("Error happened while running the script."
        "\n\nDo you want to stop execution?", MB_YESNO|MB_ICONSTOP) == IDYES);

    send(SetTaskbarProgressStateNote(CSQLToolsApp::WORKING_STATE));

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
bool DocumentProxy::InputValue (const string& prompt, string& value)
{
    bool retVal = false;

    send(SetTaskbarProgressStateNote(CSQLToolsApp::WAITING_STATE));
    send(ActivateRunningNote(m_doc));

    Common::CInputDlg dlg(NULL);
    dlg.m_prompt = prompt;
    if (dlg.DoModal() == IDOK)
    {
        value = dlg.m_value;
        retVal = true;
    }

    send(SetTaskbarProgressStateNote(CSQLToolsApp::WORKING_STATE));

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
    struct DoSqlExplainPlanNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        const string m_sql;

        DoSqlExplainPlanNote (CPLSWorksheetDoc& doc, const string& sql)
            : m_doc(doc), m_sql(sql)
        {
        }

        virtual void Deliver () 
        {
            m_doc.DoSqlExplainPlan(m_sql);
        }
    };

void DocumentProxy::DoSqlExplainPlan (const string& sql)
{
    send(DoSqlExplainPlanNote(m_doc, sql));
}

///////////////////////////////////////////////////////////////////////////////
    struct LockNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        bool m_lock;

        LockNote (CPLSWorksheetDoc& doc, bool lock)
            : m_doc(doc), m_lock(lock)
        {
        }

        virtual void Deliver () 
        {
            m_doc.Lock(m_lock);
        }
    };

void DocumentProxy::Lock (bool lock)
{
    try {
        send(LockNote(m_doc, lock));
    }                                                               
    catch (document_destroyed&) { // ignore if doc is alredy closed
    }
}

///////////////////////////////////////////////////////////////////////////////
    struct SetExecutionStartedNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        clock64_t m_at;
        int m_line;

        SetExecutionStartedNote (CPLSWorksheetDoc& doc, clock64_t at, int line)
            : m_doc(doc), m_at(at), m_line(line)
        {
        }

        virtual void Deliver () 
        {
            m_doc.SetExecutionStarted(m_at, m_line);
        }
    };

void DocumentProxy::SetExecutionStarted (clock64_t at, int line)
{
    try {
        send(SetExecutionStartedNote(m_doc, at, line));
    }                                                               
    catch (document_destroyed&) { // ignore if doc is alredy closed
    }
}

///////////////////////////////////////////////////////////////////////////////
    struct ScrollToNote : public ThreadCommunication::Note
    {
        CPLSWorksheetDoc& m_doc;
        int m_line;

        ScrollToNote (CPLSWorksheetDoc& doc, int line)
            : m_doc(doc), m_line(line)
        {
        }

        virtual void Deliver () 
        {
            if (GetSQLToolsSettings().GetBufferedExecutionOutput())
                m_doc.GetEditorView()->DelayedScrollTo(m_line);
            else
                m_doc.GetEditorView()->ScrollTo(m_line);
        }
    };

void DocumentProxy::ScrollTo (int line)
{
    send(ScrollToNote(m_doc, line));
}

///////////////////////////////////////////////////////////////////////////////
    struct CheckUserCancelNote : public ThreadCommunication::Note
    {
        arg::counted_ptr<OCI8::BreakHandler> m_breakHandler;

        CheckUserCancelNote () {}

        virtual void Deliver () 
        {
            m_breakHandler = theApp.GetBreakHandler();
        }
    };

void DocumentProxy::CheckUserCancel ()
{
    CheckUserCancelNote note;
    send(note);

    if (note.m_breakHandler.get())
    {
        CSingleLock lk(&note.m_breakHandler->GetCS(), TRUE);

        if (note.m_breakHandler->IsCalledIn(500))
        {
            ASSERT_EXCEPTION_FRAME;
            throw OCI8::UserCancel(0, "User requested cancel of current operation!");
        }
    }
}
