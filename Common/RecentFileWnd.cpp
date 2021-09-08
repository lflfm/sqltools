#include "stdafx.h"
#include "Resource.h"
#include "RecentFileWnd.h"
#include  "FileExplorerWnd.h"
#include "AppUtilities.h"
#include "MyUtf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const int ID_FPW_HISTORY = 100;

/////////////////////////////////////////////////////////////////////////////
// CFileView

RecentFileWnd::RecentFileWnd()
{
    m_Tooltips           = TRUE;
    m_PreviewInTooltips  = TRUE;
    m_PreviewLines       = 10;
    m_pFileExplorerWnd   = 0;
}

RecentFileWnd::~RecentFileWnd()
{
}

void RecentFileWnd::DoOpenFile (LPCWSTR path, unsigned int item)
{
    if (path != 0 && wcslen(path) > 0)
    {
        if (::PathFileExists(path))
            AfxGetApp()->OpenDocumentFile(path);
        else if (AfxMessageBox((L"The file \"" + CString(path) + L"\" does not exist.\n\nWould you like to remove it from the history?")
            , MB_ICONEXCLAMATION|MB_YESNO) == IDYES)
        {
            if (item != -1)
                m_recentFilesListCtrl.RemoveEntry(item);
            else
                m_recentFilesListCtrl.RemoveEntry(path);
        }
    }
}

void RecentFileWnd::SetTooltips (BOOL tooltips)
{
    m_Tooltips = tooltips;

    if (m_Tooltips)
    {
        if (m_recentFilesListCtrl.m_hWnd)
            m_recentFilesListCtrl.ModifyStyle(0, TVS_NOTOOLTIPS);
    }
    else
    {
        if (m_recentFilesListCtrl.m_hWnd)
            m_recentFilesListCtrl.ModifyStyle(TVS_NOTOOLTIPS, 0);
    }
}

void RecentFileWnd::SetSysImageList (CImageList* pImageList)
{
    m_recentFilesListCtrl.SetImageList(pImageList, LVSIL_SMALL);
}

BEGIN_MESSAGE_MAP(RecentFileWnd, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CONTEXTMENU()
    ON_WM_PAINT()
    ON_WM_SETFOCUS()
    ON_NOTIFY(NM_DBLCLK, ID_FPW_HISTORY, OnRecentFilesListCtrl_DblClick)
    ON_NOTIFY(NM_RCLICK, ID_FPW_HISTORY, OnRecentFilesListCtrl_RClick)
    ON_NOTIFY(LVN_BEGINDRAG, ID_FPW_HISTORY, OnRecentFilesListCtrl_OnBeginDrag)
    ON_NOTIFY(UDM_TOOLTIP_DISPLAY, NULL, OnNotifyDisplayTooltip)
    ON_WM_SHOWWINDOW()
    ON_COMMAND(ID_FPW_HISTORY_OPEN, &RecentFileWnd::OnOpenDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int RecentFileWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_recentFilesListCtrl.Create(
        WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
        CRect(0, 0, 0, 0), this, ID_FPW_HISTORY))
    {
            TRACE0("Failed to create open history list\n");
            return -1;
    }

    m_recentFilesListCtrl.SetExtendedStyle(m_recentFilesListCtrl.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
    m_recentFilesListCtrl.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);

    m_tooltip.Create(this, FALSE);
    m_tooltip.SetBehaviour(PPTOOLTIP_DISABLE_AUTOPOP|PPTOOLTIP_MULTIPLE_SHOW);
    m_tooltip.SetNotify(m_hWnd);
    m_tooltip.AddTool(&m_recentFilesListCtrl);
    m_tooltip.SetDirection(PPTOOLTIP_TOPEDGE_LEFT);

    SetTooltips(m_Tooltips);

    return 0;
}

void RecentFileWnd::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);

    CRect rectClient;
    GetClientRect(rectClient);

    m_recentFilesListCtrl.SetWindowPos(NULL, rectClient.left + 1, rectClient.top + 1, rectClient.Width() - 2, rectClient.Height() - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void RecentFileWnd::OnPaint()
{
    CPaintDC dc(this);

    CRect rectTree;
    m_recentFilesListCtrl.GetWindowRect(rectTree);
    ScreenToClient(rectTree);

    rectTree.InflateRect(1, 1);
    dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void RecentFileWnd::OnSetFocus(CWnd* pOldWnd)
{
    CDockablePane::OnSetFocus(pOldWnd);

    m_recentFilesListCtrl.SetFocus();
}

void RecentFileWnd::OnNotifyDisplayTooltip (NMHDR * pNMHDR, LRESULT * result)
{
    *result = 0;
    NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

    if (m_recentFilesListCtrl.m_hWnd == pNotify->hwndTool)
        NotifyDisplayTooltip(pNMHDR);
}

void RecentFileWnd::OnRecentFilesListCtrl_DblClick (NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
    if (POSITION pos = m_recentFilesListCtrl.GetFirstSelectedItemPosition())
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = m_recentFilesListCtrl.GetNextSelectedItem(pos);

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        std::wstring path = m_recentFilesListCtrl.GetPath(item.lParam);

        DoOpenFile(path.c_str(), item.lParam);
    }
}

void RecentFileWnd::OnRecentFilesListCtrl_RClick (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    if (pItem && pItem->iItem != -1)
    {
        *pResult = 1;
        CPoint point(pItem->ptAction);
        m_recentFilesListCtrl.ClientToScreen(&point);

        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = pItem->iItem;

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        CString path = m_recentFilesListCtrl.GetPath(item.lParam).c_str();

        CMenu menu;
        menu.CreatePopupMenu();

        menu.AppendMenu(MF_STRING, ID_FPW_HISTORY_OPEN, L"&Open File\tDblClick");
        menu.SetDefaultItem(ID_FPW_HISTORY_OPEN);
        menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);
        menu.AppendMenu(MF_STRING, ID_FPW_HISTORY_REMOVE, L"&Remove");
        menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);
        if (m_pFileExplorerWnd)
            menu.AppendMenu(MF_STRING, ID_FILE_SYNC_LOCATION, L"Sho&w File Location");
        menu.AppendMenu(MF_STRING, ID_FILE_COPY_LOCATION, L"Copy File Locat&ion");

        int choice = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        switch (choice)
        {
        case ID_FPW_HISTORY_OPEN:
            DoOpenFile(path, item.lParam);
            break;
        case ID_FPW_HISTORY_REMOVE:
            m_recentFilesListCtrl.RemoveEntry(item.lParam);
            break;
        case ID_FILE_SYNC_LOCATION:
            if (m_pFileExplorerWnd)
            {
                m_pFileExplorerWnd->ShowPane(TRUE, FALSE, TRUE);
                m_pFileExplorerWnd->SetCurPath(path); 
            }
            break;
        case ID_FILE_COPY_LOCATION:
            Common::AppCopyTextToClipboard(path);
            break;
        default:
            ;
        }
    }
}

void RecentFileWnd::OnRecentFilesListCtrl_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;

    if (pItem && pItem->iItem != -1)
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = pItem->iItem;

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        CString path = m_recentFilesListCtrl.GetPath(item.lParam).c_str();

        if (!path.IsEmpty())
        {
            UINT uBuffSize = sizeof(DROPFILES) + (path.GetLength() + 1 + 1) * sizeof(TCHAR);

            if (HGLOBAL hgDrop = GlobalAlloc(GHND|GMEM_SHARE, uBuffSize))
            {
                if (DROPFILES*  pDrop = (DROPFILES*)GlobalLock(hgDrop))
                {
                    memset(pDrop, 0, uBuffSize);
                    pDrop->fWide = sizeof(TCHAR) == 2;
                    pDrop->pFiles = sizeof(DROPFILES);
                    memcpy((LPBYTE(pDrop) + sizeof(DROPFILES)), (LPCWSTR)path, path.GetLength() * sizeof(TCHAR));

                    GlobalUnlock(hgDrop);

                    COleDataSource data;
                    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                    data.CacheGlobalData ( CF_HDROP, hgDrop, &etc );

                    /*DROPEFFECT dwEffect  =*/ data.DoDragDrop( DROPEFFECT_COPY );
                }
            }
        }
    }

    *pResult = 0;
}

void RecentFileWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
    if (!m_recentFilesListCtrl.IsInitialized())
        m_recentFilesListCtrl.Initialize();

    CDockablePane::OnShowWindow(bShow, nStatus);
}


BOOL RecentFileWnd::PreTranslateMessage(MSG* pMsg)
{
    if (m_Tooltips)
        m_tooltip.RelayEvent(pMsg);

    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_ESCAPE) // FRM
        {
            if (CWnd* pWnd = AfxGetMainWnd())
                if (CMDIFrameWnd* pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, pWnd))
                    if (CMDIChildWnd* pMDIChild = pFrame->MDIGetActive())
                        pMDIChild->SetFocus();
            return TRUE;
        }
        else if (pMsg->wParam == VK_RETURN)
        {
            PostMessage(WM_COMMAND, ID_FPW_HISTORY_OPEN);
            return TRUE;
        }
    }

    return CDockablePane::PreTranslateMessage(pMsg);
}

LRESULT RecentFileWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDockablePane::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

void RecentFileWnd::OnOpenDocument()
{
    int position = m_recentFilesListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (position != -1) 
    {
        int index = m_recentFilesListCtrl.GetItemData(position);
        CString path = m_recentFilesListCtrl.GetPath(index).c_str();
        DoOpenFile(path, index);
    }
}
