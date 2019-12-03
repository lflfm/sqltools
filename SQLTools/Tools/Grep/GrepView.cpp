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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UM_ENDPROCESS WM_USER

IMPLEMENT_DYNCREATE(CGrepView, CTreeCtrl)

CGrepView::CGrepView()
: m_nCounter(0),
m_pThread(0),
m_evRun(FALSE, TRUE),
m_evAbort(FALSE, TRUE)
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

int CGrepView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

    static CFont font;

    // older versions of Windows* (NT 3.51 for instance)
	// fail with DEFAULT_GUI_FONT
    if (!(HFONT)font)
    	if (!font.CreateStockObject(DEFAULT_GUI_FONT))
    		if (!font.CreatePointFont(80, "MS Sans Serif")) return -1;

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

    if (!IsRunning ())
        menu.EnableMenuItem(ID_GREP_BREAK, MF_BYCOMMAND|MF_GRAYED);

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
    BOOL bAbort = IsAbort();

    m_pThread = 0;
    m_evRun.ResetEvent();
    m_evAbort.ResetEvent();

    CString strTitle;

    if (m_nCounter)
        strTitle.Format("Found %d occurrence(s)", m_nCounter);

    AddEntry(m_nCounter ? strTitle : "No matches found", TRUE, TRUE);

    if (bAbort)
        AddEntry("Canceled", TRUE, TRUE);

    Invalidate();
    UpdateWindow();
}

void CGrepView::ClearContent ()
{
    CWaitCursor wait;
    m_nCounter = 0;
    m_strLastFileName.Empty();
    LockWindowUpdate();
    DeleteAllItems();
    UnlockWindowUpdate();
}

#pragma warning (push)
#pragma warning (disable : 4706)
void CGrepView::AddEntry (const char* szEntry, BOOL bExpanded, BOOL bInfoOnly)
{
    ASSERT(szEntry && (isalpha(szEntry[0]) || szEntry[0] == '\\'));

    if (WaitForSingleObject(m_evAbort, 0) == WAIT_OBJECT_0) return;

    TV_INSERTSTRUCT	TVStruct;
    TVStruct.hParent      = TVI_ROOT;
    TVStruct.hInsertAfter = TVI_LAST;
    TVStruct.item.mask    = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE
                        | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    TVStruct.item.state     =
    TVStruct.item.stateMask = bExpanded ? TVIS_EXPANDED : 0;

    if (bInfoOnly)
    {
        TVStruct.item.pszText   = (char*)szEntry;
        TVStruct.item.mask     |= TVIF_PARAM;
        TVStruct.item.cChildren = 0;
        TVStruct.item.iImage    = TVStruct.item.iSelectedImage = 1;
        TVStruct.item.lParam    = 1;
        InsertItem(&TVStruct);
        m_hLastItem = NULL;
    }
    else
    {
        const char* pszFirstColon, * pszSecondColon;
        if ((pszFirstColon = strchr(szEntry, ':'))
        && (pszFirstColon = strchr(pszFirstColon + 1, ':'))
        && (pszSecondColon = strchr(pszFirstColon + 1,  ':')))
        {
            CString strFileName(szEntry, pszFirstColon - szEntry);

            if (m_strLastFileName != strFileName)
            {
                TVStruct.item.pszText   = (char*)(const char*)strFileName;
                TVStruct.item.cChildren = 1;
                TVStruct.item.iImage    = TVStruct.item.iSelectedImage = 0;
                m_hLastItem = InsertItem(&TVStruct);
                m_strLastFileName = strFileName;
            }

            TVStruct.hParent      = m_hLastItem;
            TVStruct.item.mask    = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            TVStruct.item.iImage  = TVStruct.item.iSelectedImage = 2;
            TVStruct.item.pszText = (char*)pszFirstColon + 1;
            InsertItem(&TVStruct);

            m_nCounter++;
        }

  }
}
#pragma warning (pop)

void CGrepView::OnOpenFile ()
{
    HTREEITEM hItem = GetSelectedItem();

    if (hItem && !GetItemData(hItem))
    {
        HTREEITEM hParent = GetParentItem(hItem);

        int nLine = 0;
        CString strFile;
        if (hParent != TVGN_ROOT)
        {
            strFile = GetItemText(hParent);
            CString strBuffer = GetItemText(hItem);
            char* pszBuffer = strBuffer.LockBuffer();
            char* pszLineEnd = strchr(pszBuffer, ':');
            if (pszLineEnd)
            {
                *pszLineEnd = 0;
                nLine = atoi(pszBuffer);
                ASSERT(nLine);
            }
        }
        else
            strFile = GetItemText(hItem);

        CDocument* pDocument = AfxGetApp()->OpenDocumentFile(strFile);

        if (pDocument
        && pDocument->IsKindOf(RUNTIME_CLASS(CPLSWorksheetDoc)))
            ((CPLSWorksheetDoc*)pDocument)->GoTo(nLine-1);
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

void CGrepView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    UINT uFlags;
    HitTest(point, &uFlags);

    if (uFlags & TVHT_ONITEM && !(uFlags & TVHT_ONITEMICON))
	    OnOpenFile();
    else
	    CTreeCtrl::OnLButtonDblClk(nFlags, point);
}
