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

/*
    13.03.2003 bug fix, if you are not connected and display the Object List the close button is dimmed.
    26.10.2003 bug fix, multiple occurences of PUBLIC in "Schema" list ("Object List" window)
    09.11.2003 bug fix, Cancel window does not work properly on Load DDL
    21.01.2004 bug fix, "Object List" error handling chanded to avoid paranoic bug reporting
    10.02.2004 bug fix, "Object List" a small issue with cancellation of ddl loading
    22.03.2004 bug fix, CreateAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
    17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
    04.10.2006 B#XXXXXXX - cannot get DDL for db link if its name is longer than 30 chars
*/

#include "stdafx.h"
#include "COMMON/AppGlobal.h"
#include "SQLTools.h"
#include "DbSourceWnd.h"
#include "OCI8/BCursor.h"
#include "DbBrowser\DBBrowserTableList.h"
#include "DbBrowser\DBBrowserCodeList.h"
#include "DbBrowser\DBBrowserTypeList.h"
#include "DbBrowser\DBBrowserConstraintList.h"
#include "DbBrowser\DBBrowserIndexList.h"
#include "DbBrowser\DBBrowserSequenceList.h"
#include "DbBrowser\DBBrowserViewList.h"
#include "DbBrowser\DBBrowserTriggerList.h"
#include "DbBrowser\DBBrowserSynonymList.h"
#include "DbBrowser\DBBrowserDbLinkList.h"
#include "DbBrowser\DBBrowserGranteeList.h"
#include "DbBrowser\DBBrowserClusterList.h"
#include "DbBrowser\DBBrowserRecyclebinList.h"
#include "DbBrowser\DBBrowserInvalidObjectList.h"
#include "COMMON/GUICommandDictionary.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OraMetaDict;
using namespace ServerBackgroundThread;

#define IDC_LIST1 111

const int RECENT_SCHEMAS_SIZE = 6;

/////////////////////////////////////////////////////////////////////////////
// CDbSourceWnd

CDbSourceWnd::CDbSourceWnd ()
: CDialog(IDD),
  m_nItems(-1),
  m_nSelItems(-1),
  m_accelTable(NULL),
  m_schemaListStatus(SLS_EMPTY)
{
    //{{AFX_DATA_INIT(CDbSourceWnd)
    //}}AFX_DATA_INIT
    m_AllSchemas = AfxGetApp()->GetProfileInt(L"Code", L"AllSchemas",  TRUE);
    m_bValid     = AfxGetApp()->GetProfileInt(L"Code", L"Valid",  TRUE);
    m_bInvalid   = AfxGetApp()->GetProfileInt(L"Code", L"Invalid",TRUE);
    m_nViewAs    = AfxGetApp()->GetProfileInt(L"Code", L"ViewAs", 1);
    m_bShowTabTitles = AfxGetApp()->GetProfileInt(L"Code", L"ShowTabTitles",  TRUE);
}

CDbSourceWnd::~CDbSourceWnd ()
{
    try { EXCEPTION_FRAME;

        for (unsigned int i(0); i < m_wndTabLists.size(); i++)
            delete m_wndTabLists[i];

	    if (m_accelTable)
		    DestroyAcceleratorTable(m_accelTable);
    }
    _DESTRUCTOR_HANDLER_
}

DbBrowserList* CDbSourceWnd::GetCurSel () const
{
    DbBrowserList* wndListCtrl = 0;

    int nTab = m_wndTab.GetCurSel();

    if (nTab != -1)
        wndListCtrl = m_wndTabLists[nTab];

    _ASSERTE(wndListCtrl);

    return wndListCtrl;
}

BOOL CDbSourceWnd::IsVisible () const
{
    const CWnd* pWnd = GetParent();
    if (!pWnd) pWnd = this;
    return pWnd->IsWindowVisible();
}

BOOL CDbSourceWnd::Create (CWnd* pParentWnd)
{
    return CDialog::Create(IDD, pParentWnd);
}

void CDbSourceWnd::DoDataExchange (CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DS_SCHEMA,   m_wndSchemaList);
    DDX_Control(pDX, IDC_DS_STATUS,   m_wndStatus);
    //{{AFX_DATA_MAP(CDbSourceWnd)
    DDX_Control(pDX, IDC_DS_TAB, m_wndTab);
    DDX_Check(pDX, IDC_DS_VALID, m_bValid);
    DDX_Check(pDX, IDC_DS_INVALID, m_bInvalid);
    DDX_CBString(pDX, IDC_DS_SCHEMA, m_strSchema);
    DDX_Radio(pDX, IDC_DS_AS_LIST, m_nViewAs);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDbSourceWnd, CDialog)
    ON_WM_SIZE()
    ON_CBN_DROPDOWN(IDC_DS_SCHEMA, OnDropDownSchema)
    ON_CBN_SETFOCUS(IDC_DS_SCHEMA, OnSetFocusSchema)
    ON_CBN_SELCHANGE(IDC_DS_SCHEMA, OnSchemaChanged)
    ON_WM_DESTROY()
    ON_NOTIFY(TCN_SELCHANGE, IDC_DS_TAB, OnSelChangeTab)
    ON_MESSAGE_VOID(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)

    ON_BN_CLICKED(IDC_DS_AS_LIST, OnShowAsList)
    ON_BN_CLICKED(IDC_DS_AS_REPORT, OnShowAsReport)
    ON_BN_CLICKED(IDC_DS_INVALID, OnToolbarFilterChanged)
    ON_BN_CLICKED(IDC_DS_VALID, OnToolbarFilterChanged)
    ON_BN_CLICKED(IDC_DS_SETTINGS, OnSettings)
    ON_BN_CLICKED(IDC_DS_FILTER, OnFilter)
    ON_BN_CLICKED(IDC_DS_REFRESH, OnRefresh)

    ON_COMMAND(ID_DS_REFRESH, OnRefresh)
    ON_COMMAND(ID_DS_SETTINGS, OnSettings)
    ON_COMMAND(ID_DS_TAB_TITLES, OnTabTitles)

	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDbSourceWnd message handlers


BOOL CDbSourceWnd::OnInitDialog()
{
    CDialog::OnInitDialog();

    UpdateData(FALSE);

    if (m_Images.m_hImageList == NULL)
        m_Images.Create(IDB_DB_OBJ, 16, 64, RGB(0,255,0));

    m_wndTab.SetImageList(&m_Images);

    m_wndTabLists.push_back(new DBBrowserTableList());
    m_wndTabLists.push_back(new DBBrowserPkConstraintList());
    m_wndTabLists.push_back(new DBBrowserUkConstraintList());
    m_wndTabLists.push_back(new DBBrowserFkConstraintList());
    m_wndTabLists.push_back(new DBBrowserChkConstraintList());
    m_wndTabLists.push_back(new DBBrowserIndexList());
    m_wndTabLists.push_back(new DBBrowserTriggerList());
    m_wndTabLists.push_back(new DBBrowserProcedureList());
    m_wndTabLists.push_back(new DBBrowserFunctionList());
    m_wndTabLists.push_back(new DBBrowserPackageList());
    m_wndTabLists.push_back(new DBBrowserPackageBodyList());
    m_wndTabLists.push_back(new DBBrowserTypeList());
    m_wndTabLists.push_back(new DBBrowserTypeBodyList());
    m_wndTabLists.push_back(new DBBrowserJavaList());
    m_wndTabLists.push_back(new DBBrowserSequenceList());
    m_wndTabLists.push_back(new DBBrowserViewList());
    m_wndTabLists.push_back(new DBBrowserSynonymList());
    m_wndTabLists.push_back(new DBBrowserDbLinkList());
    m_wndTabLists.push_back(new DBBrowserClusterList());
    m_wndTabLists.push_back(new DBBrowserGranteeList());
    m_wndTabLists.push_back(new DBBrowserRecyclebinList());
    m_wndTabLists.push_back(new DBBrowserInvalidObjectList());

    for (unsigned int i(0); i < m_wndTabLists.size(); i++)
    {
        m_wndTabLists[i]->Create(LVS_REPORT|WS_CHILD|WS_BORDER|WS_GROUP|WS_TABSTOP|LVS_SHOWSELALWAYS,
                                                         CRect(0,0,0,0), this, IDC_LIST1);
        m_wndTabLists[i]->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
        m_wndTabLists[i]->SetOwner(this);

        m_wndTab.InsertItem(m_wndTab.GetItemCount(), 
            m_bShowTabTitles ? Common::wstr(m_wndTabLists[i]->GetTitle()).c_str() : L"", 
            m_wndTabLists[i]->GetImageIndex());
    }

    CWinApp* pApp = AfxGetApp();
    ::SendMessage(::GetDlgItem(*this, IDC_DS_REFRESH), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_REFRESH));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_AS_REPORT));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_AS_LIST));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_VALID), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_VALID));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_INVALID), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_INVALID));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_FILTER), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_FILTER));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_SETTINGS), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_SETTINGS));

    if (theApp.GetConnectOpen()) EvOnConnect();
    else                         EvOnDisconnect();

    int nTab = AfxGetApp()->GetProfileInt(L"Code", L"nTab", 0);
    _ASSERTE((unsigned int)nTab < m_wndTabLists.size());
    m_wndTab.SetCurSel(nTab);
    OnSwitchTab(nTab);

    // want to exlude these two controls from tab navigation
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_REPORT), WS_TABSTOP, 0, 0);
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_LIST), WS_TABSTOP, 0, 0);

    m_wndStatus.ModifyStyle(0, SS_OWNERDRAW);
    m_wndStatus.AlignLeft();
    //ModifyStyleEx(0, WS_EX_COMPOSITED); // for smoother animation in status

    EnableToolTips();

	if (!m_accelTable)
	{
		CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, ID_SQL_DB_SOURCE, L"dummy");
		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(menu.m_hMenu);
	}

    return TRUE;
}

void CDbSourceWnd::OnDestroy()
{
    AfxGetApp()->WriteProfileInt(L"Code", L"AllSchemas",  m_AllSchemas);

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        AfxGetApp()->WriteProfileInt(L"Code", L"nTab",    nTab);

    AfxGetApp()->WriteProfileInt(L"Code", L"Valid",   m_bValid);
    AfxGetApp()->WriteProfileInt(L"Code", L"Invalid", m_bInvalid);
    AfxGetApp()->WriteProfileInt(L"Code", L"ViewAs",  m_nViewAs);

    CDialog::OnDestroy();
}

void CDbSourceWnd::EvOnConnect ()
{
    if (theApp.GetDatabaseOpen())
    {
        CWnd* wndChild = GetWindow(GW_CHILD);
        while (wndChild)
        {
            wndChild->EnableWindow(TRUE);
            wndChild = wndChild->GetWindow(GW_HWNDNEXT);
        }

        InitSchemaList();
    }
}

void CDbSourceWnd::EvOnDisconnect ()
{
    for (unsigned int i(0); i < m_wndTabLists.size(); i++)
    {
        m_wndTabLists[i]->DeleteAllItems();
        m_wndTabLists[i]->ClearData();
    }

    CWnd* wndChild = GetWindow(GW_CHILD);
    while (wndChild)
    {
        wndChild->EnableWindow(FALSE);
        wndChild = wndChild->GetWindow(GW_HWNDNEXT);
    }

	m_recentSchemas.clear();
    m_wndSchemaList.ResetContent();
    m_schemaListStatus = SLS_EMPTY;
}

void CDbSourceWnd::OnSize (UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && nType != SIZE_MAXHIDE
    && nType != SIZE_MINIMIZED)
    {
        if (CWnd* pWnd = GetDlgItem(IDC_DS_STATUS))
        {
            CRect rect;
            pWnd->GetWindowRect(rect);
            ScreenToClient(&rect);
            pWnd->SetWindowPos(0, rect.left, cy - rect.Height() - 2, cx - rect.left, rect.Height(), SWP_NOZORDER);
            pWnd->GetWindowRect(rect);
            ScreenToClient(&rect);

            CRect rcTab;
            m_wndTab.GetWindowRect(rcTab);
            ScreenToClient(&rcTab);

            m_wndTab.SetWindowPos(0, 0, 0, cx, rect.top - rcTab.top - 4, SWP_NOMOVE|SWP_NOZORDER);

            m_wndTab.GetWindowRect(&rect);
            m_wndTab.AdjustRect(FALSE, &rect);
            ScreenToClient(rect);
            rect.InflateRect(1, 0);

            for (unsigned int i(0); i < m_wndTabLists.size(); i++)
                m_wndTabLists[i]->SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(), 0);

        }
    }
}

void CDbSourceWnd::OnSwitchTab (unsigned int nTab)
{
    _ASSERTE((unsigned int)nTab < m_wndTabLists.size());

    if (nTab < m_wndTabLists.size())
    {
        for (unsigned int i(0); i < m_wndTabLists.size(); i++)
            m_wndTabLists[i]->ShowWindow(i == nTab ? SW_SHOW : SW_HIDE);

        if (m_nViewAs == 0)
            m_wndTabLists[nTab]->ModifyStyle(LVS_REPORT, LVS_LIST, 0);
        else
            m_wndTabLists[nTab]->ModifyStyle(LVS_LIST, LVS_REPORT, 0);

        if (theApp.GetConnectOpen())
        {
            CWaitCursor wait;

            m_wndTabLists[nTab]->Refresh();
            m_wndTabLists[nTab]->ApplyQuickFilter(m_bValid ? true : false, m_bInvalid ? true : false);

            BOOL enable = m_wndTabLists[nTab]->IsQuickFilterSupported() ? TRUE : FALSE;
            ::EnableWindow(::GetDlgItem(*this, IDC_DS_VALID), enable);
            ::EnableWindow(::GetDlgItem(*this, IDC_DS_INVALID), enable);

            if (m_wndTab.m_hWnd != ::GetFocus())
                m_wndTabLists[nTab]->SetFocus();
        }

    }
}

void CDbSourceWnd::OnSelChangeTab (NMHDR*, LRESULT* pResult)
{
    OnSwitchTab(m_wndTab.GetCurSel());
    *pResult = 0;
}

void CDbSourceWnd::InitSchemaList ()
{
    m_wndSchemaList.ResetContent();

    try { EXCEPTION_FRAME;

        m_defSchema = theApp.GetCurrentSchema();
        m_strSchema = m_defSchema.c_str();
		addToRecentSchemas(Common::wstr(m_defSchema).c_str());
        m_wndSchemaList.SetCurSel(m_wndSchemaList.AddString(m_strSchema));
        m_schemaListStatus = SLS_CURRENT_ONLY;

        SetSchemaForObjectLists();
    }
    _DEFAULT_HANDLER_
}

    LPSCSTR cszSchemasSttm =
        "select username from sys.all_users a_u"
		" where username <> :schema"
        " and exists (select null from all_objects a_s where a_s.owner = a_u.username)";

    LPSCSTR cszAllSchemasSttm =
        "select username from sys.all_users a_u"
		" where username <> :schema";

	struct BackgroundTask_PopulateSchemaList : Task
    {
        CDbSourceWnd& m_dbSourceWnd;
        string m_defSchema;
        bool m_allSchemas;
        vector<string> m_schemas;

        BackgroundTask_PopulateSchemaList (CDbSourceWnd& dbSourceWnd)
            : Task("Populate SchemaList", (CWnd*)&dbSourceWnd),
            m_dbSourceWnd(dbSourceWnd),
            m_defSchema(dbSourceWnd.m_defSchema),
            m_allSchemas(dbSourceWnd.m_AllSchemas)
        {
            m_dbSourceWnd.m_schemaListStatus = CDbSourceWnd::SLS_LOADING;
            m_dbSourceWnd.m_wndSchemaList.SetBusy(true);
        }

        void DoInBackground (OciConnect& connect)
        {
            ActivePrimeExecutionOnOff onOff;

            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            OciCursor cursor(connect);
            cursor.Prepare(m_allSchemas ? cszAllSchemasSttm : cszSchemasSttm);
            cursor.Bind(":schema", m_defSchema.c_str());
            cursor.Execute();
            while (cursor.Fetch())
                m_schemas.push_back(cursor.ToString(0));
            //Sleep(1000);
        }

        void ReturnInForeground ()
        {
            if (m_dbSourceWnd.m_schemaListStatus == CDbSourceWnd::SLS_LOADING)
            {
                BOOL dropped = m_dbSourceWnd.m_wndSchemaList.GetDroppedState();
                if (dropped) 
                    m_dbSourceWnd.m_wndSchemaList.ShowDropDown(FALSE);

                vector<string>::const_iterator it = m_schemas.begin();
            
                for (; it != m_schemas.end(); ++it)
                    m_dbSourceWnd.m_wndSchemaList.AddString(Common::wstr(*it).c_str());

                // TODO: reimplement PUBLIC support! Forgot how :(
                m_dbSourceWnd.m_wndSchemaList.AddString(L"PUBLIC");

                if (dropped) 
                    m_dbSourceWnd.m_wndSchemaList.ShowDropDown(TRUE);

                m_dbSourceWnd.m_schemaListStatus = CDbSourceWnd::SLS_LOADED;
            }
            m_dbSourceWnd.m_wndSchemaList.SetBusy(false);
        }

        void ReturnErrorInForeground () 
        {
            m_dbSourceWnd.m_schemaListStatus = CDbSourceWnd::SLS_CURRENT_ONLY;
            m_dbSourceWnd.m_wndSchemaList.SetBusy(false);
        }
    };


void CDbSourceWnd::PopulateSchemaList ()
{
    if (m_schemaListStatus == SLS_CURRENT_ONLY)
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_PopulateSchemaList(*this)));
}

void CDbSourceWnd::OnDropDownSchema()
{
    PopulateSchemaList();
}

// need to populate the list before user presses any key on it
void CDbSourceWnd::OnSetFocusSchema ()
{
    PopulateSchemaList();
}

void CDbSourceWnd::SetSchemaForObjectLists ()
{
    for (unsigned int i(0); i < m_wndTabLists.size(); i++)
        m_wndTabLists[i]->SetSchema(Common::str(m_strSchema).c_str());
}

void CDbSourceWnd::addToRecentSchemas (const wchar_t* schema)
{
    //std::wstring schema = Common::wstr(_schema);

	auto it = std::find(m_recentSchemas.begin(), m_recentSchemas.end(), schema);

	if (it != m_recentSchemas.end())
		m_recentSchemas.erase(it);

	m_recentSchemas.push_back(schema);

	while (m_recentSchemas.size() > RECENT_SCHEMAS_SIZE)
		m_recentSchemas.erase(m_recentSchemas.begin());
}

void CDbSourceWnd::OnSchemaChanged ()
{
    CWaitCursor wait;

    UpdateData();
	addToRecentSchemas(m_strSchema);
    SetSchemaForObjectLists();

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->Refresh();
}

void CDbSourceWnd::OnToolbarFilterChanged ()
{
    UpdateData();

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->ApplyQuickFilter(m_bValid ? true : false, m_bInvalid ? true : false);
}

void CDbSourceWnd::OnRefresh ()
{
    CWaitCursor wait;

    if ((0xFF00 & GetKeyState(VK_CONTROL)))
        for (unsigned int i(0); i < m_wndTabLists.size(); i++)
            m_wndTabLists[i]->SetDirty(true);

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->Refresh(true);
}

void CDbSourceWnd::OnShowAsList ()
{
    m_nViewAs = 0;
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->ModifyStyle(LVS_REPORT, LVS_LIST, 0);

    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETCHECK, BST_UNCHECKED, 0L);
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETCHECK, BST_CHECKED, 0L);

    // want to exlude these two controls from tab navigation
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_REPORT), WS_TABSTOP, 0, 0);
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_LIST), WS_TABSTOP, 0, 0);
}

void CDbSourceWnd::OnShowAsReport ()
{
    m_nViewAs = 1;
    for (unsigned int i(0); i < m_wndTabLists.size(); i++)
        m_wndTabLists[i]->ModifyStyle(LVS_LIST, LVS_REPORT, 0);

    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETCHECK, BST_CHECKED, 0L);
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETCHECK, BST_UNCHECKED, 0L);

    // want to exlude these two controls from tab navigation
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_REPORT), WS_TABSTOP, 0, 0);
    ModifyStyle(::GetDlgItem(*this, IDC_DS_AS_LIST), WS_TABSTOP, 0, 0);
}

void CDbSourceWnd::OnIdleUpdateCmdUI ()
{
    //TRACE("CDbSourceWnd::OnIdleUpdateCmdUI\n");

    if (!IsVisible())
        return;

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
    {
        int nItems    = m_wndTabLists[nTab]->GetItemCount();
        int nSelItems = m_wndTabLists[nTab]->GetSelectedCount();

        if (m_nItems != nItems
        || m_nSelItems != nSelItems)
        {
            wchar_t buff[80]; 
            const int buff_size = sizeof(buff)/sizeof(buff[0]);
            buff[buff_size-1] = 0;
            m_nItems    = nItems;
            m_nSelItems = nSelItems;
            _snwprintf(buff, buff_size-1, L"Selected %d of %d", nSelItems, nItems);
            m_wndStatus.SetText(buff, RGB(0, 0, 0));
        }

        // TODO: find another way to refresh the current list
        if (m_wndTabLists[nTab]->IsDirty() 
            && theApp.GetConnectOpen() && !theApp.GetConnectSlow())
        {
            try {
                CWaitCursor wait;
                m_wndTabLists[nTab]->Refresh();
            }
            catch (...)
            {
                m_wndTabLists[nTab]->SetDirty(false); // otherwise infinite loop happens
            }
            m_wndTabLists[nTab]->ApplyQuickFilter(m_bValid ? true : false, m_bInvalid ? true : false);

            BOOL enable = m_wndTabLists[nTab]->IsQuickFilterSupported() ? TRUE : FALSE;
            ::EnableWindow(::GetDlgItem(*this, IDC_DS_VALID), enable);
            ::EnableWindow(::GetDlgItem(*this, IDC_DS_INVALID), enable);
        }

        bool busy = BkgdRequestQueue::Get().HasAnyByOwner(this);
        m_wndTabLists[nTab]->SetBusy(busy);
        m_wndStatus.StartAnimation(busy);
    }
}

void CDbSourceWnd::OnSettings()
{
    SQLToolsSettings settings = GetSQLToolsSettings();
    ShowDDLPreferences(settings, false);
}

void CDbSourceWnd::OnFilter ()
{
    m_nViewAs = 0;
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->OnFilter();
}

LRESULT CDbSourceWnd::OnHelpHitTest(WPARAM, LPARAM lParam)
{
    ASSERT_VALID(this);

    int nID = OnToolHitTest((DWORD)lParam, NULL);
    if (nID != -1)
        return HID_BASE_COMMAND+nID;

    return HID_BASE_CONTROL + IDD;
}

// See CControlBar::WindowProc
LRESULT CDbSourceWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    try { EXCEPTION_FRAME;

        if (nMsg == WM_NOTIFY)
        {
            // send these messages to the owner if not handled
            if (OnWndMsg(nMsg, wParam, lParam, &lResult))
                return lResult;
            else
            {
                // try owner next
                lResult = GetOwner()->SendMessage(nMsg, wParam, lParam);

                // special case for TTN_NEEDTEXTA and TTN_NEEDTEXTW
                if(nMsg == WM_NOTIFY)
                {
                    NMHDR* pNMHDR = (NMHDR*)lParam;
                    if (pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
                        TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
                        if ((pNMHDR->code == TTN_NEEDTEXTA && (!pTTTA->lpszText || !*pTTTA->lpszText)) ||
                            (pNMHDR->code == TTN_NEEDTEXTW && (!pTTTW->lpszText || !*pTTTW->lpszText)))
                        {
                            // not handled by owner, so let bar itself handle it
                            lResult = CDialog::WindowProc(nMsg, wParam, lParam);
                        }
                    }
                }
                return lResult;
            }
        }
        // otherwise, just handle in default way
        lResult = CDialog::WindowProc(nMsg, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return lResult;
}

void CDbSourceWnd::OnOK ()
{
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->SendMessage(WM_COMMAND, m_wndTabLists[nTab]->GetDefaultContextCommand(false));
}

void CDbSourceWnd::OnCancel ()
{
    AfxGetMainWnd()->SetFocus();
}

void CDbSourceWnd::OnTabTitles ()
{
    m_bShowTabTitles = !m_bShowTabTitles;
    AfxGetApp()->WriteProfileInt(L"Code", L"ShowTabTitles",  m_bShowTabTitles);

    TCITEM item;
    item.mask = TCIF_TEXT;
    for (unsigned i = 0; i < m_wndTabLists.size(); i++)
    {
        std::wstring buff = Common::wstr(m_bShowTabTitles ? m_wndTabLists[i]->GetTitle() : "");
        item.pszText = const_cast<wchar_t*>(buff.c_str());
        m_wndTab.SetItem(i, &item);
    }

    CRect rect;
    m_wndTab.GetWindowRect(&rect);
    m_wndTab.AdjustRect(FALSE, &rect);
    ScreenToClient(rect);
    rect.InflateRect(1, 0);

    for (i = 0; i < m_wndTabLists.size(); i++)
        m_wndTabLists[i]->SetWindowPos(0, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);
}

#define COUNT_OF(array)         (sizeof(array)/sizeof(array[0]))
#define _AfxGetDlgCtrlID(hWnd)  ((UINT)(WORD)::GetDlgCtrlID(hWnd))
// based on CWorkbookMDIFrame::OnToolTipText
BOOL CDbSourceWnd::OnToolTipText (UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

    CString strTipText;
    UINT_PTR nID = pNMHDR->idFrom;
    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool
        nID = _AfxGetDlgCtrlID((HWND)nID);
    }

    switch (nID)
    {
    case IDC_DS_SCHEMA    : strTipText = "Schema (Alt-S)";         break;
    case IDC_DS_VALID     : strTipText = "Include Valid (Alt-V)";  break;
    case IDC_DS_INVALID   : strTipText = "Include Invalid (Alt-V)"; break;
    case IDC_DS_AS_LIST   : strTipText = "Short List (Alt-L)";     break;
    case IDC_DS_AS_REPORT : strTipText = "Detail List (Alt-D)";    break;
    case IDC_DS_FILTER    : strTipText = "Filter (Alt-F)";         break;
    case IDC_DS_REFRESH   : strTipText = "Refresh (Alt-R) / Refresh All (Ctrl+Click)"; break;
    case IDC_DS_SETTINGS  : strTipText = "Settings (Alt-T)";       break;

    default:
        return FALSE;
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

void CDbSourceWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
	const int SCHEMA_COMMAND_BASE = 100;

    if (pWnd == this)
    {
        CDialog::OnContextMenu(pWnd, point);
    }
    else if (pWnd == &m_wndSchemaList)
    {
        if (point.x == -1 && point.y == -1)
        {
            CRect rc;
            m_wndSchemaList.GetWindowRect(rc);
            point.x = rc.left;
            point.y = rc.top;
        }

        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, 3, L"&Refresh the list");
        menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);

		if (m_recentSchemas.size() > 1)
		{
			auto it = m_recentSchemas.rbegin();
			auto end = m_recentSchemas.rend();
			for (int i = m_recentSchemas.size()-1; it != end; ++it, --i)
			{
				if (it->c_str() != m_strSchema)
					menu.AppendMenu(MF_STRING, SCHEMA_COMMAND_BASE + i, it->c_str());
			}

			menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);
		}
        menu.AppendMenu(MF_STRING, 1, L"&All schemas (faster)");
        menu.AppendMenu(MF_STRING, 2, L"Schemas with &objects");
        menu.CheckMenuRadioItem(1, 2, m_AllSchemas ?  1 : 2, MF_BYCOMMAND);
        int choice = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        switch (choice)
        {
        case 1:
            m_AllSchemas = TRUE;
            InitSchemaList();
            PopulateSchemaList();
            break;
        case 2:
            m_AllSchemas = FALSE;
            InitSchemaList();
            PopulateSchemaList();
            break;
        case 3:
            InitSchemaList();
            PopulateSchemaList();
            break;
		default:
			{
				unsigned int index = choice - SCHEMA_COMMAND_BASE;
				if (index < m_recentSchemas.size())
				{                                           
                    auto it = m_recentSchemas.begin();
                    std::advance(it, index);
					int cbIndex = m_wndSchemaList.FindStringExact(0, it->c_str());
					if (cbIndex != LB_ERR)
					{
						m_wndSchemaList.SetCurSel(cbIndex);
						OnSchemaChanged();
					}
				}
			}
        }
    }
    else if (pWnd == &m_wndTab)
    {
        if (point.x == -1 && point.y == -1)
        {
            CRect rc;
            m_wndTab.GetWindowRect(rc);
            point.x = rc.left;
            point.y = rc.top;
        }

        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, 1, L"&Tab Titles");
        if (m_bShowTabTitles) menu.CheckMenuItem(1, MF_BYCOMMAND|MF_CHECKED);
        int choice = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        if (choice == 1)
            OnTabTitles();
    }
}

BOOL CDbSourceWnd::PreTranslateMessage(MSG* pMsg)
{
    if (m_accelTable
    && TranslateAccelerator(AfxGetMainWnd()->m_hWnd, m_accelTable, pMsg))
        return TRUE;
    return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(SchemaComboBox, CComboBox)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL SchemaComboBox::OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT && IsBusy())
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING/*IDC_WAIT*/));
        return TRUE;
    }

    return CComboBox::OnSetCursor(pWnd, nHitTest, message);
}

void SchemaComboBox::SetBusy (bool busy)
{
    if (m_busy != busy)
    {
        m_busy = busy; 

        CPoint pt;
        GetCursorPos(&pt);
        ScreenToClient(&pt);

        CRect rc;
        GetClientRect(&rc);

        if (rc.PtInRect(pt))
            SetCursor(LoadCursor(NULL, m_busy ? IDC_APPSTARTING : IDC_ARROW));
    }
}
