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

// 23.03.2005 bug fix, flicker on resizing

#include "stdafx.h"
#include <afxcview.h>
#include "SQLTools.h"
#include <OpenGrid\GridView.h>

#include "ChildFrm.h"
#include "MainFrm.h"
#include "SQLWorksheet/Booklet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    
BOOL CMDIChildFrame::m_MaximizeFirstDocument = TRUE;

IMPLEMENT_DYNCREATE(CMDIChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CMDIChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMDIChildFrame)
	ON_COMMAND(ID_CUSTOM_NEXT_PANE, OnCustomNextPane)
	ON_COMMAND(ID_CUSTOM_PREV_PANE, OnCustomPrevPane)
	ON_COMMAND(ID_CUSTOM_NEXT_TAB, OnCustomNextTab)
	ON_COMMAND(ID_CUSTOM_PREV_TAB, OnCustomPrevTab)
	ON_COMMAND(ID_CUSTOM_SPLITTER_DOWN, OnCustomSplitterDown)
	ON_COMMAND(ID_CUSTOM_SPLITTER_UP, OnCustomSplitterUp)
	ON_WM_DESTROY()
	ON_WM_MDIACTIVATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_COMMAND(ID_CUSTOM_SPLITTER_DEF, OnCustomSplitterDefault)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CMDIChildFrame::CMDIChildFrame ()
{
}

CMDIChildFrame::~CMDIChildFrame ()
{
}

BOOL CMDIChildFrame::OnCreateClient (LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    m_wndSplitter.Create(this, pContext);

    m_wndSplitter.SetEditorView((CView*)m_wndSplitter.GetPane(0,0));
    SetActiveView(m_wndSplitter.GetEditorView());

    if (m_MaximizeFirstDocument)
    {
        CWnd* other = GetParent()->GetWindow(GW_CHILD);
        for (int i = 0; other; ) 
        {
            if (other->IsKindOf(RUNTIME_CLASS(CMDIChildFrame))
            && other != this) 
            {
                i++;
                break;
            }
            other = other->GetNextWindow();
        }
        if (!i)
            GetParent()->PostMessage(WM_MDIMAXIMIZE, (WPARAM)m_hWnd);
    }

    PostMessage(WM_COMMAND, ID_CUSTOM_SPLITTER_DEF);

    // Workbook implementation
    ASSERT_KINDOF(CMDIMainFrame, GetMDIFrame());
    ((CMDIMainFrame*)GetMDIFrame())->OnCreateChild(this);

	return TRUE;
}

void CMDIChildFrame::OnCustomNextPane()
{
    m_wndSplitter.NextPane();
}

void CMDIChildFrame::OnCustomPrevPane()
{
    m_wndSplitter.PrevPane();
}

void CMDIChildFrame::OnCustomNextTab()
{
    m_wndSplitter.NextTab();
}

void CMDIChildFrame::OnCustomPrevTab()
{
    m_wndSplitter.PrevTab();
}

void CMDIChildFrame::OnCustomSplitterDown() 
{
    m_wndSplitter.MoveDown();
}

void CMDIChildFrame::OnCustomSplitterUp() 
{
    m_wndSplitter.MoveUp();
}

void CMDIChildFrame::OnCustomSplitterDefault()
{
    m_wndSplitter.MoveToDefault();
}

// Workbook implementation

void CMDIChildFrame::OnDestroy () 
{
    CMDIChildWnd::OnDestroy();

    ((CMDIMainFrame*)GetMDIFrame())->OnDestroyChild(this);
}

void CMDIChildFrame::OnMDIActivate (BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if (bActivate && this == pActivateWnd)
        ((CMDIMainFrame*)GetMDIFrame())->OnActivateChild(this);
}

LRESULT CMDIChildFrame::OnSetText (WPARAM, LPARAM lParam)
{
    Default();

    ((CMDIMainFrame*)GetMDIFrame())->OnRenameChild(this, (LPCTSTR)lParam);

    return TRUE;
}

// the method has been added because of garbage on open document error 
BOOL CMDIChildFrame::OnEraseBkgnd(CDC* pDC)
{
    RECT rc;
    GetClientRect(&rc);
    ::FillRect(pDC->m_hDC, &rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));
    return TRUE;
}

BOOL CMDIChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // 23.03.2005 bug fix, flicker on resizing
    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    return CMDIChildWnd::PreCreateWindow(cs);
}
