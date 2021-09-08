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

#include "stdafx.h"
#include "SQLTools.h"
#include "GrepView.h"
#include "GrepThread.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UM_ENDPROCESS WM_USER

IMPLEMENT_DYNCREATE(CGrepView, CTreeCtrl)

CGrepView::CGrepView()
: m_pThread(0),
m_evRun(FALSE, TRUE),
m_evAbort(FALSE, TRUE),
m_initialInfoItem(NULL)
{
}

CGrepView::~CGrepView()
{
    try {
        if (m_pThread
        && WaitForSingleObject(m_pThread->m_hThread, 500) == WAIT_TIMEOUT)
            TerminateThread(m_pThread->m_hThread, 0);
    } _DESTRUCTOR_HANDLER_;
}

BEGIN_MESSAGE_MAP(CGrepView, CTreeCtrl)
    //{{AFX_MSG_MAP(CGrepView)
    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
    ON_WM_LBUTTONDBLCLK()
    //}}AFX_MSG_MAP
    ON_MESSAGE_VOID(UM_ENDPROCESS, OnEndProcess)
    ON_COMMAND(ID_GREP_OPEN_FILE,    OnOpenFile)
    ON_COMMAND(ID_GREP_COLLAPSE_ALL, OnCollapseAll)
    ON_COMMAND(ID_GREP_EXPAND_ALL,   OnExpandAll)
    ON_COMMAND(ID_GREP_CLEAR,        OnClear)
    ON_COMMAND(ID_GREP_BREAK,        AbortProcess)
END_MESSAGE_MAP()

BOOL CGrepView::Create (CWnd* pFrameWnd)
{
    CRect rect;
    pFrameWnd->GetClientRect(rect);
    return CTreeCtrl::Create(WS_CHILD|WS_VISIBLE, rect, pFrameWnd, 1);
}

int CGrepView::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

    static CFont font;

    // older versions of Windows* (NT 3.51 for instance)
	// fail with DEFAULT_GUI_FONT
    if (!(HFONT)font)
    	if (!font.CreatePointFont(80, L"Courier New")) return -1;
    	//if (!font.CreateStockObject(DEFAULT_GUI_FONT))
    		//if (!font.CreatePointFont(80, L"MS Sans Serif")) return -1;

    SetFont(&font);

    static CImageList images;

    if (!images.m_hImageList)
        images.Create(IDB_FIND_FILES, 14, 64, RGB(0,255,0));

    SetImageList(&images, TVSIL_NORMAL);
    ModifyStyle(0, TVS_FULLROWSELECT, 0);

	return 0;
}

void CGrepView::OnRClick (NMHDR*, LRESULT* pResult)
{
    CPoint point;
    GetCursorPos(&point);

    OnContextMenu(this, point);

    *pResult = 1;
}

void CGrepView::OnContextMenu (CWnd*, CPoint point)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDM_GREP_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);

    BOOL openFileEnabled = FALSE;
    if (point.x != -1 && point.y != -1)
    {
        CPoint pt(point);
        ScreenToClient(&pt);
        UINT nHitFlags;
        HTREEITEM hItem = HitTest(pt, &nHitFlags);
        if (nHitFlags & TVHT_ONITEM)
        {
            int index = GetItemData(hItem);
            if (index >= 0)
                openFileEnabled = TRUE;

            SelectItem(hItem);
        }
    }
    else
    {
        if (HTREEITEM hItem = GetSelectedItem()) 
        {
            int index = GetItemData(hItem);
            if (index >= 0)
                openFileEnabled = TRUE;
            CRect rect;
            GetItemRect(hItem, &rect, TRUE);
            point.x = rect.left;
            point.y = rect.bottom;
            ClientToScreen(&point);
        }
    }

    menu.EnableMenuItem(ID_GREP_OPEN_FILE, openFileEnabled ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
    menu.EnableMenuItem(ID_GREP_BREAK, IsRunning() ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CGrepView::StartProcess (CGrepThread* pThread)
{
    if (pThread
    && !m_pThread
    && !WaitForSingleObject(m_evRun, 0) == WAIT_OBJECT_0)
    {
        m_pThread = pThread;
        m_evRun.SetEvent();
        m_evAbort.ResetEvent();
        ClearContent();
    }
    else
        AfxThrowUserException();
}

void CGrepView::AbortProcess ()
{
  if (WaitForSingleObject(m_evRun, 0) == WAIT_OBJECT_0)
        m_evAbort.SetEvent();
}

void CGrepView::EndProcess ()
{
    SendMessage(UM_ENDPROCESS);
}

void CGrepView::OnEndProcess ()
{
    //BOOL bAbort = IsAbort();

    m_pThread = 0;
    m_evRun.ResetEvent();
    m_evAbort.ResetEvent();

    Invalidate();
    UpdateWindow();
}

void CGrepView::ClearContent ()
{
    CWaitCursor wait;

    {
        CSingleLock lk(&m_criticalSection, TRUE); 
        m_foundItems.clear();
        m_initialInfo.clear();
        m_initialInfoItem = NULL;
    }

    LockWindowUpdate();
    DeleteAllItems();
    UnlockWindowUpdate();
}

HTREEITEM CGrepView::addInfo (const std::wstring& text, bool error)
{
    TV_INSERTSTRUCT	TVStruct;
    TVStruct.hParent      = TVI_ROOT;
    TVStruct.hInsertAfter = TVI_LAST;
    TVStruct.item.mask    = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE
                          | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    TVStruct.item.state     =
    TVStruct.item.stateMask = 0;

    TVStruct.item.pszText   = (LPWSTR)text.c_str();
    TVStruct.item.mask     |= TVIF_PARAM;
    TVStruct.item.cChildren = 0;
    TVStruct.item.iImage    = TVStruct.item.iSelectedImage = !error ? 1 : 3;
    TVStruct.item.lParam    = -1;

    return InsertItem(&TVStruct);
}

void CGrepView::AddInitialInfo (const std::wstring& info)
{
    m_initialInfo = info;
    m_initialInfoItem = addInfo(info, false);
    SelectItem(m_initialInfoItem);
}

void CGrepView::AddInfo (const std::wstring& info)
{
    addInfo(info, false);
}

void CGrepView::AddError (const std::wstring& error)
{
    addInfo(error, true);
}

void CGrepView::AddFoundMatch (const std::wstring& path, const std::wstring& text, int line, int start, int end, int matchingLines, int totalMatchingLines, bool expanded, bool inMemory)
{
    CSingleLock lk(&m_criticalSection, TRUE); 

    HTREEITEM hFileItem = NULL;
    TV_INSERTSTRUCT	TVStruct;
    TVStruct.hParent      = TVI_ROOT;
    TVStruct.hInsertAfter = TVI_LAST;
    TVStruct.item.mask    = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE
                          | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    TVStruct.item.state     =
    TVStruct.item.stateMask = expanded ? TVIS_EXPANDED : 0;

    bool parentInserted = false;
    if (m_foundItems.empty() || m_foundItems.rbegin()->path != path)
    {
        TVStruct.item.pszText   = (LPWSTR)path.c_str();
        TVStruct.item.cChildren = 1;
        TVStruct.item.iImage    = TVStruct.item.iSelectedImage = !inMemory ? 0 : 4;
        hFileItem = InsertItem(&TVStruct);
        parentInserted = true;
    }
    else
    {
        hFileItem = GetParentItem(m_foundItems.rbegin()->hitem);
    }

    {
        std::wostringstream out;
        out << std::setw(3) << (line + 1) << L"(" << std::setw(2) << (start + 1) << L"):" << text;
        std::wstring buffer = out.str();

        TVStruct.hParent      = hFileItem;
        TVStruct.item.mask    = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        TVStruct.item.iImage  = TVStruct.item.iSelectedImage = 2;
        TVStruct.item.pszText = (LPWSTR)buffer.c_str();
        HTREEITEM hItem = InsertItem(&TVStruct);

        m_foundItems.push_back(found_item { hItem, path, text, line, start, end });
        SetItemData(hItem, m_foundItems.size() - 1);
    }

    {
        std::wostringstream out;
        out << path << L" (" << matchingLines << ((inMemory) ? L" matches in memory)" : L" matches)");
        std::wstring buffer = out.str();
        SetItemText(hFileItem, buffer.c_str());
        if (parentInserted)
            SetItemData(hFileItem, m_foundItems.size() - 1);
    }

    if (m_initialInfoItem)
    {
        std::wostringstream out;
        out << m_initialInfo << L" (" << totalMatchingLines << " matches)";
        std::wstring buffer = out.str();
        SetItemText(m_initialInfoItem, buffer.c_str());
    }

}

void CGrepView::OnOpenFile ()
{
    if (HTREEITEM hItem = GetSelectedItem()) 
    {
        int index = GetItemData(hItem);

        if (index >= 0)
        {
            CSingleLock lk(&m_criticalSection, TRUE); 

            if ((unsigned int)index < m_foundItems.size())
            {
                const found_item& item = m_foundItems.at(index);

                CDocument* pDocument = AfxGetApp()->OpenDocumentFile(item.path.c_str());

                if (pDocument
                && pDocument->IsKindOf(RUNTIME_CLASS(CPLSWorksheetDoc)))
                {
                    COEditorView* editor = ((CPLSWorksheetDoc*)pDocument)->GetEditorView();
                    
                    OpenEditor::Position pos;
                    pos.line = item.line;
                    pos.column = item.start;
                    editor->MoveToAndCenter(pos);

                    OpenEditor::Square selection;
                    selection.start.line   = item.line;
                    selection.start.column = editor->InxToPos(item.line, item.start);
                    selection.end.line     = item.line;
                    selection.end.column   = editor->InxToPos(item.line, item.end);
                    editor->SetSelection(selection);
                }
            }
        }
    }
}

void CGrepView::OnExpandAll (UINT uCode)
{
    CWaitCursor wait;
    LockWindowUpdate();

    HTREEITEM hItem = GetRootItem();
    while (hItem)
    {
        Expand(hItem, uCode);
        hItem = GetNextItem(hItem, TVGN_NEXT);
    }

    hItem = GetSelectedItem();
    if (hItem
    && !EnsureVisible(hItem))
        SelectSetFirstVisible(hItem);

    UnlockWindowUpdate();
}

void CGrepView::OnCollapseAll ()
{
    OnExpandAll(TVE_COLLAPSE);
}

void CGrepView::OnExpandAll ()
{
    OnExpandAll(TVE_EXPAND);
}

void CGrepView::OnClear ()
{
    ClearContent();
}

void CGrepView::OnLButtonDblClk (UINT nFlags, CPoint point)
{
    UINT uFlags;
    HitTest(point, &uFlags);

    if (uFlags & TVHT_ONITEM && !(uFlags & TVHT_ONITEMICON))
	    OnOpenFile();
    else
	    CTreeCtrl::OnLButtonDblClk(nFlags, point);
}

LRESULT CGrepView::WindowProc (UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CTreeCtrl::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

BOOL CGrepView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {

        switch( pMsg->wParam ) 
        {
        case VK_RETURN:
            try { EXCEPTION_FRAME;

	            OnOpenFile();
            }
            _COMMON_DEFAULT_HANDLER_
            return TRUE;
        case VK_ESCAPE:
            if (CWnd* pWnd = AfxGetMainWnd())
                if (CMDIFrameWnd* pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, pWnd))
                    if (CMDIChildWnd* pMDIChild = pFrame->MDIGetActive())
                        pMDIChild->SetFocus();
            return TRUE;
        }
    }

    return CTreeCtrl::PreTranslateMessage(pMsg);
}

