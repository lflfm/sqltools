#pragma once
#include "ServerBackgroundThread\TaskQueue.h"
#include "DocumentProxy.h"
#include "CommandPerformerImpl.h"

    struct ExecuteCtx : boost::noncopyable
    {
        // in
        CPLSWorksheetDoc::ExecutionMode mode;
        bool            stepToNext;
        OpenEditor::Square selection;
        bool            haltOnError;
        bool            substitution;
        arg::counted_ptr<BindVarHolder> bindVarHolder;

        // out
        bool            hasErrors, userCancel;
        bool            isLastStatementFailed;
        bool            isLastStatementSelect;
        std::auto_ptr<OciAutoCursor> cursor;
        double          lastExecTime;
        bool            isLastStatementBind;
        int             lastLine;
        bool            seriousError;
        string          seriousErrorText;
        int             seriousErrorLine;
        double          totalExecTime;
        int             topmostLine;
        int             cursorLine;
        int             cursorPos;

        CommandPerformerSettings settings;

        ExecuteCtx ()
            : settings(GetSQLToolsSettings()) // make a copy to pass in background thread
        {
            // in
            mode                        = CPLSWorksheetDoc::ExecutionModeCURRENT;
            stepToNext                  = false;
            haltOnError                 = false;
            substitution                = false;
            // out
            hasErrors                   = false;
            userCancel                  = false;
            isLastStatementFailed       = false;
            isLastStatementSelect       = false;
            lastExecTime                = 0;
            isLastStatementBind         = false;
            lastLine                    = -1;
            seriousError                = false;
            seriousErrorLine            = -1;
            totalExecTime               = 0;
            topmostLine                 = 0;
            cursorLine                  = -1;
            cursorPos                   = -1;
        }

        bool IsSingleStatement() const
        {
            return !(mode == CPLSWorksheetDoc::ExecutionModeALL || mode == CPLSWorksheetDoc::ExecutionModeFROM_CURSOR);
        }
    };

struct BackgroundTask_Execute : ServerBackgroundThread::Task
{
    CPLSWorksheetDoc&   m_doc;
    DocumentProxy       m_proxy;
    ExecuteCtx          m_ctx;

    BackgroundTask_Execute (CPLSWorksheetDoc&);
    ~BackgroundTask_Execute ();
    
    void DoInBackground (OciConnect&);

    void ReturnInForeground ();

    void ReturnErrorInForeground ();

    double GetExecutionTime () const  { return m_ctx.totalExecTime / CLOCKS_PER_SEC; }
};

