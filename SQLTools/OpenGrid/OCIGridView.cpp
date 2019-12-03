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

// 19.10.2002 bug fix, goto the last record does not fetch all rows
// 07.11.2003 bug fix, "Invalid vector<T> subscript" happens continually
// if a query returns error during fetching the first portion of records
// 12.10.2002 bug fix, grid column autofit has been brocken since build 38
// 2017-11-28 implemented range selection

#include "stdafx.h"
#include "resource.h"

#include "SQLTools.h"

#include <COMMON\ExceptionHelper.h>
#include <OCI8\ACursor.h>
#include "GridManager.h"
#include "OCISource.h"
#include "OCIGridView.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include "COMMON\AppGlobal.h"

using namespace ServerBackgroundThread;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;
    using Common::nvector;

    // created for calling grid.MoveToEnd after source.Fetch is done
    struct BackgroundTask_MoveToEnd : Task
    {
        HWND m_hWnd;
        GridManager& m_grid;
        eDirection m_dir;

        BackgroundTask_MoveToEnd (HWND hWnd, OG2::GridManager& grid, eDirection dir)
            : Task("MoveToEnd", 0),
            m_hWnd(hWnd),
            m_grid(grid),
            m_dir(dir)
        {
            SetSilent(true);
        }

        void DoInBackground (OciConnect&) {}

        void ReturnInForeground ()
        {
            if (::IsWindow(m_hWnd))
            {
                m_grid.MoveToEnd(m_dir);
            }
        }

    };

    class OciGridManager : public GridManager
    {
    public:
        OciGridManager (CWnd* client , OciGridSource* source)
        : GridManager(client, source, FALSE) {}

        OciGridSource* GetSource () { return static_cast<OciGridSource*>(m_pSource); }
        const OciGridSource* GetSource () const { return static_cast<const OciGridSource*>(m_pSource); }

        bool fetch (eDirection dir, int rows)
        {
            if (theApp.GetActivePrimeExecution())
            {
                Global::SetStatusText("Cannot fetch the data because the connection is busy!");
                return false;
            }

            if ((dir == edVert && GetSource()->IsTableOrientation())
            || (dir == edHorz && !GetSource()->IsTableOrientation()))
            {
                if (!GetSource()->AllRowsFetched())
                {
                    GetSource()->Fetch(*static_cast<OciGridView*>(m_pClientWnd), rows);
                    return true;
                }
            }
            return false;
        }

        virtual void AfterMove (eDirection dir, int oldVal)
        {
            if (dir == edVert
            && (m_Rulers[edVert].GetUnusedCount()
                || m_Rulers[edVert].GetLastVisiblePos() == m_pSource->GetCount(edVert)))
                fetch(dir, GetPrefetch());

            GridManager::AfterMove(dir, oldVal);
        }

        virtual void AfterScroll (eDirection dir)
        {
            if (dir == edVert
            && (m_Rulers[edVert].GetUnusedCount()
                || m_Rulers[edVert].GetLastVisiblePos() == m_pSource->GetCount(edVert)))
                fetch(dir, GetPrefetch());

            GridManager::AfterScroll(dir);
        }

        virtual bool MoveToEnd (eDirection dir)
        {
            bool fetching = fetch(dir, INT_MAX);
            if (fetching)
            {
                FrgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_MoveToEnd(*m_pClientWnd, *this, dir)));
            }
            return GridManager::MoveToEnd(dir);
        }

        virtual bool ScrollToEnd (eDirection dir)
        {
            fetch(dir, INT_MAX);
            return GridManager::ScrollToEnd(dir);
        }

        static int GetPrefetch ()
        {
            int limit = GetSQLToolsSettings().GetGridPrefetchLimit();
            return limit == 0 ? INT_MAX : (limit < 60 ? 60 : limit);
        }
    };

/////////////////////////////////////////////////////////////////////////////
// GridView

IMPLEMENT_DYNCREATE(OciGridView, GridView)

OciGridView::OciGridView()
{
    SetVisualAttributeSetName("Data Grid Window");
    m_pOciSource = new OciGridSource;
    m_pManager = new OciGridManager(this, m_pOciSource);
    m_pManager->m_Options.m_Editing = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_Rotation = true;
    m_pManager->m_Options.m_ClearRecords = true;
    m_pManager->m_Options.m_Wraparound = GetSQLToolsSettings().GetGridWraparound();
    m_pManager->m_Options.RangeSelect = true;
    m_uPopupMenuId = IDR_DATA_GRID_POPUP;
    m_nIDHelp = IDR_DATAGRID;
}

OciGridView::~OciGridView()
{
    try { EXCEPTION_FRAME;

        delete m_pManager;
        delete m_pOciSource;
    }
    _DESTRUCTOR_HANDLER_;
}

BEGIN_MESSAGE_MAP(OciGridView, GridView)
    ON_WM_INITMENUPOPUP()
	ON_WM_LBUTTONDBLCLK()

    ON_COMMAND_RANGE(ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, OnChangeColumnFit)
    ON_UPDATE_COMMAND_UI_RANGE(ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, OnUpdate_ColumnFit)

    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_OciGridIndicator)
    ON_COMMAND(ID_GRID_ROTATE, OnRotate)
    ON_COMMAND(ID_GRID_ROTATE, OnRotate)

    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditGroup)

    ON_COMMAND(ID_HELP, OnHelp)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
END_MESSAGE_MAP()

void OciGridView::Clear ()
{
    try { EXCEPTION_FRAME;

        m_pOciSource->Clear();
        m_pManager->Clear();
    }
    _DEFAULT_HANDLER_
}

void OciGridView::Refresh ()
{
    try { EXCEPTION_FRAME;

        m_pManager->Refresh();
    }
    _DEFAULT_HANDLER_
}

int OciGridView::GetCurrentRecord () const
{
    return m_pManager->GetCurrentPos(edVert);
}

int OciGridView::GetRecordCount () const
{
    return m_pOciSource->GetCount(edVert);
}

    // created for calling grid.MoveToEnd after source.Fetch is done
    struct BackgroundTask_ResizeHeaders : Task
    {
        HWND m_hWnd;
        GridManager* m_pManager;
        OciGridSource* m_pOciSource;
        vector<string> m_headers;
        std::map<int, int> m_resizedItems;

        BackgroundTask_ResizeHeaders (HWND hWnd, OG2::GridManager* manager, OciGridSource* source)
            : Task("ResizeHeaders", 0),
            m_hWnd(hWnd),
            m_pManager(manager),
            m_pOciSource(source)
        {
            SetSilent(true);

            m_resizedItems = m_pManager->GetResizedItems(edHorz);

            for (int i = 0; i < m_pOciSource->GetCount(edHorz); i++)
                m_headers.push_back(m_pOciSource->GetColHeader(i));
        }

        void DoInBackground (OciConnect&) {}

        void ReturnInForeground ()
        {
            if (::IsWindow(m_hWnd))
            {
                bool same = false;
                if (m_pOciSource->IsTableOrientation())
                if (m_pOciSource->GetCount(edHorz) == static_cast<int>(m_headers.size()))
                {
                    same = true;
                    for (int i = 0; i < m_pOciSource->GetCount(edHorz); i++)
                        if (m_pOciSource->GetColHeader(i) != m_headers[i])
                        {
                            same = false;
                            break;
                        }
                }

                if (same)
                {
                    std::map<int, int> userResizedItems;
                    std::map<int, int>::const_iterator it = m_resizedItems.begin();
                    for (; it != m_resizedItems.end(); ++it)
                        if (HIWORD(it->second)) // set by user
                            userResizedItems.insert(*it);

                    m_pManager->SetResizedItems(edHorz, userResizedItems);
                }

            }
        }

    };

void OciGridView::SetCursor (std::auto_ptr<OCI8::AutoCursor>& cursor)
{
    TaskPtr resizeHeaders;

    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if (m_pOciSource->IsTableOrientation())
    if (!m_pOciSource->IsEmpty() && settings.GetGridAllowRememColWidth())
        // create task before Clear and SetCursor
        resizeHeaders = TaskPtr(new BackgroundTask_ResizeHeaders(*this, m_pManager, m_pOciSource));

    m_pOciSource->Clear(); // it has to be called before m_pManager->Clear()
    m_pManager->Clear();

    m_pOciSource->SetCursor(*this, cursor);

    if (resizeHeaders.get()) // schedule to execute after SetCursor and before Fetch
        FrgdRequestQueue::Get().Push(resizeHeaders);

    m_pOciSource->Fetch(*this, OciGridManager::GetPrefetch());
}

void OciGridView::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    GridView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (!IsEmpty() && m_pOciSource->GetCount(edVert) > 0)
    {
        pPopupMenu->EnableMenuItem(ID_OCIGRID_DATA_FIT,        MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_OCIGRID_COLS_TO_HEADERS, MF_BYCOMMAND|MF_ENABLED);

        pPopupMenu->EnableMenuItem(ID_GRID_COPY_AS_TEXT,             MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_AS_PLSQL_VALUES,     MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_AS_PLSQL_EXPRESSION, MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_AS_PLSQL_IN_LIST,    MF_BYCOMMAND|MF_ENABLED);

        pPopupMenu->EnableMenuItem(ID_GRID_COPY_AS_PLSQL_INSERTS,  m_pOciSource->IsTableOrientation() ?  MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);

        pPopupMenu->EnableMenuItem(ID_EDIT_SELECT_ALL, MF_BYCOMMAND|MF_ENABLED);
    }

    pPopupMenu->CheckMenuRadioItem(
        ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS,
        (!GetSQLToolsSettings().GetGridColumnFitType()) ? ID_OCIGRID_DATA_FIT : ID_OCIGRID_COLS_TO_HEADERS,
        MF_BYCOMMAND);
}

void OciGridView::OnChangeColumnFit (UINT id)
{
    switch (id)
    {
    case ID_OCIGRID_DATA_FIT:           GetSQLToolsSettingsForUpdate().SetGridColumnFitType(0); break;
    case ID_OCIGRID_COLS_TO_HEADERS:    GetSQLToolsSettingsForUpdate().SetGridColumnFitType(1); break;
    }

    AutofitColumns(-1, -1, true);
}

void OciGridView::OnUpdate_ColumnFit (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!IsEmpty() && m_pOciSource->GetCount(edVert) > 0 && m_pOciSource->IsTableOrientation());

    if (pCmdUI->m_pMenu)
        pCmdUI->m_pMenu->CheckMenuRadioItem(ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, 
            !GetSQLToolsSettings().GetGridColumnFitType() ? ID_OCIGRID_DATA_FIT : ID_OCIGRID_COLS_TO_HEADERS, MF_BYCOMMAND);
    else
        pCmdUI->SetRadio(!GetSQLToolsSettings().GetGridColumnFitType() ? pCmdUI->m_nID == ID_OCIGRID_DATA_FIT : pCmdUI->m_nID == ID_OCIGRID_COLS_TO_HEADERS);
}

void OciGridView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    int nItem;

    if (!IsEmpty()
    && m_pOciSource->IsTableOrientation()
    && m_pOciSource->GetCount(edVert) > 0
    && m_pManager->IsMouseInColResizingPos(edHorz, point, nItem))
    {
        nItem += m_pManager->m_Rulers[edHorz].GetTopmost();
        AutofitColumns(nItem - 1, -1, true);
    }
    else
        GridView::OnLButtonDblClk(nFlags, point);
}

void OciGridView::AutofitColumns (int column /*= -1*/, int fromRow /*= -1*/, bool force /*= false*/)
{
    if (!IsEmpty()
    && m_pOciSource->IsTableOrientation()
    && m_pOciSource->GetCount(edVert) > 0)
    {
        CWaitCursor wait;
        {
            CClientDC dc(this);
            OnPrepareDC(&dc, 0);
            PaintGridManager::m_Dcc.m_Dc = &dc;

            bool byData     = GetSQLToolsSettings().GetGridColumnFitType() == 0;
            bool byHeader   = !byData || !GetSQLToolsSettings().GetGridAllowLessThanHeader();
            int maxColLen   = GetSQLToolsSettings().GetGridMaxColLength();
            int columnCount = m_pOciSource->GetCount(edHorz);
            int rowCount    = m_pOciSource->GetCount(edVert);
            int colStart    = 0;

            if (column >= 0 && column < columnCount)
            {
                colStart = column;
                columnCount = column + 1;
            }

            PaintGridManager::m_Dcc.m_Type[edHorz] = efNone;

            for (int col = colStart; col < columnCount; col++)
            {
                int width = (fromRow > 0) ? m_pManager->m_Rulers[edHorz].GetPixelSize(col) : 0;

                PaintGridManager::m_Dcc.m_Col = col;

                if (byHeader)
                {
                    PaintGridManager::m_Dcc.m_Row = 0;
                    PaintGridManager::m_Dcc.m_Type[edVert] = efFirst;
                    m_pOciSource->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
                    width = max(width, PaintGridManager::m_Dcc.m_Rect.Width());
                }
                if (byData)
                {
                    PaintGridManager::m_Dcc.m_Type[edVert] = efNone;
                    for (int row = (fromRow >= 0) ? fromRow : 0; row < rowCount; row++)
                    {
                        PaintGridManager::m_Dcc.m_Row = row;
                        m_pOciSource->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
                        width = max(width, PaintGridManager::m_Dcc.m_Rect.Width());
                    }
                }
                m_pManager->m_Rulers[edHorz].SetPixelSize(col, width, false, force);
            }
        }
        m_pManager->LightRefresh(edHorz);
    }
}

void OciGridView::OnRotate ()
{
    GridView::OnRotate();
    if (m_pOciSource->IsTableOrientation())
        AutofitColumns(-1, -1, true);
}

void OciGridView::OnUpdate_OciGridIndicator (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);

    StatusCxt statusCxt(
        m_pManager->GetCurrentPos(edVert) + 1,
        m_pOciSource->GetCount(edVert),
        m_pOciSource->AllRowsFetched());

    if (m_statusCxt != statusCxt)
    {
        m_statusCxt = statusCxt;

        if (m_statusCxt.m_numOfRec) {
            m_statusText.Format("Record %d of %d", m_statusCxt.m_curRec, m_statusCxt.m_numOfRec);

            if (!m_statusCxt.m_allRowsFetched)
                m_statusText += ", last not fetched.";
        }
        else
            m_statusText = "No records";
    }

    pCmdUI->SetText(m_statusText);
}

void OciGridView::OnSettingsChanged ()
{
    GridView::OnSettingsChanged();

    m_pManager->m_Options.m_Wraparound = GetSQLToolsSettings().GetGridWraparound();
}

void OciGridView::SetPaleColors (bool pale)
{
    if (m_pManager->m_Options.m_PaleColors != pale)
    {
        m_pManager->m_Options.m_PaleColors = pale;
        Invalidate(FALSE);
    }
}

void OciGridView::OnEditSelectAll ()
{
    m_pOciSource->FetchAll();
    m_pManager->SelectAll();
}

void OciGridView::OnHelp ()
{
    if (::GetFocus() == m_hWnd)
        SendMessage(WM_COMMANDHELP);
}

LRESULT OciGridView::OnCommandHelp (WPARAM, LPARAM lParam)
{
    if (lParam == 0 && m_nIDHelp != 0)
        lParam = HID_BASE_RESOURCE + m_nIDHelp;
    if (lParam != 0)
    {
        CWinApp* pApp = AfxGetApp();
        if (pApp != NULL)
            pApp->WinHelpInternal(lParam);
        return TRUE;
    }
    return FALSE;
}

LRESULT OciGridView::OnHelpHitTest(WPARAM, LPARAM)
{
    if (m_nIDHelp != 0)
        return HID_BASE_RESOURCE + m_nIDHelp;
    return 0;
}
