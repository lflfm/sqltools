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

// 25.12.2004 (ak) R1091240 - Find Object/Object Viewer improvements
// 09.01.2005 (ak) replaced CComboBoxEx with CComboBox because of unpredictable CComboBoxEx
// 20.03.2005 (ak) bug fix, an exception on "Find Object", if some name component is null
// 20.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

#include "stdafx.h"
#include "SQLTools.h"
#include "COMMON\AppGlobal.h"
#include <COMMON/DlgDataExt.h>
#include "COMMON\StrHelpers.h"
#include "DbBrowser\ObjectViewerWnd.h"
#include "SQLWorksheet/PlsSqlParser.h"
#include "ObjectTree_Builder.h"
#include "COMMON/GUICommandDictionary.h"
#include "SQLWorksheet\SQLWorksheetDoc.h" // for LoadDDL

#include "ObjectTree_Builder.h"
using namespace ObjectTree;

#include "FindObjectsTask.h"
using namespace ServerBackgroundThread;

////////////////////////////////////////////////////////////////////////////////
// CObjectViewerWnd
CObjectViewerWnd::CObjectViewerWnd()
: CDialog(IDD),
m_accelTable(NULL),
m_selChanged(false),
m_pInputBoxContent(0)
{
    SetHelpID(IDD);
    GetSQLToolsSettings().AddSubscriber(this);

    const SQLToolsSettings settings = GetSQLToolsSettings();
    m_treeViewer.SetClassicTreeLook(settings.GetObjectViewerClassicTreeLook());
    m_infoTip = settings.GetObjectViewerInfotips();
}

CObjectViewerWnd::~CObjectViewerWnd()
{
    try { EXCEPTION_FRAME;

	    if (m_accelTable)
		    DestroyAcceleratorTable(m_accelTable);

        m_pInputBoxContent.reset(0);
    }
    _DESTRUCTOR_HANDLER_
}

BEGIN_MESSAGE_MAP(CObjectViewerWnd, CDialog)
    ON_WM_SIZE()
    ON_CBN_SELCHANGE(IDC_OIT_INPUT, OnInput_SelChange)
    ON_CBN_CLOSEUP(IDC_OIT_INPUT, OnInput_CloseUp)
	ON_NOTIFY(UDM_TOOLTIP_DISPLAY, NULL, OnNotifyDisplayTooltip)
END_MESSAGE_MAP()

BOOL CObjectViewerWnd::Create (CWnd* pParentWnd)
{
    BOOL retVal = CDialog::Create(IDD, pParentWnd);

    if (retVal)
    {
        m_treeViewer.ModifyStyle(0, TVS_NOTOOLTIPS);
	    m_tooltip.Create(this, FALSE);
	    m_tooltip.SetBehaviour(PPTOOLTIP_DISABLE_AUTOPOP|PPTOOLTIP_MULTIPLE_SHOW);
	    m_tooltip.SetNotify(m_hWnd);
	    m_tooltip.AddTool(&m_treeViewer);
        m_tooltip.SetDirection(PPTOOLTIP_TOPEDGE_LEFT);
    }

    return retVal;
}

void CObjectViewerWnd::DoDataExchange (CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_CBString(pDX, IDC_OIT_INPUT, m_input);
    DDX_Control(pDX, IDC_OIT_INPUT, m_inputBox);
    DDX_Control(pDX, IDC_OIT_TREE, m_treeViewer);
}

void CObjectViewerWnd::ShowObject (const std::string& name)
{
    m_input = name;
    UpdateData(FALSE);
    OnOK();
}

void CObjectViewerWnd::ShowObject (const ObjectTree::ObjectDescriptor& obj, const vector<string>& drilldown)
{
    m_treeViewer.ShowObject(obj, drilldown);
}

BOOL CObjectViewerWnd::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_treeViewer.OnInit();
    //m_Images.Create(IDB_SQL_GENERAL_LIST, 16, 64, RGB(0,255,0));

    m_input = "%";
    setQuestionIcon();

    if (theApp.GetDatabaseOpen())
        EvOnConnect();
    else
        EvOnDisconnect();

	if (!m_accelTable)
	{
		CMenu menu;
        menu.CreatePopupMenu();

        menu.AppendMenu(MF_STRING, ID_SQL_OBJ_VIEWER, "dummy");
        menu.AppendMenu(MF_STRING, ID_VIEW_FILE_PANEL, "dummy");
        menu.AppendMenu(MF_STRING, ID_SQL_DB_SOURCE, "dummy");
        menu.AppendMenu(MF_STRING, ID_FILE_SHOW_GREP_OUTPUT, "dummy");

        //menu.AppendMenu(MF_STRING, ID_EDIT_COPY, "dummy");
		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(menu.m_hMenu);
	}

    return TRUE;  // return TRUE unless you set the focus to a control
}

void CObjectViewerWnd::setQuestionIcon ()
{
    m_inputBox.ResetContent();
    int inx = m_inputBox.AddString(m_input.c_str());
    m_inputBox.SetItemData(inx, (DWORD)-1);
    m_inputBox.SetCurSel(0);
}

    struct BackgroundTask_FindObjectsForViewer : FindObjectsTask
    {
        CObjectViewerWnd& m_viewerWnd;

        BackgroundTask_FindObjectsForViewer (CObjectViewerWnd& viewerWnd, const string& input)
            : FindObjectsTask(input, (CWnd*)&viewerWnd),
            m_viewerWnd(viewerWnd)
        {
            m_silent = true;
        }

        void ReturnInForeground ()
        {
             m_viewerWnd.m_treeViewer.ShowWaitCursor(false);

            if (!m_error.empty()) // too many objects
            {
                ::MessageBeep(MB_ICONSTOP);
                AfxMessageBox(m_error.c_str(), MB_OK|MB_ICONSTOP);
                m_viewerWnd.m_inputBox.SetFocus();
            }
            else
            {
                if (m_result.size() > 0)
                {
                    m_viewerWnd.m_inputBox.ResetContent();
                    std::vector<ObjectDescriptor>::const_iterator it = m_result.begin();
                    for (int i = 0; it != m_result.end(); ++it, ++i)
                    {
                        std::string object(it->owner + '.' + it->name);

                        int inx = m_viewerWnd.m_inputBox.AddString(object.c_str());
                        m_viewerWnd.m_inputBox.SetItemData(inx, i);
                    }

                    m_viewerWnd.m_pInputBoxContent.reset(new ObjectDescriptorArray(m_result));

                    if (m_result.size() == 1
                        || (
                            m_result.size() == 2
                            && m_result.at(0).owner == m_result.at(1).owner
                            && m_result.at(0).name == m_result.at(1).name
                            &&
                            (
                                (m_result.at(0).type == "PACKAGE" && m_result.at(1).type == "PACKAGE BODY")
                                || (m_result.at(0).type == "TYPE" && m_result.at(1).type == "TYPE BODY")
                            )
                        )
                    )
                    {
                        m_viewerWnd.m_inputBox.SetCurSel(0);
                        m_viewerWnd.OnInput_SelChange();
                    }
                    else
                    {
                        m_viewerWnd.m_inputBox.PostMessage(CB_SHOWDROPDOWN, TRUE);
                    }
                }
                else
                {
                    ::MessageBeep(MB_ICONSTOP);
                    Global::SetStatusText('<' + m_input + "> not found!");
                    m_viewerWnd.m_inputBox.SetFocus();
                    m_viewerWnd.setQuestionIcon();
                }
            }
        }

    };

void CObjectViewerWnd::OnOK ()
{
    try { EXCEPTION_FRAME;

	    // Check existence of an open DB connection.
	    // B#1191428 - % in disconnected 'Object viewer'
        if (!theApp.GetConnectOpen()) {
		    throw Common::AppException("Find object: not connected to database!");
	    }
	    if (!theApp.GetDatabaseOpen()) {
		    throw Common::AppException("Find object: database is not open!");
	    }

        if (UpdateData())
        {
			CWaitCursor wait;

            m_treeViewer.ShowWaitCursor(true);
			m_treeViewer.DeleteAllItems();
            m_pInputBoxContent.reset(0);

            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_FindObjectsForViewer(*this, m_input)));
        }
    }
    _COMMON_DEFAULT_HANDLER_
}

void CObjectViewerWnd::OnCancel ()
{
    AfxGetMainWnd()->SetFocus();
}

void CObjectViewerWnd::EvOnConnect ()
{
    if (theApp.GetDatabaseOpen())
    {
        CWnd* wndChild = GetWindow(GW_CHILD);
        while (wndChild)
        {
            wndChild->EnableWindow(TRUE);
            wndChild = wndChild->GetWindow(GW_HWNDNEXT);
        }
    }
    m_treeViewer.EvOnConnect();
}

void CObjectViewerWnd::EvOnDisconnect ()
{
    m_treeViewer.EvOnDisconnect();

    CWnd* wndChild = GetWindow(GW_CHILD);
    while (wndChild)
    {
        wndChild->EnableWindow(FALSE);
        wndChild = wndChild->GetWindow(GW_HWNDNEXT);
    }
}

void CObjectViewerWnd::OnSettingsChanged ()
{
    const SQLToolsSettings settings= GetSQLToolsSettings();

    m_treeViewer.SetClassicTreeLook(settings.GetObjectViewerClassicTreeLook());

    if (m_infoTip != settings.GetObjectViewerInfotips())
    {
        m_infoTip = settings.GetObjectViewerInfotips();

        if (m_infoTip)
        {
            m_treeViewer.ModifyStyle(0, TVS_NOTOOLTIPS);
	        m_tooltip.Create(this, FALSE);
	        m_tooltip.SetBehaviour(PPTOOLTIP_DISABLE_AUTOPOP|PPTOOLTIP_MULTIPLE_SHOW);
	        m_tooltip.SetNotify(m_hWnd);
	        m_tooltip.AddTool(&m_treeViewer);
            m_tooltip.SetDirection(PPTOOLTIP_TOPEDGE_LEFT);
        }
        else
        {
            m_treeViewer.ModifyStyle(TVS_NOTOOLTIPS, 0);
	        m_tooltip.DestroyWindow();
        }
    }

    m_treeViewer.Invalidate();
}

// CObjectViewerWnd message handlers
void CObjectViewerWnd::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && m_inputBox.m_hWnd
    && nType != SIZE_MAXHIDE && nType != SIZE_MINIMIZED)
    {
        CRect rect;
        GetClientRect(&rect);

        CRect comboRect;
        m_inputBox.GetWindowRect(&comboRect);
        int comboH = comboRect.Height() + 2;

        HDWP hdwp = ::BeginDeferWindowPos(10);
        ::DeferWindowPos(hdwp, m_inputBox, 0, rect.left, rect.top,
            rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_treeViewer, 0, rect.left,
            rect.top + comboH, rect.Width(), rect.Height() - comboH, SWP_NOZORDER);
        ::EndDeferWindowPos(hdwp);
    }
}

void CObjectViewerWnd::OnInput_CloseUp ()
{
    if (m_selChanged)
        OnInput_SelChange();
}

void CObjectViewerWnd::OnInput_SelChange ()
{
    try { EXCEPTION_FRAME;

        m_selChanged = true;

        if (m_inputBox.GetDroppedState())
            return;

        m_selChanged = false;

        int inx = m_inputBox.GetCurSel();
        if (inx != CB_ERR)
        {
            if (m_pInputBoxContent.get())
                m_treeViewer.ShowObject(m_pInputBoxContent.get()->at(inx));
        }
    }
    _COMMON_DEFAULT_HANDLER_
}



BOOL CObjectViewerWnd::PreTranslateMessage(MSG* pMsg)
{
	if (m_accelTable
    && TranslateAccelerator(AfxGetMainWnd()->m_hWnd, m_accelTable, pMsg))
        return TRUE;

    if (m_infoTip)
	    m_tooltip.RelayEvent(pMsg);

    return CDialog::PreTranslateMessage(pMsg);;
}

LRESULT CObjectViewerWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

void CObjectViewerWnd::OnNotifyDisplayTooltip (NMHDR * pNMHDR, LRESULT * result)
{
	*result = 0;
	NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

	if (NULL == pNotify->hwndTool)
	{
		//Order to update a tooltip for a current Tooltip Help
		//He has not a handle of the window
		//If you want change tooltip's parameter than make it's here
	}
	else if (m_treeViewer.m_hWnd == pNotify->hwndTool)
    {
        TVHITTESTINFO ti;
		ti.pt = *pNotify->pt;
        m_treeViewer.ScreenToClient(&ti.pt);

		m_treeViewer.HitTest(&ti);
		UINT nFlags = ti.flags;

		if (nFlags & TVHT_ONITEM)
		{
            pNotify->ti->sTooltip = m_treeViewer.GetTooltipText(ti.hItem).c_str();

            CRect rect, rcCtrl;
			m_treeViewer.GetItemRect(ti.hItem, rect, TRUE);
            m_treeViewer.GetWindowRect(&rcCtrl);
			CPoint pt = rcCtrl.TopLeft();
			rect.OffsetRect(pt);
			rect.OffsetRect(1, 1);
			pt = rect.TopLeft();
			pt.y = rect.bottom;
			*pNotify->pt = pt;
        }
    }
}
