/* 
    Copyright (C) 2002 Aleksey Kochetov

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
    07.03.2003 improvement, shortcut/acceleration descriptions nave been included in toolbar tooltips 
    07.03.2003 bug fix, wrong icon for a new document
    16.03.2003 bug fix, a woorkbook tab toltip shows a wrong path if any left tab has been closed
    23.03.2003 improvement, added a new command - mdi last window - default shortcut is Ctrl+Tab
    20.06.2014 bug fix, when Double-Click to close file with changes, file seems closed, but filename remain in workbook
*/

// FRM: add context menu to the tabs
// FRM: add shortcut key to the docking control headers

#include "stdafx.h"
#include "resource.h"
#include <Shlwapi.h>
#include "ExceptionHelper.h"
#include "WorkbookMDIFrame.h"
#include "GUICommandDictionary.h"
#include "AppUtilities.h"
#include "COMMON/CustomShellContextMenu.h"

using namespace Common;

// default key names
LPCTSTR cszMainFrame = L"MainFrame";
LPCTSTR cszWP = L"WP";

IMPLEMENT_DYNCREATE(CWorkbookMDIFrame, CMDIFrameWndEx)

CWorkbookMDIFrame::CWorkbookMDIFrame()
: m_defaultFileExtension("TXT"),
m_ctxmenuFileProperties(true), m_ctxmenuTortoiseGit(true), m_ctxmenuTortoiseSvn(true)

{
    IDC_MF_FILEPANEL = (AFX_IDW_CONTROLBAR_LAST - 1);
    IDC_MF_HISTORY   = (AFX_IDW_CONTROLBAR_LAST - 2);

    if (AfxGetApp()->IsKindOf(RUNTIME_CLASS(CWinAppEx)))
        m_cszProfileName = ((CWinAppEx*)AfxGetApp())->GetRegistryBase();
    else
        m_cszProfileName = L"Workspace";

    m_cszMainFrame   = cszMainFrame;
    m_cszWP          = cszWP;

    m_hActiveChild = m_hLastChild = m_hSkipChild = NULL;
    m_bMDINextSeq  = FALSE;

    m_MDITabsCtrlTabSwitchesToPrevActive = TRUE;
}

CWorkbookMDIFrame::~CWorkbookMDIFrame()
{
}


BEGIN_MESSAGE_MAP(CWorkbookMDIFrame, CMDIFrameWndEx)
    ON_WM_SYSCOMMAND()
    ON_WM_CLOSE()
    ON_WM_SHOWWINDOW()
    ON_COMMAND(ID_VIEW_FILE_PANEL, OnViewFilePanel)
    ON_COMMAND(ID_VIEW_HISTORY, OnViewHistory)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FILE_PANEL, OnUpdateViewFilePanel)
    ON_UPDATE_COMMAND_UI(ID_VIEW_HISTORY, OnUpdateViewHistory)
    ON_COMMAND(ID_FILE_SYNC_LOCATION, OnViewFilePanelSync)
	ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
    // toolbar "tooltip" notification
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
    ON_COMMAND(ID_WINDOW_LAST, OnLastWindow)
    ON_COMMAND(ID_WINDOW_PREV, OnPrevWindow)
    ON_COMMAND(ID_WINDOW_NEXT, OnNextWindow)
END_MESSAGE_MAP()

static HICON GetShellIcon (int csidl)
{
    HICON hIcon = NULL;
    IMalloc * pShellMalloc = NULL;
    HRESULT hres = SHGetMalloc(&pShellMalloc);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST ppidl = NULL;
        hres = SHGetSpecialFolderLocation(NULL, csidl, &ppidl);
        if (SUCCEEDED(hres))
        {
            SHFILEINFO shFinfo;
            if (SHGetFileInfo((LPCWSTR)ppidl, 0, &shFinfo, sizeof(shFinfo), SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON))
            {
                hIcon = shFinfo.hIcon;
            }
            pShellMalloc->Free(ppidl);
        }

        pShellMalloc->Release();
    }
    return hIcon;
}

int CWorkbookMDIFrame::DoCreate ()
{
    if (!m_wndFilePanel.Create(L"File Explorer", this, CRect(0, 0, 200, 200), TRUE, IDC_MF_FILEPANEL, 
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI)
    )
    {
        TRACE0("Failed to create File View window\n");
        return FALSE; // failed to create
    }
    if (!m_wndRecentFile.Create(L"History", this, CRect(0, 0, 200, 200), TRUE, IDC_MF_HISTORY, 
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI)
    )
    {
        TRACE0("Failed to create File View window\n");
        return FALSE; // failed to create
    }
    
    m_wndFilePanel.SetIcon(GetShellIcon(CSIDL_PERSONAL/*CSIDL_DRIVES*/), FALSE);
    m_wndRecentFile.SetIcon(GetShellIcon(CSIDL_RECENT), FALSE);

    m_wndRecentFile.SetSysImageList(&m_wndFilePanel.GetSysImageList());
    m_wndRecentFile.SetFileExplorerWnd(&m_wndFilePanel);

    // older versions of Windows* (NT 3.51 for instance)
    // fail with DEFAULT_GUI_FONT
    if (!m_font.CreateStockObject(DEFAULT_GUI_FONT))
        if (!m_font.CreatePointFont(80, L"MS Sans Serif"))
            return -1;

    m_wndFilePanel.SetFont(&m_font);

    m_wndFilePanel.EnableDocking(CBRS_ALIGN_ANY);
    //m_wndFilePanel.SetMDITabbed(TRUE);
    DockPane(&m_wndFilePanel);

    m_wndRecentFile.EnableDocking(CBRS_ALIGN_ANY);
    //m_wndRecentFile.SetMDITabbed(TRUE);
    CDockablePane* pTabbedBar = NULL;
    m_wndRecentFile.AttachToTabWnd(&m_wndFilePanel, DM_SHOW, FALSE, &pTabbedBar);

    //LoadMDIState(L""); FRM

    return 0;
}

void CWorkbookMDIFrame::OnClose()
{
    CMDIFrameWndEx::OnClose();
}

void CWorkbookMDIFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CMDIFrameWndEx::OnShowWindow(bShow, nStatus);
}

// FRM
void CWorkbookMDIFrame::GetOrderedChildren (std::vector<CMDIChildWnd*>& orderedChildren) 
{
    orderedChildren.clear();

    const CObList& list = GetMDITabGroups();
    for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
    {
        const CObject* obj = list.GetNext(pos);
        CMFCTabCtrl* pTabWnd = DYNAMIC_DOWNCAST(CMFCTabCtrl, obj);
        int nTabsNum = pTabWnd->GetTabsNum();
        for (int i = 0; i < nTabsNum; i++)
        {
            CMDIChildWndEx* pNextWnd = DYNAMIC_DOWNCAST(CMDIChildWndEx, pTabWnd->GetTabWnd(i));
            if (pNextWnd)
                orderedChildren.push_back(pNextWnd);
        }
    }
}

void CWorkbookMDIFrame::OnActivateChild (CMDIChildWnd* pWnd)
{
    m_hLastChild = m_hActiveChild;
    m_hActiveChild = pWnd ? pWnd->m_hWnd : NULL;
}

int CWorkbookMDIFrame::GetImageByDocument (const CDocument* pDoc)
{
#pragma message("\tSmall improvement: default extension should be taken from setting")
    if (pDoc)
    {
        CString path = pDoc->GetPathName();

        if (path.IsEmpty()) 
            path = L"*." + m_defaultFileExtension;
        
        BOOL newDoc = !PathFileExists(path);

        SHFILEINFO shFinfo;
        if (SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &shFinfo, sizeof(shFinfo), 
                (newDoc ? SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES : SHGFI_ICON|SHGFI_SMALLICON)))
        {
            DestroyIcon(shFinfo.hIcon);
            return shFinfo.iIcon;
        }
    }
    return -1;
}


void CWorkbookMDIFrame::ActivateChild (CMDIChildWnd* child)
{
    BOOL bMaximized;
    MDIGetActive(&bMaximized);

    if (!bMaximized  && child->IsIconic())
        child->MDIRestore();
    else
        child->MDIActivate();
}


/*
    This handler remove Workbook Tab resizing bug on maximize/restore
*/
void CWorkbookMDIFrame::OnSysCommand (UINT nID, LONG lParam)
{
    CFrameWnd::OnSysCommand(nID, lParam);

    UINT nItemID = (nID & 0xFFF0);

    // 16.03.2003 bug fix, violetion in debug mode when the application is closing
    if ((nItemID == SC_MAXIMIZE || nItemID == SC_RESTORE) && !m_bHelpMode)
        RecalcLayout(TRUE/*bNotify*/);
}

//static
bool CWorkbookMDIFrame::has_focus (HWND hWndControl)
{
    HWND hWnd = ::GetFocus();

    do 
    {
        if (hWnd == hWndControl)
            return true;

        hWnd = ::GetParent(hWnd);
    }
    while (hWnd);

    return false;
}


void CWorkbookMDIFrame::OnViewFilePanel()
{
    bool focus = has_focus(m_wndFilePanel);

    if (!m_wndFilePanel.IsWindowVisible()) // in case if it is hidden behind another docking control
        m_wndFilePanel.ShowPane(TRUE, FALSE, TRUE);
    else 
        m_wndFilePanel.ShowPane(!m_wndFilePanel.IsVisible(), FALSE, TRUE);

    if (focus && !m_wndFilePanel.IsVisible())
        SetFocus();
}

void CWorkbookMDIFrame::OnViewHistory()
{
    bool focus = has_focus(m_wndRecentFile);

    if (!m_wndRecentFile.IsWindowVisible()) // in case if it is hidden behind another docking control
        m_wndRecentFile.ShowPane(TRUE, FALSE, TRUE);
    else 
        m_wndRecentFile.ShowPane(!m_wndFilePanel.IsVisible(), FALSE, TRUE);

    if (focus && !m_wndRecentFile.IsVisible())
        SetFocus();
}

void CWorkbookMDIFrame::OnUpdateViewFilePanel(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_wndFilePanel.IsVisible());
}

void CWorkbookMDIFrame::OnUpdateViewHistory(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_wndRecentFile.IsVisible());
}

void CWorkbookMDIFrame::OnViewFilePanelSync ()
{
    CFrameWnd* pFrame = MDIGetActive();

    if (pFrame)
    {
        CDocument* pDoc = pFrame->GetActiveDocument();

        if (pDoc)
        {
            CString path = pDoc->GetPathName();

            if (!path.IsEmpty())
            {
                m_wndFilePanel.ShowPane(TRUE, FALSE, TRUE);
                m_wndFilePanel.SetCurPath(path);
                m_wndFilePanel.SetFocus();
            }
        }
    }
}

LRESULT CWorkbookMDIFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CMDIFrameWndEx::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

LRESULT CWorkbookMDIFrame::OnGetTabToolTip (WPARAM /*wp*/, LPARAM lp)
{
	CMFCTabToolTipInfo* pInfo = (CMFCTabToolTipInfo*) lp;
	ASSERT(pInfo != NULL);
	if (!pInfo)
	{
		return 0;
	}

	ASSERT_VALID(pInfo->m_pTabWnd);

	if (!pInfo->m_pTabWnd->IsMDITab())
	{
		return 0;
	}

	CFrameWnd* pFrame = DYNAMIC_DOWNCAST(CFrameWnd, pInfo->m_pTabWnd->GetTabWnd(pInfo->m_nTabIndex));
	if (pFrame == NULL)
	{
		return 0;
	}

	CDocument* pDoc = pFrame->GetActiveDocument();
	if (pDoc == NULL)
	{
		return 0;
	}

    CString path = pDoc->GetPathName();
	pInfo->m_strText = !path.IsEmpty() ? path : L"<Path not specified>";

	return 0;
}

#define COUNT_OF(array)         (sizeof(array)/sizeof(array[0]))
#define _AfxGetDlgCtrlID(hWnd)  ((UINT)(WORD)::GetDlgCtrlID(hWnd))

BOOL CWorkbookMDIFrame::OnToolTipText (UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    TCHAR szFullText[256];
    CString strTipText;
    UINT_PTR nID = pNMHDR->idFrom;
    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool
        nID = _AfxGetDlgCtrlID((HWND)nID);
    }

    if (nID != 0) // will be zero on a separator
    {
        // don't handle the message if no string resource found
        if (AfxLoadString((UINT)nID, szFullText) == 0)
            return FALSE;

        // this is the command id, not the button index
        AfxExtractSubString(strTipText, szFullText, 1, '\n');

        std::string accLabel;
        if (Common::GUICommandDictionary::GetCommandAccelLabel(static_cast<Common::Command>(nID), accLabel))
        {
            strTipText += " (";
            strTipText += accLabel.c_str();
            strTipText +=  ")";
        }
    }

#ifndef _UNICODE
    if (pNMHDR->code == TTN_NEEDTEXTA)
        lstrcpyn(pTTTA->szText, strTipText, COUNT_OF(pTTTA->szText));
    else
        _mbstowcsz(pTTTW->szText, strTipText, COUNT_OF(pTTTW->szText));
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
        _wcstombsz(pTTTA->szText, strTipText, COUNT_OF(pTTTA->szText));
    else
        lstrcpyn(pTTTW->szText, strTipText, COUNT_OF(pTTTW->szText));
#endif
    *pResult = 0;

    // bring the tooltip window above other popup windows
    ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
        SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

    return TRUE;    // message was handled
}

void CWorkbookMDIFrame::OnLastWindow ()
{
    if (m_MDITabsCtrlTabSwitchesToPrevActive)
    {
        if (!m_bMDINextSeq && m_hLastChild && ::IsWindow(m_hLastChild) )
        {
            CMDIChildWnd* active = MDIGetActive();
            m_hSkipChild = active ? active->m_hWnd : NULL;

            MDIActivate(FromHandle(m_hLastChild));
            m_bMDINextSeq = TRUE;
        }
        else
        {
            MDINext();

            CMDIChildWnd* active = MDIGetActive();
        
            if (active && active->m_hWnd == m_hSkipChild)
                MDINext();

            m_hSkipChild = NULL;
        }
    }
    else
        MDINext();

    EnsureActiveTabVisible();
}

void CWorkbookMDIFrame::OnPrevWindow ()
{
    std::vector<CMDIChildWnd*> views;
    GetOrderedChildren(views);

    CMDIChildWnd* active = MDIGetActive();

    auto it = std::find(views.begin(), views.end(), active);
    if (it != views.end())
    {
        if (it != views.begin())
            ActivateChild(*--it);
        else
            ActivateChild(*views.rbegin());
    }

    EnsureActiveTabVisible();
}

void CWorkbookMDIFrame::OnNextWindow ()
{
    std::vector<CMDIChildWnd*> views;
    GetOrderedChildren(views);

    CMDIChildWnd* active = MDIGetActive();

    auto it = std::find(views.begin(), views.end(), active);
    if (it != views.end())
    {
        if (++it != views.end())
            ActivateChild(*it);
        else
            ActivateChild(*views.begin());
    }

    EnsureActiveTabVisible();
}

void CWorkbookMDIFrame::EnsureActiveTabVisible ()
{
    int index;
    if (CMDIChildWnd* active = MDIGetActive())
        if (CMFCTabCtrl* mfcTabs = m_wndClientArea.FindTabWndByChild(active->m_hWnd, index))
            mfcTabs->EnsureVisible(index);
}

BOOL CWorkbookMDIFrame::PreTranslateMessage (MSG* pMsg)
{

    if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_CONTROL)
    {
        m_bMDINextSeq = FALSE;
        m_hSkipChild = NULL;
    }

    return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}

BOOL CWorkbookMDIFrame::OnShowMDITabContextMenu (CPoint point, DWORD /*dwAllowedItems*/, BOOL bTabDrop)
{
    if (!bTabDrop)
    {
        CMDIChildWnd* active = MDIGetActive();
        if (0xFF00 & GetKeyState(VK_CONTROL) && active)
        {
            int index;
            CMFCTabCtrl* mfcTabs = m_wndClientArea.FindTabWndByChild(active->m_hWnd, index);
            if (mfcTabs)
                mfcTabs->OnShowTabDocumentsMenu(point);
            return TRUE;
        }
        else
        {
            CMenu menu;
            VERIFY(menu.LoadMenu(IDR_OE_WORKBOOK_POPUP));
            CMenu* pPopup = menu.GetSubMenu(0);

            ASSERT(pPopup != NULL);
            Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

            if ((m_ctxmenuFileProperties || m_ctxmenuTortoiseGit || m_ctxmenuTortoiseSvn) && 0xFF00 & GetKeyState(VK_SHIFT))
            {
                if (CFrameWnd* pFrame = MDIGetActive())
                {
                    if (CDocument* pDoc = pFrame->GetActiveDocument())
                    {
                        CString path = pDoc->GetPathName();

                        if (::PathFileExists(path))
                        {
                            if (UINT idCommand = CustomShellContextMenu(this, point, path, pPopup, m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn).ShowContextMenu())
                                SendMessage(WM_COMMAND, idCommand, 0);
                            return TRUE;
                        }
                    }
                }
            }

            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
            return TRUE;
        }
    }
    return FALSE;
}
