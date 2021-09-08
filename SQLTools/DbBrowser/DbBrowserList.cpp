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

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DbBrowserList.h"
#include "MetaDict\MetaDictionary.h"
#include "MetaDict\Loader.h"
#include "SQLWorksheet\SqlDocCreater.h"
#include "COMMON\AppGlobal.h"
#include "COMMON\StrHelpers.h"
#include "Dlg\ConfirmationDlg.h"
#include "MetaDict\TextOutput.h"
#include "COMMON\SimpleDragDataSource.h"
#include "MainFrm.h"
#include "DbBrowser\ObjectViewerWnd.h"
#include "COMMON/GUICommandDictionary.h"
#include "SQLWorksheet\SQLWorksheetDoc.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "ObjectTree_Builder.h" // for ObjectTree::ObjectDescriptor
#include <ActivePrimeExecutionNote.h>

// 24.09.2007 some improvements taken from sqltools++ by Randolf Geist

    using namespace OraMetaDict;
    using namespace ServerBackgroundThread;

    CImageList DbBrowserList::m_images, DbBrowserList::m_stateImages;
    HACCEL DbBrowserList::m_accelTable;

DbBrowserList::DbBrowserList (Common::ListCtrlDataProvider& dataProvider, bool useStateImage)
: Common::CManagedListCtrl(dataProvider),
m_dirty(true),
m_busy(false),
m_useStateImage(useStateImage)
{
}

DbBrowserList::~DbBrowserList()
{
	//if (m_accelTable)
	//	DestroyAcceleratorTable(m_accelTable);
}

void DbBrowserList::GetSelEntries (std::vector<unsigned int>& _entries) const
{
    std::vector<unsigned int> entries;

    int item = -1;
    while ((item = GetNextItem(item, LVNI_SELECTED)) != -1) 
        entries.push_back(GetItemData(item));

    swap(_entries, entries);
}

bool DbBrowserList::IsSelectionEmpty () const
{
    return GetNextItem(-1, LVNI_SELECTED) == -1;
}

	struct BackgroundTask_ListSqlBatch : Task
    {
        DbBrowserList& m_list;
        unsigned int m_entry;
        string m_statement;
        bool m_refresh;

        static void Schedule (DbBrowserList& list, const char* taskName, const char* sttm, bool refresh)
        {
            CWaitCursor wait;

            std::list<TaskPtr> group;

            vector<unsigned int> entries;
            list.GetSelEntries(entries);

            std::vector<unsigned int>::const_iterator it = entries.begin();
            for (; it != entries.end(); ++it)
            {
                Common::Substitutor subst;
                subst.AddPair("<SCHEMA>", list.GetSchema());
                subst.AddPair("<NAME>", list.GetObjectName(*it));
                subst.AddPair("<TABLE_NAME>", list.GetTableName(*it));
                subst.AddPair("<TYPE>", list.GetObjectType(*it));
                subst << list.refineDoSqlForSel(sttm, *it);

                group.push_back(TaskPtr(new BackgroundTask_ListSqlBatch(list, taskName, *it, subst.GetResult(), refresh)));
            }

            BkgdRequestQueue::Get().PushGroup(group);
        }

        BackgroundTask_ListSqlBatch (DbBrowserList& list, const char* taskName, unsigned int entry, const char* sttm, bool refresh)
            : Task(taskName /*"Execute SQL for selection"*/, list.GetParent()),
            m_list(list),
            m_entry(entry),
            m_statement(sttm),
            m_refresh(refresh)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                connect.ExecuteStatement(m_statement.c_str());
            }
            catch (const OciException& x)
            {
                MessageBeep(MB_ICONHAND);
                ostringstream msg;
                msg << '"' << GetName() << '"' << endl
                    << "\tfailed with the following error:" << endl
                    << x.what();

                if (AfxMessageBox(Common::wstr(msg.str()).c_str(), MB_ICONHAND|MB_OKCANCEL) == IDCANCEL)
                {
                    //SetError(x.what());
                    BkgdRequestQueue::Get().RemoveGroup(GetGroupId());
                }
            }
        }

        void ReturnInForeground ()
        {
            if (m_refresh)
                m_list.RefreshEntry(m_entry);
        }
    };


// Parameter "sttm" might constains placeholders <SCHEMA>, <NAME> , <TYPE>
void DbBrowserList::DoSqlForSel (const char* taskName, const char* sttm, bool refresh)
{
    BackgroundTask_ListSqlBatch::Schedule(*this, taskName, sttm, refresh);
}

    struct ObjectDescriptor 
    { 
        string owner, name, type; 
        ObjectDescriptor (const char* _owner, const char* _name, const char* _type)
            : owner(_owner), name(_name), type(_type) {}
    };

	struct BackgroundTask_ListLoad : Task
    {
        vector<ObjectDescriptor> m_objects;
        SQLToolsSettings m_settings;
        bool m_asOne, m_preload;
        Dictionary m_dictionary;

        BackgroundTask_ListLoad (DbBrowserList& list, vector<ObjectDescriptor> objects, const SQLToolsSettings& settings, bool asOne, bool preload)
            : Task("Load object(s)", list.GetParent()),
            m_objects(objects),
            m_settings(settings),
            m_asOne(asOne),
            m_preload(preload)
        {
            m_dictionary.SetRewriteSharedDuplicate(true);
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                //Sleep(10000);
                Loader loader(connect, m_dictionary);
                loader.Init();

                if (m_preload && !m_objects.empty())
                    loader.LoadObjects(m_objects.begin()->owner.c_str(), "%", m_objects.begin()->type.c_str(), 
                        m_settings, m_settings.GetSpecWithBody(), m_settings.GetBodyWithSpec(), true/*bUseLike*/);

                vector<ObjectDescriptor>::const_iterator it = m_objects.begin();
                for (; it != m_objects.end(); ++it)
                {
                    try {
                        m_dictionary.LookupObject(it->owner.c_str(), it->name.c_str(), it->type.c_str());
                    } 
                    catch (const XNotFound&) 
                    {
                        loader.LoadObjects(it->owner.c_str(), it->name.c_str(), it->type.c_str(),
                            m_settings,  m_settings.GetSpecWithBody(), m_settings.GetBodyWithSpec(), false/*bUseLike*/);
                    }
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            SqlDocCreater docCreater(m_asOne/*singleDocument*/, true/*useBanner*/, false/*supressGroupByDDL*/, !m_asOne/*objectNameAsTitle*/);

            vector<ObjectDescriptor>::const_iterator it = m_objects.begin();
            for (; it != m_objects.end(); ++it)
              docCreater.Put(m_dictionary.LookupObject(it->owner.c_str(), it->name.c_str(), it->type.c_str()), m_settings);
        }
    };

void DbBrowserList::doLoad (bool asOne)
{
    CWaitCursor wait;

    std::vector<unsigned int> entries;
    GetSelEntries(entries);

    SQLToolsSettings settings = GetSQLToolsSettings();

    if (!entries.empty()
    && ShowDDLPreferences(settings, true)) // it might be shown only if <shift> is pressed 
    {

        bool preload = (
                GetMonoType() // only monotype objects can be preloaded
                && settings.GetPreloadDictionary()
                && entries.size() > 1
                && entries.size() >= (GetTotalEntryCount()*settings.GetPreloadStartPercent())/100
            );

        vector<ObjectDescriptor> objects;
        std::vector<unsigned int>::const_iterator it = entries.begin();
        for (; it != entries.end(); ++it)
            if (strcmp(GetObjectType(*it), "JAVA CLASS"))
                objects.push_back(ObjectDescriptor(GetSchema(), GetObjectName(*it), GetObjectType(*it)));

        if (entries.size() != objects.size()) // some was skipped
        {
            MessageBeep(MB_ICONERROR);
		    Global::SetStatusText("Skipping \"JAVA CLASS\" object(s)!");
        }

        if (!objects.empty())
            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListLoad(*this, objects, settings, asOne, preload)));
    }
}

void DbBrowserList::MakeDropSql (int entry, std::string& sql) const
{
    Common::Substitutor subst;
    subst.AddPair("<SCHEMA>", GetSchema());
    subst.AddPair("<NAME>", GetObjectName(entry));
    subst.AddPair("<TYPE>", GetObjectType(entry));
    subst <<  ("DROP <TYPE> \"<SCHEMA>\".\"<NAME>\"");
    sql = subst.GetResult();
}

	struct BackgroundTask_ListDrop : Task
    {
        DbBrowserList& m_list;
        int m_entry;
        string m_statement;

        BackgroundTask_ListDrop (DbBrowserList& list, int entry)
            : Task("Drop object", list.GetParent()),
            m_list(list),
            m_entry(entry)
        {
            list.MakeDropSql(entry, m_statement);
            SetName(m_statement);
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                connect.ExecuteStatement(m_statement.c_str());
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            m_list.MarkAsDeleted(m_entry);
            m_list.OnDeleteEntry(m_entry);
        }
    };

void DbBrowserList::DoDrop (int entry)
{
    BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListDrop(*this, entry)));
}

void DbBrowserList::GetListSelectionAsText (CString& text)
{
    if (!IsSelectionEmpty())
    {
        bool b_schema_name = GetSQLToolsSettings().m_ShemaName;
        TextOutputInMEM out(GetSQLToolsSettings().m_LowerNames);

        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (bool comma = false; it != data.end(); ++it, comma = true)
        {
            if (comma) out.Put(", ");
            out.PutOwnerAndName (GetSchema(), GetObjectName(*it), b_schema_name);
        }

	    text = Common::wstr(out).c_str();
    }
    else
        text.Empty();
}

void DbBrowserList::DoShowInObjectView (const string& schema, const string& name, const string& type, const std::vector<std::string>& drilldown)
{
    ObjectTree::ObjectDescriptor obj;
    obj.owner = schema;
    obj.name = name;
    obj.type = type;
    CObjectViewerWnd& viewer = ((CMDIMainFrame*)AfxGetMainWnd())->ShowTreeViewer();
    viewer.ShowObject(obj, drilldown);
}

BEGIN_MESSAGE_MAP(DbBrowserList, Common::CManagedListCtrl)
    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
    ON_COMMAND(IDC_DS_AS_LIST, OnShowAsList)
    ON_COMMAND(IDC_DS_AS_REPORT, OnShowAsReport)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
    ON_COMMAND(ID_DS_REFRESH, OnRefresh)
    ON_COMMAND(ID_DS_SETTINGS, OnSettings)
    ON_COMMAND(ID_SQL_LOAD, OnLoad)
    ON_COMMAND(ID_SQL_LOAD_WITH_DBMS_METADATA, OnLoadWithDbmsMetadata)
    ON_COMMAND(ID_DS_LOAD_ALL_IN_ONE, OnLoadAsOne)
    ON_COMMAND(ID_EDIT_DELETE_WORD_TO_RIGHT, OnDrop)
    ON_COMMAND(ID_DS_FILTER, OnFilter)
    ON_COMMAND(ID_EDIT_COPY, OnCopy)
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnLvnBegindrag)
    ON_COMMAND(ID_SQL_DESCRIBE, OnShowInObjectView)
    ON_COMMAND(ID_DS_TAB_TITLES, OnTabTitles)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

int DbBrowserList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CManagedListCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    ModifyStyle(LVS_SINGLESEL, 0);

    // 30.03.2003 optimization, removed images from Object Browser for Win95/Win98/WinMe because of resource cost
    if (!(::GetVersion() & 0x80000000))
    {
        if (m_images.m_hImageList == NULL)
            m_images.Create(IDB_DB_OBJ, 16, 64, RGB(0,255,0));

        SetImageList(&m_images, LVSIL_SMALL);

        if (m_useStateImage)
        {
            if (m_stateImages.m_hImageList == NULL)
                m_stateImages.Create(IDB_DB_OBJ_STATE_V2, 16, 64, RGB(0,255,0));

            SetImageList(&m_stateImages, LVSIL_STATE);

            SetCallbackMask(LVIS_STATEIMAGEMASK);
        }
    }

	if (!m_accelTable)
	{
		CMenu menu;
        menu.CreatePopupMenu();
        BuildContexMenu(&menu);
		Common::GUICommandDictionary::AddAccelDescriptionToMenu(menu.m_hMenu);
		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(menu.m_hMenu);
	}

    return 0;
}

BOOL DbBrowserList::PreTranslateMessage (MSG* pMsg)
{
	if (m_accelTable)
		if (TranslateAccelerator(m_hWnd, m_accelTable, pMsg) == 0)
			return CManagedListCtrl::PreTranslateMessage(pMsg);
		else
			return true;
	else
		return CManagedListCtrl::PreTranslateMessage(pMsg);
}

void DbBrowserList::BuildContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING,        ID_DS_REFRESH,         L"&Refresh");
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING|grayed, ID_SQL_DESCRIBE,       L"Show in &Object Viewer");
    pMenu->AppendMenu(MF_STRING|grayed, ID_SQL_LOAD,           L"&Load DDL");
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_LOAD_ALL_IN_ONE, L"L&oad DDL in one file");
    pMenu->AppendMenu(MF_STRING|grayed, ID_SQL_LOAD_WITH_DBMS_METADATA, L"Load DDL with Dbms_&Metadata");
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING|grayed, ID_EDIT_DELETE_WORD_TO_RIGHT,   L"&Drop");
    pMenu->AppendMenu(MF_SEPARATOR);

    ExtendContexMenu(pMenu);

    pMenu->AppendMenu(MF_STRING|grayed, ID_EDIT_COPY,          L"&Copy");
    pMenu->AppendMenu(MF_STRING|grayed, ID_EDIT_SELECT_ALL,    L"Select &All");
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING,        ID_DS_FILTER,          L"&Filter...");                      
    pMenu->AppendMenu(MF_SEPARATOR);

    {
        CMenu viewMenu;
        viewMenu.CreateMenu();

        viewMenu.AppendMenu(MF_STRING,        ID_DS_TAB_TITLES,      L"Tab Titles");
        viewMenu.AppendMenu(MF_SEPARATOR);
        viewMenu.AppendMenu(MF_STRING,        IDC_DS_AS_LIST,        L"Short &List");
        viewMenu.AppendMenu(MF_STRING,        IDC_DS_AS_REPORT,      L"&Detail List");
        
        if (CDbSourceWnd* parent = static_cast<CDbSourceWnd*>(GetParent()))
            if (parent->AreTabTitlesVisible())
                viewMenu.CheckMenuItem(ID_DS_TAB_TITLES, MF_BYCOMMAND|MF_CHECKED);

        viewMenu.CheckMenuRadioItem(IDC_DS_AS_LIST, IDC_DS_AS_REPORT, 
            (LVS_LIST == (LVS_TYPEMASK & GetWindowLong(m_hWnd, GWL_STYLE))) 
            ? IDC_DS_AS_LIST : IDC_DS_AS_REPORT, MF_BYCOMMAND);
        
        pMenu->AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu.m_hMenu), L"&View");
        viewMenu.Detach();
    }

    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING,        ID_DS_SETTINGS,        L"DDL &Preferences...");                      

    pMenu->SetDefaultItem(GetDefaultContextCommand(false));
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pMenu->m_hMenu, 4);
}

void DbBrowserList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    if (point.x == -1 && point.y == -1)
    {
        CRect rc;
        GetWindowRect(rc);
        point.x = rc.left;
        point.y = rc.top;

        int item = GetNextItem(-1, LVNI_FOCUSED);
        if (item == -1)
            item = GetNextItem(-1, LVNI_SELECTED);
        if (item != -1
        && GetItemRect(item, &rc, LVIR_LABEL)) 
        {
            point.x = rc.left;
            point.y = rc.bottom;
            ClientToScreen(&point);
        }
    }

    CMenu menu;
    menu.CreatePopupMenu();
    BuildContexMenu(&menu);
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void DbBrowserList::OnNMDblclk(NMHDR* pNMHDR, LRESULT *pResult)
{
    NMITEMACTIVATE* pNMITEMACTIVATE = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    bool bAltPressed = (pNMHDR && (pNMITEMACTIVATE->uKeyFlags & LVKF_ALT));
    SendMessage(WM_COMMAND, GetDefaultContextCommand(bAltPressed));
    *pResult = 0;
}

void DbBrowserList::OnLvnBegindrag (NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	CString test;
    GetListSelectionAsText(test);
	Common::SimpleDragDataSource(test).DoDragDrop(DROPEFFECT_COPY);

    *pResult = 0;
}

void DbBrowserList::OnShowAsList()
{
    GetParent()->SendMessage(WM_COMMAND, IDC_DS_AS_LIST);
}

void DbBrowserList::OnShowAsReport()
{
    GetParent()->SendMessage(WM_COMMAND, IDC_DS_AS_REPORT);
}

void DbBrowserList::OnSelectAll()
{
    for (int i = 0; SetItemState(i, LVIS_SELECTED, LVIS_SELECTED); ++i);
}

void DbBrowserList::OnRefresh ()
{
    CWaitCursor wait;
    Refresh(true);
}

void DbBrowserList::OnSettings()
{
    GetParent()->SendMessage(WM_COMMAND, ID_DS_SETTINGS);
}

void DbBrowserList::OnLoadAsOne ()
{
    CWaitCursor wait;
    doLoad(true);
}

void DbBrowserList::OnLoad ()
{
    CWaitCursor wait;
    doLoad(false);
}

void DbBrowserList::OnLoadWithDbmsMetadata ()
{
    CWaitCursor wait;
    
    std::vector<unsigned int> data;
    GetSelEntries(data);

    std::vector<unsigned int>::const_iterator it = data.begin();
    for (; it != data.end(); ++it)
    {
        string type = GetObjectType(*it);
        
        if (type == "DATABASE LINK") // TODO: revise the fix
            type = "DB_LINK";
        
        CPLSWorksheetDoc::LoadDDLWithDbmsMetadata(GetSchema(), GetObjectName(*it), type);    
    }
}

void DbBrowserList::OnDrop ()
{
    CWaitCursor wait;

    try {
        
        bool dropAll = false;
        CConfirmationDlg dlg(this);

        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (; it != data.end(); ++it)
        {
            bool dropIt = false;

            if (dropAll)
                dropIt = true;
            else
            {
                dlg.m_strText.Format(L"Are you sure you want to drop \"%s\"?", Common::wstr(GetObjectName(*it)).c_str());
                int retVal = dlg.DoModal();
                SetFocus();

                switch (retVal) 
                {
                case IDCANCEL:
                    AfxThrowUserException();
                case ID_CNF_ALL:
                    dropAll = true;
                case IDYES:
                    dropIt = true;
                case IDNO:
                    break;
                }
            }

            try
            {
                if (dropIt)
                    DoDrop(*it);
            }
            catch (const OciException& x)
            {
                MessageBeep(MB_ICONHAND);
                ostringstream msg;
                msg << "Drop \"" << GetObjectName(*it) << "\" failed with the following error:\n\n" << x.what();
                if (AfxMessageBox(Common::wstr(msg.str()).c_str(), MB_ICONHAND|MB_OKCANCEL) == IDCANCEL)
                    AfxThrowUserException();

            }
        }
    }
    catch (CUserException*)
    {
        MessageBeep((UINT)-1);
    }
}


void DbBrowserList::OnFilter ()
{
    ShowFilterDialog();
}

void DbBrowserList::OnCopy ()
{
    CWaitCursor wait;

    if (!IsSelectionEmpty()) 
    {
        CString text;
        GetListSelectionAsText(text);
		Common::AppCopyTextToClipboard(text);
		Global::SetStatusText(L"Copied to clipboard: " + text);
    }
}

void DbBrowserList::OnShowInObjectView ()
{
    std::vector<unsigned int> data;
    GetSelEntries(data);

    if (!data.empty())
    {
        //TextOutputInMEM out;
        //out.PutOwnerAndName(GetSchema(), GetObjectName(data.front()), true);
        //CObjectViewerWnd& viewer = ((CMDIMainFrame*)AfxGetMainWnd())->ShowTreeViewer();
        //viewer.ShowObject(out.GetData());
        DoShowInObjectView(GetSchema(), GetObjectName(data.front()), GetObjectType(data.front()));
	}
}

void DbBrowserList::OnTabTitles ()
{
    if (CWnd* parent = GetParent())
        parent->SendMessage(WM_COMMAND, ID_DS_TAB_TITLES);
}

void DbBrowserList::SetBusy (bool busy)             
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

BOOL DbBrowserList::OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT && IsBusy())
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING/*IDC_WAIT*/));
        return TRUE;
    }

    return CManagedListCtrl::OnSetCursor(pWnd, nHitTest, message);
}

//TODO: ability to hide any of lists
//TODO: add flexibility to GetListSelectionAsText, add alternatives for drag & drop with Alt pressed
//TODO: ddl wrong admin_iot (oracle sample)
