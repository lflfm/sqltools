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

// 06.10.2002 changes, OnOutputOpen has been moved to GridView
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables

#pragma once
#ifndef __PLSWORKSHEETDOC_H__
#define __PLSWORKSHEETDOC_H__

#include "SQLTools.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEDocManager.h"
#include "BindVariableTypes.h"

    using std::string;
    using std::vector;

    class OciGridView;
    class COutputView;
    class CStatView;
    class CHistoryView;
    class CExplainPlanTextView;

    class CommandParser;
    class CMDIMainFrame;
    class CMDIChildFrame;
    class C2PaneSplitter;
    class ExplainPlanView2;
	class BindGridView;
    class BindVarHolder;

    class SQLToolsSettings;

    namespace OCI8 { class Variable; }

    struct ExecuteCtx;


/** @brief Manages a single worksheet window, and all associated grids.
 */
class CPLSWorksheetDoc : public COEDocument
{
    friend class CBooklet;
    friend class SqlDocCreater;
    friend struct BackgroundTask_LoadDDLWithDbmsMetadata;
    friend struct BackgroundTask_DoSqlQuickQuery;

    bool m_LoadedFromDB;
    bool m_freshErrors;

    COEditorView* m_pEditor;
    CBooklet*     m_pBooklet;
    union
    {
        struct
        {
            OciGridView*  m_pDbGrid;
            CStatView*    m_pStatGrid;
            ExplainPlanView2* m_pExplainPlan;
            CExplainPlanTextView* m_pXPlan;
            COutputView*  m_pOutput;
            CHistoryView* m_pHistory;
			BindGridView* m_pBindGrid;
        };
        CView* m_BookletFamily[7];
    };

    arg::counted_ptr<BindVarHolder> m_bindVarHolder;
    clock64_t m_executionStartedAt;

protected:
    CString m_newPathName;
    virtual void GetNewPathName (CString& newName) const;
    void ActivateEditor ();
    void ActivateTab (CView*);

	CPLSWorksheetDoc();

	DECLARE_DYNCREATE(CPLSWorksheetDoc)

// Operations
public:
    static CMDIMainFrame* GetMainFrame () { return (CMDIMainFrame*)AfxGetApp()->m_pMainWnd; }
    static CPLSWorksheetDoc* GetActiveDoc ();

    C2PaneSplitter* GetSplitter ();
    COEditorView* GetEditorView ()        { return m_pEditor; }

    ExplainPlanView2* GetExplainPlanView ()         { return m_pExplainPlan; }
    CExplainPlanTextView* GetExplainPlanTextView () { return m_pXPlan; }

    void ActivatePlanTab ();

    void Init ();

    enum ExecutionStyle { ExecutionStyleOLD = 0, ExecutionStyleNEW = 1, ExecutionStyleTOAD = 2 };
    enum ExecutionMode  { ExecutionModeALL = 0, ExecutionModeFROM_CURSOR, ExecutionModeCURRENT, ExecutionModeCURRENT_ALT };

    void DoSqlExecute    (ExecutionMode, bool stepToNext = false);
    void AfterSqlExecute (ExecuteCtx&);
    void ShowLastError ();
    void AdjustCurrentStatementPos (ExecutionMode mode, ExecutionStyle style, OpenEditor::Square& sel, bool& emptySel);

    void SkipToTheNextStatement (int fromLine);
    void ShowSelectResult (std::auto_ptr<OciAutoCursor>& cursor, double lastExecTime);
    void ShowOutput (bool error);
    void AdjustErrorPosToLastLine (int& line, int& col) const;

    void DoSqlExplainPlan (const string&);

    void GoTo (int);

    void SetFreshErrors ()                                                           { m_freshErrors = true; }
    void PutError (const string& str, int line =-1, int pos =-1, bool skip = false);
    void PutMessage (const string& str, int line =-1, int pos =-1);
    void PutDbmsMessage (const string& str, int line =-1, int pos =-1);
    void MoveToEndOfOutput ();
    void AddStatementToHistory (time_t startTime, const std::string& duration, const string& connectDesc, const string& sqlSttm);
	void RefreshBindView (const vector<string>& data);
    void LoadStatistics (const vector<int>& data);
    void Lock (bool lock);
    void SetExecutionStarted (clock64_t at, int line); 
    void SetHighlighting  (int fistHighlightedLine, int lastHighlightedLine, bool updateWindow = false);

    static
    void LoadDDL (const string& owner, const string& name, const string& type);
    static
    void LoadDDLWithDbmsMetadata (const string& owner, const string& name, const string& type);

    void DoSqlLoad (bool withDbmsMetadata);
    void DoSqlQuickQuery (const char* query, const char* message); // accepts <TABLE> placeholder

    static void DoSqlQueryInNew (const string& query, const string& title);
    static void DoSqlQuickQuery (const string& query, const string& message);
    static void DoSqlQuery (const string& query, const string& title, const string& message, bool quick);

protected:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void OnCloseDocument();
	virtual BOOL CanCloseFrame (CFrameWnd* pFrame);
    virtual void OnTimer (UINT nIDEvent);
    virtual void OnContextMenuInit (CMenu*);

public:
	virtual void SetTitle (LPCTSTR lpszTitle);
	virtual ~CPLSWorksheetDoc();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSqlExecute();
	afx_msg void OnSqlExecuteFromCursor();
    afx_msg void OnSqlExecuteInSQLPlus();
	afx_msg void OnSqlExecuteCurrent();
	afx_msg void OnSqlExecuteCurrentAlt();
	afx_msg void OnSqlExecuteCurrentAndStep();
	afx_msg void OnSqlDescribe();
	afx_msg void OnSqlExplainPlan();
	afx_msg void OnSqlLoad();
    afx_msg void OnSqlLoadWithDbmsMetadata();
	afx_msg void OnUpdate_SqlGroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_SqlExecuteGroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_ErrorGroup(CCmdUI* pCmdUI);
	afx_msg void OnSqlCurrError();
	afx_msg void OnSqlNextError();
	afx_msg void OnSqlPrevError();
    afx_msg void OnSqlHistoryGet();
    afx_msg void OnSqlHistoryStepforwardAndGet();
    afx_msg void OnSqlHistoryGetAndStepback();
    afx_msg void OnUpdate_SqlHistory (CCmdUI *pCmdUI);
    afx_msg void OnUpdate_OciGridIndicator (CCmdUI* pCmdUI);
    afx_msg void OnSqlHelp();
    afx_msg void OnWindowNew();
    afx_msg void OnUpdate_WindowNew (CCmdUI *pCmdUI);
    afx_msg void OnSqlCommit ();
    afx_msg void OnSqlRollback ();
    afx_msg void OnEditAutocomplete();
    afx_msg void OnSqlQuickQuery();
    afx_msg void OnSqlQuickCount();
    afx_msg void OnFindInFiles();
};

//{{AFX_INSERT_LOCATION}}

#endif//__PLSWORKSHEETDOC_H__
