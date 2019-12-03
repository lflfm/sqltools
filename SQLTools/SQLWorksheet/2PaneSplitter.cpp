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

/*
    03.03.2002 bug fix, splitter jumps after window resizing with <Shift>
*/

#include "stdafx.h"
#include "SQLTools.h"
#include "2PaneSplitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C2PaneSplitter


IMPLEMENT_DYNCREATE(C2PaneSplitter, CSplitterWnd)

BEGIN_MESSAGE_MAP(C2PaneSplitter, CSplitterWnd)
	//{{AFX_MSG_MAP(C2PaneSplitter)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C2PaneSplitter::C2PaneSplitter()
{
    m_ititialized = false;
    m_pwndBooklet = 0;
    LowPaneHight[0] =
    LowPaneHight[1] = 0;
    m_defaultHight = GetSQLToolsSettings().GetMainWindowFixedPaneDefaultHeight(); //defailt is 150;
    m_defaultHightRow = !GetSQLToolsSettings().GetMainWindowFixedPane() ? 0 : 1;  //default is 1
}


C2PaneSplitter::~C2PaneSplitter()
{
    //DON'T DELETE m_pwndBooklet because it's CView object
    //delete m_pwndBooklet;
}


BOOL C2PaneSplitter::Create (CWnd* pParent, CCreateContext* pContext)
{
    CreateStatic(pParent, 2, 1);
    SetRowInfo(0, 200, 35);
    SetRowInfo(1, 100, 45);
    CreateView(0, 0, pContext->m_pNewViewClass,  CSize(100, 100), pContext);
    CreateView(1, 0, RUNTIME_CLASS(CBooklet), CSize(100, 100), pContext);

    m_pwndBooklet = (CBooklet*)GetPane(1, 0);

    //if (!m_pwndBooklet || 
    //!m_pwndBooklet->SetDocument((CPLSWorksheetDoc*)(pContext->m_pCurrentDoc)))
    //    return true;

    m_ititialized = true;

    return TRUE;
}


void C2PaneSplitter::MoveDown()
{
    int h[2];
    GetRowInfo(0, h[0], h[1]);
    SetRowInfo(0, h[0]+50, h[1]);
    RecalcLayout();
    GetRowInfo(1, LowPaneHight[0], LowPaneHight[1]);
}


void C2PaneSplitter::MoveUp()
{
    int h[2];
    GetRowInfo(0, h[0], h[1]);
    SetRowInfo(0, max(0, h[0]-50), h[1]);
    RecalcLayout();
    GetRowInfo(1, LowPaneHight[0], LowPaneHight[1]);
}

void C2PaneSplitter::MoveToDefault()
{
    int h[4];
    GetRowInfo(0, h[0], h[1]);
    GetRowInfo(1, h[2], h[3]);

    if (!m_defaultHightRow)
    {
        SetRowInfo(0, m_defaultHight, h[1]);
    }
    else
    {
        int H = h[0] + h[2];
        SetRowInfo(0, max(m_defaultHight, H - m_defaultHight), h[1]);
    }

    RecalcLayout();
    GetRowInfo(1, LowPaneHight[0], LowPaneHight[1]);
}


void C2PaneSplitter::StopTracking (BOOL bAccept)
{
    CSplitterWnd::StopTracking(bAccept);

    if (bAccept)
        GetRowInfo(1, LowPaneHight[0], LowPaneHight[1]);
}

void C2PaneSplitter::OnSize (UINT nType, int cx, int cy)
{
    CSplitterWnd::OnSize(nType, cx, cy);

    if (m_ititialized
    && (nType == SIZE_MAXIMIZED || nType == SIZE_RESTORED)
    && m_defaultHightRow == 1)
    {
        // 03.03.2002 bug fix, splitter jumps after window resizing with <Shift>
        if ((0xFF00 & GetKeyState(VK_LBUTTON)) && (0xFF00 & GetKeyState(VK_SHIFT)))
        {
            RecalcLayout();
            GetRowInfo(1, LowPaneHight[0], LowPaneHight[1]);
        }
        else
        {
            int h2[4];
            GetRowInfo(0, h2[0], h2[1]);
            GetRowInfo(1, h2[2], h2[3]);
            int dy = h2[2] - LowPaneHight[0];
            SetRowInfo(0, ((h2[0] + dy) > h2[1]) ? (h2[0] + dy) : 0, h2[1]);
            SetRowInfo(1, ((h2[2] - dy) > h2[3]) ? (h2[2] - dy) : 0, h2[3]);
            RecalcLayout();
        }
    }
}

void C2PaneSplitter::SetActiveView (CView* pView)
{
    CWnd* pFrame = GetParent();

    if (pView->IsKindOf(RUNTIME_CLASS(CView))
    && pFrame->IsKindOf(RUNTIME_CLASS(CFrameWnd)) )
        ((CFrameWnd*)pFrame)->SetActiveView((CView*)pView);
}

void C2PaneSplitter::NextPane()
{
    ASSERT(m_pwndBooklet);
    if (GetFocus() != m_pwndSQLEdit)
    {
        SetActiveView(m_pwndSQLEdit);
        m_pwndSQLEdit->SetFocus();
    }
    else
    {
        CView* pView;
        VERIFY(m_pwndBooklet->GetActiveTab(pView));
        SetActiveView((CView*)pView);
        pView->SetFocus();
    }
}

void C2PaneSplitter::PrevPane()
{
    ASSERT(m_pwndBooklet);
    if (GetFocus() != m_pwndSQLEdit)
    {
        SetActiveView(m_pwndSQLEdit);
        m_pwndSQLEdit->SetFocus();
    }
    else
    {
        CView* pView;
        VERIFY(m_pwndBooklet->GetActiveTab(pView));
        SetActiveView((CView*)pView);
        pView->SetFocus();
    }
}

void C2PaneSplitter::NextTab()
{
    int i;
    VERIFY(m_pwndBooklet->GetActiveTab(i));
    m_pwndBooklet->ActivateTab(i < m_pwndBooklet->GetTabCount() - 1 ? ++i : 0);
}

void C2PaneSplitter::PrevTab()
{
    int i;
    VERIFY(m_pwndBooklet->GetActiveTab(i));
    m_pwndBooklet->ActivateTab(i > 0 ? --i : m_pwndBooklet->GetTabCount() - 1);
}

