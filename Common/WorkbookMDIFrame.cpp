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

#include "stdafx.h"
#include <Shlwapi.h>
#include "COMMON/ExceptionHelper.h"
#include "COMMON/WorkbookMDIFrame.h"
#include "COMMON/GUICommandDictionary.h"
#include "COMMON/AppUtilities.h"

using namespace Common;

// default key names
const char* cszProfileName = "Workspace\\";
const char* cszMainFrame = "MainFrame";
const char* cszWP = "WP";


void CWorkbookControlBar::OnContextMenu (CWnd*, CPoint point)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_OE_CBAR_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);

    if (m_pDockBar && m_pDockContext)
        pPopup->CheckMenuItem(ID_CBAR_DOCKING, 
                m_pDockContext->m_pBar->IsFloating() 
                    ? MF_BYCOMMAND|MF_UNCHECKED : MF_BYCOMMAND|MF_CHECKED);

    ASSERT(pPopup != NULL);
    ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CWorkbookControlBar::OnCbar_Docking ()
{
	if (m_pDockBar != NULL)
	{
		ASSERT(m_pDockContext != NULL);
		m_pDockContext->ToggleDocking();
	}
}

void CWorkbookControlBar::OnCbar_Hide ()
{
    ((CFrameWnd*)AfxGetMainWnd())->ShowControlBar(this, FALSE, TRUE);
    AfxGetMainWnd()->SetFocus();
}


BEGIN_MESSAGE_MAP(CWorkbookControlBar, baseCMyBar)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_CBAR_DOCKING, OnCbar_Docking)
    ON_COMMAND(ID_CBAR_HIDE, OnCbar_Hide)
END_MESSAGE_MAP()

// CWorkbookMDIFrame

IMPLEMENT_DYNCREATE(CWorkbookMDIFrame, CMDIFrameWnd)

CWorkbookMDIFrame::CWorkbookMDIFrame()
: m_defaultFileExtension("TXT")
{
    m_bCloseFileOnTabDblClick = FALSE;
    m_bSaveMainWinPosition = TRUE;
    m_bShowed = FALSE;
    IDC_MF_WORKBOOK_BAR  = (AFX_IDW_CONTROLBAR_LAST - 1);
    IDC_MF_FILEPANEL_BAR = (AFX_IDW_CONTROLBAR_LAST - 2);
    IDC_MF_FILEPANEL     = (AFX_IDW_CONTROLBAR_LAST - 3);
    m_cszProfileName = cszProfileName;
    m_cszMainFrame   = cszMainFrame;
    m_cszWP          = cszWP;

    m_hActiveChild = m_hLastChild = m_hSkipChild = NULL;
    m_bMDINextSeq  = FALSE;
}

CWorkbookMDIFrame::~CWorkbookMDIFrame()
{
}


BEGIN_MESSAGE_MAP(CWorkbookMDIFrame, CMDIFrameWnd)
	ON_NOTIFY(TCN_SELCHANGE, IDC_OE_WORKBOOK_TAB, OnChangeWorkbookTab)
	ON_NOTIFY(NM_DBLCLK, IDC_OE_WORKBOOK_TAB, OnDblClickOnWorkbookTab)
	ON_WM_SYSCOMMAND()
    ON_WM_CLOSE()
    ON_WM_SHOWWINDOW()
	ON_COMMAND(ID_VIEW_WORKBOOK, OnViewWorkbook)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WORKBOOK, OnUpdateViewWorkbook)
    ON_COMMAND(ID_VIEW_FILE_PANEL, OnViewFilePanel)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FILE_PANEL, OnUpdateViewFilePanel)
    ON_COMMAND(ID_FILE_SYNC_LOCATION, OnViewFilePanelSync)
    ON_UPDATE_COMMAND_UI(ID_FILE_SYNC_LOCATION, OnUpdateViewFilePanelSync)
	// toolbar "tooltip" notification
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
    ON_COMMAND(ID_WINDOW_LAST, OnLastWindow)
END_MESSAGE_MAP()


int CWorkbookMDIFrame::DoCreate (BOOL loadBarState)
{
#ifdef _SCB_REPLACE_MINIFRAME
    m_pFloatingFrameClass = RUNTIME_CLASS(CSCBMiniDockFrameWnd);
#endif //_SCB_REPLACE_MINIFRAME

    if (!m_wndFilePanelBar.Create("File panel", this, IDC_MF_FILEPANEL_BAR))
	{
		TRACE0("Failed to create instant bar\n");
		return -1;
	}
    m_wndFilePanelBar.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndFilePanelBar.SetSCBStyle(m_wndFilePanelBar.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndFilePanelBar.SetBarStyle(m_wndFilePanelBar.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndFilePanelBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndFilePanelBar, AFX_IDW_DOCKBAR_LEFT);
    
	if (!m_wndFilePanel.Create(WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN
        |TCS_BUTTONS|TCS_FLATBUTTONS|TCS_FOCUSNEVER, 
        CRect(0, 0, 0, 0), &m_wndFilePanelBar, IDC_MF_FILEPANEL)
    )
	{
		TRACE0("Failed to create instant bar child\n");
		return -1;
	}

    if (!m_wndWorkbookBar.Create(this, IDC_MF_WORKBOOK_BAR))
	{
		TRACE0("Failed to create Workbook bar\n");
		return -1;
	}

	// older versions of Windows* (NT 3.51 for instance)
	// fail with DEFAULT_GUI_FONT
	if (!m_font.CreateStockObject(DEFAULT_GUI_FONT))
		if (!m_font.CreatePointFont(80, "MS Sans Serif"))
			return -1;

    m_wndFilePanel.SetFont(&m_font);
    //m_wndWorkbookBar.SetImageList(&m_wndFilePanel.GetSysImageList());

    if (loadBarState)
        LoadBarState();

    return 0;
}

void CWorkbookMDIFrame::OnClose()
{
    WINDOWPLACEMENT wp;
	wp.length = sizeof wp;
    if (GetWindowPlacement(&wp))
    {
        if (wp.showCmd == SW_MINIMIZE)
        {
            wp.showCmd = SW_SHOWNORMAL;
            wp.flags   = 0;
        }
        AfxGetApp()->WriteProfileBinary(m_cszMainFrame, m_cszWP, (LPBYTE)&wp, sizeof(wp));
    }

    SaveBarState(m_cszProfileName);
    CSizingControlBar::GlobalSaveState(this, m_cszProfileName);

    CMDIFrameWnd::OnClose();
}

void CWorkbookMDIFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
    if (m_bSaveMainWinPosition && !m_bShowed)
    {
        m_bShowed = TRUE;

        BYTE *pWP;
        UINT size;

        if (AfxGetApp()->GetProfileBinary(m_cszMainFrame, m_cszWP, &pWP, &size)
        && size == sizeof(WINDOWPLACEMENT))
        {
	        SetWindowPlacement((WINDOWPLACEMENT*)pWP);
	        delete []pWP;
            return;
        }
    }

    CMDIFrameWnd::OnShowWindow(bShow, nStatus);
}

BOOL CWorkbookMDIFrame::VerifyBarState ()
{
    CDockState state;
    state.LoadState(m_cszProfileName);

    for (int i = 0; i < state.m_arrBarInfo.GetSize(); i++)
    {
        CControlBarInfo* pInfo = (CControlBarInfo*)state.m_arrBarInfo[i];
        ASSERT(pInfo != NULL);
        int nDockedCount = pInfo->m_arrBarID.GetSize();
        if (nDockedCount > 0)
        {
            // dockbar
            for (int j = 0; j < nDockedCount; j++)
            {
                UINT nID = (UINT) pInfo->m_arrBarID[j];
                if (nID == 0) continue; // row separator
                if (nID > 0xFFFF)
                    nID &= 0xFFFF; // placeholder - get the ID
                if (GetControlBar(nID) == NULL)
                    return FALSE;
            }
        }
        
        if (!pInfo->m_bFloating) // floating dockbars can be created later
            if (GetControlBar(pInfo->m_nBarID) == NULL)
                return FALSE; // invalid bar ID
    }

    return TRUE;
}

BOOL CWorkbookMDIFrame::LoadBarState ()
{
    if (VerifyBarState())
    {
        CMDIFrameWnd::LoadBarState(m_cszProfileName);
        CSizingControlBar::GlobalLoadState(this, m_cszProfileName);
        ShowControlBar(&m_wndFilePanelBar, m_wndFilePanelBar.IsVisible(), FALSE);
        m_wndFilePanel.OnShowControl(m_wndFilePanelBar.IsVisible());
        return TRUE;
    }
    return FALSE;
}

void CWorkbookMDIFrame::OnCreateChild (CMDIChildWnd* pWnd)
{
    static CString strTitle;
    pWnd->GetWindowText(strTitle);

    {
        TCITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = TCIF_TEXT|TCIF_PARAM;
        item.pszText = (LPSTR)(LPCSTR)strTitle;
        item.lParam  = (LPARAM)pWnd;
        m_wndWorkbookBar.AppendItem(item);
    }

    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask    = LVIF_TEXT|LVIF_PARAM;
        item.pszText = (LPSTR)(LPCSTR)strTitle;
        item.lParam  = (LPARAM)pWnd;
        m_wndFilePanel.OpenFiles_Append(item);
    }
}

void CWorkbookMDIFrame::OnDestroyChild (CMDIChildWnd* pWnd)
{
    m_wndWorkbookBar.RemoveItemByParam((LPARAM)pWnd);
    m_wndFilePanel.OpenFiles_RemoveByParam((LPARAM)pWnd);

    // 16.03.2003 bug fix, a woorkbook tab toltip shows a wrong path if any left tab has been closed
    CWnd* pMDIClient = GetWindow(GW_CHILD);
    CWnd* pMDIChild = (pMDIClient) ? pMDIClient->GetWindow(GW_CHILD) : 0;

    while (pMDIChild)
    {
        if (pMDIChild->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
        {
            CString title;
            pMDIChild->GetWindowText(title);

            if (pMDIChild != pWnd)
                OnRenameChild((CMDIChildWnd*)pMDIChild, title);
        }

        pMDIChild = pMDIChild->GetNextWindow();
    }
}

/*
  implementation is quite paranoic but that is because i don't want any surprises calling the method for background backup
*/
void CWorkbookMDIFrame::GetOrderedChildren (std::vector<CMDIChildWnd*>& orderedChildren) const 
{
    orderedChildren.clear();

    std::vector<LPARAM> rawOrder;
    m_wndWorkbookBar.GetAllTabParams(rawOrder);

    std::vector<CMDIChildWnd*> children;
    CWnd* pMDIClient = GetWindow(GW_CHILD);
    CWnd* pMDIChild = (pMDIClient) ? pMDIClient->GetWindow(GW_CHILD) : 0;
    while (pMDIChild)
    {
        if (pMDIChild->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
            children.push_back((CMDIChildWnd*)pMDIChild);
        pMDIChild = pMDIChild->GetNextWindow();
    }

    for (std::vector<LPARAM>::const_iterator it = rawOrder.begin(); it != rawOrder.end(); ++it)
        if (std::find(children.begin(), children.end(), (CMDIChildWnd*)*it) != children.end())
            orderedChildren.push_back((CMDIChildWnd*)*it);
}

void CWorkbookMDIFrame::OnActivateChild (CMDIChildWnd* pWnd)
{
    m_hLastChild = m_hActiveChild;
    m_hActiveChild = pWnd ? pWnd->m_hWnd : NULL;
    m_wndWorkbookBar.ActivateItemByParam((LPARAM)pWnd);
    m_wndFilePanel.OpenFiles_ActivateByParam((LPARAM)pWnd);
}

void CWorkbookMDIFrame::OnRenameChild (CMDIChildWnd* pWnd, LPCTSTR szTitle)
{
    LPARAM selected = m_wndFilePanel.OpenFiles_GetCurSelParam();
    int iImage = GetImageByDocument(pWnd->GetActiveDocument());

    {
        TCITEM item;
        item.mask = TCIF_TEXT;
        item.pszText = (LPTSTR)szTitle;
        //if (iImage != -1) 
        //{
            item.iImage = iImage;
            item.mask |= TCIF_IMAGE;
        //}
        CString path = pWnd->GetActiveDocument()->GetPathName();
        if (path.IsEmpty()) path = "<Path not specified>";
        m_wndWorkbookBar.UpdateItemByParam((LPARAM)pWnd, item, path);
    }

    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask    = LVIF_TEXT|LVIF_PARAM;
        item.pszText = (LPTSTR)szTitle;
        item.lParam  = (LPARAM)pWnd;
        //if (iImage != -1) 
        //{
            item.iImage = iImage;
            item.mask |= LVIF_IMAGE;
        //}
        m_wndFilePanel.OpenFiles_UpdateByParam((LPARAM)pWnd, item);
    }

    // 2017-12-01 bug fix, restoring selection after item being renamed 
    if (selected)
        m_wndFilePanel.OpenFiles_ActivateByParam(selected);
}

int CWorkbookMDIFrame::GetImageByDocument (const CDocument* pDoc)
{
#pragma message("\tSmall improvement: default extension should be taken from setting")
    if (pDoc)
    {
        CString path = pDoc->GetPathName();

        if (path.IsEmpty()) path   = "*." + m_defaultFileExtension;
        
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


void CWorkbookMDIFrame::OnChangeWorkbookTab (NMHDR*, LRESULT* pResult)
{
    LPARAM param = m_wndWorkbookBar.GetCurSelParam();
    
    if (param)
    {
        CMDIChildWnd* child = (CMDIChildWnd*)param;
        _ASSERT(child->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));
        ActivateChild(child);
    }

    *pResult = 0;
}


void CWorkbookMDIFrame::OnDblClickOnWorkbookTab (NMHDR*, LRESULT* pResult)
{
    if (m_bCloseFileOnTabDblClick)
        PostMessage(WM_COMMAND, ID_FILE_CLOSE);   // bug fix, when Double-Click to close file with changes, file seems closed, but filename remain in workbook

    *pResult = 1;
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

void CWorkbookMDIFrame::OnViewWorkbook ()
{
    ShowControlBar(&m_wndWorkbookBar, !m_wndWorkbookBar.IsVisible(), TRUE);
}


void CWorkbookMDIFrame::OnUpdateViewWorkbook (CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndWorkbookBar.IsVisible());
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
    bool focus = has_focus(m_wndFilePanelBar);

    ShowControlBar(&m_wndFilePanelBar, !m_wndFilePanelBar.IsVisible(), TRUE);
    m_wndFilePanel.OnShowControl(m_wndFilePanelBar.IsVisible());

    if (focus && !m_wndFilePanelBar.IsVisible())
        SetFocus();
}

void CWorkbookMDIFrame::OnUpdateViewFilePanel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndFilePanelBar.IsVisible());
}


void CWorkbookMDIFrame::OnViewFilePanelSync ()
{
    LPARAM param = m_wndWorkbookBar.GetCurSelParam();
    
    if (param)
    {
        CMDIChildWnd* child = (CMDIChildWnd*)param;
        _ASSERT(child->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));

        CDocument* pDocum = child->GetActiveDocument();
        if (pDocum)
        {
            CString path = pDocum->GetPathName();
            if (!path.IsEmpty())
            {
                if (!m_wndFilePanelBar.IsVisible())
                {
                    ShowControlBar(&m_wndFilePanelBar, TRUE, TRUE);
                    m_wndFilePanel.OnShowControl(TRUE);
                }
                m_wndFilePanel.SetCurPath(path);
            }
        }
    }
}

void CWorkbookMDIFrame::OnUpdateViewFilePanelSync (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!m_wndWorkbookBar.IsEpmty() ? TRUE : FALSE);
}

void CWorkbookMDIFrame::DockControlBarLeftOf (CControlBar* Bar, CControlBar* LeftOf)
{
	UINT n;
	DWORD dw;
	CRect rect;

	// get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
	RecalcLayout();
	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(1,1);
	dw=LeftOf->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP)            ? AFX_IDW_DOCKBAR_TOP    : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0)   ? AFX_IDW_DOCKBAR_LEFT   : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0)  ? AFX_IDW_DOCKBAR_RIGHT  : n;

	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line.  By calculating a rectangle, we in effect
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar(Bar,n,&rect);
}

LRESULT CWorkbookMDIFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CMDIFrameWnd::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

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

BOOL CWorkbookMDIFrame::PreTranslateMessage (MSG* pMsg)
{

    if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_CONTROL)
    {
        m_bMDINextSeq = FALSE;
        m_hSkipChild = NULL;
    }

    return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

