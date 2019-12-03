#include "stdafx.h"
#include "ExecuteTask.h"
#include "SQLWorksheetDoc.h"
#include "CommandPerformerImpl.h"
#include "COMMON\StrHelpers.h"
#include <OCI8/ACursor.h>
#include "BindSource.h" // for BindVarHolder
#include "StatView.h" // for StatGauge

using Common::print_time;
using namespace ServerBackgroundThread;

BackgroundTask_Execute::BackgroundTask_Execute (CPLSWorksheetDoc& doc) 
    : Task("Execute script/statement", 0),
    m_doc(doc),
    m_proxy(doc)
{
    doc.Lock(true);
}

BackgroundTask_Execute::~BackgroundTask_Execute ()
{
}

// TODO#2: consider auto switching to Output tab for showing execution progress
void BackgroundTask_Execute::DoInBackground (OciConnect& connect)
{
    clock64_t startTime = clock64(), endTime = 0;
    int line = -1;

    try
    {
        m_proxy.SetActivePrimeExecution(true);

        StatGauge::BeginMeasure(connect);

        if (m_ctx.IsSingleStatement())
            m_ctx.settings.autoscrollToExecutedStatement = false;

        CommandPerformerImpl performer(m_proxy, connect, m_ctx.bindVarHolder, m_ctx.settings);
        CommandParser commandParser(performer, m_ctx.substitution, m_ctx.settings.ignoreCommmentsAfterSemicolon);
        commandParser.Start(m_ctx.selection.start.line, m_ctx.selection.start.column);

        int exceptionCount = 0;
        line = m_ctx.selection.start.line;
        int offset = m_ctx.selection.start.column;
        int nlines = m_proxy.GetLineCount();
        for (; line < nlines && line <= m_ctx.selection.end.line; line++)
        {
            string str;
            m_proxy.GetLine(line, str);

            int len = (line == m_ctx.selection.end.line) ? min<int>(str.length(), m_ctx.selection.end.column) : str.length();

            commandParser.PutLine((len - offset > 0) ? &str.at(offset) : "", len - offset);
            offset = 0;

            if (m_ctx.IsSingleStatement() && performer.GetStatementCount())
                break;

            if (m_ctx.haltOnError && performer.GetExceptionCount() != exceptionCount)
            {
                if (m_ctx.settings.highlightExecutedStatement)
                {
                    int first, last;
                    if (commandParser.GetLastExecutedLines(first, last))
                        m_proxy.SetHighlighting(first, last, true);
                }

                if (performer.IsCancelled() || m_proxy.AskIfUserWantToStopOnError())
                    break;

                exceptionCount = performer.GetExceptionCount();
            }
        }
        commandParser.PutEOF();

        endTime = clock64();

        m_ctx.hasErrors             = performer.HasErrors();
        m_ctx.isLastStatementFailed = performer.IsLastStatementFailed();
        m_ctx.isLastStatementSelect = performer.IsLastStatementSelect();
        m_ctx.cursor                = performer.GetCursorOwnership();
        m_ctx.lastExecTime          = performer.GetLastExecTime();
        m_ctx.isLastStatementBind   = performer.IsLastStatementBind();
        m_ctx.lastLine              = line;

        if (m_ctx.isLastStatementFailed || !m_ctx.isLastStatementSelect)
		    m_ctx.cursor.reset(0); // not going use it so lets destroy it in the background thread
    }
    // the most of oci exeptions well be proceesed in ExecuteSql method,
    // but if it's a fatal problem (like connection lost), we receive it here
    catch (const OCI8::UserCancel& x)
    {
        endTime = clock64();
        m_ctx.userCancel = true;
        m_ctx.seriousError = true;
        m_ctx.seriousErrorText = x.what();
        m_ctx.seriousErrorLine = -1;
        try { m_ctx.cursor.reset(0); } catch (...) {}
    }
    catch (const OciException& x)
    {
        endTime = clock64();
        m_ctx.seriousError = true;
        m_ctx.seriousErrorText = x.what();
        m_ctx.seriousErrorLine = -1;
        try { m_ctx.cursor.reset(0); } catch (...) {}
    }
    // currently we break script execution on substitution exception,
    // but probably we should ask a user
    catch (const Common::AppException& x)
    {
        endTime = clock64();
        m_ctx.seriousError = true;
        m_ctx.seriousErrorText = x.what();
        m_ctx.seriousErrorLine = line;
        try { m_ctx.cursor.reset(0); } catch (...) {}
    }
    catch (const DocumentProxy::document_destroyed& x)
    {
        try { m_ctx.cursor.reset(0); } catch (...) {}
        m_proxy.SetActivePrimeExecution(false);
        SetError(x.what());
        throw;
    }
    catch (...)
    {
        try { m_ctx.cursor.reset(0); } catch (...) {}
        m_proxy.SetActivePrimeExecution(false);
        throw;
    }

    m_ctx.totalExecTime = double(endTime - startTime);

    vector<int> stats;
    StatGauge::EndMeasure(connect, stats);
    m_proxy.LoadStatistics(stats);
    
    m_proxy.SetActivePrimeExecution(false);
}

void BackgroundTask_Execute::ReturnInForeground ()
{
    if (IsOk())
    {
        m_proxy.TestDocumnet();
        m_doc.AfterSqlExecute(m_ctx); 

        OpenEditor::Position pos = m_doc.GetEditorView()->GetPosition();
        if (pos.line == m_ctx.cursorLine && pos.column == m_ctx.cursorPos)
            m_doc.GetEditorView()->SetTopmostLine(m_ctx.topmostLine);
    }
    m_proxy.Lock(false);
    FlashWindow(*AfxGetMainWnd(), TRUE);
}

void BackgroundTask_Execute::ReturnErrorInForeground ()
{
    m_proxy.Lock(false);
    FlashWindow(*AfxGetMainWnd(), TRUE);
}
