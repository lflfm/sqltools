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
    11.10.2006 bug fix, invalid path in tooltip after tab reorganization 
*/

#include "stdafx.h"
#include "COMMON/WorkbookBar.h"
#include "COMMON/GUICommandDictionary.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorkbookBar dialog


void CWorkbookBar::DoDataExchange (CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWorkbookBar)
	DDX_Control(pDX, IDC_OE_WORKBOOK_TAB, m_wndTab);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWorkbookBar, CDialogBar)
	//{{AFX_MSG_MAP(CWorkbookBar)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
    ON_NOTIFY(NM_RCLICK, IDC_OE_WORKBOOK_TAB, OnTab_RClick)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_CBAR_HIDE, OnCbar_Hide)
    ON_NOTIFY_EX_RANGE(TTN_GETDISPINFO, 0, 0xFFFF, OnTab_TTNNeedText)
END_MESSAGE_MAP()


BOOL CWorkbookBar::Create (CWnd* pParentWnd, UINT uID)
{
    if (CDialogBar::Create(pParentWnd, IDD_OE_WORKBOOK_BAR, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, uID)) 
    {
        EnableDocking(CBRS_ALIGN_BOTTOM);
        return TRUE;
    }
    return FALSE;
}


void CWorkbookBar::AppendItem (TCITEM& item)
{
    VERIFY(m_wndTab.InsertItem(m_wndTab.GetItemCount(), &item) != -1);
}


void CWorkbookBar::UpdateItemByParam (LPARAM param, TCITEM& item, LPCSTR tooltipText)
{
    int nItem = FindTabByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(m_wndTab.SetItem(nItem, &item));
    
    SetToolTipsText(nItem, param, tooltipText);
}

void CWorkbookBar::RemoveItemByParam (LPARAM param)
{
    int nItem = FindTabByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(m_wndTab.DeleteItem(nItem));

    m_tooltips.erase(param);
}


void CWorkbookBar::ActivateItemByParam (LPARAM param)
{
    int nItem = FindTabByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        m_wndTab.SetCurSel(nItem);
}

void CWorkbookBar::GetAllTabParams (std::vector<LPARAM>& vec) const
{
    vec.clear();

    TCITEM tcItem;
    tcItem.mask = TCIF_PARAM;
    int nItems = m_wndTab.GetItemCount();

    for (int i(0); i < nItems; i++) 
    {
        VERIFY(m_wndTab.GetItem(i, &tcItem));
        vec.push_back(tcItem.lParam);
    }
}

int CWorkbookBar::FindTabByParam (LPARAM param) const
{
    TCITEM tcItem;
    tcItem.mask = TCIF_PARAM;
    int nItems = m_wndTab.GetItemCount();

    for (int i(0); i < nItems; i++) 
    {
        VERIFY(m_wndTab.GetItem(i, &tcItem));

        if (tcItem.lParam == param)
            return i;
    }

    return -1;
}


LPARAM CWorkbookBar::GetCurSelParam ()
{
    TCITEM tcItem;
    tcItem.mask = TCIF_PARAM;
    tcItem.lParam = 0;

    int nItem = m_wndTab.GetCurSel();
    ASSERT(nItem != -1);
    VERIFY(m_wndTab.GetItem(nItem, &tcItem));

    return tcItem.lParam;
}

void CWorkbookBar::SetToolTipsText (int nItem, LPARAM lparam, LPCSTR tooltipText)
{
    m_tooltips[lparam] = tooltipText;

    HWND hToolTip = TabCtrl_GetToolTips(m_wndTab.m_hWnd);

    if (hToolTip)
    {
        TOOLINFO info;
        memset(&info, 0, sizeof(0));
        info.cbSize     = sizeof(info);
        info.hwnd       = m_wndTab.m_hWnd;
        //info.lpszText   = const_cast<char*>(tooltipText);
        info.lpszText   = LPSTR_TEXTCALLBACK;
        info.uId        = nItem;
        info.lParam     = lparam;

        ::SendMessage(hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&info);
    }
}

BOOL CWorkbookBar::OnTab_TTNNeedText (UINT /*id*/, NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
    LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)pNMHDR;
    
    TCITEM item;
    memset(&item, 0, sizeof item);
    item.mask = TCIF_PARAM; 
    m_wndTab.GetItem(lpnmtdi->hdr.idFrom, &item);
    TooltipMap::const_iterator it = m_tooltips.find(item.lParam);
    lpnmtdi->lpszText = (it != m_tooltips.end()) ? (LPSTR)it->second.c_str() : 0;
    
    return (it != m_tooltips.end()) ? TRUE : FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWorkbookBar message handlers

LRESULT CWorkbookBar::OnInitDialog (WPARAM, LPARAM) 
{
    UpdateData(FALSE);

    CRect rect;
    m_wndTab.GetItemRect(-1, &rect);
    m_wndTab.SetItemSize(CSize(180, rect.Height()));
    m_wndTab.ModifyStyle(0, TCS_BUTTONS|TCS_FLATBUTTONS|TCS_FOCUSNEVER|TCS_TOOLTIPS, 0);
    m_wndTab.SetExtendedStyle(TCS_EX_FLATSEPARATORS, TCS_EX_FLATSEPARATORS);
    m_wndTab.EnableToolTips();

    return TRUE;
}


CSize CWorkbookBar::CalcFixedLayout (BOOL bStretch, BOOL bHorz)
{
    CSize size = CDialogBar::CalcFixedLayout(bStretch, bHorz);
    if (bHorz) 
    {
        CRect rect;
        GetParent()->GetClientRect(&rect);
        size.cx = rect.Width();
    }
    return size;
}

void CWorkbookBar::OnSize (UINT nType, int cx, int cy) 
{
    CDialogBar::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0 && m_wndTab.m_hWnd)
        m_wndTab.SetWindowPos(0, 0, 0, cx, cy - 2, SWP_NOMOVE|SWP_NOZORDER);
}

void CWorkbookBar::OnTab_RClick (NMHDR*, LRESULT* pResult)
{
    CPoint point;
    ::GetCursorPos(&point);
    
    m_wndTab.ScreenToClient(&point);
    m_wndTab.SendMessage(WM_LBUTTONDOWN, 0, MAKEWPARAM(point.x, point.y));
    m_wndTab.SendMessage(WM_LBUTTONUP, 0, MAKEWPARAM(point.x, point.y));

    ::GetCursorPos(&point);

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_OE_WORKBOOK_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);

    ASSERT(pPopup != NULL);
    ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());

    *pResult = 1;
}

void CWorkbookBar::OnContextMenu (CWnd*, CPoint point)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_OE_CBAR_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);

    pPopup->CheckMenuItem(ID_CBAR_DOCKING, MF_BYCOMMAND|MF_CHECKED);
    pPopup->EnableMenuItem(ID_CBAR_DOCKING, MF_BYCOMMAND|MF_GRAYED);

    ASSERT(pPopup != NULL);
    ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CWorkbookBar::OnCbar_Hide ()
{
    ((CFrameWnd*)AfxGetMainWnd())->ShowControlBar(this, FALSE, TRUE);
    AfxGetMainWnd()->SetFocus();
}

LRESULT CWorkbookBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialogBar::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}
