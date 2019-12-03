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

// 30.09.2002 bug fix, wrong error positioning on EXEC (raised in pl/sql procedure)
// 01.10.2002 bug fix, wrong error positioning on execution of selected text
//           if selection does not starts at start of line
// 06.10.2002 changes, OnOutputOpen has been moved to GridView
// 09.03.2003 bug fix, error position fails sometimes
// 10.03.2003 bug fix, find object fails on object with schema
// 25.03.2003 bug refix, find object fails on object with schema
// 03.06.2003 bug fix, "Open In Editor" does not set document type
// 05.06.2003 improvement, output/error postions is based on internal line IDs and more stable on editing text above
// 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db
//            if "save files when switching takss" is activated.
// 29.06.2003 bug fix, "Open In Editor" fails on comma/tab delimited formats
// 07.12.2003 bug fix, CreareAs property is ignored for ddl scripts (always in Unix format)
// 27.06.2004 bug fix, Autocommit does not work
// 22.11.2004 bug fix, all compilation errors are ignored after "alter session set current schema"
// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 09.01.2005 (ak) R1092514: SQL*Plus CONNECT/DISCONNECT commands - small improvements
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 07.02.2005 (ak) R1105003: Bind variables
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 06.05.2009 bug fix - dbms_output line limit increased from 255 to 4000 chars
// 17.10.2011 bug fix, Find Object F12 does not recognize an object name if it is sql*plus keyword
// 17.10.2011 bug fix, Load DDL Script Ctrl+F12 throws the exception "Invalid object type"
//                     on a synonym that is not longer valid
// 01.10.2013 bug fix, "invalid QuickArray<T> subscript" exception on "Execute Current" F10 
//                     if the cursor is beyond of the End Of single-line File 
// 2014-05-11 bug fix, commit was not disabled in read-only mode
// 2015-07-26 bug fix, QuickQuery (F11) ignores "NULL representation" setting in Data Grid 2
// 2015-09-27 bug fix, QuickQuery (F11) / QuickQCount (Ctrl+F11) do not work with multiline selection

#include "stdafx.h"
#include <sstream>
#include <iomanip>
#include "COMMON/ExceptionHelper.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON\StrHelpers.h"
#include "SQLWorksheetDoc.h"
#include "SQLToolsSettings.h"
#include "OutputView.h"
#include "StatView.h"
#include "HistoryView.h"
#include "2PaneSplitter.h"
#include "ExplainPlanView2.h"
#include "ExplainPlanTextView.h"
#include "BindGridView.h"

#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include <SQLWorksheet/CommandParser.h>
#include "MainFrm.h"
#include "Booklet.h"
#include "DbBrowser/DbSourceWnd.h"
#include <OpenGrid/OCIGridView.h>
#include <OpenGrid/OCISource.h>
#include <OCI8/BCursor.h>
#include <OCI8/ACursor.h>
#include <OCI8/Statement.h>
#include "OCI8/SQLFunctionCodes.h"
#include "CommandPerformerImpl.h"
#include "SqlDocCreater.h"
#include "SessionCache.h"
//#include "AutocompleteTemplate.h"

#include "MetaDict\Loader.h"
#include "DbBrowser\ObjectTree_Builder.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "DbBrowser\FindObjectsTask.h"

#include "ErrorLoader.h"


//using namespace ObjectTree;
using namespace ServerBackgroundThread;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using namespace OraMetaDict;

	typedef CPLSWorksheetDoc::ExecutionStyle ExecutionStyle;
    static int backward_scan_for_delimiter_line (COEditorView* pEditor, ExecutionStyle style, int line);
    static int forward_scan_for_delimiter_line (COEditorView* pEditor, ExecutionStyle style, int line);

/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc

IMPLEMENT_DYNCREATE(CPLSWorksheetDoc, COEDocument)

CPLSWorksheetDoc::CPLSWorksheetDoc()
: m_bindVarHolder(new BindVarHolder)
{
    m_LoadedFromDB = false;
    m_freshErrors = true;
    m_pEditor   = 0;
    m_pBooklet  = 0;
    m_executionStartedAt = 0;

    for (int i = 0; i < sizeof(m_BookletFamily) / sizeof(m_BookletFamily[0]); i++)
        m_BookletFamily[i] = 0;
}

void CPLSWorksheetDoc::Serialize (CArchive& ar)
{
    if (ar.IsStoring())
    {
        _CHECK_AND_THROW_(0, "CPLSWorksheetDoc::Serialize: method does not support writing into CArchive");
    }
    else
    {
        CFile* pFile = ar.GetFile();
        ASSERT(pFile->GetPosition() == 0);
        DWORD dwFileSize = (DWORD)pFile->GetLength();

        LPVOID vmdata = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT, PAGE_READWRITE);
        try
        {
            _CHECK_AND_THROW_(ar.Read((char*)vmdata, dwFileSize) == dwFileSize, "CPLSWorksheetDoc::Serialize: reading error from CArchive");
            SetText(vmdata, dwFileSize);
        }
        catch (...)
        {
            if (vmdata != GetVMData())
                VirtualFree(vmdata, 0, MEM_RELEASE);
            throw;
        }
    }
}

void CPLSWorksheetDoc::Init ()
{
    m_pEditor   = (COEditorView*)m_viewList.GetHead();
    m_pBooklet  = (CBooklet*)m_viewList.GetTail();
    m_pOutput   = new COutputView;
    m_pDbGrid   = new OciGridView;
    m_pStatGrid = new CStatView;
    m_pExplainPlan = new ExplainPlanView2;
    m_pXPlan    = new CExplainPlanTextView;
	m_pBindGrid = new BindGridView;

    if (!m_pHistory)
        m_pHistory  = new CHistoryView;
}

    struct BackgroundTask_BindVarHolder : Task
    {
        arg::counted_ptr<BindVarHolder> m_bindVarHolder;

        BackgroundTask_BindVarHolder ()
            : Task("BindVarHolder", 0)
        {
            SetSilent(true);
        }

        void DoInBackground (OciConnect&)
        {
            m_bindVarHolder.reset(0);
        }
    };

CPLSWorksheetDoc::~CPLSWorksheetDoc()
{
    // the document owns the bid variable holder but cannot use in the foreground thread
    // it has to send to the thread that owns the connection to destroy it
    TaskPtr task(new BackgroundTask_BindVarHolder);
    static_cast<BackgroundTask_BindVarHolder*>(task.get())->m_bindVarHolder.swap(m_bindVarHolder);
    FrgdRequestQueue::Get().Push(task);
}

// 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db
//            if "save files when switching takss" is activated.
void CPLSWorksheetDoc::GetNewPathName (CString& newName) const
{
    COEDocument::GetNewPathName(newName);

    if (newName.IsEmpty())
        newName = m_newPathName;
}

void CPLSWorksheetDoc::ActivateEditor ()
{
    m_pEditor->GetParentFrame()->SetActiveView(m_pEditor);
    m_pEditor->SetFocus();
}

void CPLSWorksheetDoc::ActivateTab (CView* view)
{
    if (!m_pBooklet->IsTabPinned())
        m_pBooklet->ActivateTab(view);
}

void CPLSWorksheetDoc::ActivatePlanTab ()
{
    if (!m_pBooklet->IsTabPinned())
		m_pBooklet->ActivatePlanTab();
}


BEGIN_MESSAGE_MAP(CPLSWorksheetDoc, COEDocument)
    //{{AFX_MSG_MAP(CPLSWorksheetDoc)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_SQL_EXECUTE, OnSqlExecute)
    ON_COMMAND(ID_SQL_EXECUTE_FROM_CURSOR, OnSqlExecuteFromCursor)
    ON_COMMAND(ID_SQL_EXECUTE_CURRENT, OnSqlExecuteCurrent)
    ON_COMMAND(ID_SQL_EXECUTE_CURRENT_ALT, OnSqlExecuteCurrentAlt)
    ON_COMMAND(ID_SQL_EXECUTE_CURRENT_AND_STEP, OnSqlExecuteCurrentAndStep)

    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE,                    OnUpdate_SqlExecuteGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_FROM_CURSOR,        OnUpdate_SqlExecuteGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_CURRENT,            OnUpdate_SqlExecuteGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_CURRENT_ALT,        OnUpdate_SqlExecuteGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_CURRENT_AND_STEP,   OnUpdate_SqlExecuteGroup)

    ON_COMMAND(ID_SQL_EXECUTE_IN_SQLPLUS, OnSqlExecuteInSQLPlus)
	ON_COMMAND(ID_SQL_LOAD, OnSqlLoad)
	ON_COMMAND(ID_SQL_LOAD_WITH_DBMS_METADATA, OnSqlLoadWithDbmsMetadata)
    ON_COMMAND(ID_SQL_DESCRIBE, OnSqlDescribe)
    ON_COMMAND(ID_SQL_EXPLAIN_PLAN, OnSqlExplainPlan)

    ON_UPDATE_COMMAND_UI(ID_SQL_LOAD, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_LOAD_WITH_DBMS_METADATA, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_DESCRIBE, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXPLAIN_PLAN, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_IN_SQLPLUS, OnUpdate_SqlGroup)

    ON_COMMAND(ID_SQL_CURR_ERROR, OnSqlCurrError)
    ON_COMMAND(ID_SQL_NEXT_ERROR, OnSqlNextError)
    ON_COMMAND(ID_SQL_PREV_ERROR, OnSqlPrevError)
    ON_UPDATE_COMMAND_UI(ID_SQL_CURR_ERROR, OnUpdate_ErrorGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_NEXT_ERROR, OnUpdate_ErrorGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_PREV_ERROR, OnUpdate_ErrorGroup)

    ON_COMMAND(ID_SQL_HISTORY_GET, OnSqlHistoryGet)
    ON_COMMAND(ID_SQL_HISTORY_STEPFORWARD_AND_GET, OnSqlHistoryStepforwardAndGet)
    ON_COMMAND(ID_SQL_HISTORY_GET_AND_STEPBACK, OnSqlHistoryGetAndStepback)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_GET, OnUpdate_SqlHistory)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_STEPFORWARD_AND_GET, OnUpdate_SqlHistory)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_GET_AND_STEPBACK, OnUpdate_SqlHistory)

    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_OciGridIndicator)

    // Standard Mail Support
    ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
    ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)
    ON_COMMAND(ID_SQL_HELP, OnSqlHelp)

    ON_COMMAND(ID_WINDOW_NEW, OnWindowNew)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_NEW, OnUpdate_WindowNew)

    ON_COMMAND(ID_SQL_COMMIT, OnSqlCommit)
    ON_COMMAND(ID_SQL_ROLLBACK, OnSqlRollback)

    ON_UPDATE_COMMAND_UI(ID_SQL_COMMIT, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_ROLLBACK, OnUpdate_SqlGroup)

    ON_COMMAND(ID_EDIT_AUTOCOMPLETE, OnEditAutocomplete)

    ON_COMMAND(ID_SQL_QUICK_QUERY, OnSqlQuickQuery)
    ON_COMMAND(ID_SQL_QUICK_COUNT, OnSqlQuickCount)
    ON_UPDATE_COMMAND_UI(ID_SQL_QUICK_QUERY, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_QUICK_COUNT, OnUpdate_SqlGroup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc commands

void CPLSWorksheetDoc::OnUpdate_SqlGroup (CCmdUI* pCmdUI)
{
    //ASSERT(ID_SQL_EXECUTE_CURRENT_AND_STEP - ID_SQL_EXPLAIN_PLAN == 6);

    if (theApp.GetConnectOpen() && theApp.GetDatabaseOpen() && !theApp.GetActivePrimeExecution())
    {
        if (theApp.GetConnectReadOnly() 
        && (pCmdUI->m_nID == ID_SQL_COMMIT || pCmdUI->m_nID == ID_SQL_ROLLBACK || pCmdUI->m_nID == ID_SQL_EXECUTE_IN_SQLPLUS)
        )
            pCmdUI->Enable(FALSE);
        else
            pCmdUI->Enable(TRUE);
    }
    else
        pCmdUI->Enable(FALSE);
}

void CPLSWorksheetDoc::OnUpdate_SqlExecuteGroup (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!theApp.GetActivePrimeExecution() ? TRUE : FALSE);
}

void CPLSWorksheetDoc::OnSqlExecute()
{
    DoSqlExecute(ExecutionModeALL);
}

void CPLSWorksheetDoc::OnSqlExecuteFromCursor()
{
    DoSqlExecute(ExecutionModeFROM_CURSOR);
}

void CPLSWorksheetDoc::OnSqlExecuteCurrent()
{
    DoSqlExecute(ExecutionModeCURRENT);
}

void CPLSWorksheetDoc::OnSqlExecuteCurrentAlt()
{
    DoSqlExecute(ExecutionModeCURRENT_ALT);
}

void CPLSWorksheetDoc::OnSqlExecuteCurrentAndStep()
{
    DoSqlExecute(ExecutionModeCURRENT, true);
}


void CPLSWorksheetDoc::AdjustCurrentStatementPos (ExecutionMode mode, ExecutionStyle style, OpenEditor::Square& sel, bool& emptySel)
{
    int currentLine = m_pEditor->GetPosition().line;

    //OpenEditor::Square sel;
    m_pEditor->GetSelection(sel);
    sel.normalize();

    emptySel = sel.is_empty();
    if (emptySel) // make default selection
    {
        sel.start.column = 0;

        if (mode == CPLSWorksheetDoc::ExecutionModeALL) // from the top
            sel.start.line = 0;
        else 
            sel.start.line = backward_scan_for_delimiter_line(m_pEditor, style, currentLine);

        // to the bottom
        sel.end.column = INT_MAX;
        sel.end.line = forward_scan_for_delimiter_line(m_pEditor, style, currentLine);
    }
    // convert positions to indexes (because of tabs)
    sel.start.column = m_pEditor->PosToInx(sel.start.line, sel.start.column);
    sel.end.column   = m_pEditor->PosToInx(sel.end.line, sel.end.column);

    // searching for the beginning of the current statement
    if (emptySel && style == ExecutionStyleNEW)
    {
        int first, last;
        DummyPerformerImpl performer(DocumentProxy(*this));
        CommandParser commandParser(performer, false, GetSQLToolsSettings().GetIgnoreCommmentsAfterSemicolon());
        commandParser.SetSkipDefineUndefine(); // don't want to change substitution variables during pre-scan
        commandParser.Start(sel.start.line, sel.start.column);

        int line = sel.start.line;
        int offset = sel.start.column;
        int nlines = m_pEditor->GetLineCount();
        for (; line < nlines && line <= sel.end.line; line++)
        {
            int len;
            const char* str;
            m_pEditor->GetLine(line, str, len);

            if (line == sel.end.line) len = min(len, sel.end.column);

            commandParser.PutLine(str + offset, len - offset);
            offset = 0;

            if (commandParser.GetLastExecutedLines(first, last)
            && currentLine >= first && currentLine <= last)
                break;
        }
		commandParser.PutEOF();
        if (commandParser.GetLastExecutedLines(first, last))
		if (currentLine >= first && currentLine <= last)
            sel.start.line = first;
		else
			sel.start.line = currentLine;
    }
}

void CPLSWorksheetDoc::OnSqlExplainPlan()
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        ExecutionStyle style = (ExecutionStyle)GetSQLToolsSettings().GetExecutionStyle();

        switch (style)
        {
        case ExecutionStyleNEW:
        case ExecutionStyleOLD:
        case ExecutionStyleTOAD:
            break;
        default:
            ASSERT(0);
            style = ExecutionStyleOLD;
        }

        try
        {
            bool emptySel;
            OpenEditor::Square sel;
            AdjustCurrentStatementPos(ExecutionModeCURRENT, style, sel, emptySel);

            SqlPlanPerformerImpl performer(DocumentProxy(*this), true);
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution(), GetSQLToolsSettings().GetIgnoreCommmentsAfterSemicolon());
            commandParser.SetSqlOnly(true);
            commandParser.Start(sel.start.line, sel.start.column, true/*inPlSqlMode*/);

            int line = sel.start.line;
            int offset = sel.start.column;
            int nlines = m_pEditor->GetLineCount();
            for (; line < nlines && line <= sel.end.line; line++)
            {
                int len;
                const char* str;
                m_pEditor->GetLine(line, str, len);

                if (line == sel.end.line) len = min(len, sel.end.column);

                commandParser.PutLine(str + offset, len - offset);
                offset = 0;

                if (performer.IsCompleted())
                    break;
            }
            if (!performer.IsCompleted())
                commandParser.PutEOF();

            if (emptySel)
            {
                int first, last;
                if (commandParser.GetLastExecutedLines(first, last))
                    m_pEditor->SetHighlighting(first, last);
            }
        }
        // the most of oci exeptions well be proceesed in ExecuteSql method,
        // but if it's a fatal problem (like connection lost), we receive it here
        catch (const OciException& x)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x, MB_OK|MB_ICONHAND);
        }
        // currently we break script execution on substitution exception,
        // but probably we should ask a user
        catch (const CommandParserException& x)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x.what(), MB_OK|MB_ICONHAND);
        }
    }
    // any other exceptions are fatal, a bug report is calling here
    _DEFAULT_HANDLER_
}

    static OpenEditor::DelimitersMap s_delim(" \t\r\n");

    static
    bool is_blank_line (const char *str, const int len)
    {
        if (!str) return true;

	    for (int i = 0; i < len; i++)
		    if (!s_delim[*(str + i)])
			    return false;

	    return true;
    }

    static
    bool is_slash_line (const char *str, const int len)
    {
        if (!str) return false;

        static OpenEditor::DelimitersMap delim(" \t\r\n");

        int i = 0;
        // skip spaces before
	    for (; i < len; i++)
		    if (!s_delim[*(str + i)])
			    break;

        if (i >= len || str[i++] != '/')
            return false;

        // check the restof the string
	    for (; i < len; i++)
		    if (!s_delim[*(str + i)])
			    return false;

	    return true;
    }

    static
    int backward_scan_for_delimiter_line (COEditorView* pEditor, ExecutionStyle style, int line)
    {
        if (line <= 0) return 0;

        int nlines = pEditor->GetLineCount();
        
        if (line == nlines) // added for TOAD style
            line = max(0, nlines-1);
        
        // 01.10.2013 bug fix, "invalid QuickArray<T> subscript" exception on "Execute Current" F10 
        //                     if the cursor is beyond of the End Of single-line File 
        if (line > nlines || !line) 
            return line;

        switch (style)
        {
        default:
        case CPLSWorksheetDoc::ExecutionStyleOLD:
            return line;
        case CPLSWorksheetDoc::ExecutionStyleNEW:
            {
                while (--line > 0)
                {
                    int len;
                    const char* str;
                    pEditor->GetLine(line, str, len);
                    if (is_slash_line(str, len))
                    {
                        line++;
                        break;
                    }
                }
                return line;
            }
        case CPLSWorksheetDoc::ExecutionStyleTOAD:
            {   
                while (--line > 0)
                {
                    int len;
                    const char* str;
                    pEditor->GetLine(line, str, len);
                    if (is_blank_line(str, len))
                    {
                        line++;
                        break;
                    }
                }
                return line;
            }
        }
    }

    static
    int forward_scan_for_delimiter_line (COEditorView* pEditor, ExecutionStyle style, int line)
    {
        int nlines = pEditor->GetLineCount();
        if (line >= nlines) 
            return max(0, nlines-1);

        switch (style)
        {
        default:
            return INT_MAX;
        case CPLSWorksheetDoc::ExecutionStyleTOAD:
            {   
                do 
                {
                    int len;
                    const char* str;
                    pEditor->GetLine(line, str, len);
                    if (is_blank_line(str, len))
                        break;
                }
                while (++line < nlines);

                return line ? --line : 0;
            }
        }
    }

#include "ExecuteTask.h"

void CPLSWorksheetDoc::DoSqlExecute (ExecutionMode mode, bool stepToNext /*= false*/)
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        if (theApp.GetActivePrimeExecution())
        {
            AfxMessageBox("The connection is currently busy. Please try again later.", MB_OK|MB_ICONSTOP);
            AfxThrowUserException();
        }

        ExecutionStyle style = ExecutionStyleOLD;

        switch (mode)
        {
        case ExecutionModeCURRENT:
            style = (ExecutionStyle)GetSQLToolsSettings().GetExecutionStyle();
            break;
        case ExecutionModeCURRENT_ALT:
            style = (ExecutionStyle)GetSQLToolsSettings().GetExecutionStyleAlt();
            break;
        case ExecutionModeALL:
            style = ExecutionStyleOLD;
            break;
        case ExecutionModeFROM_CURSOR:
            // only ExecutionStyleNEW and ExecutionStyleOLD are supported
            style = (ExecutionStyle)GetSQLToolsSettings().GetExecutionStyle();
            if (style == ExecutionStyleTOAD)
                style = (ExecutionStyle)GetSQLToolsSettings().GetExecutionStyleAlt();
            if (style == ExecutionStyleTOAD)
                style = ExecutionStyleOLD;
        }

        switch (style)
        {
        case ExecutionStyleNEW:
        case ExecutionStyleOLD:
        case ExecutionStyleTOAD:
            break;
        default:
            ASSERT(0);
            style = ExecutionStyleOLD;
        }

        m_pOutput->Clear();

        bool emptySel;
        OpenEditor::Square sel;
        AdjustCurrentStatementPos(mode, style, sel, emptySel);

        TaskPtr task = TaskPtr(new BackgroundTask_Execute(*this));

        {
            BackgroundTask_Execute* exeTask = static_cast<BackgroundTask_Execute*>(task.get());
            exeTask->m_ctx.mode = mode;
            exeTask->m_ctx.stepToNext = stepToNext;
            exeTask->m_ctx.selection = sel;
            exeTask->m_ctx.haltOnError = GetSQLToolsSettings().GetExecutionHaltOnError();
            exeTask->m_ctx.substitution = GetSQLToolsSettings().GetScanForSubstitution();
            exeTask->m_ctx.topmostLine = m_pEditor->GetTopmostLine();
            exeTask->m_ctx.cursorLine = m_pEditor->GetPosition().line;
            exeTask->m_ctx.cursorPos  = m_pEditor->GetPosition().column;
            exeTask->m_ctx.bindVarHolder = m_bindVarHolder;
        }

        Global::SetStatusText(""); // erase old status message
        FrgdRequestQueue::Get().Push(task);
    }
    _DEFAULT_HANDLER_
}

void CPLSWorksheetDoc::AfterSqlExecute (ExecuteCtx& ctx)
{
    if (!ctx.seriousError)
    {
        bool switchToEditor = m_pEditor->GetParentFrame()->GetActiveView() == m_pEditor ? true : false;

        m_pOutput->OnMoveHome(); // move the the top of the output log

        if (!ctx.isLastStatementFailed && ctx.isLastStatementSelect)
		    ShowSelectResult(ctx.cursor, ctx.lastExecTime);

	    if (!ctx.hasErrors && ctx.isLastStatementBind)
            ActivateTab(m_pBindGrid);

        if (ctx.hasErrors || !(ctx.isLastStatementSelect || ctx.isLastStatementBind))
            ShowOutput(ctx.hasErrors);

        if (switchToEditor)
            m_pEditor->GetParentFrame()->SetActiveView(m_pEditor);

        // reset modified flag for script which was made by DDL reverse eng
        // add an option to disable this behavior
        if (ctx.mode == ExecutionModeALL && m_LoadedFromDB) // execute
            SetModifiedFlag(FALSE);

        if (!ctx.hasErrors && ctx.mode != ExecutionModeALL && ctx.stepToNext) // step by step
            SkipToTheNextStatement(ctx.lastLine);

        //TODO#TEST: confirnm that is not required
        //if (ctx.highlightExecutedStatement)
        //{
        //    if (ctx.lastSttmlastLine != -1)
        //        m_pEditor->SetHighlighting(ctx.lastSttmFirstLine, ctx.lastSttmlastLine, false);
        //}

        PutMessage("Total execution time " + Common::print_time(ctx.totalExecTime/ CLOCKS_PER_SEC));
        m_pOutput->FlushBuffer();
    }
    else
    {
        PutError(ctx.seriousErrorText, ctx.seriousErrorLine);
        PutMessage("Total execution time " + Common::print_time(ctx.totalExecTime/ CLOCKS_PER_SEC));
        m_pOutput->FlushBuffer();

        ShowOutput(true);
        if (!ctx.userCancel)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(ctx.seriousErrorText.c_str(), MB_OK|MB_ICONHAND);
        }
    }
}

void CPLSWorksheetDoc::ShowLastError ()
{
    m_pOutput->NextError(true);
    ActivateTab(m_pOutput);
    OnSqlCurrError();
}

void CPLSWorksheetDoc::SkipToTheNextStatement (int curLine)
{
    int line = curLine + 1;
    int nlines = m_pEditor->GetLineCount();

    for (; line < nlines; line++)
    {
        int len;
        const char* str;
        m_pEditor->GetLine(line, str, len);

        bool empty = true;

        for (int i = 0; empty && i < len; i++)
            if (!isspace(str[i]))
                empty = false;

        if (!empty) break;
    }

    GoTo(line);
}

void CPLSWorksheetDoc::ShowSelectResult (std::auto_ptr<OciAutoCursor>& cursor, double lastExecTime)
{
    ActivateTab(m_pDbGrid);
    clock64_t startTime = clock64();
    m_pDbGrid->SetCursor(cursor);

    ostringstream out;
	out << "Statement executed in ";
	out << Common::print_time(lastExecTime);
    out << Common::print_time(double(clock64() - startTime)/ CLOCKS_PER_SEC);

    Global::SetStatusText(out.str());
}


void CPLSWorksheetDoc::ShowOutput (bool error)
{
    ActivateTab(m_pOutput);

    if (error)
    {
        MessageBeep(MB_ICONHAND);
        m_pOutput->FirstError(true);
        OnSqlCurrError();
    }
    else
        m_pOutput->FirstError();
}

void CPLSWorksheetDoc::OnSqlCurrError ()
{
    OpenEditor::LineId lid;
    OpenEditor::Position pos;
    if (m_pOutput->GetCurrError(pos.line, pos.column, lid))
    {
        if (lid.Valid()
        && (!(pos.line < m_pEditor->GetLineCount()) || lid != m_pEditor->GetLineId(pos.line)))
        {
            if (!m_pEditor->FindById(lid, pos.line))
            {
                MessageBeep(MB_ICONHAND);
                Global::SetStatusText("Cannot find the error/message line. The original line has been replaced or deleted.", TRUE);
            }
        }

        if (pos.line < m_pEditor->GetLineCount()) // 09.03.2003 bug fix, error position fails sometimes
            pos.column = m_pEditor->InxToPos(pos.line, pos.column);

        m_pEditor->MoveToAndCenter(pos);

        ActivateEditor();
    }
    m_freshErrors = false;
}

void CPLSWorksheetDoc::OnSqlNextError ()
{
    if (m_freshErrors || m_pOutput->NextError(true))
        OnSqlCurrError();
}

void CPLSWorksheetDoc::OnSqlPrevError ()
{
    if (m_freshErrors || m_pOutput->PrevError(true))
        OnSqlCurrError();
}

void CPLSWorksheetDoc::AdjustErrorPosToLastLine (int& line, int& col) const
{
    if (col == 0 && line == m_pEditor->GetLineCount())
        col = m_pEditor->GetLineLength(--line);
}

void CPLSWorksheetDoc::OnUpdate_ErrorGroup (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_pOutput->IsEmpty());
}

/*! \fn void CPLSWorksheetDoc::OnSqlDescribe()
 *
 * \brief Find text under cursor, set it in the 'Object Viewer' and get description of it.
 */
void CPLSWorksheetDoc::OnSqlDescribe()
{
    try { EXCEPTION_FRAME;

		std::string buff;
		OpenEditor::Square sel;

		// 25.03.2003 bug refix, find object fails on object with schema
		string delims = GetSettings().GetDelimiters();
		string::size_type pos = delims.find('.');
		if (pos != string::npos) delims.erase(pos, 1);

        pos = delims.find('&');
		if (pos != string::npos) delims.erase(pos, 1);

		CObjectViewerWnd& viewer = GetMainFrame()->ShowTreeViewer();

		if (m_pEditor->GetBlockOrWordUnderCursor(buff,  sel, true/*only one line*/, delims.c_str()))
        {
            // added performer/parser to expand substitution
            DummyPerformerImpl performer(DocumentProxy(*this));
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution(), GetSQLToolsSettings().GetIgnoreCommmentsAfterSemicolon());
            commandParser.Start(0, 0, true/*inPlSqlMode*/);
            commandParser.PutLine(buff.c_str(), buff.length());
            commandParser.PutEOF();

            viewer.ShowObject(performer.GetSqlText());
        }

        if (!GetSQLToolsSettings().GetObjectViewerRetainFocusInEditor())
		    viewer.SetFocus();
	}
	_DEFAULT_HANDLER_
}


	struct BackgroundTask_LoadDDL : Task
    {
        SQLToolsSettings m_settings;
        string m_owner, m_name, m_type, m_ddl;
        Dictionary m_dictionary;
        const DbObject* m_objects[2];

        BackgroundTask_LoadDDL (SQLToolsSettings& settings, const string& owner, const string& name, const string& type)
            : Task("Load object", 0),
            m_settings(settings),
            m_owner(owner), 
            m_name(name), 
            m_type(type)
        {
            m_objects[0] = m_objects[1] = 0;
        }

        void DoInBackground (OciConnect& connect)
        {
            Loader loader(connect, m_dictionary);
            loader.Init();
            loader.LoadObjects(
                m_owner.c_str(), m_name.c_str(), m_type.c_str(),
                m_settings,
                m_settings.GetSpecWithBody(),
                m_settings.GetBodyWithSpec(),
                false/*bUseLike*/
                );

            m_objects[0] = &m_dictionary.LookupObject(m_owner.c_str(), m_name.c_str(), m_type.c_str());

            if (const Synonym* pSynonym = dynamic_cast<const Synonym*>(m_objects[0]))
            {
			    // there is no support for remote objects
			    if (pSynonym->m_strRefDBLink.empty())
			    {
				    std::string buff = "\"" + pSynonym->m_strRefOwner
					    + "\".\"" + pSynonym->m_strRefName + "\"";

                    // call FindObjects to retrieve object's type
                    std::vector<ObjectTree::ObjectDescriptor> result;
			        ObjectTree::FindObjects(connect, buff.c_str(), result);

                    if (!result.empty())
				    {
					    loader.LoadObjects(
						    result.begin()->owner.c_str(), 
                            result.begin()->name.c_str(), 
                            result.begin()->type.c_str(),
						    m_settings,
						    m_settings.GetSpecWithBody(),
						    m_settings.GetBodyWithSpec(),
						    false/*bUseLike*/
						    );

					    m_objects[1] = &m_dictionary.LookupObject(
                            result.begin()->owner.c_str(), 
                            result.begin()->name.c_str(), 
                            result.begin()->type.c_str()
                            );
                    }
			    }
            }
        }

        void ReturnInForeground ()
        {
            if (m_objects[0])
            {
                SqlDocCreater docCreater(true/*singleDocument*/, false/*useBanner*/, true/*supressGroupByDDL*/, true/*objectNameAsTitle*/);
                for (int i = 0; i < 2; ++i)
                    if (m_objects[i])
                        docCreater.Put(*m_objects[i], m_settings);
            }
        }
    };

void CPLSWorksheetDoc::LoadDDL (const string& owner, const string& name, const string& type)
{
    CWaitCursor wait;
    // make local copy of sqltools settings
    SQLToolsSettings settings = GetSQLToolsSettings();
    if (ShowDDLPreferences(settings, true/*bLocal*/))
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_LoadDDL(settings, owner, name, type)));
}

	struct BackgroundTask_LoadDDLWithDbmsMetadata : Task
    {
        string m_owner, m_name, m_type, m_ddl;

        BackgroundTask_LoadDDLWithDbmsMetadata (const string& owner, const string& name, const string& type)
            : Task("Load object using DBMS_METADATA", 0),
            m_owner(owner), 
            m_name(name), 
            m_type(type)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OCI8::CLobVar ddl(connect, 1024*1024);
                OciStatement cursor(connect);
                cursor.Prepare(
                    "BEGIN "
                    "dbms_metadata.set_transform_param(dbms_metadata.session_transform, 'SQLTERMINATOR', true); "
                    ":ddl := dbms_metadata.get_ddl(object_type => :object_type, name => :name, schema => :schema); " 
                    "END;"
                    );
                OciStringVar type_(m_type);
                OciStringVar name_(m_name);
                OciStringVar owner_(m_owner);
                cursor.Bind(":ddl", ddl);
                cursor.Bind(":object_type", type_);
                cursor.Bind(":name", name_);
                cursor.Bind(":schema", owner_);
                cursor.Execute(1, true/*guaranteedSafe*/);
                ddl.GetString(m_ddl);
            }
            catch (const OciException& x)
            {
                if (x == 31603)
                {
                    MessageBeep(MB_ICONHAND);
                    ostringstream msg;
                    msg << "dbms_metadata.get_ddl failed with the following error:" << endl << endl
                        << x.what()
                        << endl << endl
                        << "You need SELECT_CATALOG_ROLE in the order to get DDL form another schema.";
                    
                    SetError(msg.str());
                }
                else
                    SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CDocTemplate* pDocTemplate = theApp.GetPLSDocTemplate();
            ASSERT(pDocTemplate);

            if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
            {
                ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);
                ASSERT(pDoc->m_pEditor);

                pDoc->DefaultFileFormat();
                pDoc->SetText(m_ddl.c_str(), m_ddl.length());
                pDoc->SetModifiedFlag(FALSE);
                pDoc->m_LoadedFromDB = true;
                pDoc->m_newPathName  = (m_name + ".sql").c_str();

                CString title(m_owner.c_str());
                if (!title.IsEmpty()) title += '.';
                title += pDoc->m_newPathName;
                pDoc->SetTitle(title);

                pDoc->m_pOutput->Clear();

                pDoc->ActivateTab(pDoc->m_pOutput);
                pDoc->ActivateEditor();
            }
        }
    };

void CPLSWorksheetDoc::LoadDDLWithDbmsMetadata (const string& owner, const string& name, const string& type)
{
    BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_LoadDDLWithDbmsMetadata(owner, name, type)));
}


    struct BackgroundTask_FindAndLoadDLL : ObjectTree::FindObjectsTask
    {
        bool m_withDbmsMetadata;

        BackgroundTask_FindAndLoadDLL (const std::string& input, bool withDbmsMetadata)
            : FindObjectsTask(input, 0),
            m_withDbmsMetadata(withDbmsMetadata)
        {
            m_silent = true;
        }

        void ReturnInForeground ()
        {
            if (!m_error.empty())  // too many objects
            {
                ::MessageBeep(MB_ICONSTOP);
                AfxMessageBox(m_error.c_str(), MB_OK|MB_ICONSTOP);
            }
            else
            {
			    if (m_result.size() > 0)
                {
                    if (m_withDbmsMetadata)
                        CPLSWorksheetDoc::LoadDDLWithDbmsMetadata(m_result.begin()->owner, m_result.begin()->name, m_result.begin()->type);
                    else
                        CPLSWorksheetDoc::LoadDDL(m_result.begin()->owner, m_result.begin()->name, m_result.begin()->type);
                }
                else
                {
                    ::MessageBeep(MB_ICONSTOP);
                    Global::SetStatusText('<' + m_input + "> not found!");
                }
            }
        }

    };

void CPLSWorksheetDoc::DoSqlLoad (bool withDbmsMetadata)
{
    try { EXCEPTION_FRAME;

        std::string buff;
        OpenEditor::Square sel;

        // 10.03.2003 bug fix, find object fails on object with schema
        string delims = GetSettings().GetDelimiters();
        string::size_type pos = delims.find('.');
        if (pos != string::npos) delims.erase(pos, 1);

        pos = delims.find('&');
		if (pos != string::npos) delims.erase(pos, 1);

        if (m_pEditor->GetBlockOrWordUnderCursor(buff,  sel, true/*only one line*/, delims.c_str()))
        {
            // added performer/parser to expand substitution
            DummyPerformerImpl performer(DocumentProxy(*this));
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution(), GetSQLToolsSettings().GetIgnoreCommmentsAfterSemicolon());
            commandParser.Start(0, 0, true/*inPlSqlMode*/);
            commandParser.PutLine(buff.c_str(), buff.length());
            commandParser.PutEOF();

            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_FindAndLoadDLL(performer.GetSqlText().c_str(), withDbmsMetadata)));
        }

    }
    _DEFAULT_HANDLER_
}

void CPLSWorksheetDoc::OnSqlLoad ()
{
    DoSqlLoad(false);
}

void CPLSWorksheetDoc::OnSqlLoadWithDbmsMetadata ()
{
    DoSqlLoad(true);
}

C2PaneSplitter* CPLSWorksheetDoc::GetSplitter ()
{
    C2PaneSplitter* splitter = (C2PaneSplitter*)CView::GetParentSplitter(m_pEditor, FALSE);
    ASSERT_KINDOF(C2PaneSplitter, splitter);
    return splitter;
}

void CPLSWorksheetDoc::GoTo (int line)
{
    OpenEditor::Position pos;
    pos.column = 0;
    pos.line = line;
    m_pEditor->MoveToAndCenter(pos);
}


void CPLSWorksheetDoc::OnSqlHelp()
{
    string helpPath;
    Global::GetHelpPath(helpPath);
    helpPath += "\\sqlqkref.chm";

    std::string key;
    OpenEditor::Square sel;
    if (m_pEditor->GetBlockOrWordUnderCursor(key,  sel, true/*only one line*/))
    {
        HH_AKLINK link;
        memset(&link, 0, sizeof link);
        link.cbStruct     = sizeof(HH_AKLINK) ;
        link.pszKeywords  = key.c_str();
        link.fIndexOnFail = TRUE ;

        // 24.05.2002  SQLTools hangs on a context-sensitive help call there is more than one topic, which is found by keyword
        ::HtmlHelp(GetDesktopWindow(), (const char*)helpPath.c_str(), HH_KEYWORD_LOOKUP, (DWORD)&link);
    }
    else
        ::HtmlHelp(*AfxGetMainWnd(), (const char*)helpPath.c_str(), HH_DISPLAY_TOPIC, 0);
}


void CPLSWorksheetDoc::PutError (const string& str, int line, int pos, bool skip)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutError(str.c_str(), line, pos, lineId, skip);
}

void CPLSWorksheetDoc::PutMessage (const string& str, int line, int pos)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutMessage(str.c_str(), line, pos, lineId);
}

void CPLSWorksheetDoc::PutDbmsMessage (const string& str, int line, int pos)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutDbmsMessage(str.c_str(), line, pos, lineId);
}

void CPLSWorksheetDoc::MoveToEndOfOutput ()                                                        
{ 
    m_pOutput->OnMoveEnd(); 
}

void CPLSWorksheetDoc::AddStatementToHistory (time_t startTime, const std::string& duration, const string& connectDesc, const string& sqlSttm)
{
    m_pHistory->AddStatement(startTime, duration, connectDesc, sqlSttm);
}

void CPLSWorksheetDoc::LoadStatistics (const vector<int>& data)
{
    m_pStatGrid->LoadStatistics(data);
}

void CPLSWorksheetDoc::RefreshBindView (const vector<string>& data)
{
	m_pBindGrid->Refresh(data);
}

void CPLSWorksheetDoc::OnSqlHistoryGet()
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if ((settings.GetHistoryEnabled()))
    {
        std::string text;

        if (m_pHistory->GetHistoryEntry(text))
        {
            switch (settings.GetHistoryAction())
            {
            case SQLToolsSettings::Copy:
                Common::AppSetClipboardText(text.c_str(), text.length(), CF_TEXT);
                break;
            case SQLToolsSettings::Paste:
                if (!m_pEditor->IsSelectionEmpty())
                    m_pEditor->DeleteBlock();
                m_pEditor->InsertBlock(text.c_str(), !settings.GetHistoryKeepSelection());
                break;
            case SQLToolsSettings::ReplaceAll:
                m_pEditor->OnCmdMsg(ID_EDIT_SELECT_ALL, 0, 0, 0);
                m_pEditor->OnCmdMsg(ID_EDIT_DELETE, 0, 0, 0);
                m_pEditor->InsertBlock (text.c_str(), !settings.GetHistoryKeepSelection());
                break;
            }
        }

        ActivateEditor();
    }
    else
        MessageBeep((UINT)-1);

}

void CPLSWorksheetDoc::OnSqlHistoryStepforwardAndGet()
{
    if ((GetSQLToolsSettings().GetHistoryEnabled()))
    {
        m_pHistory->StepForward();
        OnSqlHistoryGet();
    }
}

void CPLSWorksheetDoc::OnSqlHistoryGetAndStepback()
{
    if ((GetSQLToolsSettings().GetHistoryEnabled()))
    {
        OnSqlHistoryGet();
        m_pHistory->StepBackward();
    }
}

void CPLSWorksheetDoc::OnUpdate_SqlHistory (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(GetSQLToolsSettings().GetHistoryEnabled() && !m_pHistory->IsEmpty());
}

BOOL CPLSWorksheetDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!COEDocument::OnOpenDocument(lpszPathName))
        return FALSE;

    if (!m_pHistory)
        m_pHistory  = new CHistoryView;

    _ASSERTE(m_pHistory);

    if (m_pHistory)
        try { EXCEPTION_FRAME;

            m_pHistory->Load(lpszPathName);
        }
        _DEFAULT_HANDLER_;

    return TRUE;
}

BOOL CPLSWorksheetDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    _ASSERTE(m_pHistory);

    if (m_pHistory)
        try { EXCEPTION_FRAME;

            m_pHistory->Save(lpszPathName);
        }
        _DEFAULT_HANDLER_;

    return COEDocument::OnSaveDocument(lpszPathName);
}

void CPLSWorksheetDoc::OnCloseDocument()
{
    _ASSERTE(m_pHistory);

    if (m_pHistory && !GetPathName().IsEmpty())
        try { EXCEPTION_FRAME;

            m_pHistory->Save(GetPathName());
        }
        _DEFAULT_HANDLER_;


    return COEDocument::OnCloseDocument();
}

BOOL CPLSWorksheetDoc::CanCloseFrame (CFrameWnd* pFrame)
{
    // TODO#2: replace IsLocked with hasRunningTask
    if (IsLocked()) // = it has a running task!
    {
        if (AfxMessageBox("There is an active query/statement. \n\nDo you really want to ignore that and close the script?", 
            MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) != IDYES
        )
        return FALSE;

        theApp.DoCancel(true/*silent*/);
    }

    return COEDocument::CanCloseFrame(pFrame);
}

void CPLSWorksheetDoc::OnUpdate_OciGridIndicator (CCmdUI* pCmdUI)
{
    if (m_pDbGrid)
        m_pDbGrid->OnUpdate_OciGridIndicator(pCmdUI);
}

void CPLSWorksheetDoc::OnWindowNew ()
{
    // this command is not supported
    // the default implementation crashes the application
}

void CPLSWorksheetDoc::OnUpdate_WindowNew (CCmdUI *pCmdUI)
{
    // see CPLSWorksheetDoc::OnWindowNew
    pCmdUI->Enable(FALSE);
}

    struct CommitTask : ServerBackgroundThread::Task
    {
        CPLSWorksheetDoc& m_doc;
        bool m_done;

        CommitTask (CPLSWorksheetDoc& doc) 
            : ServerBackgroundThread::Task("Commit", 0), 
            m_doc(doc),
            m_done(false)
        {
        }
        virtual void DoInBackground (OciConnect& connect) 
        { 
            if (connect.IsOpen()) {
                connect.Commit(); 
                m_done = true;
            }
        }
        virtual void ReturnInForeground ()              
        {
            if (m_done) {
                m_doc.PutMessage("Committed.");
                m_doc.ShowOutput(false);
            }
        };
    };

void CPLSWorksheetDoc::OnSqlCommit ()
{
    try { EXCEPTION_FRAME;
        ServerBackgroundThread::FrgdRequestQueue::Get()
            .Push(ServerBackgroundThread::TaskPtr(new CommitTask(*this)));
    }
    _DEFAULT_HANDLER_;
}

    struct RollbackTask : ServerBackgroundThread::Task
    {
        CPLSWorksheetDoc& m_doc;
        bool m_done;

        RollbackTask (CPLSWorksheetDoc& doc) 
            : ServerBackgroundThread::Task("Rollback", 0), 
            m_doc(doc),
            m_done(false)
        {
        }
        virtual void DoInBackground (OciConnect& connect) 
        { 
            if (connect.IsOpen()) {
                connect.Rollback(); 
                m_done = true;
            }
        }
        virtual void ReturnInForeground ()              
        {
            if (m_done) {
                m_doc.PutMessage("Rollbacked.");
                m_doc.ShowOutput(false);
            }
        };
    };

void CPLSWorksheetDoc::OnSqlRollback ()
{
    try { EXCEPTION_FRAME;
        ServerBackgroundThread::FrgdRequestQueue::Get()
            .Push(ServerBackgroundThread::TaskPtr(new RollbackTask(*this)));
    }
    _DEFAULT_HANDLER_;
}

void CPLSWorksheetDoc::OnEditAutocomplete ()
{
    try { EXCEPTION_FRAME;

        if (theApp.GetConnectOpen())
        {
    	    string delims = GetSettings().GetDelimiters();
		    string::size_type pos = delims.find('.');
		    if (pos != string::npos) delims.erase(pos, 1);

            string buff;
            OpenEditor::Square sqr;
            if (m_pEditor->GetBlockOrWordUnderCursor(buff, sqr, true, delims.c_str()) 
            && !buff.empty() && *buff.rbegin() == '.')
            {
                OpenEditor::TemplatePtr tmpl = SessionCache::GetAutocompleteSubobjectTemplate(buff);
            
                if (tmpl.get() && tmpl->GetCount() > 0)
                    m_pEditor->DoEditExpandTemplate(tmpl, false/*canModifyTemplate*/);
            }
            else
            {
                OpenEditor::TemplatePtr tmpl = SessionCache::GetAutocompleteTemplate();

                if (tmpl.get() && tmpl->GetCount() > 0)
                    m_pEditor->DoEditExpandTemplate(tmpl, false/*canModifyTemplate*/);
            }
        }
        else
            MessageBeep((UINT)-1);
    }
    _DEFAULT_HANDLER_;
}

struct BackgroundTask_DoSqlQuickQuery : ServerBackgroundThread::Task
{
    CPLSWorksheetDoc& m_doc;
    DocumentProxy m_proxy;
    CommandPerformerSettings m_settings;
    string m_query, m_message;
    std::auto_ptr<OciAutoCursor> m_cursor;

    BackgroundTask_DoSqlQuickQuery (CPLSWorksheetDoc& doc, const string& query, const string& message)
        : ServerBackgroundThread::Task("Quick query", 0),
        m_doc(doc),
        m_proxy(doc),
        m_settings(GetSQLToolsSettings()),
        m_query(query), m_message(message)
    {
        SetSilent(true);
    }
    
    void DoInBackground (OciConnect& connect)
    {
        m_proxy.SetActivePrimeExecution(true);

        try
        {
            m_proxy.SetExecutionStarted(clock64(), -1);

            m_cursor.reset(new OciAutoCursor(connect, 1000));

            m_cursor->Prepare(
                m_query.rbegin() != m_query.rend() && *m_query.rbegin() == ';' 
                ? m_query.substr(0, m_query.size()-1).c_str() : m_query.c_str()
            );

            // 2015-07-29 bug fix, QuickQuery (F11) ignores "NULL representation" setting in Data Grid 2
            m_cursor->SetNumberFormat      (m_settings.gridNlsNumberFormat);
            m_cursor->SetDateFormat        (m_settings.gridNlsDateFormat);
            m_cursor->SetTimeFormat        (m_settings.gridNlsTimeFormat);
            m_cursor->SetTimestampFormat   (m_settings.gridNlsTimestampFormat);
            m_cursor->SetTimestampTzFormat (m_settings.gridNlsTimestampTzFormat);
            m_cursor->SetStringNull        (m_settings.gridNullRepresentation);
            m_cursor->SetSkipLobs          (m_settings.gridSkipLobs);
            m_cursor->SetStringLimit       (m_settings.gridMaxLobLength);
            m_cursor->SetBlobHexRowLength  (m_settings.gridBlobHexRowLength);

            m_cursor->Execute();
        }
        catch (const OciException& x)
        {
            SetError(x.what());
            m_cursor.reset(0);
        }
        catch (...)
        {
            try {
                m_cursor.reset(0);
            }
            catch (...) {}

            try {
                m_proxy.SetActivePrimeExecution(false);
                m_proxy.SetExecutionStarted(0, -1);
            }
            catch (...) {}
        }

        m_proxy.SetActivePrimeExecution(false);
        m_proxy.SetExecutionStarted(0, -1);
    }

    void ReturnInForeground ()
    {
        if (m_proxy.IsOk()) // check if document was not closed
        {
            m_doc.ActivateTab(m_doc.m_pDbGrid);
            m_doc.m_pDbGrid->SetCursor(m_cursor);
            Global::SetStatusText(m_message);
        }
    }
};

void CPLSWorksheetDoc::DoSqlQuickQuery (const char* query, const char* message) // accepts <TABLE> placeholder
{
    CWaitCursor wait;

    if (theApp.GetActivePrimeExecution())
    {
        AfxMessageBox("The connection is currently busy. Please try again later.", MB_OK|MB_ICONSTOP);
        AfxThrowUserException();
    }

    try { EXCEPTION_FRAME;

        ActivePrimeExecutionFrame frame;

		std::string buff;
		OpenEditor::Square sel;

		// 25.03.2003 bug refix, find object fails on object with schema
		string delims = GetSettings().GetDelimiters();
		string::size_type pos = delims.find('.');
		if (pos != string::npos) delims.erase(pos, 1);

        pos = delims.find('&');
		if (pos != string::npos) delims.erase(pos, 1);

		if (m_pEditor->GetBlockOrWordUnderCursor(buff,  sel, false/*only one line*/, delims.c_str()))
        {
            // added performer/parser to expand substitution
            DummyPerformerImpl performer(DocumentProxy(*this));
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution(), GetSQLToolsSettings().GetIgnoreCommmentsAfterSemicolon());
            commandParser.Start(0, 0, true/*inPlSqlMode*/);
            commandParser.PutLine(buff.c_str(), buff.length());
            commandParser.PutEOF();

            Common::Substitutor subst;
            subst.AddPair("<TABLE>", performer.GetSqlText().c_str());
            subst << query;
            string _query = subst.GetResult();

            TaskPtr task = TaskPtr(new BackgroundTask_DoSqlQuickQuery(*this, _query, message));
            FrgdRequestQueue::Get().Push(task);
        }
	}
	_DEFAULT_HANDLER_
}

void CPLSWorksheetDoc::OnSqlQuickQuery ()
{
    DoSqlQuickQuery((theApp.GetServerVersion() > OCI8::esvServer81X) 
        ? "select * from (<TABLE>)" : "select * from <TABLE>", "Quick query from <TABLE>");
}

void CPLSWorksheetDoc::OnSqlQuickCount ()
{
    DoSqlQuickQuery((theApp.GetServerVersion() > OCI8::esvServer81X) 
        ? "select count(*) from (<TABLE>)" : "select count(*) from <TABLE>", "Quick count query from <TABLE>");
}

//static
CPLSWorksheetDoc* CPLSWorksheetDoc::GetActiveDoc ()
{
    if (CWnd* pWnd = AfxGetMainWnd())
    {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)))
        {
            if (CMDIChildWnd * pChild = ((CMDIFrameWnd*)pWnd)->MDIGetActive())
                if (CDocument* pActiveDoc = pChild->GetActiveDocument())
                    if (pActiveDoc->IsKindOf(RUNTIME_CLASS(CPLSWorksheetDoc)))
                        return (CPLSWorksheetDoc*)pActiveDoc;
        }
        else if (pWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        {
            if (CDocument* pActiveDoc = ((CFrameWnd*)pWnd)->GetActiveDocument())
                if (pActiveDoc->IsKindOf(RUNTIME_CLASS(CPLSWorksheetDoc)))
                        return (CPLSWorksheetDoc*)pActiveDoc;
        }        
    }
    return 0;
}

//static
void CPLSWorksheetDoc::DoSqlQueryInNew (const string& query, const string& title)
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
        if (CDocTemplate* pDocTemplate = AfxGetApp()->GetNextDocTemplate(pos))
        {
            if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
            {
                ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);

                CMemFile of(256);
                of.Write(query.c_str(), query.size());
                of.SeekToBegin();
                ((CDocument*)pDoc)->Serialize(CArchive(&of, CArchive::load));
                pDoc->SetTitle(title.c_str());
                pDoc->SetClassSetting("PL/SQL");
                pDoc->DefaultFileFormat();
                
                //pDoc->GetEditorView()->SelectAll();
                //pDoc->GetEditorView()->OnEditNormalizeText();
                //pDoc->GetEditorView()->ClearSelection();

	            pDoc->GetSplitter()->SetDefaultHight(0, 50);
                pDoc->SetModifiedFlag(FALSE);
                pDoc->DoSqlExecute(CPLSWorksheetDoc::ExecutionModeALL);
            }
        }
    }
    catch (CUserException*)
    {
        MessageBeep((UINT)-1);
    }
    _DEFAULT_HANDLER_
}

//static
void CPLSWorksheetDoc::DoSqlQuickQuery (const string& query, const string& message)
{
    CWaitCursor wait;

    if (theApp.GetActivePrimeExecution())
    {
        AfxMessageBox("The connection is currently busy. Please try again later.", MB_OK|MB_ICONSTOP);
        AfxThrowUserException();
    }

    try { EXCEPTION_FRAME;

        if (CPLSWorksheetDoc* doc = CPLSWorksheetDoc::GetActiveDoc())
        {
            TaskPtr task = TaskPtr(new BackgroundTask_DoSqlQuickQuery(*doc, query, message));
            FrgdRequestQueue::Get().Push(task);
        }
	}
	_DEFAULT_HANDLER_
}

void CPLSWorksheetDoc::DoSqlQuery (const string& query, const string& title, const string& message, bool quick)
{
    CPLSWorksheetDoc* doc = CPLSWorksheetDoc::GetActiveDoc();

    if (quick && doc)
        DoSqlQuickQuery(query, message);
    else
        DoSqlQueryInNew(query, title);
}

void CPLSWorksheetDoc::Lock (bool lock)
{
    if (lock != IsLocked())
    {
        COEDocument::Lock(lock);

        if (lock)
        {
            m_pEditor->SetTimer(COEditorView::DOC_TIMER_1, 1000, NULL);
        }
        else
        {
            const CString& title = GetTitle();
            if (title.GetLength() >= 2 && title.GetAt(0) == '!')
                CDocument::SetTitle(title.GetString() + 2);

            m_pEditor->KillTimer(COEditorView::DOC_TIMER_1);
            m_pEditor->SetPaleIfLocked(false);
            m_pDbGrid->SetPaleColors(false);
        }
    }
}

void CPLSWorksheetDoc::SetExecutionStarted (clock64_t at, int line) 
{ 
    m_executionStartedAt = at; 
    m_pEditor->SetTimer(COEditorView::DOC_TIMER_2, 1000, NULL);
    if (line != -1) m_pEditor->SetExecutionLine(line);
} 

void CPLSWorksheetDoc::SetHighlighting (int fistHighlightedLine, int lastHighlightedLine, bool updateWindow)
{
    m_pEditor->SetAutoscrollOnHighlighting(GetSQLToolsSettings().GetAutoscrollExecutedStatement());
    m_pEditor->SetHighlighting(fistHighlightedLine, lastHighlightedLine, updateWindow);
}

void CPLSWorksheetDoc::OnTimer (UINT nIDEvent)
{
    switch (nIDEvent)
    {
    case COEditorView::DOC_TIMER_1: 
        m_pEditor->KillTimer(nIDEvent);
        m_pEditor->SetPaleIfLocked(IsLocked());
        m_pDbGrid->SetPaleColors(IsLocked());
        if (IsLocked())
        {
            const CString& title = GetTitle();
            if (title.GetLength() > 0 && title.GetAt(0) != '!')
                CDocument::SetTitle("! " + title);
        }
        break;
    case COEditorView::DOC_TIMER_2:
        if (m_executionStartedAt == 0)
        {
            m_pEditor->KillTimer(nIDEvent);
        }
        else
        {
            double secs = double(clock64() - m_executionStartedAt)/ CLOCKS_PER_SEC;
            Global::SetStatusText("The current step execution time: " + Common::print_time(secs));
        }
    }
}

void CPLSWorksheetDoc::OnContextMenuInit (CMenu* pMenu)
{
    if (pMenu)
    {
        pMenu->InsertMenu(0, MF_SEPARATOR|MF_BYPOSITION);
        pMenu->InsertMenu(0, MF_STRING|MF_BYPOSITION, ID_SQL_QUICK_COUNT, "&Count query");
        pMenu->InsertMenu(0, MF_STRING|MF_BYPOSITION, ID_SQL_QUICK_QUERY, "&Query table under cursor");                      
        pMenu->InsertMenu(0, MF_STRING|MF_BYPOSITION, ID_SQL_DESCRIBE,    "&Find object under cursor");                      
    }
}

void CPLSWorksheetDoc::SetTitle (LPCTSTR lpszTitle)
{
    __super::SetTitle(lpszTitle);

    if (IsLocked())
    {
        const CString& title = GetTitle();
        if (title.GetLength() > 0 && title.GetAt(0) != '!')
            CDocument::SetTitle("! " + title);
    }
}
